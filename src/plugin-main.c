#include "plugin-main.h"
#include "voice-recognition/vosk-engine.h"
#include "voice-recognition/phrase-detector.h"
#include "audio-capture/wasapi-capture.h"
#include "audio-capture/device-enum.h"
#include "replay-control/replay-buffer.h"
#include "settings/plugin-settings.h"
#include "settings/properties-ui.h"
#include "settings/settings-dialog.hpp"

#include <obs-module.h>
#include <obs-frontend-api.h>
#include <util/platform.h>
#include <util/threading.h>

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("obs-garmin-replay", "en-US")

// Global plugin data
struct garmin_plugin_data g_plugin_data = {0};

// Audio buffer size for processing
#define AUDIO_BUFFER_SIZE 4096

// Recognition thread function
static DWORD WINAPI recognition_thread_func(LPVOID data)
{
    (void)data;
    short audio_buffer[AUDIO_BUFFER_SIZE];

    blog(LOG_INFO, "[Garmin Replay] Recognition thread started");

    // Initialize COM for this thread
    HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    if (FAILED(hr)) {
        blog(LOG_ERROR, "[Garmin Replay] Failed to initialize COM: 0x%08lX", hr);
        return 1;
    }

    // Initialize WASAPI capture
    g_plugin_data.capture = wasapi_capture_create(g_plugin_data.device_id);
    if (!g_plugin_data.capture) {
        blog(LOG_ERROR, "[Garmin Replay] Failed to create audio capture");
        CoUninitialize();
        return 1;
    }

    // Initialize Vosk engine
    char model_path[512];
    get_vosk_model_path(model_path, sizeof(model_path));
    g_plugin_data.vosk = vosk_engine_create(model_path);
    if (!g_plugin_data.vosk) {
        blog(LOG_ERROR, "[Garmin Replay] Failed to create Vosk engine");
        wasapi_capture_destroy(g_plugin_data.capture);
        g_plugin_data.capture = NULL;
        CoUninitialize();
        return 1;
    }

    // Start audio capture
    if (!wasapi_capture_start(g_plugin_data.capture)) {
        blog(LOG_ERROR, "[Garmin Replay] Failed to start audio capture");
        vosk_engine_destroy(g_plugin_data.vosk);
        wasapi_capture_destroy(g_plugin_data.capture);
        g_plugin_data.vosk = NULL;
        g_plugin_data.capture = NULL;
        CoUninitialize();
        return 1;
    }

    snprintf(g_plugin_data.status_text, sizeof(g_plugin_data.status_text),
             "Listening...");

    // Main recognition loop
    while (g_plugin_data.thread_running) {
        // Read audio from microphone
        int samples = wasapi_capture_read(g_plugin_data.capture,
                                          audio_buffer, AUDIO_BUFFER_SIZE);

        if (samples <= 0) {
            continue;
        }

        // Process through Vosk
        int result = vosk_engine_process(g_plugin_data.vosk, audio_buffer, samples);

        if (result == 1) {
            // Final result available
            const char *json = vosk_engine_get_result(g_plugin_data.vosk);

            // Check for trigger phrase
            float confidence = phrase_detector_check(json, g_plugin_data.sensitivity, g_plugin_data.language);

            if (confidence > 0.5f) {
                blog(LOG_INFO, "[Garmin Replay] Voice command detected! Confidence: %.2f",
                     confidence);

                // Check if replay buffer is active
                if (!replay_buffer_is_active()) {
                    // Start the replay buffer
                    blog(LOG_INFO, "[Garmin Replay] Replay buffer not active, starting it...");
                    snprintf(g_plugin_data.status_text, sizeof(g_plugin_data.status_text),
                             "Starting replay buffer...");

                    obs_frontend_replay_buffer_start();

                    blog(LOG_INFO, "[Garmin Replay] Replay buffer started. Say the command again to save.");
                    snprintf(g_plugin_data.status_text, sizeof(g_plugin_data.status_text),
                             "Buffer started! Say again to save.");
                } else {
                    // Replay buffer is active, save it
                    snprintf(g_plugin_data.status_text, sizeof(g_plugin_data.status_text),
                             "Command detected! Saving...");

                    // Save replay buffer
                    if (g_plugin_data.restart_mode == 1) {
                        replay_buffer_save_and_restart();
                    } else {
                        replay_buffer_save();
                    }

                    snprintf(g_plugin_data.status_text, sizeof(g_plugin_data.status_text),
                             "Saved! Listening...");
                }

                // Reset recognizer for next command
                vosk_engine_reset(g_plugin_data.vosk);
            }
        }
    }

    // Cleanup
    wasapi_capture_stop(g_plugin_data.capture);
    wasapi_capture_destroy(g_plugin_data.capture);
    vosk_engine_destroy(g_plugin_data.vosk);
    g_plugin_data.capture = NULL;
    g_plugin_data.vosk = NULL;

    CoUninitialize();

    blog(LOG_INFO, "[Garmin Replay] Recognition thread stopped");
    return 0;
}

void start_voice_recognition(void)
{
    if (g_plugin_data.thread_running) {
        return;
    }

    blog(LOG_INFO, "[Garmin Replay] Starting voice recognition...");

    g_plugin_data.thread_running = true;
    g_plugin_data.recognition_thread = CreateThread(
        NULL, 0, recognition_thread_func, NULL, 0, NULL);

    if (!g_plugin_data.recognition_thread) {
        g_plugin_data.thread_running = false;
        blog(LOG_ERROR, "[Garmin Replay] Failed to create recognition thread");
    }
}

void stop_voice_recognition(void)
{
    if (!g_plugin_data.thread_running) {
        return;
    }

    blog(LOG_INFO, "[Garmin Replay] Stopping voice recognition...");

    g_plugin_data.thread_running = false;

    if (g_plugin_data.recognition_thread) {
        WaitForSingleObject(g_plugin_data.recognition_thread, 5000);
        CloseHandle(g_plugin_data.recognition_thread);
        g_plugin_data.recognition_thread = NULL;
    }
}

void get_vosk_model_path(char *path, size_t max_len)
{
    // Select model based on language setting
    const char *model_name;
    switch (g_plugin_data.language) {
    case GARMIN_LANG_GERMAN:
        model_name = "vosk-model-small-de-0.15";
        break;
    case GARMIN_LANG_FRENCH:
        model_name = "vosk-model-small-fr-0.22";
        break;
    case GARMIN_LANG_ENGLISH:
    default:
        model_name = "vosk-model-small-en-us-0.15";
        break;
    }

    // Try to get model from plugin data directory
    char model_subpath[256];
    snprintf(model_subpath, sizeof(model_subpath), "models/%s", model_name);

    char *data_path = obs_module_file(model_subpath);
    if (data_path) {
        strncpy(path, data_path, max_len - 1);
        path[max_len - 1] = '\0';
        bfree(data_path);
        blog(LOG_INFO, "[Garmin Replay] Using model: %s", path);
        return;
    }

    // Fallback to relative path
    snprintf(path, max_len, "data/obs-plugins/obs-garmin-replay/models/%s", model_name);
    blog(LOG_INFO, "[Garmin Replay] Using fallback model path: %s", path);
}

// Frontend event callback
static void on_frontend_event(enum obs_frontend_event event, void *data)
{
    (void)data;

    switch (event) {
    case OBS_FRONTEND_EVENT_EXIT:
        stop_voice_recognition();
        break;
    default:
        break;
    }
}

// Tools menu callback
static void on_tools_menu_clicked(void *data)
{
    (void)data;

    // Show the settings dialog
    garmin_show_settings_dialog_qt();
}

bool obs_module_load(void)
{
    blog(LOG_INFO, "[Garmin Replay] Plugin loading (version %s)...",
         PLUGIN_VERSION);

    // Initialize plugin data
    memset(&g_plugin_data, 0, sizeof(g_plugin_data));
    g_plugin_data.settings = obs_data_create();

    // Load settings
    garmin_load_settings();

    // Register frontend event callback
    obs_frontend_add_event_callback(on_frontend_event, NULL);

    // Add Tools menu item
    obs_frontend_add_tools_menu_item(
        obs_module_text("GarminReplay.ToolsMenu"),
        on_tools_menu_clicked,
        NULL);

    // Start voice recognition if enabled
    if (g_plugin_data.enabled) {
        start_voice_recognition();
    }

    blog(LOG_INFO, "[Garmin Replay] Plugin loaded successfully");
    return true;
}

void obs_module_unload(void)
{
    blog(LOG_INFO, "[Garmin Replay] Plugin unloading...");

    // Stop voice recognition
    stop_voice_recognition();

    // Remove frontend callback
    obs_frontend_remove_event_callback(on_frontend_event, NULL);

    // Cleanup settings
    if (g_plugin_data.settings) {
        obs_data_release(g_plugin_data.settings);
        g_plugin_data.settings = NULL;
    }

    // Free device ID
    if (g_plugin_data.device_id) {
        bfree(g_plugin_data.device_id);
        g_plugin_data.device_id = NULL;
    }

    blog(LOG_INFO, "[Garmin Replay] Plugin unloaded");
}

const char *obs_module_description(void)
{
    return "Voice-activated replay buffer control. Say 'Garmin, save video' to save.";
}

const char *obs_module_name(void)
{
    return "obs-garmin-replay";
}
