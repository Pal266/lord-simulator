; Inno Setup script for Lord Simulator.
; Packages the Windows x64 packaged build into LordSimulatorSetup.exe.
; Compile with: ISCC.exe Installer\LordSimulator.iss
; The packaged build location can be overridden with /DPackagedDir="<path>".

#ifndef PackagedDir
  #define PackagedDir "..\Packaged\Windows"
#endif

#define AppName "Lord Simulator"
#define AppExeName "LordSimulator.exe"
#define AppVersion "0.1.0"

[Setup]
AppId={{6F1C2A9E-4B7D-4E2A-9C3F-1D8B5A7E3C21}
AppName={#AppName}
AppVersion={#AppVersion}
DefaultDirName={autopf}\{#AppName}
DisableProgramGroupPage=yes
OutputDir=Output
OutputBaseFilename=LordSimulatorSetup
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
Compression=lzma2
SolidCompression=yes
PrivilegesRequired=admin
UninstallDisplayName={#AppName}
UninstallDisplayIcon={app}\{#AppExeName}

[Files]
Source: "{#PackagedDir}\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs

[Run]
Filename: "{app}\Engine\Extras\Redist\en-us\UEPrereqSetup_x64.exe"; Parameters: "/quiet /norestart"; StatusMsg: "Installing Unreal Engine prerequisites..."; Flags: waituntilterminated skipifdoesntexist
