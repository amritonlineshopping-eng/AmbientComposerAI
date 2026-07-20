@echo off
setlocal EnableExtensions
title Ambient Composer AI - Setup

REM ===========================================================================
REM  Ambient Composer AI - one-time Windows setup (auto-updating).
REM
REM  Your friend runs THIS ONE FILE once. It downloads a single self-contained
REM  updater script, installs the current plugin build, and switches on silent
REM  automatic updates. After this, fixes install themselves in the background.
REM ===========================================================================

set "REPO=amritonlineshopping-eng/AmbientComposerAI"
set "RELBASE=https://github.com/%REPO%/releases/download/windows-latest"
set "STATE=%ProgramData%\AmbientComposerAI"
set "SCRIPT=%STATE%\AmbientComposerAI-AutoUpdate.ps1"

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

echo [1/2] Downloading the installer/updater...
powershell -NoProfile -ExecutionPolicy Bypass -Command "$ErrorActionPreference='Stop'; [Net.ServicePointManager]::SecurityProtocol=[Net.SecurityProtocolType]::Tls12; $u='%RELBASE%/AmbientComposerAI-AutoUpdate.ps1'; $o='%SCRIPT%'; for($i=1;$i -le 6;$i++){ try{ Invoke-WebRequest -UseBasicParsing $u -OutFile $o; if((Test-Path $o) -and (Get-Item $o).Length -gt 0){ exit 0 } }catch{ Start-Sleep -Seconds 3 } }; exit 1"

if not exist "%SCRIPT%" (
    echo.
    echo ERROR: could not download the installer.
    echo Please check your internet connection and run this file again.
    echo.
    pause & exit /b 1
)

echo [2/2] Installing the plugin and turning on automatic updates...
echo       (this can take a minute the first time)
powershell -NoProfile -ExecutionPolicy Bypass -File "%SCRIPT%" -Setup
set "SETUPRC=%errorlevel%"

echo.
REM --- verify the result honestly ---
set "TASKOK=0"
schtasks /Query /TN "AmbientComposerAI-AutoUpdate" >nul 2>&1 && set "TASKOK=1"
set "PLUGOK=0"
if exist "%CommonProgramFiles%\VST3\Ambient Composer AI.vst3\Contents" set "PLUGOK=1"

echo ============================================================
if "%PLUGOK%"=="1" if "%TASKOK%"=="1" (
    echo    DONE - everything is set up!
    echo.
    echo    * The plugin is installed.
    echo    * It will keep itself up to date automatically - you
    echo      never need to run this again.
    echo.
    echo    Next: open FL Studio, then
    echo      Options -^> Manage plugins -^> "Find installed plugins",
    echo    and add "Ambient Composer AI" to a channel.
    echo ============================================================
    echo.
    pause & endlocal & exit /b 0
)

REM --- something didn't fully complete: say so clearly ---
echo    NOT FULLY FINISHED - here's what happened:
echo.
if "%PLUGOK%"=="1" ( echo    * Plugin installed: YES ) else ( echo    * Plugin installed: NO )
if "%TASKOK%"=="1" ( echo    * Automatic updates: ON  ) else ( echo    * Automatic updates: NOT set up )
echo.
echo    Please make sure you are connected to the internet and simply
echo    run this file again - running it more than once is completely safe.
echo    If it keeps failing, send back a photo of this window.
echo ============================================================
echo.
pause
endlocal
