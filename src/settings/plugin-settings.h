#ifndef PLUGIN_SETTINGS_H
#define PLUGIN_SETTINGS_H

#ifdef __cplusplus
extern "C" {
#endif

// Load settings from OBS config
void garmin_load_settings(void);

// Save settings to OBS config
void garmin_save_settings(void);

// Get the settings config file path
const char *garmin_get_settings_path(void);

#ifdef __cplusplus
}
#endif

#endif // PLUGIN_SETTINGS_H
