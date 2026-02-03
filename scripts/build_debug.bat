@echo off
setlocal enabledelayedexpansion

echo ==========================================
echo      LSAA - Build Script (Debug + NMake)
echo ==========================================

:: 1. Detection Env (BuildTools 2019 / VS 2022)
set "VS_PATH="

:: Check vswhere
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if exist "!VSWHERE!" (
    for /f "usebackq tokens=*" %%i in (`"!VSWHERE!" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do (
        set "VS_DIR=%%i"
        if exist "!VS_DIR!\VC\Auxiliary\Build\vcvars64.bat" (
            set "VS_PATH=!VS_DIR!\VC\Auxiliary\Build\vcvars64.bat"
            echo [INFO] Detected: !VS_DIR!
        )
    )
)

:: Fallback Check
if not defined VS_PATH (
    if exist "C:\Program Files (x86)\Microsoft Visual Studio\2019\BuildTools\VC\Auxiliary\Build\vcvars64.bat" (
        set "VS_PATH=C:\Program Files (x86)\Microsoft Visual Studio\2019\BuildTools\VC\Auxiliary\Build\vcvars64.bat"
    )
)

:: Load Env
if defined VS_PATH (
    if "%VSCMD_VER%"=="" (
        echo [INFO] Loading MSVC Environment...
        call "!VS_PATH!" > nul
        if errorlevel 1 goto :Fail
    )
) else (
    echo [ERROR] Visual Studio Environment not found.
    pause
    exit /b 1
)

:: 2. Build
if exist build rmdir /s /q build

echo [INFO] Generating CMake project (NMake)...
:: On force "NMake Makefiles" pour utiliser le compilateur actif sans chercher l'IDE
cmake -G "NMake Makefiles" -S . -B build
if errorlevel 1 goto :Fail

echo [INFO] Building project...
cmake --build build --config Debug
if errorlevel 1 goto :Fail

echo.
echo [SUCCESS] Build complete.
echo [INFO] Running lsaa-core.exe...
echo ------------------------------------------
if exist "build\bin\lsaa-core.exe" (
    ".\build\bin\lsaa-core.exe"
) else (
    echo [ERROR] Executable not found at build\bin\lsaa-core.exe
    dir build /s
)
echo.
echo ------------------------------------------
exit /b 0

:Fail
echo.
echo [ERROR] Build failed.
pause
exit /b 1
