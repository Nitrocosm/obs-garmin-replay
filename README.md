# OBS Garmin Voice Replay

A native OBS Studio plugin that saves your replay buffer using voice commands. Just say the trigger phrase and your replay is saved!

its also all vibecoded slop but this is small so it should be okay!

## Features

- **Voice-activated replay saving** - No hotkeys needed
- **Multi-language support** - English, German, and French
- **Offline recognition** - Uses Vosk for privacy-friendly local speech recognition
- **Auto-start replay buffer** - Automatically starts the buffer if it's not running
- **Settings UI** - Configure microphone, language, sensitivity, and more

## Trigger Phrases

| Language | Phrase |
|----------|--------|
| English | "save video" |
| German | "video speichern" |
| French | "enregistrer video" |

## Installation

### Option 1: Installer (Recommended)

Download the latest installer from [Releases](../../releases) and run it. The installer will:
- Detect your OBS installation
- Install the plugin and required DLLs
- Let you choose which language models to install

### Option 2: Manual Installation

1. Download the release zip
2. Extract to your OBS installation folder (e.g., `C:\Program Files\obs-studio`)
3. The folder structure should be:
   ```
   obs-studio/
   ├── obs-plugins/64bit/
   │   ├── obs-garmin-replay.dll
   │   ├── libvosk.dll
   │   ├── libgcc_s_seh-1.dll
   │   ├── libstdc++-6.dll
   │   └── libwinpthread-1.dll
   └── data/obs-plugins/obs-garmin-replay/
       ├── locale/
       │   ├── en-US.ini
       │   ├── de-DE.ini
       │   └── fr-FR.ini
       └── models/
           └── vosk-model-small-en-us-0.15/
   ```

## Usage

1. Open OBS Studio
2. Go to **Tools → Garmin Voice Replay**
3. Configure your settings:
   - Enable voice recognition
   - Select your microphone
   - Choose your language
   - Adjust sensitivity (lower = more forgiving)
   - Choose save mode (Save Only or Save and Restart)
4. Start your replay buffer
5. Say the trigger phrase to save!

## Building from Source

### Prerequisites

- **Windows 10/11** (64-bit)
- **Visual Studio 2022** with C++ desktop development workload
- **CMake 3.28+**
- **Git**

### Step 1: Clone the Repository

```bash
git clone https://github.com/nitrocosm/obs-garmin-replay.git
cd obs-garmin-replay
```

### Step 2: Download Vosk SDK

1. Download the Vosk API from: https://github.com/alphacep/vosk-api/releases
   - Get `vosk-win64-0.3.45.zip` (or latest version)
2. Extract to `deps/vosk/` so you have:
   ```
   deps/vosk/
   ├── include/
   │   └── vosk_api.h
   ├── lib/
   │   └── libvosk.lib
   └── bin/
       ├── libvosk.dll
       ├── libgcc_s_seh-1.dll
       ├── libstdc++-6.dll
       └── libwinpthread-1.dll
   ```

### Step 3: Download Vosk Models

Download the language models you need from https://alphacephei.com/vosk/models:

| Language | Model | Size |
|----------|-------|------|
| English | [vosk-model-small-en-us-0.15](https://alphacephei.com/vosk/models/vosk-model-small-en-us-0.15.zip) | ~40 MB |
| German | [vosk-model-small-de-0.15](https://alphacephei.com/vosk/models/vosk-model-small-de-0.15.zip) | ~45 MB |
| French | [vosk-model-small-fr-0.22](https://alphacephei.com/vosk/models/vosk-model-small-fr-0.22.zip) | ~41 MB |

Extract each model to `data/models/`:
```
data/models/
├── vosk-model-small-en-us-0.15/
├── vosk-model-small-de-0.15/
└── vosk-model-small-fr-0.22/
```

### Step 4: Configure and Build

```bash
# Configure (downloads OBS SDK automatically)
cmake --preset windows-x64

# Build
cmake --build build_x64 --config Release
```

The plugin will be built to `build_x64/Release/obs-garmin-replay.dll`

### Step 5: Install for Testing

Copy the built files to your OBS installation:

```bash
# Run as Administrator
install.bat
```

Or manually copy:
- `build_x64/Release/*.dll` → `C:\Program Files\obs-studio\obs-plugins\64bit\`
- `data/locale/*` → `C:\Program Files\obs-studio\data\obs-plugins\obs-garmin-replay\locale\`
- `data/models/*` → `C:\Program Files\obs-studio\data\obs-plugins\obs-garmin-replay\models\`

### Building the Installer

1. Install [Inno Setup](https://jrsoftware.org/isinfo.php)
2. Open `installer/installer.iss` in Inno Setup Compiler
3. Press F9 to build
4. Installer outputs to `installer_output/`

## Configuration

Settings are stored in: `%APPDATA%\obs-studio\plugin_config\obs-garmin-replay\garmin-replay.json`

| Setting | Description |
|---------|-------------|
| `enabled` | Enable/disable voice recognition |
| `device_id` | Microphone device ID (empty = default) |
| `sensitivity` | Recognition sensitivity 1-100 (lower = more forgiving) |
| `language` | 0 = English, 1 = German, 2 = French |
| `restart_mode` | 0 = Save only, 1 = Save and restart buffer |

## How It Works

1. The plugin captures audio from your microphone using Windows WASAPI
2. Audio is resampled to 16kHz mono and fed to Vosk
3. Vosk performs offline speech recognition (no internet required)
4. When a trigger phrase is detected, the plugin saves the replay buffer via OBS Frontend API
5. If the replay buffer isn't running, it automatically starts it

## Troubleshooting

### Plugin not loading
- Check OBS logs: **Help → Log Files → View Current Log**
- Look for `[Garmin Replay]` messages
- Ensure all DLLs are in `obs-plugins/64bit/`
- Ensure models are in `data/obs-plugins/obs-garmin-replay/models/`

### Voice not recognized
- Check your microphone is selected correctly in settings
- Try lowering the sensitivity slider (e.g., 50)
- Check the OBS log to see what Vosk is hearing vs. the trigger phrase
- Speak clearly at normal volume
- Reduce background noise

### Replay buffer not saving
- Make sure Replay Buffer is configured in **Settings → Output → Replay Buffer**
- If not running, the plugin will auto-start it - say the phrase again to save

## Support

**There is no support for this plugin.** It works on my machine. If it doesn't work on yours, that's unfortunate.

Feature requests will be ignored unless they are really funny.

## License

MIT License

## Credits

- [Vosk](https://alphacephei.com/vosk/) - Offline speech recognition
- [OBS Studio](https://obsproject.com/) - Open Broadcaster Software
