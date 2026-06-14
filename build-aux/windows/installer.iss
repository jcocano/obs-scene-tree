; Inno Setup installer for the Scene Tree OBS Studio plugin.
;
; Per-user install (no administrator rights) into OBS Studio's plugins folder:
;     %AppData%\obs-studio\plugins\obs-tree\bin\64bit\obs-tree.dll
;     %AppData%\obs-studio\plugins\obs-tree\data\...
;
; All configurable values are passed by Package-Windows.ps1 via /D defines;
; the fallbacks below let you also compile the script by hand for testing.

#ifndef AppName
  #define AppName "obs-tree"
#endif
#ifndef AppVersion
  #define AppVersion "0.0.0"
#endif
#ifndef SourceDir
  #define SourceDir "..\..\release\RelWithDebInfo\" + AppName
#endif
#ifndef OutputDir
  #define OutputDir "..\..\release"
#endif
#ifndef OutputBaseName
  #define OutputBaseName AppName + "-windows-x64"
#endif

[Setup]
AppId={{8E7C2A14-6F3B-4D5E-9A2C-1B0D3E4F5A6B}}
AppName=Scene Tree (OBS plugin)
AppVersion={#AppVersion}
AppVerName=Scene Tree {#AppVersion}
AppPublisher=jcocano
AppPublisherURL=https://github.com/jcocano/obs-scene-tree
AppSupportURL=https://github.com/jcocano/obs-scene-tree/issues
DefaultDirName={userappdata}\obs-studio\plugins\{#AppName}
DisableDirPage=yes
DisableProgramGroupPage=yes
PrivilegesRequired=lowest
OutputDir={#OutputDir}
OutputBaseFilename={#OutputBaseName}
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
UninstallDisplayName=Scene Tree (OBS plugin)
#ifdef SetupIcon
SetupIconFile={#SetupIcon}
UninstallDisplayIcon={app}\bin\64bit\{#AppName}.dll
#endif

[Files]
Source: "{#SourceDir}\bin\64bit\*"; DestDir: "{app}\bin\64bit"; Flags: recursesubdirs ignoreversion
Source: "{#SourceDir}\data\*"; DestDir: "{app}\data"; Flags: recursesubdirs ignoreversion

[Messages]
; Remind the user that OBS must be restarted for the dock to appear.
FinishedLabelNoIcons=Scene Tree was installed.%n%nRestart OBS Studio, then enable the "Scene Tree" dock from the Docks menu.
FinishedLabel=Scene Tree was installed.%n%nRestart OBS Studio, then enable the "Scene Tree" dock from the Docks menu.
