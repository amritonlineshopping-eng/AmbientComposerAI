@echo off
setlocal EnableExtensions
title Ambient Composer AI - Setup

REM ===========================================================================
REM  Ambient Composer AI - one-time Windows setup (auto-updating).
REM
REM  Your friend runs THIS ONE FILE once. It:
REM    1. downloads the tiny updater scripts from the public GitHub repo
REM    2. installs the current plugin build
REM    3. schedules silent automatic updates (at logon + hourly)
REM
REM  After this, every fix pushed to the repo installs itself in the
REM  background - no compiler, no reinstalling, nothing else to run.
REM ===========================================================================

set "REPO=amritonlineshopping-eng/AmbientComposerAI"
set "RAWBASE=https://raw.githubusercontent.com/%REPO%/main/windows-autoupdate"
set "STATE=%ProgramData%\AmbientComposerAI"

REM --- Re-launch elevated (needed to install into the shared plugin folder) ---
net session >nul 2>&1
if %errorlevel% neq 0 (
    echo Requesting administrator permission...
    powershell -NoProfile -Command "Start-Process -FilePath '%~f0' -Verb RunAs"
    exit /b
)

echo ============================================================
echo    Ambient Composer AI  -  one-time setup
echo ============================================================
echo.
echo Installs the plugin and a small background updater that keeps
echo it up to date automatically. You only run this once.
echo.

if not exist "%STATE%" mkdir "%STATE%"

echo [1/3] Downloading the updater...
powershell -NoProfile -ExecutionPolicy Bypass -Command "[Net.ServicePointManager]::SecurityProtocol=[Net.SecurityProtocolType]::Tls12; try { Invoke-WebRequest -UseBasicParsing '%RAWBASE%/Update-AmbientComposerAI.ps1' -OutFile '%STATE%\Update-AmbientComposerAI.ps1'; Invoke-WebRequest -UseBasicParsing '%RAWBASE%/Register-AutoUpdate.ps1' -OutFile '%STATE%\Register-AutoUpdate.ps1' } catch { exit 1 }"
if not exist "%STATE%\Update-AmbientComposerAI.ps1" (
    echo.
    echo ERROR: could not download the updater. Check your internet connection,
    echo make sure the GitHub repository is public, then run this again.
    echo.
    pause & exit /b 1
)

echo [2/3] Installing the plugin now...
powershell -NoProfile -ExecutionPolicy Bypass -File "%STATE%\Update-AmbientComposerAI.ps1"

echo [3/3] Turning on automatic updates...
powershell -NoProfile -ExecutionPolicy Bypass -File "%STATE%\Register-AutoUpdate.ps1"

echo.
echo ============================================================
echo    DONE!
echo.
echo    The plugin is installed and will keep itself up to date
echo    automatically in the background - you never run this again.
echo.
echo    Next: open FL Studio, then
echo      Options -^> Manage plugins -^> "Find installed plugins",
echo    and add "Ambient Composer AI" to a channel.
echo ============================================================
echo.
pause
endlocal
