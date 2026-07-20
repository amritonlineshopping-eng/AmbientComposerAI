# ============================================================================
#  Ambient Composer AI - auto-updater
#
#  Runs silently in the background (via a scheduled task). Checks the repo's
#  rolling "windows-latest" release; if it's newer than what's installed, it
#  downloads the plugin and swaps it into the shared VST3 folder. FL Studio
#  loads the new plugin the next time it opens.
#
#  Safe by design: never throws out of the task (always exits 0), and if FL
#  Studio has the plugin open (file locked) it just logs and retries next run.
# ============================================================================

$ErrorActionPreference = 'Stop'

$Repo = 'amritonlineshopping-eng/AmbientComposerAI'
$Tag  = 'windows-latest'
$Base = "https://github.com/$Repo/releases/download/$Tag"

$StateDir      = Join-Path $env:ProgramData 'AmbientComposerAI'
$InstalledFile = Join-Path $StateDir 'installed.txt'
$LogFile       = Join-Path $StateDir 'update.log'
$Tmp           = Join-Path $StateDir 'tmp'

function Log([string]$m) {
    $line = ('{0}  {1}' -f (Get-Date -Format 'yyyy-MM-dd HH:mm:ss'), $m)
    try { Add-Content -Path $LogFile -Value $line } catch { }
}

try {
    # GitHub requires TLS 1.2 (Windows PowerShell 5.1 may default to older).
    [Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12
    New-Item -ItemType Directory -Force -Path $StateDir | Out-Null

    # --- 1. what version is published? ------------------------------------
    $remote = (Invoke-WebRequest -UseBasicParsing -Uri "$Base/version.txt").Content.Trim()
    if ([string]::IsNullOrWhiteSpace($remote)) { Log 'no remote version found; skipping'; exit 0 }

    # --- 2. what is installed locally? ------------------------------------
    $local = ''
    if (Test-Path $InstalledFile) { $local = (Get-Content $InstalledFile -Raw).Trim() }

    if ($remote -eq $local) { Log "up to date ($remote)"; exit 0 }
    Log "update available (installed='$local'  latest='$remote')"

    # --- 3. download + extract --------------------------------------------
    if (Test-Path $Tmp) { Remove-Item $Tmp -Recurse -Force }
    New-Item -ItemType Directory -Force -Path $Tmp | Out-Null
    $zip = Join-Path $Tmp 'plugin.zip'
    Invoke-WebRequest -UseBasicParsing -Uri "$Base/AmbientComposerAI-VST3.zip" -OutFile $zip
    Expand-Archive -Path $zip -DestinationPath $Tmp -Force

    $srcVst3 = Join-Path $Tmp 'Ambient Composer AI.vst3'
    if (-not (Test-Path $srcVst3)) { Log 'downloaded archive did not contain the VST3; aborting'; exit 0 }

    # --- 4. install the VST3 into the shared VST3 folder ------------------
    $vst3Dir = Join-Path $env:CommonProgramFiles 'VST3'
    New-Item -ItemType Directory -Force -Path $vst3Dir | Out-Null
    $dstVst3 = Join-Path $vst3Dir 'Ambient Composer AI.vst3'
    try {
        if (Test-Path $dstVst3) { Remove-Item $dstVst3 -Recurse -Force }
        Copy-Item $srcVst3 $dstVst3 -Recurse -Force
    }
    catch {
        # Most likely FL Studio is open and holding the plugin DLL. Leave the
        # installed version unchanged so we retry on the next run.
        Log "could not replace the VST3 (is FL Studio open?) - will retry next run: $($_.Exception.Message)"
        exit 0
    }

    # --- 5. standalone app (optional, per-user; non-fatal) ----------------
    $srcExe = Join-Path $Tmp 'Ambient Composer AI.exe'
    if (Test-Path $srcExe) {
        try {
            $appDir = Join-Path $env:LOCALAPPDATA 'Programs\Ambient Composer AI'
            New-Item -ItemType Directory -Force -Path $appDir | Out-Null
            Copy-Item $srcExe (Join-Path $appDir 'Ambient Composer AI.exe') -Force
        }
        catch { Log "standalone app not updated (non-fatal): $($_.Exception.Message)" }
    }

    # --- 6. record success ------------------------------------------------
    Set-Content -Path $InstalledFile -Value $remote -NoNewline
    Remove-Item $Tmp -Recurse -Force -ErrorAction SilentlyContinue
    Log "installed $remote"
    exit 0
}
catch {
    # Never fail the scheduled task on a transient issue (e.g. offline).
    Log "error (will retry next run): $($_.Exception.Message)"
    exit 0
}
