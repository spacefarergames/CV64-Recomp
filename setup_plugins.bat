@echo off
REM ============================================================================
REM Castlevania 64 PC Recomp - Plugin Setup Script
REM ============================================================================
REM This script downloads mupen64plus core and plugins from official sources
REM Source: https://github.com/mupen64plus/mupen64plus-core
REM ============================================================================

echo ============================================
echo  Castlevania 64 PC Recomp - Plugin Setup
echo ============================================
echo.

set PLUGINS_DIR=%~dp0plugins
set OUTPUT_DIR=%~dp0x64\Release
set TEMP_DIR=%~dp0temp

echo Creating directories...
if not exist "%PLUGINS_DIR%" mkdir "%PLUGINS_DIR%"
if not exist "%OUTPUT_DIR%" mkdir "%OUTPUT_DIR%"
if not exist "%TEMP_DIR%" mkdir "%TEMP_DIR%"

echo.
echo ============================================
echo  Downloading from Official mupen64plus
echo ============================================
echo.

REM Try to clone mupen64plus-core for building from source
echo Checking for Git...
where git >nul 2>&1
if %ERRORLEVEL% EQU 0 (
    echo Git found! Cloning mupen64plus-core repository...
    
    if not exist "%TEMP_DIR%\mupen64plus-core" (
        git clone --depth 1 https://github.com/mupen64plus/mupen64plus-core.git "%TEMP_DIR%\mupen64plus-core"
    ) else (
        echo Repository already exists, updating...
        cd /d "%TEMP_DIR%\mupen64plus-core"
        git pull
        cd /d "%~dp0"
    )
    
    echo.
    echo Repository cloned to: %TEMP_DIR%\mupen64plus-core
    echo.
    echo To build from source, you need:
    echo   - Visual Studio 2019/2022 with C++ workload
    echo   - SDL2 development libraries
    echo   - See: %TEMP_DIR%\mupen64plus-core\README.md
    echo.
) else (
    echo Git not found. Skipping source clone.
)

echo.
echo ============================================
echo  Downloading Pre-built Binaries
echo ============================================
echo.

REM Download from mupen64plus-win32-deps (contains prebuilt binaries)
echo Downloading mupen64plus Windows dependencies...
powershell -Command "& {$ProgressPreference = 'SilentlyContinue'; try { $releases = Invoke-RestMethod -Uri 'https://api.github.com/repos/mupen64plus/mupen64plus-win32-deps/releases/latest' -UseBasicParsing; Write-Host 'Latest release:' $releases.tag_name; $asset = $releases.assets | Where-Object { $_.name -like '*64*' } | Select-Object -First 1; if ($asset) { Write-Host 'Downloading:' $asset.name; Invoke-WebRequest -Uri $asset.browser_download_url -OutFile '%TEMP_DIR%\deps.zip' -UseBasicParsing; Write-Host '[OK] Downloaded' } else { Write-Host 'No x64 asset found' } } catch { Write-Host 'Download failed:' $_.Exception.Message } }"

REM Extract if downloaded
if exist "%TEMP_DIR%\deps.zip" (
    echo Extracting dependencies...
    powershell -Command "& {Expand-Archive -Path '%TEMP_DIR%\deps.zip' -DestinationPath '%TEMP_DIR%\deps' -Force}"
    
    REM Copy DLLs
    for /R "%TEMP_DIR%\deps" %%F in (*.dll) do (
        echo Found: %%~nxF
        if "%%~nxF"=="mupen64plus.dll" (
            copy /Y "%%F" "%OUTPUT_DIR%\" && echo [OK] Copied to output
            copy /Y "%%F" "%PLUGINS_DIR%\" && echo [OK] Copied to plugins
        ) else if "%%~nxF"=="SDL2.dll" (
            copy /Y "%%F" "%OUTPUT_DIR%\" && echo [OK] Copied SDL2.dll
        ) else (
            copy /Y "%%F" "%PLUGINS_DIR%\" && echo [OK] Copied to plugins
        )
    )
)

REM Download GLideN64 (best video plugin)
echo.
echo Downloading GLideN64 video plugin...
powershell -Command "& {$ProgressPreference = 'SilentlyContinue'; try { $releases = Invoke-RestMethod -Uri 'https://api.github.com/repos/gonetz/GLideN64/releases/latest' -UseBasicParsing; Write-Host 'Latest GLideN64:' $releases.tag_name; $asset = $releases.assets | Where-Object { $_.name -like '*mupen64plus*' -and $_.name -like '*win*' } | Select-Object -First 1; if ($asset) { Write-Host 'Downloading:' $asset.name; Invoke-WebRequest -Uri $asset.browser_download_url -OutFile '%TEMP_DIR%\gliden64.zip' -UseBasicParsing; Write-Host '[OK] Downloaded' } } catch { Write-Host 'GLideN64 download failed:' $_.Exception.Message } }"

if exist "%TEMP_DIR%\gliden64.zip" (
    echo Extracting GLideN64...
    powershell -Command "& {Expand-Archive -Path '%TEMP_DIR%\gliden64.zip' -DestinationPath '%TEMP_DIR%\gliden64' -Force}"
    
    for /R "%TEMP_DIR%\gliden64" %%F in (*.dll *.ini) do (
        copy /Y "%%F" "%PLUGINS_DIR%\" && echo [OK] %%~nxF
    )
)

echo.
echo Cleaning up temporary files...
rmdir /S /Q "%TEMP_DIR%\deps" 2>nul
rmdir /S /Q "%TEMP_DIR%\gliden64" 2>nul
del "%TEMP_DIR%\deps.zip" 2>nul
del "%TEMP_DIR%\gliden64.zip" 2>nul

echo.
echo ============================================
echo  Setup Summary
echo ============================================
echo.
echo Plugin sources (official mupen64plus):
echo   Core:  https://github.com/mupen64plus/mupen64plus-core
echo   RSP:   https://github.com/mupen64plus/mupen64plus-rsp-hle
echo   Audio: https://github.com/mupen64plus/mupen64plus-audio-sdl
echo   Input: https://github.com/mupen64plus/mupen64plus-input-sdl
echo   Video: https://github.com/gonetz/GLideN64
echo.
echo Required files:
echo   - mupen64plus.dll (core library)
echo   - mupen64plus-video-GLideN64.dll (graphics)
echo   - mupen64plus-audio-sdl.dll (audio)
echo   - mupen64plus-input-sdl.dll (input)
echo   - mupen64plus-rsp-hle.dll (RSP)
echo   - SDL2.dll (dependency)
echo.
echo Plugin directory: %PLUGINS_DIR%
echo Output directory: %OUTPUT_DIR%
echo.
echo If automatic download failed, please download manually
echo from the GitHub repositories listed above.
echo ============================================
pause
