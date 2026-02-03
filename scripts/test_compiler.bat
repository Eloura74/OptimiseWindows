@echo off
setlocal enabledelayedexpansion

echo ==========================================
echo      LSAA - CL Compiler Test (Fix)
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
    )
) else (
    echo [ERROR] Visual Studio Environment not found.
    pause
    exit /b 1
)

:: 2. Compilation sur fichier existant
if not exist "scripts\test.cpp" (
    echo [ERROR] scripts\test.cpp not found.
    pause
    exit /b 1
)

echo.
echo [INFO] Compiling scripts\test.cpp...
echo ------------------------------------------
cd scripts
cl test.cpp /EHsc
echo ------------------------------------------

:: 3. Verification
if exist test.exe (
    echo.
    echo [SUCCESS] Compilation successful!
    echo [INFO] Executing test.exe:
    echo ------------------------------------------
    test.exe
    echo ------------------------------------------
    del test.exe test.obj
) else (
    echo.
    echo [ERROR] Compilation failed.
)

pause
