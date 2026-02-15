#!/usr/bin/env pwsh
# Download 64-bit mupen64plus plugins with GLideN64
# Handles .7z files properly

$ErrorActionPreference = "Stop"
[Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12

Write-Host ""
Write-Host "============================================" -ForegroundColor Cyan
Write-Host " Downloading 64-bit Plugins + GLideN64" -ForegroundColor Cyan  
Write-Host "============================================" -ForegroundColor Cyan
Write-Host ""

$outputDir = "x64\Release"
New-Item -ItemType Directory -Force -Path $outputDir | Out-Null

$tempDir = "$env:TEMP\m64p_download_$(Get-Random)"
New-Item -ItemType Directory -Force -Path $tempDir | Out-Null

# Check for 7zip
$7zPath = $null
if (Test-Path "C:\Program Files\7-Zip\7z.exe") {
    $7zPath = "C:\Program Files\7-Zip\7z.exe"
} elseif (Test-Path "C:\Program Files (x86)\7-Zip\7z.exe") {
    $7zPath = "C:\Program Files (x86)\7-Zip\7z.exe"
} elseif (Get-Command 7z -ErrorAction SilentlyContinue) {
    $7zPath = "7z"
}

# =============================================
# 1. Download GLideN64 (64-bit)
# =============================================
Write-Host "=== Downloading GLideN64 ===" -ForegroundColor Yellow

$gliden64Url = "https://github.com/gonetz/GLideN64/releases/download/Public_Release_4_0/GLideN64_Public_Release_4.0.7z"
$gliden64File = "$tempDir\GLideN64.7z"

try {
    Write-Host "  Downloading from GitHub releases..." -ForegroundColor Gray
    $ProgressPreference = 'SilentlyContinue'
    Invoke-WebRequest -Uri $gliden64Url -OutFile $gliden64File -UseBasicParsing
    
    if (Test-Path $gliden64File) {
        Write-Host "  Download complete!" -ForegroundColor Green
        
        # Extract
        $extractDir = "$tempDir\GLideN64"
        
        if ($7zPath) {
            Write-Host "  Extracting with 7-Zip..." -ForegroundColor Gray
            & $7zPath x $gliden64File -o"$extractDir" -y | Out-Null
        } else {
            Write-Host "  7-Zip not found, trying PowerShell..." -ForegroundColor Yellow
            # Try using .NET to handle 7z (won't work for all 7z files)
            Write-Host "  ERROR: 7-Zip required for .7z files" -ForegroundColor Red
            Write-Host "  Please install 7-Zip from: https://www.7-zip.org/" -ForegroundColor Cyan
            Write-Host "  Or manually extract: $gliden64File" -ForegroundColor Cyan
        }
        
        if (Test-Path $extractDir) {
            # Find and copy the 64-bit Windows DLL
            $win64Dir = Get-ChildItem -Path $extractDir -Recurse -Directory | Where-Object { $_.Name -match "Windows.*64|Win64|x64" } | Select-Object -First 1
            
            if ($win64Dir) {
                Get-ChildItem -Path $win64Dir.FullName -Filter "*.dll" | ForEach-Object {
                    $destName = "mupen64plus-video-GLideN64.dll"
                    Copy-Item $_.FullName -Destination "$outputDir\$destName" -Force
                    Write-Host "  [OK] $destName ($('{0:N0}' -f ($_.Length / 1KB)) KB)" -ForegroundColor Green
                }
                # Copy INI files
                Get-ChildItem -Path $win64Dir.FullName -Filter "*.ini" | ForEach-Object {
                    Copy-Item $_.FullName -Destination $outputDir -Force
                    Write-Host "  [OK] $($_.Name)" -ForegroundColor Green
                }
            } else {
                # Try to find any GLideN64 dll
                Get-ChildItem -Path $extractDir -Recurse -Filter "*.dll" | Where-Object { $_.Length -gt 1MB } | ForEach-Object {
                    Copy-Item $_.FullName -Destination "$outputDir\mupen64plus-video-GLideN64.dll" -Force
                    Write-Host "  [OK] mupen64plus-video-GLideN64.dll" -ForegroundColor Green
                }
            }
        }
    }
} catch {
    Write-Host "  [FAILED] GLideN64 download: $($_.Exception.Message)" -ForegroundColor Red
}

# =============================================
# 2. Download mupen64plus core bundle (64-bit)
# =============================================
Write-Host ""
Write-Host "=== Downloading mupen64plus Core ===" -ForegroundColor Yellow

# Try m64p project first (they have good 64-bit builds)
$m64pUrls = @(
    "https://github.com/loganmc10/m64p/releases/latest/download/m64p-win64.zip",
    "https://github.com/m64p/m64p/releases/latest/download/m64p-win64.zip"
)

$coreDownloaded = $false
foreach ($url in $m64pUrls) {
    if ($coreDownloaded) { break }
    
    try {
        Write-Host "  Trying: $url" -ForegroundColor Gray
        $bundleZip = "$tempDir\m64p.zip"
        Invoke-WebRequest -Uri $url -OutFile $bundleZip -UseBasicParsing -ErrorAction Stop
        
        if (Test-Path $bundleZip) {
            $extractDir = "$tempDir\m64p"
            Expand-Archive -Path $bundleZip -DestinationPath $extractDir -Force
            
            # Copy core
            Get-ChildItem -Path $extractDir -Recurse -Filter "mupen64plus.dll" | Select-Object -First 1 | ForEach-Object {
                Copy-Item $_.FullName -Destination $outputDir -Force
                Write-Host "  [OK] mupen64plus.dll ($('{0:N0}' -f ($_.Length / 1KB)) KB)" -ForegroundColor Green
                $coreDownloaded = $true
            }
            
            # Copy SDL2
            Get-ChildItem -Path $extractDir -Recurse -Filter "SDL2.dll" | Select-Object -First 1 | ForEach-Object {
                Copy-Item $_.FullName -Destination $outputDir -Force
                Write-Host "  [OK] SDL2.dll ($('{0:N0}' -f ($_.Length / 1KB)) KB)" -ForegroundColor Green
            }
            
            # Copy plugins (but NOT video - we use GLideN64)
            Get-ChildItem -Path $extractDir -Recurse -Filter "mupen64plus-rsp*.dll" | Select-Object -First 1 | ForEach-Object {
                Copy-Item $_.FullName -Destination $outputDir -Force
                Write-Host "  [OK] $($_.Name) (RSP)" -ForegroundColor Green
            }
            
            Get-ChildItem -Path $extractDir -Recurse -Filter "mupen64plus-audio*.dll" | Select-Object -First 1 | ForEach-Object {
                Copy-Item $_.FullName -Destination $outputDir -Force
                Write-Host "  [OK] $($_.Name) (Audio)" -ForegroundColor Green
            }
            
            Get-ChildItem -Path $extractDir -Recurse -Filter "mupen64plus-input*.dll" | Select-Object -First 1 | ForEach-Object {
                Copy-Item $_.FullName -Destination $outputDir -Force
                Write-Host "  [OK] $($_.Name) (Input)" -ForegroundColor Green
            }
        }
    } catch {
        Write-Host "  Failed: $($_.Exception.Message)" -ForegroundColor Gray
    }
}

# If still missing core, try official releases
if (!$coreDownloaded) {
    Write-Host "  Trying official mupen64plus releases..." -ForegroundColor Yellow
    try {
        # Get latest release info
        $releases = Invoke-RestMethod -Uri "https://api.github.com/repos/mupen64plus/mupen64plus-core/releases" -Headers @{"User-Agent"="CV64"}
        $latestRelease = $releases | Where-Object { $_.assets.Count -gt 0 } | Select-Object -First 1
        
        if ($latestRelease) {
            $win64Asset = $latestRelease.assets | Where-Object { $_.name -match "win64" -and $_.name -match "\.zip$" } | Select-Object -First 1
            
            if ($win64Asset) {
                Write-Host "  Found: $($win64Asset.name)" -ForegroundColor Gray
                $bundleZip = "$tempDir\mupen64plus-official.zip"
                Invoke-WebRequest -Uri $win64Asset.browser_download_url -OutFile $bundleZip -UseBasicParsing
                
                $extractDir = "$tempDir\mupen64plus-official"
                Expand-Archive -Path $bundleZip -DestinationPath $extractDir -Force
                
                Get-ChildItem -Path $extractDir -Recurse -Filter "*.dll" | ForEach-Object {
                    Copy-Item $_.FullName -Destination $outputDir -Force
                    Write-Host "  [OK] $($_.Name)" -ForegroundColor Green
                }
            }
        }
    } catch {
        Write-Host "  Official releases failed: $($_.Exception.Message)" -ForegroundColor Red
    }
}

# =============================================
# 3. Verification
# =============================================
Write-Host ""
Write-Host "============================================" -ForegroundColor Cyan
Write-Host " Verification - Checking 64-bit DLLs" -ForegroundColor Cyan
Write-Host "============================================" -ForegroundColor Cyan
Write-Host ""

$requiredFiles = @(
    @{ Name = "mupen64plus.dll"; Desc = "Core emulator"; MinSize = 2MB },
    @{ Name = "mupen64plus-video-GLideN64.dll"; Desc = "GLideN64 video"; MinSize = 1MB },
    @{ Name = "mupen64plus-rsp-hle.dll"; Desc = "RSP plugin"; MinSize = 50KB },
    @{ Name = "SDL2.dll"; Desc = "SDL2 library"; MinSize = 500KB }
)

$allOk = $true
foreach ($file in $requiredFiles) {
    $path = Join-Path $outputDir $file.Name
    if (Test-Path $path) {
        $size = (Get-Item $path).Length
        $sizeKB = [math]::Round($size / 1KB, 0)
        
        # Check if it's likely 64-bit (larger than expected minimum)
        if ($size -ge $file.MinSize) {
            Write-Host "  [OK] $($file.Name) - ${sizeKB} KB (64-bit)" -ForegroundColor Green
        } else {
            Write-Host "  [??] $($file.Name) - ${sizeKB} KB (may be 32-bit!)" -ForegroundColor Yellow
            $allOk = $false
        }
    } else {
        Write-Host "  [MISSING] $($file.Name) - $($file.Desc)" -ForegroundColor Red
        $allOk = $false
    }
}

Write-Host ""
Write-Host "Optional:" -ForegroundColor White
@("mupen64plus-audio-sdl.dll", "mupen64plus-input-sdl.dll", "GLideN64.ini", "GLideN64.custom.ini") | ForEach-Object {
    $path = Join-Path $outputDir $_
    if (Test-Path $path) {
        Write-Host "  [OK] $_" -ForegroundColor Green
    } else {
        Write-Host "  [--] $_" -ForegroundColor Gray
    }
}

Write-Host ""
if ($allOk) {
    Write-Host "============================================" -ForegroundColor Green
    Write-Host " SUCCESS! All 64-bit plugins ready!" -ForegroundColor Green
    Write-Host "============================================" -ForegroundColor Green
    Write-Host ""
    Write-Host "You can now run: x64\Release\CV64_RMG.exe" -ForegroundColor Cyan
} else {
    Write-Host "============================================" -ForegroundColor Red
    Write-Host " Some files missing or incorrect!" -ForegroundColor Red
    Write-Host "============================================" -ForegroundColor Red
    Write-Host ""
    Write-Host "Manual download required:" -ForegroundColor Yellow
    Write-Host ""
    Write-Host "1. GLideN64 (64-bit Windows):" -ForegroundColor White
    Write-Host "   https://github.com/gonetz/GLideN64/releases" -ForegroundColor Cyan
    Write-Host "   Extract Windows/x64 folder, rename DLL to:" -ForegroundColor Gray
    Write-Host "   mupen64plus-video-GLideN64.dll" -ForegroundColor Gray
    Write-Host ""
    Write-Host "2. mupen64plus 64-bit bundle:" -ForegroundColor White
    Write-Host "   https://m64p.github.io/" -ForegroundColor Cyan
    Write-Host "   or https://github.com/loganmc10/m64p/releases" -ForegroundColor Cyan
    Write-Host ""
    
    if (!$7zPath) {
        Write-Host "NOTE: Install 7-Zip to auto-extract GLideN64:" -ForegroundColor Yellow
        Write-Host "   https://www.7-zip.org/" -ForegroundColor Cyan
    }
}

# Cleanup
Write-Host ""
Remove-Item -Path $tempDir -Recurse -Force -ErrorAction SilentlyContinue
