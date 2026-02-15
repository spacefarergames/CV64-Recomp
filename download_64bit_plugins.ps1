#!/usr/bin/env pwsh
# Download 64-bit mupen64plus plugins including GLideN64
# This script downloads ONLY 64-bit versions

$ErrorActionPreference = "Continue"

Write-Host ""
Write-Host "============================================" -ForegroundColor Cyan
Write-Host " Downloading 64-bit mupen64plus Plugins" -ForegroundColor Cyan
Write-Host "============================================" -ForegroundColor Cyan
Write-Host ""

# Create directories
$outputDir = "x64\Release"
$pluginsDir = "plugins"
New-Item -ItemType Directory -Force -Path $outputDir | Out-Null
New-Item -ItemType Directory -Force -Path $pluginsDir | Out-Null

# Clean up old 32-bit DLLs
Write-Host "Cleaning up old DLLs..." -ForegroundColor Yellow
Remove-Item "$outputDir\*.dll" -Force -ErrorAction SilentlyContinue
Remove-Item "$pluginsDir\*.dll" -Force -ErrorAction SilentlyContinue

# URLs for 64-bit downloads
$gliden64Url = "https://github.com/gonetz/GLideN64/releases/download/Public_Release_4.0/GLideN64_Public_Release_4.0.zip"
$m64pCoreUrl = "https://github.com/mupen64plus/mupen64plus-core/releases/download/2.5.9/mupen64plus-bundle-win64-2.5.9.zip"
$rspHleUrl = "https://github.com/mupen64plus/mupen64plus-rsp-hle/releases/download/2.5.9/mupen64plus-rsp-hle-win64-2.5.9.zip"
$audioSdlUrl = "https://github.com/mupen64plus/mupen64plus-audio-sdl/releases/download/2.5.9/mupen64plus-audio-sdl-win64-2.5.9.zip"
$inputSdlUrl = "https://github.com/mupen64plus/mupen64plus-input-sdl/releases/download/2.5.9/mupen64plus-input-sdl-win64-2.5.9.zip"

# Alternative: m64p project has pre-built 64-bit bundles
$m64pBundleUrl = "https://m64p.github.io/m64p/m64p-win64.zip"

$tempDir = "$env:TEMP\m64p_downloads"
New-Item -ItemType Directory -Force -Path $tempDir | Out-Null

function Download-And-Extract {
    param($url, $name, $targetDir, $filter)
    
    Write-Host "Downloading $name..." -ForegroundColor Yellow
    $zipPath = "$tempDir\$name.zip"
    
    try {
        [Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12
        $ProgressPreference = 'SilentlyContinue'
        Invoke-WebRequest -Uri $url -OutFile $zipPath -UseBasicParsing
        
        if (Test-Path $zipPath) {
            Write-Host "  Extracting..." -ForegroundColor Gray
            $extractDir = "$tempDir\$name"
            Expand-Archive -Path $zipPath -DestinationPath $extractDir -Force
            
            # Copy matching files
            Get-ChildItem -Path $extractDir -Recurse -Filter $filter | ForEach-Object {
                Copy-Item $_.FullName -Destination $targetDir -Force
                Write-Host "  [OK] $($_.Name)" -ForegroundColor Green
            }
            return $true
        }
    } catch {
        Write-Host "  [FAILED] $($_.Exception.Message)" -ForegroundColor Red
        return $false
    }
    return $false
}

# Download GLideN64 (64-bit)
Write-Host ""
Write-Host "=== GLideN64 Video Plugin ===" -ForegroundColor Cyan
$gliden64Downloaded = $false

# Try Public Release 4.0
try {
    Write-Host "Trying GLideN64 Public Release 4.0..." -ForegroundColor Yellow
    $zipPath = "$tempDir\gliden64.zip"
    [Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12
    $ProgressPreference = 'SilentlyContinue'
    Invoke-WebRequest -Uri $gliden64Url -OutFile $zipPath -UseBasicParsing
    
    if (Test-Path $zipPath) {
        $extractDir = "$tempDir\gliden64"
        Expand-Archive -Path $zipPath -DestinationPath $extractDir -Force
        
        # Find the 64-bit DLL (usually in a subfolder)
        $gliden64Dll = Get-ChildItem -Path $extractDir -Recurse -Filter "*GLideN64*.dll" | 
                       Where-Object { $_.FullName -match "x64|64|win64" -or $_.Directory.Name -match "x64|64|win64" } |
                       Select-Object -First 1
        
        if (!$gliden64Dll) {
            # Just get any GLideN64 dll
            $gliden64Dll = Get-ChildItem -Path $extractDir -Recurse -Filter "*GLideN64*.dll" | Select-Object -First 1
        }
        
        if ($gliden64Dll) {
            # Copy DLL
            Copy-Item $gliden64Dll.FullName -Destination "$outputDir\mupen64plus-video-GLideN64.dll" -Force
            Write-Host "  [OK] mupen64plus-video-GLideN64.dll" -ForegroundColor Green
            
            # Copy INI files
            Get-ChildItem -Path $extractDir -Recurse -Filter "*.ini" | ForEach-Object {
                Copy-Item $_.FullName -Destination $outputDir -Force
                Write-Host "  [OK] $($_.Name)" -ForegroundColor Green
            }
            $gliden64Downloaded = $true
        }
    }
} catch {
    Write-Host "  [FAILED] $($_.Exception.Message)" -ForegroundColor Red
}

# If GLideN64 failed, try alternative source
if (!$gliden64Downloaded) {
    Write-Host "Trying alternative GLideN64 source..." -ForegroundColor Yellow
    try {
        # Try nightly/continuous build
        $altUrl = "https://github.com/gonetz/GLideN64/releases/latest/download/GLideN64_Windows.zip"
        Invoke-WebRequest -Uri $altUrl -OutFile "$tempDir\gliden64_alt.zip" -UseBasicParsing
        Expand-Archive -Path "$tempDir\gliden64_alt.zip" -DestinationPath "$tempDir\gliden64_alt" -Force
        
        Get-ChildItem -Path "$tempDir\gliden64_alt" -Recurse -Filter "*.dll" | ForEach-Object {
            if ($_.Name -match "GLideN64") {
                Copy-Item $_.FullName -Destination "$outputDir\mupen64plus-video-GLideN64.dll" -Force
                Write-Host "  [OK] mupen64plus-video-GLideN64.dll" -ForegroundColor Green
                $gliden64Downloaded = $true
            }
        }
        Get-ChildItem -Path "$tempDir\gliden64_alt" -Recurse -Filter "*.ini" | ForEach-Object {
            Copy-Item $_.FullName -Destination $outputDir -Force
            Write-Host "  [OK] $($_.Name)" -ForegroundColor Green
        }
    } catch {
        Write-Host "  Alternative also failed" -ForegroundColor Red
    }
}

# Download mupen64plus core (64-bit bundle)
Write-Host ""
Write-Host "=== mupen64plus Core & Plugins ===" -ForegroundColor Cyan

# Try official bundle first
try {
    Write-Host "Downloading official 64-bit bundle..." -ForegroundColor Yellow
    $bundleZip = "$tempDir\m64p-bundle.zip"
    Invoke-WebRequest -Uri $m64pCoreUrl -OutFile $bundleZip -UseBasicParsing
    
    if (Test-Path $bundleZip) {
        $extractDir = "$tempDir\m64p-bundle"
        Expand-Archive -Path $bundleZip -DestinationPath $extractDir -Force
        
        # Copy core and dependencies
        Get-ChildItem -Path $extractDir -Recurse -Filter "mupen64plus.dll" | Select-Object -First 1 | ForEach-Object {
            Copy-Item $_.FullName -Destination $outputDir -Force
            Write-Host "  [OK] mupen64plus.dll (CORE)" -ForegroundColor Green
        }
        
        Get-ChildItem -Path $extractDir -Recurse -Filter "SDL2.dll" | Select-Object -First 1 | ForEach-Object {
            Copy-Item $_.FullName -Destination $outputDir -Force
            Write-Host "  [OK] SDL2.dll" -ForegroundColor Green
        }
        
        # Try zlib variants
        Get-ChildItem -Path $extractDir -Recurse -Filter "zlib*.dll" | Select-Object -First 1 | ForEach-Object {
            Copy-Item $_.FullName -Destination $outputDir -Force
            Write-Host "  [OK] $($_.Name)" -ForegroundColor Green
        }
        
        # RSP plugin
        Get-ChildItem -Path $extractDir -Recurse -Filter "*rsp*.dll" | Select-Object -First 1 | ForEach-Object {
            Copy-Item $_.FullName -Destination $outputDir -Force
            Write-Host "  [OK] $($_.Name) (RSP)" -ForegroundColor Green
        }
        
        # Audio plugin
        Get-ChildItem -Path $extractDir -Recurse -Filter "*audio*.dll" | Select-Object -First 1 | ForEach-Object {
            Copy-Item $_.FullName -Destination $outputDir -Force
            Write-Host "  [OK] $($_.Name) (AUDIO)" -ForegroundColor Green
        }
        
        # Input plugin
        Get-ChildItem -Path $extractDir -Recurse -Filter "*input*.dll" | Select-Object -First 1 | ForEach-Object {
            Copy-Item $_.FullName -Destination $outputDir -Force
            Write-Host "  [OK] $($_.Name) (INPUT)" -ForegroundColor Green
        }
    }
} catch {
    Write-Host "  [FAILED] $($_.Exception.Message)" -ForegroundColor Red
}

# Also try m64p project bundle as backup
Write-Host ""
Write-Host "=== Checking m64p project for missing files ===" -ForegroundColor Cyan
try {
    $m64pZip = "$tempDir\m64p-win64.zip"
    Invoke-WebRequest -Uri $m64pBundleUrl -OutFile $m64pZip -UseBasicParsing -ErrorAction SilentlyContinue
    
    if (Test-Path $m64pZip) {
        $extractDir = "$tempDir\m64p-project"
        Expand-Archive -Path $m64pZip -DestinationPath $extractDir -Force
        
        # Copy any missing DLLs
        if (!(Test-Path "$outputDir\mupen64plus.dll")) {
            Get-ChildItem -Path $extractDir -Recurse -Filter "mupen64plus.dll" | Select-Object -First 1 | ForEach-Object {
                Copy-Item $_.FullName -Destination $outputDir -Force
                Write-Host "  [OK] mupen64plus.dll (from m64p)" -ForegroundColor Green
            }
        }
        
        if (!(Test-Path "$outputDir\SDL2.dll")) {
            Get-ChildItem -Path $extractDir -Recurse -Filter "SDL2.dll" | Select-Object -First 1 | ForEach-Object {
                Copy-Item $_.FullName -Destination $outputDir -Force
                Write-Host "  [OK] SDL2.dll (from m64p)" -ForegroundColor Green
            }
        }
    }
} catch {
    Write-Host "  m64p backup download skipped" -ForegroundColor Gray
}

# Verify downloads
Write-Host ""
Write-Host "============================================" -ForegroundColor Cyan
Write-Host " Verification" -ForegroundColor Cyan
Write-Host "============================================" -ForegroundColor Cyan
Write-Host ""

$requiredFiles = @{
    "mupen64plus.dll" = "Core emulator"
    "mupen64plus-video-GLideN64.dll" = "Video plugin (GLideN64)"
    "mupen64plus-rsp-hle.dll" = "RSP plugin"
    "SDL2.dll" = "SDL2 dependency"
}

$optionalFiles = @{
    "mupen64plus-audio-sdl.dll" = "Audio plugin"
    "mupen64plus-input-sdl.dll" = "Input plugin"
    "zlib1.dll" = "zlib dependency"
    "GLideN64.ini" = "GLideN64 config"
}

Write-Host "Required files:" -ForegroundColor White
$allRequired = $true
foreach ($file in $requiredFiles.Keys) {
    $path = Join-Path $outputDir $file
    if (Test-Path $path) {
        $size = (Get-Item $path).Length
        $arch = if ($size -gt 500KB) { "64-bit" } else { "32-bit?" }
        Write-Host "  [OK] $file ($arch)" -ForegroundColor Green
    } else {
        Write-Host "  [MISSING] $file - $($requiredFiles[$file])" -ForegroundColor Red
        $allRequired = $false
    }
}

Write-Host ""
Write-Host "Optional files:" -ForegroundColor White
foreach ($file in $optionalFiles.Keys) {
    $path = Join-Path $outputDir $file
    if (Test-Path $path) {
        Write-Host "  [OK] $file" -ForegroundColor Green
    } else {
        Write-Host "  [---] $file" -ForegroundColor Yellow
    }
}

Write-Host ""
if ($allRequired) {
    Write-Host "============================================" -ForegroundColor Green
    Write-Host " All required 64-bit plugins downloaded!" -ForegroundColor Green
    Write-Host "============================================" -ForegroundColor Green
} else {
    Write-Host "============================================" -ForegroundColor Red
    Write-Host " Some required files are missing!" -ForegroundColor Red
    Write-Host " Manual download may be required." -ForegroundColor Red
    Write-Host "============================================" -ForegroundColor Red
    Write-Host ""
    Write-Host "Manual download links:" -ForegroundColor Yellow
    Write-Host "  GLideN64: https://github.com/gonetz/GLideN64/releases" -ForegroundColor Cyan
    Write-Host "  mupen64plus: https://github.com/mupen64plus/mupen64plus-core/releases" -ForegroundColor Cyan
    Write-Host "  m64p bundle: https://m64p.github.io/" -ForegroundColor Cyan
}

Write-Host ""

# Cleanup
Remove-Item -Path $tempDir -Recurse -Force -ErrorAction SilentlyContinue
