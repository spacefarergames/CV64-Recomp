@echo off
REM ============================================================================
REM Castlevania 64 PC Recomp - Build and Test Script
REM ============================================================================
REM This script builds the project and sets up for testing
REM ============================================================================

echo ============================================
echo  Castlevania 64 PC Recomp - Build ^& Test
echo ============================================
echo.

set PROJECT_DIR=%~dp0
set OUTPUT_DIR=%PROJECT_DIR%x64\Release
set PLUGINS_DIR=%OUTPUT_DIR%\plugins
set ASSETS_DIR=%OUTPUT_DIR%\assets

REM Build the project
echo [1/4] Building project...
msbuild CV64_RMG.sln /p:Configuration=Release /p:Platform=x64 /verbosity:minimal
if %ERRORLEVEL% neq 0 (
    echo Build failed!
    pause
    exit /b 1
)
echo Build successful!
echo.

REM Create directories
echo [2/4] Creating directories...
if not exist "%OUTPUT_DIR%" mkdir "%OUTPUT_DIR%"
if not exist "%PLUGINS_DIR%" mkdir "%PLUGINS_DIR%"
if not exist "%ASSETS_DIR%" mkdir "%ASSETS_DIR%"
echo Done.
echo.

REM Copy plugins
echo [3/4] Setting up plugins...
powershell -ExecutionPolicy Bypass -File "%PROJECT_DIR%scripts\copy_plugins.ps1" -Configuration Release
echo.

REM Copy patch configs
echo [4/4] Copying configuration files...
if exist "%PROJECT_DIR%patches" (
    copy /Y "%PROJECT_DIR%patches\*.ini" "%OUTPUT_DIR%\" >nul
    echo Configuration files copied.
) else (
    echo No patch configs found.
)
echo.

echo ============================================
echo  Build Complete!
echo ============================================
echo.
echo Output directory: %OUTPUT_DIR%
echo.
echo To test:
echo   1. Place your Castlevania 64 ROM in:
echo      %ASSETS_DIR%
echo.
echo   2. Run: %OUTPUT_DIR%\CV64_RMG.exe
echo.
echo Expected ROM names:
echo   - Castlevania (U) (V1.2) [!].z64  (recommended)
echo   - Castlevania (USA).z64
echo   - Castlevania.z64
echo.
pause
