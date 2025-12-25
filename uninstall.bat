@echo off
echo ============================================
echo  OBS Garmin Voice Replay - Uninstaller
echo ============================================
echo.

set "OBS_DIR=C:\Program Files\obs-studio"

echo This will remove the Garmin Voice Replay plugin from:
echo %OBS_DIR%
echo.
pause

echo.
echo Removing plugin DLLs...
del /f /q "%OBS_DIR%\obs-plugins\64bit\obs-garmin-replay.dll" 2>nul
del /f /q "%OBS_DIR%\obs-plugins\64bit\libvosk.dll" 2>nul
del /f /q "%OBS_DIR%\obs-plugins\64bit\libstdc++-6.dll" 2>nul
del /f /q "%OBS_DIR%\obs-plugins\64bit\libgcc_s_seh-1.dll" 2>nul
del /f /q "%OBS_DIR%\obs-plugins\64bit\libwinpthread-1.dll" 2>nul

echo Removing plugin data folder...
rmdir /s /q "%OBS_DIR%\data\obs-plugins\obs-garmin-replay" 2>nul

echo Removing user settings...
rmdir /s /q "%APPDATA%\obs-studio\plugin_config\obs-garmin-replay" 2>nul

echo.
echo ============================================
echo  Uninstall complete!
echo ============================================
echo.
echo Removed:
echo  - Plugin DLLs from obs-plugins\64bit
echo  - Plugin data (models, locales)
echo  - User settings
echo.
pause
