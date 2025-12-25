; Inno Setup Script for OBS Garmin Voice Replay Plugin
; Download Inno Setup from: https://jrsoftware.org/isinfo.php

#define MyAppName "OBS Garmin Voice Replay"
#define MyAppVersion "1.0.0"
#define MyAppPublisher "Garmin Companion"
#define MyAppURL "https://github.com/garmin-companion"

[Setup]
; Basic info
AppId={{E8F3A2B1-5C4D-4E6F-8A9B-1C2D3E4F5A6B}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppPublisher={#MyAppPublisher}
AppPublisherURL={#MyAppURL}
AppSupportURL={#MyAppURL}
AppUpdatesURL={#MyAppURL}

; Install location - default to OBS plugins folder
DefaultDirName={autopf}\obs-studio
DirExistsWarning=no
DisableDirPage=no

; No start menu folder needed
DisableProgramGroupPage=yes

; Output settings
OutputDir=..\installer_output
OutputBaseFilename=OBS-Garmin-Voice-Replay-Setup-{#MyAppVersion}
; SetupIconFile=..\data\icon.ico  ; Uncomment if you add an icon
Compression=lzma2
SolidCompression=yes

; Require admin for Program Files
PrivilegesRequired=admin

; Modern wizard style
WizardStyle=modern
WizardSizePercent=100

; Uninstall info
UninstallDisplayIcon={app}\obs-plugins\64bit\obs-garmin-replay.dll
UninstallDisplayName={#MyAppName}

; License (optional - comment out if no license file)
; LicenseFile=..\LICENSE

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"
Name: "german"; MessagesFile: "compiler:Languages\German.isl"
Name: "french"; MessagesFile: "compiler:Languages\French.isl"

[Messages]
SelectDirLabel3=Please select your OBS Studio installation folder.
SelectDirBrowseLabel=OBS Studio is usually installed in "C:\Program Files\obs-studio". Click Next to continue, or click Browse to select a different folder.

[Types]
Name: "full"; Description: "Full installation (with all language models)"
Name: "compact"; Description: "Compact installation (English model only)"
Name: "custom"; Description: "Custom installation"; Flags: iscustom

[Components]
Name: "plugin"; Description: "Plugin files"; Types: full compact custom; Flags: fixed
Name: "models"; Description: "Voice recognition models"; Types: full compact custom
Name: "models\english"; Description: "English model (~40 MB)"; Types: full compact custom; Flags: fixed
Name: "models\german"; Description: "German model (~45 MB)"; Types: full
Name: "models\french"; Description: "French model (~41 MB)"; Types: full

[Files]
; Plugin DLL
Source: "..\build_x64\Release\obs-garmin-replay.dll"; DestDir: "{app}\obs-plugins\64bit"; Flags: ignoreversion; Components: plugin

; Vosk DLLs
Source: "..\build_x64\Release\libvosk.dll"; DestDir: "{app}\obs-plugins\64bit"; Flags: ignoreversion; Components: plugin
Source: "..\build_x64\Release\libstdc++-6.dll"; DestDir: "{app}\obs-plugins\64bit"; Flags: ignoreversion; Components: plugin
Source: "..\build_x64\Release\libgcc_s_seh-1.dll"; DestDir: "{app}\obs-plugins\64bit"; Flags: ignoreversion; Components: plugin
Source: "..\build_x64\Release\libwinpthread-1.dll"; DestDir: "{app}\obs-plugins\64bit"; Flags: ignoreversion; Components: plugin

; Locale files
Source: "..\data\locale\en-US.ini"; DestDir: "{app}\data\obs-plugins\obs-garmin-replay\locale"; Flags: ignoreversion; Components: plugin
Source: "..\data\locale\de-DE.ini"; DestDir: "{app}\data\obs-plugins\obs-garmin-replay\locale"; Flags: ignoreversion; Components: plugin
Source: "..\data\locale\fr-FR.ini"; DestDir: "{app}\data\obs-plugins\obs-garmin-replay\locale"; Flags: ignoreversion; Components: plugin

; Vosk Models
Source: "..\data\models\vosk-model-small-en-us-0.15\*"; DestDir: "{app}\data\obs-plugins\obs-garmin-replay\models\vosk-model-small-en-us-0.15"; Flags: ignoreversion recursesubdirs createallsubdirs; Components: models\english
Source: "..\data\models\vosk-model-small-de-0.15\*"; DestDir: "{app}\data\obs-plugins\obs-garmin-replay\models\vosk-model-small-de-0.15"; Flags: ignoreversion recursesubdirs createallsubdirs; Components: models\german
Source: "..\data\models\vosk-model-small-fr-0.22\*"; DestDir: "{app}\data\obs-plugins\obs-garmin-replay\models\vosk-model-small-fr-0.22"; Flags: ignoreversion recursesubdirs createallsubdirs; Components: models\french

[Code]
function InitializeSetup(): Boolean;
begin
  Result := True;
end;

function NextButtonClick(CurPageID: Integer): Boolean;
var
  OBSExe: String;
begin
  Result := True;

  // On directory selection page, verify OBS is installed there
  if CurPageID = wpSelectDir then
  begin
    OBSExe := ExpandConstant('{app}\bin\64bit\obs64.exe');
    if not FileExists(OBSExe) then
    begin
      if MsgBox('OBS Studio does not appear to be installed in this folder.' + #13#10 + #13#10 +
                'Could not find: ' + OBSExe + #13#10 + #13#10 +
                'Are you sure you want to install here?',
                mbConfirmation, MB_YESNO) = IDNO then
      begin
        Result := False;
      end;
    end;
  end;
end;

procedure CurStepChanged(CurStep: TSetupStep);
begin
  if CurStep = ssPostInstall then
  begin
    // Could add post-install tasks here
  end;
end;

[UninstallDelete]
; Clean up the plugin data folder on uninstall
Type: filesandordirs; Name: "{app}\data\obs-plugins\obs-garmin-replay"
