# ============================================================================
#  Registers the background auto-update task for Ambient Composer AI.
#  Run once (the setup .bat does this for you, elevated).
#
#  The task runs the updater at logon and every hour, with highest privileges
#  and the interactive user's token - so updates are silent (no UAC) and land
#  in the machine-wide VST3 folder without ever prompting again.
# ============================================================================

$ErrorActionPreference = 'Stop'

$state   = Join-Path $env:ProgramData 'AmbientComposerAI'
$updater = Join-Path $state 'Update-AmbientComposerAI.ps1'
if (-not (Test-Path $updater)) { throw "Updater not found at $updater" }

$arg = '-NoProfile -ExecutionPolicy Bypass -WindowStyle Hidden -File "{0}"' -f $updater
$action = New-ScheduledTaskAction -Execute 'powershell.exe' -Argument $arg

# Triggers: at logon, plus every hour thereafter (indefinitely).
$trigLogon  = New-ScheduledTaskTrigger -AtLogOn
$trigHourly = New-ScheduledTaskTrigger -Once -At ((Get-Date).AddMinutes(3)) `
                -RepetitionInterval (New-TimeSpan -Hours 1)

# Run as the current (interactive) user, elevated - no stored password needed.
$principal = New-ScheduledTaskPrincipal -UserId "$env:USERDOMAIN\$env:USERNAME" `
                -LogonType Interactive -RunLevel Highest

$settings = New-ScheduledTaskSettingsSet -StartWhenAvailable `
                -AllowStartIfOnBatteries -DontStopIfGoingOnBatteries `
                -ExecutionTimeLimit (New-TimeSpan -Minutes 30)

Register-ScheduledTask -TaskName 'AmbientComposerAI-AutoUpdate' `
    -Description 'Keeps the Ambient Composer AI plugin up to date automatically.' `
    -Action $action -Trigger $trigLogon, $trigHourly `
    -Principal $principal -Settings $settings -Force | Out-Null

Write-Host 'Automatic updates enabled (task: AmbientComposerAI-AutoUpdate).'
