#include "settings-dialog.hpp"
#include "plugin-settings.h"
#include "../plugin-main.h"
#include "../audio-capture/device-enum.h"

#include <obs-module.h>
#include <obs-frontend-api.h>

#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QCheckBox>
#include <QComboBox>
#include <QSlider>
#include <QPushButton>
#include <QGroupBox>
#include <QMessageBox>
#include <QIcon>

// Settings dialog class
class GarminSettingsDialog : public QDialog {
public:
    GarminSettingsDialog(QWidget *parent = nullptr);
    ~GarminSettingsDialog();

private:
    void setupUI();
    void loadSettings();
    void saveSettings();
    void refreshDevices();
    void onOkClicked();
    void onApplyClicked();
    void onCancelClicked();
    void onSensitivityChanged(int value);

    // UI elements
    QCheckBox *enabledCheck;
    QComboBox *deviceCombo;
    QPushButton *refreshBtn;
    QComboBox *languageCombo;
    QSlider *sensitivitySlider;
    QLabel *sensitivityLabel;
    QComboBox *restartModeCombo;
    QLabel *statusLabel;
};

GarminSettingsDialog::GarminSettingsDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(obs_module_text("GarminReplay.ToolsMenu"));
    setMinimumWidth(400);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    setupUI();
    loadSettings();
}

GarminSettingsDialog::~GarminSettingsDialog()
{
}

void GarminSettingsDialog::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    // === Enable Section ===
    QGroupBox *enableGroup = new QGroupBox(obs_module_text("GarminReplay.Enable"));
    QVBoxLayout *enableLayout = new QVBoxLayout(enableGroup);

    enabledCheck = new QCheckBox(obs_module_text("GarminReplay.Enable"));
    enableLayout->addWidget(enabledCheck);

    mainLayout->addWidget(enableGroup);

    // === Audio Device Section ===
    QGroupBox *audioGroup = new QGroupBox(obs_module_text("GarminReplay.Microphone"));
    QVBoxLayout *audioLayout = new QVBoxLayout(audioGroup);

    QHBoxLayout *deviceLayout = new QHBoxLayout();
    deviceCombo = new QComboBox();
    deviceCombo->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    refreshBtn = new QPushButton(obs_module_text("GarminReplay.RefreshDevices"));
    refreshBtn->setFixedWidth(120);
    connect(refreshBtn, &QPushButton::clicked, this, &GarminSettingsDialog::refreshDevices);

    deviceLayout->addWidget(deviceCombo);
    deviceLayout->addWidget(refreshBtn);
    audioLayout->addLayout(deviceLayout);

    mainLayout->addWidget(audioGroup);

    // === Language Section ===
    QGroupBox *langGroup = new QGroupBox(obs_module_text("GarminReplay.Language"));
    QVBoxLayout *langLayout = new QVBoxLayout(langGroup);

    languageCombo = new QComboBox();
    languageCombo->addItem(obs_module_text("GarminReplay.LangEnglish"), GARMIN_LANG_ENGLISH);
    languageCombo->addItem(obs_module_text("GarminReplay.LangGerman"), GARMIN_LANG_GERMAN);
    languageCombo->addItem(obs_module_text("GarminReplay.LangFrench"), GARMIN_LANG_FRENCH);
    langLayout->addWidget(languageCombo);

    QLabel *langDesc = new QLabel(obs_module_text("GarminReplay.LanguageDesc"));
    langDesc->setWordWrap(true);
    langDesc->setStyleSheet("color: gray; font-size: 10px;");
    langLayout->addWidget(langDesc);

    mainLayout->addWidget(langGroup);

    // === Sensitivity Section ===
    QGroupBox *sensGroup = new QGroupBox(obs_module_text("GarminReplay.Sensitivity"));
    QVBoxLayout *sensLayout = new QVBoxLayout(sensGroup);

    QHBoxLayout *sliderLayout = new QHBoxLayout();
    sensitivitySlider = new QSlider(Qt::Horizontal);
    sensitivitySlider->setRange(1, 100);
    sensitivitySlider->setTickPosition(QSlider::TicksBelow);
    sensitivitySlider->setTickInterval(10);
    connect(sensitivitySlider, &QSlider::valueChanged, this, &GarminSettingsDialog::onSensitivityChanged);

    sensitivityLabel = new QLabel("70");
    sensitivityLabel->setFixedWidth(30);
    sensitivityLabel->setAlignment(Qt::AlignCenter);

    sliderLayout->addWidget(sensitivitySlider);
    sliderLayout->addWidget(sensitivityLabel);
    sensLayout->addLayout(sliderLayout);

    QLabel *sensDesc = new QLabel(obs_module_text("GarminReplay.SensitivityDesc"));
    sensDesc->setWordWrap(true);
    sensDesc->setStyleSheet("color: gray; font-size: 10px;");
    sensLayout->addWidget(sensDesc);

    mainLayout->addWidget(sensGroup);

    // === After Saving Section ===
    QGroupBox *saveGroup = new QGroupBox(obs_module_text("GarminReplay.RestartMode"));
    QVBoxLayout *saveLayout = new QVBoxLayout(saveGroup);

    restartModeCombo = new QComboBox();
    restartModeCombo->addItem(obs_module_text("GarminReplay.SaveOnly"), 0);
    restartModeCombo->addItem(obs_module_text("GarminReplay.SaveAndRestart"), 1);
    saveLayout->addWidget(restartModeCombo);

    mainLayout->addWidget(saveGroup);

    // === Status Section ===
    QGroupBox *statusGroup = new QGroupBox(obs_module_text("GarminReplay.Status"));
    QVBoxLayout *statusLayout = new QVBoxLayout(statusGroup);

    statusLabel = new QLabel(obs_module_text("GarminReplay.StatusDisabled"));
    statusLayout->addWidget(statusLabel);

    mainLayout->addWidget(statusGroup);

    // === Buttons ===
    mainLayout->addStretch();

    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();

    QPushButton *okBtn = new QPushButton(tr("OK"));
    QPushButton *applyBtn = new QPushButton(tr("Apply"));
    QPushButton *cancelBtn = new QPushButton(tr("Cancel"));

    okBtn->setDefault(true);

    connect(okBtn, &QPushButton::clicked, this, &GarminSettingsDialog::onOkClicked);
    connect(applyBtn, &QPushButton::clicked, this, &GarminSettingsDialog::onApplyClicked);
    connect(cancelBtn, &QPushButton::clicked, this, &GarminSettingsDialog::onCancelClicked);

    buttonLayout->addWidget(okBtn);
    buttonLayout->addWidget(applyBtn);
    buttonLayout->addWidget(cancelBtn);

    mainLayout->addLayout(buttonLayout);

    // Populate devices
    refreshDevices();
}

void GarminSettingsDialog::refreshDevices()
{
    deviceCombo->clear();
    deviceCombo->addItem(obs_module_text("GarminReplay.DefaultMicrophone"), QString(""));

    device_list_t *devices = device_enum_microphones();
    if (devices) {
        for (int i = 0; i < devices->count; i++) {
            deviceCombo->addItem(
                QString::fromUtf8(devices->devices[i].name),
                QString::fromUtf8(devices->devices[i].id)
            );
        }
        device_list_free(devices);
    }
}

void GarminSettingsDialog::loadSettings()
{
    enabledCheck->setChecked(g_plugin_data.enabled);

    // Find and select the current device
    QString currentDevice = g_plugin_data.device_id ?
        QString::fromUtf8(g_plugin_data.device_id) : QString("");
    int deviceIndex = deviceCombo->findData(currentDevice);
    if (deviceIndex >= 0) {
        deviceCombo->setCurrentIndex(deviceIndex);
    }

    // Select current language
    int langIndex = languageCombo->findData(g_plugin_data.language);
    if (langIndex >= 0) {
        languageCombo->setCurrentIndex(langIndex);
    }

    sensitivitySlider->setValue(g_plugin_data.sensitivity);
    sensitivityLabel->setText(QString::number(g_plugin_data.sensitivity));

    // Select restart mode
    int modeIndex = restartModeCombo->findData(g_plugin_data.restart_mode);
    if (modeIndex >= 0) {
        restartModeCombo->setCurrentIndex(modeIndex);
    }

    // Update status
    if (g_plugin_data.enabled && g_plugin_data.thread_running) {
        statusLabel->setText(obs_module_text("GarminReplay.StatusListening"));
        statusLabel->setStyleSheet("color: green; font-weight: bold;");
    } else if (g_plugin_data.enabled) {
        statusLabel->setText(obs_module_text("GarminReplay.StatusError"));
        statusLabel->setStyleSheet("color: red; font-weight: bold;");
    } else {
        statusLabel->setText(obs_module_text("GarminReplay.StatusDisabled"));
        statusLabel->setStyleSheet("color: gray;");
    }
}

void GarminSettingsDialog::saveSettings()
{
    bool wasEnabled = g_plugin_data.enabled;
    int oldLanguage = g_plugin_data.language;

    // Update plugin state
    g_plugin_data.enabled = enabledCheck->isChecked();
    g_plugin_data.sensitivity = sensitivitySlider->value();
    g_plugin_data.language = languageCombo->currentData().toInt();
    g_plugin_data.restart_mode = restartModeCombo->currentData().toInt();

    // Update device ID
    if (g_plugin_data.device_id) {
        bfree(g_plugin_data.device_id);
        g_plugin_data.device_id = NULL;
    }
    QString deviceId = deviceCombo->currentData().toString();
    if (!deviceId.isEmpty()) {
        g_plugin_data.device_id = bstrdup(deviceId.toUtf8().constData());
    }

    // Save to file
    garmin_save_settings();

    // Handle enable/disable and language changes
    bool needsRestart = (g_plugin_data.language != oldLanguage) && g_plugin_data.enabled;

    if (g_plugin_data.enabled && !wasEnabled) {
        start_voice_recognition();
    } else if (!g_plugin_data.enabled && wasEnabled) {
        stop_voice_recognition();
    } else if (needsRestart) {
        // Restart to load new language model
        stop_voice_recognition();
        start_voice_recognition();
    }

    // Update status display
    loadSettings();
}

void GarminSettingsDialog::onSensitivityChanged(int value)
{
    sensitivityLabel->setText(QString::number(value));
}

void GarminSettingsDialog::onOkClicked()
{
    saveSettings();
    accept();
}

void GarminSettingsDialog::onApplyClicked()
{
    saveSettings();
}

void GarminSettingsDialog::onCancelClicked()
{
    reject();
}

// C interface for the dialog
static GarminSettingsDialog *settingsDialog = nullptr;

extern "C" {

void garmin_show_settings_dialog_qt(void)
{
    // Get the main OBS window as parent
    QWidget *parent = (QWidget *)obs_frontend_get_main_window();

    if (settingsDialog) {
        // Dialog already exists, just show it
        settingsDialog->raise();
        settingsDialog->activateWindow();
        return;
    }

    settingsDialog = new GarminSettingsDialog(parent);
    settingsDialog->setAttribute(Qt::WA_DeleteOnClose);

    QObject::connect(settingsDialog, &QDialog::destroyed, []() {
        settingsDialog = nullptr;
    });

    settingsDialog->show();
}

void garmin_close_settings_dialog_qt(void)
{
    if (settingsDialog) {
        settingsDialog->close();
        settingsDialog = nullptr;
    }
}

} // extern "C"
