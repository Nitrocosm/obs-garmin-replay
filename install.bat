@echo off
setlocal

set OBS_DIR=C:\Program Files\obs-studio
set PLUGIN_NAME=obs-garmin-replay
set BUILD_DIR=%~dp0build_x64\Release

echo Installing %PLUGIN_NAME% to OBS Studio...
echo.

:: Check if OBS exists
if not exist "%OBS_DIR%\bin\64bit\obs64.exe" (
    echo ERROR: OBS Studio not found at %OBS_DIR%
    echo Please edit this script and set OBS_DIR to your OBS installation path.
    pause
    exit /b 1
)

:: Create directories
echo Creating directories...
mkdir "%OBS_DIR%\obs-plugins\64bit" 2>nul
mkdir "%OBS_DIR%\data\obs-plugins\%PLUGIN_NAME%\locale" 2>nul
mkdir "%OBS_DIR%\data\obs-plugins\%PLUGIN_NAME%\models" 2>nul

:: Copy plugin DLL
echo Copying plugin DLL...
copy /Y "%BUILD_DIR%\%PLUGIN_NAME%.dll" "%OBS_DIR%\obs-plugins\64bit\"

:: Copy Vosk DLLs
echo Copying Vosk DLLs...
copy /Y "%BUILD_DIR%\libvosk.dll" "%OBS_DIR%\obs-plugins\64bit\"
copy /Y "%BUILD_DIR%\libstdc++-6.dll" "%OBS_DIR%\obs-plugins\64bit\"
copy /Y "%BUILD_DIR%\libgcc_s_seh-1.dll" "%OBS_DIR%\obs-plugins\64bit\"
copy /Y "%BUILD_DIR%\libwinpthread-1.dll" "%OBS_DIR%\obs-plugins\64bit\"

:: Copy locale files
echo Copying locale files...
copy /Y "%~dp0data\locale\*.ini" "%OBS_DIR%\data\obs-plugins\%PLUGIN_NAME%\locale\"

:: Copy models if they exist
if exist "%~dp0data\models" (
    echo Copying Vosk models...
    xcopy /E /I /Y "%~dp0data\models" "%OBS_DIR%\data\obs-plugins\%PLUGIN_NAME%\models\"
) else (
    echo.
    echo WARNING: No Vosk models found in data\models\
    echo Please download models from https://alphacephei.com/vosk/models
    echo and extract them to: %OBS_DIR%\data\obs-plugins\%PLUGIN_NAME%\models\
    echo.
    echo Recommended models:
    echo   - vosk-model-small-en-us-0.15 ^(English^)
    echo   - vosk-model-small-de-0.15 ^(German^)
    echo   - vosk-model-small-fr-0.22 ^(French^)
)

echo.
echo Installation complete!
echo.
echo To use the plugin:
echo 1. Start OBS Studio
echo 2. Go to Tools ^> Garmin Voice Replay
echo 3. Enable the plugin and configure your microphone
echo 4. Start your Replay Buffer
echo 5. Say "Garmin, save video" to save your replay!
echo.
pause
