#define AppName "StompForge"
#define AppVersion "0.0.2"
#define AppChannel "alpha"
#define AppPublisher "Vlodzimej Garlic"
#define BuildRoot "..\build\StompForge_artefacts\Release"

[Setup]
AppId={{38C13A90-8E4D-45C7-9592-7FA70B38379B}
AppName={#AppName}
AppVersion={#AppVersion}
AppVerName={#AppName} {#AppVersion} {#AppChannel}
AppPublisher={#AppPublisher}
VersionInfoVersion={#AppVersion}.0
VersionInfoCompany={#AppPublisher}
VersionInfoDescription={#AppName} audio effect installer
VersionInfoProductName={#AppName}
VersionInfoProductVersion={#AppVersion}
DefaultDirName={autopf}\{#AppName}
DefaultGroupName={#AppName}
UninstallDisplayName={#AppName} {#AppVersion}
OutputDir=..\dist
OutputBaseFilename=StompForge-{#AppVersion}-{#AppChannel}-Windows-x64-Setup
Compression=lzma2/ultra64
SolidCompression=yes
WizardStyle=modern
SetupIconFile=..\Assets\stompforge-icon.ico
UninstallDisplayIcon={app}\StompForge.exe
PrivilegesRequired=admin
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
DisableProgramGroupPage=yes
CloseApplications=yes
RestartApplications=no

[Types]
Name: "full"; Description: "VST3 and Standalone"
Name: "custom"; Description: "Custom installation"; Flags: iscustom

[Components]
Name: "vst3"; Description: "VST3 plugin"; Types: full custom; Flags: fixed
Name: "standalone"; Description: "Standalone application"; Types: full custom

[Files]
Source: "{#BuildRoot}\VST3\StompForge.vst3\*"; DestDir: "{commoncf64}\VST3\StompForge.vst3"; Components: vst3; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "{#BuildRoot}\Standalone\StompForge.exe"; DestDir: "{app}"; Components: standalone; Flags: ignoreversion
Source: "..\THIRD_PARTY_NOTICES.md"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\LICENSE"; DestDir: "{app}"; Flags: ignoreversion

[Icons]
Name: "{group}\StompForge"; Filename: "{app}\StompForge.exe"; Components: standalone
Name: "{autodesktop}\StompForge"; Filename: "{app}\StompForge.exe"; Components: standalone; Tasks: desktopicon

[Tasks]
Name: "desktopicon"; Description: "Create a desktop shortcut"; Components: standalone; Flags: unchecked

[Run]
Filename: "{app}\StompForge.exe"; Description: "Launch StompForge"; Components: standalone; Flags: nowait postinstall skipifsilent
