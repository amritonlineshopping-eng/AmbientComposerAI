# ============================================================================
#  Ambient Composer AI - installer + auto-updater (single self-contained file)
#
#  Two modes:
#    * (default)   used by the scheduled task: check the rolling release and,
#                  if it's newer than what's installed, download + install it.
#    * -Setup      first-time install AND register the background scheduled task
#                  (run once by Setup-AmbientComposerAI.bat, elevated).
#
#  Safe by design: in normal mode it never throws out of the task (always
#  exits 0); if FL Studio has the plugin open (file locked) it just logs and
#  retries next run.
# ============================================================================
param(
    [switch]$Setup
)

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

# Download with a few retries (guards against transient network hiccups).
function Get-File([string]$url, [string]$outFile) {
    for ($i = 1; $i -le 5; $i++) {
        try {
            Invoke-WebRequest -UseBasicParsing -Uri $url -OutFile $outFile
            if ((Test-Path $outFile) -and (Get-Item $outFile).Length -gt 0) { return $true }
        }
        catch { Start-Sleep -Seconds 3 }
    }
    return $false
}

# ---- the install / update step (used by both modes) ----------------------
function Install-Update {
    New-Item -ItemType Directory -Force -Path $StateDir | Out-Null

    $remote = ''
    if (Test-Path $Tmp) { Remove-Item $Tmp -Recurse -Force }
    New-Item -ItemType Directory -Force -Path $Tmp | Out-Null

    $vtxt = Join-Path $Tmp 'version.txt'
    if (-not (Get-File "$Base/version.txt" $vtxt)) { Log 'could not fetch version.txt'; return $false }
    $remote = (Get-Content $vtxt -Raw).Trim()
    if ([string]::IsNullOrWhiteSpace($remote)) { Log 'empty remote version'; return $false }

    $local = ''
    if (Test-Path $InstalledFile) { $local = (Get-Content $InstalledFile -Raw).Trim() }
    if ($remote -eq $local) { Log "up to date ($remote)"; return $true }
    Log "update available (installed='$local'  latest='$remote')"

    $zip = Join-Path $Tmp 'plugin.zip'
    if (-not (Get-File "$Base/AmbientComposerAI-VST3.zip" $zip)) { Log 'could not download plugin zip'; return $false }
    Expand-Archive -Path $zip -DestinationPath $Tmp -Force

    $srcVst3 = Join-Path $Tmp 'Ambient Composer AI.vst3'
    if (-not (Test-Path $srcVst3)) { Log 'archive missing VST3'; return $false }

    # install VST3 into the shared VST3 folder
    $vst3Dir = Join-Path $env:CommonProgramFiles 'VST3'
    New-Item -ItemType Directory -Force -Path $vst3Dir | Out-Null
    $dstVst3 = Join-Path $vst3Dir 'Ambient Composer AI.vst3'
    try {
        if (Test-Path $dstVst3) { Remove-Item $dstVst3 -Recurse -Force }
        Copy-Item $srcVst3 $dstVst3 -Recurse -Force
    }
    catch {
        Log "could not replace the VST3 (is FL Studio open?) - will retry next run: $($_.Exception.Message)"
        return $false   # do NOT record version; retry later
    }

    # standalone app (optional, per-user; non-fatal)
    $srcExe = Join-Path $Tmp 'Ambient Composer AI.exe'
    if (Test-Path $srcExe) {
        try {
            $appDir = Join-Path $env:LOCALAPPDATA 'Programs\Ambient Composer AI'
            New-Item -ItemType Directory -Force -Path $appDir | Out-Null
            Copy-Item $srcExe (Join-Path $appDir 'Ambient Composer AI.exe') -Force
        }
        catch { Log "standalone app not updated (non-fatal): $($_.Exception.Message)" }
    }

    Set-Content -Path $InstalledFile -Value $remote -NoNewline
    Remove-Item $Tmp -Recurse -Force -ErrorAction SilentlyContinue
    Log "installed $remote"
    return $true
}

# ---- register the background auto-update task (setup mode only) -----------
function Register-Task {
    $self = $PSCommandPath
    if ([string]::IsNullOrWhiteSpace($self)) { $self = Join-Path $StateDir 'AmbientComposerAI-AutoUpdate.ps1' }

    $arg = '-NoProfile -ExecutionPolicy Bypass -WindowStyle Hidden -File "{0}"' -f $self
    $action = New-ScheduledTaskAction -Execute 'powershell.exe' -Argument $arg

    $trigLogon  = New-ScheduledTaskTrigger -AtLogOn
    $trigHourly = New-ScheduledTaskTrigger -Once -At ((Get-Date).AddMinutes(3)) `
                    -RepetitionInterval (New-TimeSpan -Hours 1)

    $principal = New-ScheduledTaskPrincipal -UserId "$env:USERDOMAIN\$env:USERNAME" `
                    -LogonType Interactive -RunLevel Highest

    $settings = New-ScheduledTaskSettingsSet -StartWhenAvailable `
                    -AllowStartIfOnBatteries -DontStopIfGoingOnBatteries `
                    -ExecutionTimeLimit (New-TimeSpan -Minutes 30)

    Register-ScheduledTask -TaskName 'AmbientComposerAI-AutoUpdate' `
        -Description 'Keeps the Ambient Composer AI plugin up to date automatically.' `
        -Action $action -Trigger $trigLogon, $trigHourly `
        -Principal $principal -Settings $settings -Force | Out-Null
}

# ============================== main ======================================
if ($Setup) {
    # First-time setup: install now, then switch on automatic updates.
    # Here we DO surface failures (non-zero exit) so the .bat can report honestly.
    try {
        $ok = Install-Update
        Register-Task
        if ($ok) { Write-Host 'Installed and automatic updates enabled.'; exit 0 }
        else     { Write-Host 'Automatic updates enabled, but the plugin download did not complete (will retry automatically).'; exit 2 }
    }
    catch {
        Write-Host "Setup problem: $($_.Exception.Message)"
        exit 1
    }
}
else {
    # Scheduled-task mode: never fail the task on a transient issue.
    try { [void](Install-Update) } catch { Log "error (will retry next run): $($_.Exception.Message)" }
    exit 0
}
