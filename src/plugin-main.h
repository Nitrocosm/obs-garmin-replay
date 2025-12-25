#ifndef PLUGIN_MAIN_H
#define PLUGIN_MAIN_H

#include <obs-module.h>
#include <obs-frontend-api.h>
#include <windows.h>

#ifdef __cplusplus
extern "C" {
#endif

// Forward declarations
typedef struct vosk_engine vosk_engine_t;
typedef struct wasapi_capture wasapi_capture_t;

// Language options (prefixed to avoid Windows SDK conflicts)
#define GARMIN_LANG_ENGLISH 0
#define GARMIN_LANG_GERMAN  1
#define GARMIN_LANG_FRENCH  2

// Plugin state structure
struct garmin_plugin_data {
    // Settings
    obs_data_t *settings;
    bool enabled;

    // Audio capture
    wasapi_capture_t *capture;
    char *device_id;

    // Voice recognition
    vosk_engine_t *vosk;
    int sensitivity;
    int restart_mode;
    int language;  // GARMIN_LANG_ENGLISH, GARMIN_LANG_GERMAN, or GARMIN_LANG_FRENCH

    // Recognition thread
    HANDLE recognition_thread;
    volatile bool thread_running;

    // Status
    char status_text[256];
};

// Global plugin data
extern struct garmin_plugin_data g_plugin_data;

// Thread functions
void start_voice_recognition(void);
void stop_voice_recognition(void);

// Settings functions
void garmin_load_settings(void);
void garmin_save_settings(void);

// Utility
void get_vosk_model_path(char *path, size_t max_len);

#ifdef __cplusplus
}
#endif

#endif // PLUGIN_MAIN_H
