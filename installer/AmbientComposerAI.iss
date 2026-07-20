; ============================================================================
;  Ambient Composer AI — Windows installer (Inno Setup 6)
;
;  Produces a single "download and run" setup .exe that installs:
;    * the VST3 plugin  -> C:\Program Files\Common Files\VST3
;    * the Standalone app -> C:\Program Files\Ambient Composer AI  (+ shortcuts)
;
;  Built by the GitHub Actions Windows job (see .github/workflows/build.yml):
;      ISCC.exe installer\AmbientComposerAI.iss
;  The plugin/exe are statically linked against the MSVC runtime, so the
;  target machine needs NO Visual C++ redistributable.
; ============================================================================

#define MyAppName "Ambient Composer AI"
#define MyAppVersion "1.0.0"
#define MyAppPublisher "AmritAudio"
#define MyAppExeName "Ambient Composer AI.exe"

; Build artefacts live one level up from this script (repo-root\build\...).
#define BuildDir "..\build\AmbientComposerAI_artefacts\Release"

[Setup]
AppId={{7F3C9A64-2B1E-4C7A-9E11-AC1D0BEEF001}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppPublisher={#MyAppPublisher}
DefaultDirName={autopf}\{#MyAppName}
DefaultGroupName={#MyAppName}
DisableProgramGroupPage=yes
OutputDir=Output
OutputBaseFilename=AmbientComposerAI-Setup
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
; Writing to Program Files / Common Files needs elevation.
PrivilegesRequired=admin
; This is a 64-bit plugin.
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
UninstallDisplayName={#MyAppName}

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "desktopicon"; Description: "Create a desktop shortcut for the standalone app"; GroupDescription: "Additional shortcuts:"

[Files]
; --- Standalone application ---
Source: "{#BuildDir}\Standalone\{#MyAppExeName}"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\README.md"; DestDir: "{app}"; DestName: "README.txt"; Flags: ignoreversion isreadme

; --- VST3 plugin (a folder bundle on Windows) -> Common Files\VST3 ---
Source: "{#BuildDir}\VST3\{#MyAppName}.vst3\*"; DestDir: "{commoncf}\VST3\{#MyAppName}.vst3"; \
    Flags: ignoreversion recursesubdirs createallsubdirs

[Icons]
Name: "{group}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"
Name: "{group}\Uninstall {#MyAppName}"; Filename: "{uninstallexe}"
Name: "{autodesktop}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; Tasks: desktopicon

[Run]
Filename: "{app}\{#MyAppExeName}"; Description: "Launch {#MyAppName} now"; Flags: nowait postinstall skipifsilent

[UninstallDelete]
; Remove the VST3 bundle folder on uninstall.
Type: filesandordirs; Name: "{commoncf}\VST3\{#MyAppName}.vst3"
