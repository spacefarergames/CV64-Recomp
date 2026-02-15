@echo off
REM ============================================================================
REM Castlevania 64 PC Recomp - Download mupen64plus Plugins
REM ============================================================================
REM This script downloads pre-built mupen64plus plugins from official releases
REM Source: https://github.com/mupen64plus/mupen64plus-core
REM ============================================================================

echo ============================================
echo  Download mupen64plus Plugins (Official)
echo ============================================
echo.

set OUTPUT_DIR=%~dp0x64\Release
set PLUGINS_DIR=%OUTPUT_DIR%\plugins
set TEMP_DIR=%~dp0temp

echo Creating directories...
if not exist "%OUTPUT_DIR%" mkdir "%OUTPUT_DIR%"
if not exist "%PLUGINS_DIR%" mkdir "%PLUGINS_DIR%"
if not exist "%TEMP_DIR%" mkdir "%TEMP_DIR%"

REM ============================================================================
REM Download from official mupen64plus repositories
REM ============================================================================

echo.
echo Downloading mupen64plus bundle from official repository...

REM Try to get the latest release from mupen64plus-win32-deps first (contains all prebuilt binaries)
powershell -Command "& {$ProgressPreference = 'SilentlyContinue'; try { $releases = Invoke-RestMethod -Uri 'https://api.github.com/repos/mupen64plus/mupen64plus-win32-deps/releases/latest' -UseBasicParsing; $asset = $releases.assets | Where-Object { $_.name -like '*x64*' -or $_.name -like '*64*' } | Select-Object -First 1; if ($asset) { Invoke-WebRequest -Uri $asset.browser_download_url -OutFile '%TEMP_DIR%\m64p-deps.zip' -UseBasicParsing } } catch { Write-Host 'Primary download failed, trying alternative...' } }"

REM If that fails, try the mupen64plus-bundle repository
if not exist "%TEMP_DIR%\m64p-deps.zip" (
    echo Trying mupen64plus-bundle repository...
    powershell -Command "& {$ProgressPreference = 'SilentlyContinue'; try { $releases = Invoke-RestMethod -Uri 'https://api.github.com/repos/mupen64plus/mupen64plus-bundle/releases/latest' -UseBasicParsing; $asset = $releases.assets | Where-Object { $_.name -like '*win64*' -or $_.name -like '*windows*64*' } | Select-Object -First 1; if ($asset) { Invoke-WebRequest -Uri $asset.browser_download_url -OutFile '%TEMP_DIR%\m64p-deps.zip' -UseBasicParsing } } catch { Write-Host 'Bundle download failed' } }"
)

REM Fallback to downloading individual components
if not exist "%TEMP_DIR%\m64p-deps.zip" (
    echo.
    echo Downloading individual components from mupen64plus repositories...
    
    REM Download mupen64plus-core
    echo Downloading mupen64plus-core...
    powershell -Command "& {$ProgressPreference = 'SilentlyContinue'; try { $releases = Invoke-RestMethod -Uri 'https://api.github.com/repos/mupen64plus/mupen64plus-core/releases/latest' -UseBasicParsing; $asset = $releases.assets | Where-Object { $_.name -like '*msvc*' -or $_.name -like '*win*' -or $_.name -like '*.dll' } | Select-Object -First 1; if ($asset) { Invoke-WebRequest -Uri $asset.browser_download_url -OutFile '%TEMP_DIR%\core.zip' -UseBasicParsing; Write-Host '[OK] Core downloaded' } else { Write-Host 'No suitable core asset found' } } catch { Write-Host 'Core download failed' } }"
    
    REM Download video plugin (GLideN64)
    echo Downloading GLideN64 video plugin...
    powershell -Command "& {$ProgressPreference = 'SilentlyContinue'; try { $releases = Invoke-RestMethod -Uri 'https://api.github.com/repos/gonetz/GLideN64/releases/latest' -UseBasicParsing; $asset = $releases.assets | Where-Object { $_.name -like '*mupen64plus*' -and $_.name -like '*win*' } | Select-Object -First 1; if ($asset) { Invoke-WebRequest -Uri $asset.browser_download_url -OutFile '%TEMP_DIR%\video.zip' -UseBasicParsing; Write-Host '[OK] Video plugin downloaded' } } catch { Write-Host 'Video plugin download failed' } }"
    
    REM Download RSP plugin
    echo Downloading RSP-HLE plugin...
    powershell -Command "& {$ProgressPreference = 'SilentlyContinue'; try { $releases = Invoke-RestMethod -Uri 'https://api.github.com/repos/mupen64plus/mupen64plus-rsp-hle/releases/latest' -UseBasicParsing; $asset = $releases.assets | Where-Object { $_.name -like '*msvc*' -or $_.name -like '*win*' } | Select-Object -First 1; if ($asset) { Invoke-WebRequest -Uri $asset.browser_download_url -OutFile '%TEMP_DIR%\rsp.zip' -UseBasicParsing; Write-Host '[OK] RSP plugin downloaded' } } catch { Write-Host 'RSP plugin download failed' } }"
    
    REM Download audio plugin
    echo Downloading Audio-SDL plugin...
    powershell -Command "& {$ProgressPreference = 'SilentlyContinue'; try { $releases = Invoke-RestMethod -Uri 'https://api.github.com/repos/mupen64plus/mupen64plus-audio-sdl/releases/latest' -UseBasicParsing; $asset = $releases.assets | Where-Object { $_.name -like '*msvc*' -or $_.name -like '*win*' } | Select-Object -First 1; if ($asset) { Invoke-WebRequest -Uri $asset.browser_download_url -OutFile '%TEMP_DIR%\audio.zip' -UseBasicParsing; Write-Host '[OK] Audio plugin downloaded' } } catch { Write-Host 'Audio plugin download failed' } }"
    
    REM Download input plugin
    echo Downloading Input-SDL plugin...
    powershell -Command "& {$ProgressPreference = 'SilentlyContinue'; try { $releases = Invoke-RestMethod -Uri 'https://api.github.com/repos/mupen64plus/mupen64plus-input-sdl/releases/latest' -UseBasicParsing; $asset = $releases.assets | Where-Object { $_.name -like '*msvc*' -or $_.name -like '*win*' } | Select-Object -First 1; if ($asset) { Invoke-WebRequest -Uri $asset.browser_download_url -OutFile '%TEMP_DIR%\input.zip' -UseBasicParsing; Write-Host '[OK] Input plugin downloaded' } } catch { Write-Host 'Input plugin download failed' } }"
)

REM ============================================================================
REM Extract and copy files
REM ============================================================================

echo.
echo Extracting downloaded files...

REM Extract main bundle if downloaded
if exist "%TEMP_DIR%\m64p-deps.zip" (
    powershell -Command "& {Expand-Archive -Path '%TEMP_DIR%\m64p-deps.zip' -DestinationPath '%TEMP_DIR%\m64p' -Force}"
)

REM Extract individual components
if exist "%TEMP_DIR%\core.zip" (
    powershell -Command "& {Expand-Archive -Path '%TEMP_DIR%\core.zip' -DestinationPath '%TEMP_DIR%\m64p' -Force}"
)
if exist "%TEMP_DIR%\video.zip" (
    powershell -Command "& {Expand-Archive -Path '%TEMP_DIR%\video.zip' -DestinationPath '%TEMP_DIR%\m64p' -Force}"
)
if exist "%TEMP_DIR%\rsp.zip" (
    powershell -Command "& {Expand-Archive -Path '%TEMP_DIR%\rsp.zip' -DestinationPath '%TEMP_DIR%\m64p' -Force}"
)
if exist "%TEMP_DIR%\audio.zip" (
    powershell -Command "& {Expand-Archive -Path '%TEMP_DIR%\audio.zip' -DestinationPath '%TEMP_DIR%\m64p' -Force}"
)
if exist "%TEMP_DIR%\input.zip" (
    powershell -Command "& {Expand-Archive -Path '%TEMP_DIR%\input.zip' -DestinationPath '%TEMP_DIR%\m64p' -Force}"
)

echo Copying plugins...

REM Search and copy mupen64plus core DLL
for /R "%TEMP_DIR%\m64p" %%F in (mupen64plus.dll) do (
    copy /Y "%%F" "%OUTPUT_DIR%\" 2>nul && echo [OK] mupen64plus.dll
)

REM Search and copy plugins
for /R "%TEMP_DIR%\m64p" %%F in (mupen64plus-video-*.dll GLideN64*.dll) do (
    copy /Y "%%F" "%PLUGINS_DIR%\" 2>nul && echo [OK] %%~nxF
)
for /R "%TEMP_DIR%\m64p" %%F in (mupen64plus-audio-*.dll) do (
    copy /Y "%%F" "%PLUGINS_DIR%\" 2>nul && echo [OK] %%~nxF
)
for /R "%TEMP_DIR%\m64p" %%F in (mupen64plus-input-*.dll) do (
    copy /Y "%%F" "%PLUGINS_DIR%\" 2>nul && echo [OK] %%~nxF
)
for /R "%TEMP_DIR%\m64p" %%F in (mupen64plus-rsp-*.dll) do (
    copy /Y "%%F" "%PLUGINS_DIR%\" 2>nul && echo [OK] %%~nxF
)

REM Copy SDL2.dll if found
for /R "%TEMP_DIR%\m64p" %%F in (SDL2.dll) do (
    copy /Y "%%F" "%OUTPUT_DIR%\" 2>nul && echo [OK] SDL2.dll
)

REM Copy zlib if found
for /R "%TEMP_DIR%\m64p" %%F in (zlib*.dll) do (
    copy /Y "%%F" "%OUTPUT_DIR%\" 2>nul && echo [OK] %%~nxF
)

REM Copy GLideN64 config if found
for /R "%TEMP_DIR%\m64p" %%F in (GLideN64.ini GLideN64.custom.ini) do (
    copy /Y "%%F" "%PLUGINS_DIR%\" 2>nul
)

echo.
echo Cleaning up...
rmdir /S /Q "%TEMP_DIR%" 2>nul

REM ============================================================================
REM Verify installation
REM ============================================================================

echo.
echo Verifying installation...
set MISSING=0

if not exist "%OUTPUT_DIR%\mupen64plus.dll" (
    echo [MISSING] mupen64plus.dll
    set MISSING=1
)
if not exist "%PLUGINS_DIR%\mupen64plus-video-GLideN64.dll" (
    if not exist "%PLUGINS_DIR%\GLideN64.dll" (
        echo [MISSING] Video plugin
        set MISSING=1
    )
)
if not exist "%PLUGINS_DIR%\mupen64plus-rsp-hle.dll" (
    echo [MISSING] RSP plugin
    set MISSING=1
)

if %MISSING%==1 (
    echo.
    echo ============================================
    echo  Some files could not be downloaded!
    echo ============================================
    echo.
    echo Please download manually from:
    echo   https://github.com/mupen64plus/mupen64plus-core/releases
    echo   https://github.com/mupen64plus/mupen64plus-win32-deps/releases
    echo   https://github.com/gonetz/GLideN64/releases
    echo.
    echo Required files:
    echo   mupen64plus.dll              -^> %OUTPUT_DIR%
    echo   mupen64plus-video-GLideN64.dll -^> %PLUGINS_DIR%
    echo   mupen64plus-audio-sdl.dll    -^> %PLUGINS_DIR%
    echo   mupen64plus-input-sdl.dll    -^> %PLUGINS_DIR%
    echo   mupen64plus-rsp-hle.dll      -^> %PLUGINS_DIR%
    echo.
    pause
    exit /b 1
)

echo.
echo ============================================
echo  Plugin download complete!
echo ============================================
echo.
echo Plugins source: https://github.com/mupen64plus
echo.
echo Next step: Place your Castlevania 64 ROM in:
echo   %OUTPUT_DIR%\assets\
echo.
pause
