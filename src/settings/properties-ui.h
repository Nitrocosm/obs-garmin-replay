#ifndef PROPERTIES_UI_H
#define PROPERTIES_UI_H

#include <obs-module.h>

// Get the plugin properties for the settings dialog
obs_properties_t *garmin_get_properties(void *data);

// Get default values for properties
void garmin_get_defaults(obs_data_t *settings);

// Show the settings dialog
void garmin_show_settings_dialog(void);

#endif // PROPERTIES_UI_H
