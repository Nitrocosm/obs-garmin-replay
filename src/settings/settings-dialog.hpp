#ifndef SETTINGS_DIALOG_HPP
#define SETTINGS_DIALOG_HPP

#ifdef __cplusplus
extern "C" {
#endif

// Show the settings dialog (Qt-based)
void garmin_show_settings_dialog_qt(void);

// Close the settings dialog if open
void garmin_close_settings_dialog_qt(void);

#ifdef __cplusplus
}
#endif

#endif // SETTINGS_DIALOG_HPP
