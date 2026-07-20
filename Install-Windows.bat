@echo off
setlocal EnableExtensions EnableDelayedExpansion
title Ambient Composer AI - Windows Installer

REM ===========================================================================
REM  Ambient Composer AI - one-click build + install for Windows.
REM
REM  Double-click this file. It will (once):
REM    1. find the Visual Studio C++ tools / CMake
REM    2. build the VST3 plugin + standalone app (Release, 64-bit)
REM    3. install the VST3 into the shared VST3 folder (FL Studio scans it)
REM    4. install the standalone app + a desktop shortcut
REM
REM  Prerequisite: "Visual Studio 2022 Community" with the
REM  "Desktop development with C++" workload (free). See INSTALL-WINDOWS.txt.
REM ===========================================================================

REM --- Re-launch elevated: writing into Program Files\Common Files needs admin.
net session >nul 2>&1
if %errorlevel% neq 0 (
    echo Requesting administrator permission...
    powershell -NoProfile -Command "Start-Process -FilePath '%~f0' -Verb RunAs"
    exit /b
)

set "ROOT=%~dp0"
pushd "%ROOT%"

REM Resolve this OUTSIDE any parenthesised block: the "(x86)" parens can confuse
REM the batch parser when %ProgramFiles(x86)% appears inside an if(...) block.
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"

echo ============================================================
echo    Ambient Composer AI  -  build ^& install  (Windows)
echo ============================================================
echo.

REM --- Locate CMake: PATH first, then Visual Studio's bundled copy ------------
set "CMAKE="
where cmake >nul 2>nul && set "CMAKE=cmake"
if not defined CMAKE if exist "%VSWHERE%" (
    for /f "usebackq tokens=*" %%i in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "VSPATH=%%i"
    if defined VSPATH (
        set "VSCMAKE=!VSPATH!\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
        if exist "!VSCMAKE!" set "CMAKE=!VSCMAKE!"
    )
)

if not defined CMAKE (
    echo.
    echo ERROR: Could not find CMake / the Visual Studio C++ tools.
    echo.
    echo Please install "Visual Studio 2022 Community" ^(free^) and, in its
    echo installer, TICK the "Desktop development with C++" workload, then
    echo run this file again.
    echo    https://visualstudio.microsoft.com/downloads/
    echo.
    pause
    popd & exit /b 1
)
echo Using CMake: !CMAKE!
echo.

REM --- 1/4  Configure (downloads JUCE the first time; needs internet) ---------
echo [1/4] Configuring the project ^(first run downloads a component^)...
"!CMAKE!" -S "%ROOT%." -B "%ROOT%build" -G "Visual Studio 17 2022" -A x64
if errorlevel 1 (
    echo.
    echo Configure FAILED. If you have a different Visual Studio version,
    echo open "INSTALL-WINDOWS.txt" for help.
    pause
    popd & exit /b 1
)

REM --- 2/4  Build ------------------------------------------------------------
echo.
echo [2/4] Building ^(this takes a few minutes the first time^)...
"!CMAKE!" --build "%ROOT%build" --config Release --parallel
if errorlevel 1 (
    echo.
    echo Build FAILED. See the messages above.
    pause
    popd & exit /b 1
)

set "ARTE=%ROOT%build\AmbientComposerAI_artefacts\Release"

REM --- 3/4  Install the VST3 into the shared VST3 folder ---------------------
echo.
echo [3/4] Installing the VST3 plugin for FL Studio...
set "VST3DST=%CommonProgramFiles%\VST3\Ambient Composer AI.vst3"
robocopy "%ARTE%\VST3\Ambient Composer AI.vst3" "%VST3DST%" /E /IS /NFL /NDL /NJH /NJS >nul
if exist "%VST3DST%\Contents" (
    echo     installed: "%VST3DST%"
) else (
    echo     WARNING: the VST3 folder was not found after copying.
)

REM --- 4/4  Install the standalone app + a desktop shortcut ------------------
echo.
echo [4/4] Installing the standalone app...
set "APPDST=%LOCALAPPDATA%\Programs\Ambient Composer AI"
if not exist "%APPDST%" mkdir "%APPDST%"
copy /Y "%ARTE%\Standalone\Ambient Composer AI.exe" "%APPDST%\" >nul
powershell -NoProfile -Command "$s=(New-Object -ComObject WScript.Shell).CreateShortcut([Environment]::GetFolderPath('Desktop')+'\Ambient Composer AI.lnk'); $s.TargetPath='%APPDST%\Ambient Composer AI.exe'; $s.WorkingDirectory='%APPDST%'; $s.Save()" >nul 2>&1
echo     installed: "%APPDST%\Ambient Composer AI.exe"

echo.
echo ============================================================
echo    DONE!
echo.
echo    * VST3 installed for FL Studio.
echo    * Standalone app + desktop shortcut created.
echo.
echo    Next: open FL Studio, then
echo      Options -^> Manage plugins -^> "Find installed plugins",
echo    and add "Ambient Composer AI" to a channel.
echo ============================================================
echo.
pause
popd
endlocal
