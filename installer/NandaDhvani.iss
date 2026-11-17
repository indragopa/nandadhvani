#define AppName "NandaDhvani"
#ifndef AppVersion
  #define AppVersion "0.1.0"
#endif
#define AppPublisher "NandaDhvani contributors"
#define AppExeName "NandaDhvani.exe"

[Setup]
AppId={{0B4B246E-1A65-41EC-9139-5D4F413CA3C8}
AppName={#AppName}
AppVersion={#AppVersion}
AppPublisher={#AppPublisher}
DefaultDirName={autopf}\NandaDhvani
DefaultGroupName=NandaDhvani
OutputDir=output
OutputBaseFilename=NandaDhvani-Setup
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
ArchitecturesInstallIn64BitMode=x64
PrivilegesRequired=lowest
UninstallDisplayIcon={app}\{#AppExeName}

[Files]
Source: "..\dist\NandaDhvani-Portable\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs

[Icons]
Name: "{group}\NandaDhvani"; Filename: "{app}\{#AppExeName}"
Name: "{autodesktop}\NandaDhvani"; Filename: "{app}\{#AppExeName}"; Tasks: desktopicon

[Tasks]
Name: "desktopicon"; Description: "Create a &desktop shortcut"; GroupDescription: "Additional icons:"; Flags: unchecked

[Run]
Filename: "{app}\{#AppExeName}"; Description: "Launch NandaDhvani"; Flags: postinstall nowait skipifsilent
