#include "plugin-settings.h"
#include "../plugin-main.h"

#include <obs-module.h>
#include <util/config-file.h>
#include <util/platform.h>

#include <stdio.h>
#include <string.h>

#define CONFIG_SECTION "GarminReplay"

static char settings_path[512] = {0};

const char *garmin_get_settings_path(void)
{
    if (settings_path[0] == '\0') {
        char *config_dir = obs_module_config_path("");
        if (config_dir) {
            snprintf(settings_path, sizeof(settings_path),
                     "%sgarmin-replay.json", config_dir);
            bfree(config_dir);
        }
    }
    return settings_path;
}

void garmin_load_settings(void)
{
    const char *path = garmin_get_settings_path();

    blog(LOG_INFO, "[Garmin Replay] Loading settings from: %s", path);

    // Try to load settings from file
    obs_data_t *data = obs_data_create_from_json_file(path);
    if (!data) {
        // Set defaults
        blog(LOG_INFO, "[Garmin Replay] No settings file found, using defaults");
        g_plugin_data.enabled = false;
        g_plugin_data.sensitivity = 70;
        g_plugin_data.restart_mode = 0;
        g_plugin_data.language = GARMIN_LANG_ENGLISH;
        g_plugin_data.device_id = NULL;

        // Store defaults in settings
        obs_data_set_bool(g_plugin_data.settings, "enabled", false);
        obs_data_set_int(g_plugin_data.settings, "sensitivity", 70);
        obs_data_set_int(g_plugin_data.settings, "restart_mode", 0);
        obs_data_set_int(g_plugin_data.settings, "language", GARMIN_LANG_ENGLISH);
        obs_data_set_string(g_plugin_data.settings, "device_id", "");
        return;
    }

    // Load values
    g_plugin_data.enabled = obs_data_get_bool(data, "enabled");
    g_plugin_data.sensitivity = (int)obs_data_get_int(data, "sensitivity");
    g_plugin_data.restart_mode = (int)obs_data_get_int(data, "restart_mode");
    g_plugin_data.language = (int)obs_data_get_int(data, "language");

    // Validate language
    if (g_plugin_data.language < GARMIN_LANG_ENGLISH || g_plugin_data.language > GARMIN_LANG_FRENCH) {
        g_plugin_data.language = GARMIN_LANG_ENGLISH;
    }

    const char *device_id = obs_data_get_string(data, "device_id");
    if (device_id && strlen(device_id) > 0) {
        g_plugin_data.device_id = bstrdup(device_id);
    }

    // Validate sensitivity
    if (g_plugin_data.sensitivity < 1) g_plugin_data.sensitivity = 1;
    if (g_plugin_data.sensitivity > 100) g_plugin_data.sensitivity = 100;

    // Copy to plugin settings
    obs_data_apply(g_plugin_data.settings, data);

    obs_data_release(data);

    blog(LOG_INFO, "[Garmin Replay] Settings loaded: enabled=%d, sensitivity=%d, restart_mode=%d, language=%d",
         g_plugin_data.enabled, g_plugin_data.sensitivity, g_plugin_data.restart_mode, g_plugin_data.language);
}

void garmin_save_settings(void)
{
    const char *path = garmin_get_settings_path();

    // Ensure directory exists
    char *dir = obs_module_config_path("");
    if (dir) {
        os_mkdirs(dir);
        bfree(dir);
    }

    // Update settings data from plugin state
    obs_data_set_bool(g_plugin_data.settings, "enabled", g_plugin_data.enabled);
    obs_data_set_int(g_plugin_data.settings, "sensitivity", g_plugin_data.sensitivity);
    obs_data_set_int(g_plugin_data.settings, "restart_mode", g_plugin_data.restart_mode);
    obs_data_set_int(g_plugin_data.settings, "language", g_plugin_data.language);

    if (g_plugin_data.device_id) {
        obs_data_set_string(g_plugin_data.settings, "device_id", g_plugin_data.device_id);
    } else {
        obs_data_set_string(g_plugin_data.settings, "device_id", "");
    }

    // Save to file
    if (obs_data_save_json(g_plugin_data.settings, path)) {
        blog(LOG_INFO, "[Garmin Replay] Settings saved to: %s", path);
    } else {
        blog(LOG_ERROR, "[Garmin Replay] Failed to save settings to: %s", path);
    }
}
