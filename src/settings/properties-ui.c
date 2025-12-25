#include "properties-ui.h"
#include "plugin-settings.h"
#include "../plugin-main.h"
#include "../audio-capture/device-enum.h"

#include <obs-module.h>
#include <obs-frontend-api.h>

// Callback when enabled toggle changes
static bool on_enabled_changed(obs_properties_t *props, obs_property_t *p,
                               obs_data_t *settings)
{
    (void)p;

    bool enabled = obs_data_get_bool(settings, "enabled");

    // Enable/disable other properties
    obs_property_set_enabled(obs_properties_get(props, "device_id"), enabled);
    obs_property_set_enabled(obs_properties_get(props, "sensitivity"), enabled);
    obs_property_set_enabled(obs_properties_get(props, "restart_mode"), enabled);
    obs_property_set_enabled(obs_properties_get(props, "language"), enabled);
    obs_property_set_enabled(obs_properties_get(props, "refresh_devices"), enabled);

    return true;  // Refresh UI
}

// Callback for refresh devices button
static bool on_refresh_devices(obs_properties_t *props, obs_property_t *p,
                               void *data)
{
    (void)p;
    (void)data;

    obs_property_t *device_list = obs_properties_get(props, "device_id");
    obs_property_list_clear(device_list);

    // Add default option
    obs_property_list_add_string(device_list,
                                 obs_module_text("GarminReplay.DefaultMicrophone"),
                                 "");

    // Enumerate and add microphones
    device_list_t *devices = device_enum_microphones();
    if (devices) {
        for (int i = 0; i < devices->count; i++) {
            obs_property_list_add_string(device_list,
                                         devices->devices[i].name,
                                         devices->devices[i].id);
        }
        device_list_free(devices);
    }

    return true;  // Refresh UI
}

// Callback when settings are applied
static void on_settings_update(void *data, obs_data_t *settings)
{
    (void)data;

    bool was_enabled = g_plugin_data.enabled;

    // Update plugin state
    g_plugin_data.enabled = obs_data_get_bool(settings, "enabled");
    g_plugin_data.sensitivity = (int)obs_data_get_int(settings, "sensitivity");
    g_plugin_data.restart_mode = (int)obs_data_get_int(settings, "restart_mode");
    g_plugin_data.language = (int)obs_data_get_int(settings, "language");

    // Update device ID
    const char *device_id = obs_data_get_string(settings, "device_id");
    if (g_plugin_data.device_id) {
        bfree(g_plugin_data.device_id);
        g_plugin_data.device_id = NULL;
    }
    if (device_id && strlen(device_id) > 0) {
        g_plugin_data.device_id = bstrdup(device_id);
    }

    // Save settings
    garmin_save_settings();

    // Start/stop voice recognition as needed
    if (g_plugin_data.enabled && !was_enabled) {
        start_voice_recognition();
    } else if (!g_plugin_data.enabled && was_enabled) {
        stop_voice_recognition();
    } else if (g_plugin_data.enabled && was_enabled) {
        // Restart to apply new device/settings
        stop_voice_recognition();
        start_voice_recognition();
    }
}

obs_properties_t *garmin_get_properties(void *data)
{
    (void)data;

    obs_properties_t *props = obs_properties_create();
    obs_property_t *p;

    // === Header/Info ===
    obs_properties_add_text(props, "info",
                            obs_module_text("GarminReplay.Info"),
                            OBS_TEXT_INFO);

    // === Enable Toggle ===
    p = obs_properties_add_bool(props, "enabled",
                                obs_module_text("GarminReplay.Enable"));
    obs_property_set_modified_callback(p, on_enabled_changed);

    // === Microphone Selection ===
    p = obs_properties_add_list(props, "device_id",
                                obs_module_text("GarminReplay.Microphone"),
                                OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_STRING);

    // Add default option
    obs_property_list_add_string(p,
                                 obs_module_text("GarminReplay.DefaultMicrophone"),
                                 "");

    // Populate with available devices
    device_list_t *devices = device_enum_microphones();
    if (devices) {
        for (int i = 0; i < devices->count; i++) {
            obs_property_list_add_string(p,
                                         devices->devices[i].name,
                                         devices->devices[i].id);
        }
        device_list_free(devices);
    }

    // Refresh button
    obs_properties_add_button(props, "refresh_devices",
                              obs_module_text("GarminReplay.RefreshDevices"),
                              on_refresh_devices);

    // === Sensitivity Slider ===
    p = obs_properties_add_int_slider(props, "sensitivity",
                                      obs_module_text("GarminReplay.Sensitivity"),
                                      1, 100, 1);
    obs_property_set_long_description(p,
                                      obs_module_text("GarminReplay.SensitivityDesc"));

    // === Language Selection ===
    p = obs_properties_add_list(props, "language",
                                obs_module_text("GarminReplay.Language"),
                                OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);
    obs_property_list_add_int(p, obs_module_text("GarminReplay.LangEnglish"), GARMIN_LANG_ENGLISH);
    obs_property_list_add_int(p, obs_module_text("GarminReplay.LangGerman"), GARMIN_LANG_GERMAN);
    obs_property_list_add_int(p, obs_module_text("GarminReplay.LangFrench"), GARMIN_LANG_FRENCH);
    obs_property_set_long_description(p,
                                      obs_module_text("GarminReplay.LanguageDesc"));

    // === Restart Mode ===
    p = obs_properties_add_list(props, "restart_mode",
                                obs_module_text("GarminReplay.RestartMode"),
                                OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);
    obs_property_list_add_int(p, obs_module_text("GarminReplay.SaveOnly"), 0);
    obs_property_list_add_int(p, obs_module_text("GarminReplay.SaveAndRestart"), 1);

    // === Trigger Phrases Info ===
    obs_properties_add_text(props, "phrases_info",
                            obs_module_text("GarminReplay.TriggerPhrases"),
                            OBS_TEXT_INFO);

    return props;
}

void garmin_get_defaults(obs_data_t *settings)
{
    obs_data_set_default_bool(settings, "enabled", false);
    obs_data_set_default_string(settings, "device_id", "");
    obs_data_set_default_int(settings, "sensitivity", 70);
    obs_data_set_default_int(settings, "restart_mode", 0);
    obs_data_set_default_int(settings, "language", GARMIN_LANG_ENGLISH);
}

// Dialog close callback
static void on_dialog_closed(void *data)
{
    (void)data;
    // Settings are applied via the update callback
}

void garmin_show_settings_dialog(void)
{
    // Get current settings
    obs_data_t *settings = obs_data_create();
    obs_data_set_bool(settings, "enabled", g_plugin_data.enabled);
    obs_data_set_int(settings, "sensitivity", g_plugin_data.sensitivity);
    obs_data_set_int(settings, "restart_mode", g_plugin_data.restart_mode);
    obs_data_set_int(settings, "language", g_plugin_data.language);

    if (g_plugin_data.device_id) {
        obs_data_set_string(settings, "device_id", g_plugin_data.device_id);
    } else {
        obs_data_set_string(settings, "device_id", "");
    }

    // Create and show properties dialog
    obs_properties_t *props = garmin_get_properties(NULL);

    // Note: OBS doesn't have a direct API for showing a properties dialog
    // from a plugin. This would typically be done through a source or dock.
    // For now, settings are managed through the Tools menu toggle
    // and the config file directly.

    obs_properties_destroy(props);
    obs_data_release(settings);
}
