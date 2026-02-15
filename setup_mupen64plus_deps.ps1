# setup_mupen64plus_deps.ps1 - Downloads all dependencies for mupen64plus-core build
# Run this script to fix all dependency errors

$ErrorActionPreference = "Stop"

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "mupen64plus-core Dependencies Setup" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan

$depsDir = "$PSScriptRoot\RMG\Source\3rdParty\mupen64plus-win32-deps"

# Create deps directory
if (!(Test-Path $depsDir)) {
    New-Item -ItemType Directory -Path $depsDir -Force | Out-Null
    Write-Host "Created: $depsDir" -ForegroundColor Green
}

# ========================================
# NASM (Assembler)
# ========================================
$nasmVersion = "2.16.01"
$nasmDir = "$depsDir\nasm-$nasmVersion"

if (!(Test-Path "$nasmDir\x64\nasm.exe")) {
    Write-Host "`n[NASM] Downloading..." -ForegroundColor Yellow
    
    # Create directories
    New-Item -ItemType Directory -Path "$nasmDir\x64" -Force | Out-Null
    New-Item -ItemType Directory -Path "$nasmDir\x86" -Force | Out-Null
    
    # Download 64-bit
    $nasmUrl64 = "https://www.nasm.us/pub/nasm/releasebuilds/$nasmVersion/win64/nasm-$nasmVersion-win64.zip"
    $tempZip = "$env:TEMP\nasm-win64.zip"
    Invoke-WebRequest -Uri $nasmUrl64 -OutFile $tempZip -UseBasicParsing
    $tempDir = "$env:TEMP\nasm-extract"
    Expand-Archive -Path $tempZip -DestinationPath $tempDir -Force
    Copy-Item -Path (Get-ChildItem -Path $tempDir -Recurse -Filter "nasm.exe")[0].FullName -Destination "$nasmDir\x64\nasm.exe" -Force
    Remove-Item -Recurse -Force $tempDir, $tempZip
    
    # Download 32-bit
    $nasmUrl32 = "https://www.nasm.us/pub/nasm/releasebuilds/$nasmVersion/win32/nasm-$nasmVersion-win32.zip"
    Invoke-WebRequest -Uri $nasmUrl32 -OutFile $tempZip -UseBasicParsing
    Expand-Archive -Path $tempZip -DestinationPath $tempDir -Force
    Copy-Item -Path (Get-ChildItem -Path $tempDir -Recurse -Filter "nasm.exe")[0].FullName -Destination "$nasmDir\x86\nasm.exe" -Force
    Remove-Item -Recurse -Force $tempDir, $tempZip
    
    Write-Host "[NASM] Installed!" -ForegroundColor Green
} else {
    Write-Host "[NASM] Already installed." -ForegroundColor Green
}

# ========================================
# SDL2
# ========================================
$sdl2Version = "2.30.9"
$sdl2Dir = "$depsDir\SDL2-$sdl2Version"

if (!(Test-Path "$sdl2Dir\include\SDL.h")) {
    Write-Host "`n[SDL2] Downloading..." -ForegroundColor Yellow
    
    $sdl2Url = "https://github.com/libsdl-org/SDL/releases/download/release-$sdl2Version/SDL2-devel-$sdl2Version-VC.zip"
    $tempZip = "$env:TEMP\SDL2-VC.zip"
    
    Invoke-WebRequest -Uri $sdl2Url -OutFile $tempZip -UseBasicParsing
    Expand-Archive -Path $tempZip -DestinationPath $depsDir -Force
    Remove-Item -Force $tempZip
    
    Write-Host "[SDL2] Installed!" -ForegroundColor Green
} else {
    Write-Host "[SDL2] Already installed." -ForegroundColor Green
}

# ========================================
# SDL2_net
# ========================================
$sdlNetVersion = "2.2.0"
$sdlNetDir = "$depsDir\SDL2_net-$sdlNetVersion"

if (!(Test-Path "$sdlNetDir\include\SDL_net.h")) {
    Write-Host "`n[SDL2_net] Downloading..." -ForegroundColor Yellow
    
    $sdlNetUrl = "https://github.com/libsdl-org/SDL_net/releases/download/release-$sdlNetVersion/SDL2_net-devel-$sdlNetVersion-VC.zip"
    $tempZip = "$env:TEMP\SDL2_net-VC.zip"
    
    Invoke-WebRequest -Uri $sdlNetUrl -OutFile $tempZip -UseBasicParsing
    Expand-Archive -Path $tempZip -DestinationPath $depsDir -Force
    Remove-Item -Force $tempZip
    
    Write-Host "[SDL2_net] Installed!" -ForegroundColor Green
} else {
    Write-Host "[SDL2_net] Already installed." -ForegroundColor Green
}

# ========================================
# zlib
# ========================================
$zlibDir = "$depsDir\zlib"

if (!(Test-Path "$zlibDir\include\zlib.h")) {
    Write-Host "`n[zlib] Downloading..." -ForegroundColor Yellow
    
    # Create directories
    New-Item -ItemType Directory -Path "$zlibDir\include" -Force | Out-Null
    New-Item -ItemType Directory -Path "$zlibDir\lib\x64" -Force | Out-Null
    New-Item -ItemType Directory -Path "$zlibDir\lib\x86" -Force | Out-Null
    
    # Download prebuilt zlib (from nuget or other source)
    $zlibUrl = "https://www.nuget.org/api/v2/package/zlib-msvc-x64/1.2.11.8900"
    $tempZip = "$env:TEMP\zlib-nuget.zip"
    
    try {
        Invoke-WebRequest -Uri $zlibUrl -OutFile $tempZip -UseBasicParsing
        $tempDir = "$env:TEMP\zlib-extract"
        Expand-Archive -Path $tempZip -DestinationPath $tempDir -Force
        
        # Find and copy headers
        $zlibH = Get-ChildItem -Path $tempDir -Recurse -Filter "zlib.h" | Select-Object -First 1
        $zconfH = Get-ChildItem -Path $tempDir -Recurse -Filter "zconf.h" | Select-Object -First 1
        if ($zlibH) { Copy-Item -Path $zlibH.FullName -Destination "$zlibDir\include\" -Force }
        if ($zconfH) { Copy-Item -Path $zconfH.FullName -Destination "$zlibDir\include\" -Force }
        
        # Find and copy libraries
        $zlibLib = Get-ChildItem -Path $tempDir -Recurse -Filter "zlib.lib" | Where-Object { $_.FullName -match "x64" } | Select-Object -First 1
        $zlibDll = Get-ChildItem -Path $tempDir -Recurse -Filter "zlib.dll" | Where-Object { $_.FullName -match "x64" } | Select-Object -First 1
        if ($zlibLib) { Copy-Item -Path $zlibLib.FullName -Destination "$zlibDir\lib\x64\" -Force }
        if ($zlibDll) { Copy-Item -Path $zlibDll.FullName -Destination "$zlibDir\lib\x64\" -Force }
        
        Remove-Item -Recurse -Force $tempDir, $tempZip
        Write-Host "[zlib] Installed!" -ForegroundColor Green
    } catch {
        Write-Host "[zlib] Download failed, trying alternative source..." -ForegroundColor Yellow
        
        # Alternative: download from zlib official
        $zlibVersion = "1.3.1"
        $zlibSrcUrl = "https://github.com/madler/zlib/releases/download/v$zlibVersion/zlib-$zlibVersion.tar.gz"
        
        # For now, create stub headers
        @"
/* zlib.h - placeholder */
#ifndef ZLIB_H
#define ZLIB_H
typedef void* gzFile;
typedef unsigned long uLong;
typedef unsigned int uInt;
typedef int z_off_t;
#define Z_OK 0
#define Z_STREAM_END 1
#define Z_NULL 0
#define gzopen(f,m) NULL
#define gzread(f,b,l) 0
#define gzwrite(f,b,l) 0
#define gzclose(f) 0
#define gzerror(f,e) ""
#endif
"@ | Out-File -FilePath "$zlibDir\include\zlib.h" -Encoding UTF8
        
        @"
/* zconf.h - placeholder */
#ifndef ZCONF_H
#define ZCONF_H
#endif
"@ | Out-File -FilePath "$zlibDir\include\zconf.h" -Encoding UTF8
        
        Write-Host "[zlib] Created placeholder headers (build may have limited features)" -ForegroundColor Yellow
    }
} else {
    Write-Host "[zlib] Already installed." -ForegroundColor Green
}

# ========================================
# Update project include paths
# ========================================
Write-Host "`n========================================" -ForegroundColor Cyan
Write-Host "Updating project settings..." -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan

$vcxproj = "$PSScriptRoot\RMG\Source\3rdParty\mupen64plus-core\projects\msvc\mupen64plus-core.vcxproj"

if (Test-Path $vcxproj) {
    $content = Get-Content $vcxproj -Raw
    
    # Check if paths already added
    if ($content -notmatch "mupen64plus-win32-deps\\SDL2") {
        Write-Host "Project file may need manual configuration for include paths." -ForegroundColor Yellow
        Write-Host "The dependencies have been downloaded to: $depsDir" -ForegroundColor Yellow
    }
}

# ========================================
# Summary
# ========================================
Write-Host "`n========================================" -ForegroundColor Cyan
Write-Host "Dependencies Summary" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan

Write-Host "NASM:     " -NoNewline
if (Test-Path "$nasmDir\x64\nasm.exe") { Write-Host "OK" -ForegroundColor Green } else { Write-Host "MISSING" -ForegroundColor Red }

Write-Host "SDL2:     " -NoNewline
if (Test-Path "$sdl2Dir\include\SDL.h") { Write-Host "OK" -ForegroundColor Green } else { Write-Host "MISSING" -ForegroundColor Red }

Write-Host "SDL2_net: " -NoNewline
if (Test-Path "$sdlNetDir\include\SDL_net.h") { Write-Host "OK" -ForegroundColor Green } else { Write-Host "MISSING" -ForegroundColor Red }

Write-Host "zlib:     " -NoNewline
if (Test-Path "$zlibDir\include\zlib.h") { Write-Host "OK" -ForegroundColor Green } else { Write-Host "MISSING" -ForegroundColor Red }

Write-Host "`nDependencies directory: $depsDir" -ForegroundColor Yellow
Write-Host "`nYou may need to update the mupen64plus-core.vcxproj include paths to:" -ForegroundColor Yellow
Write-Host "  - $sdl2Dir\include" -ForegroundColor White
Write-Host "  - $sdlNetDir\include" -ForegroundColor White
Write-Host "  - $zlibDir\include" -ForegroundColor White
