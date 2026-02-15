# setup_nasm.ps1 - Downloads and configures NASM for mupen64plus-core build
# Run this script to fix the assembly compiler error

$ErrorActionPreference = "Stop"

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "NASM Setup for mupen64plus-core" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan

# NASM version expected by the project
$nasmVersion = "2.16.01"
$nasmUrl = "https://www.nasm.us/pub/nasm/releasebuilds/$nasmVersion/win64/nasm-$nasmVersion-win64.zip"
$nasmUrl32 = "https://www.nasm.us/pub/nasm/releasebuilds/$nasmVersion/win32/nasm-$nasmVersion-win32.zip"

# Target directory (relative to mupen64plus-core project)
$targetDir = "$PSScriptRoot\RMG\Source\3rdParty\mupen64plus-win32-deps\nasm-$nasmVersion"
$tempZip64 = "$env:TEMP\nasm-$nasmVersion-win64.zip"
$tempZip32 = "$env:TEMP\nasm-$nasmVersion-win32.zip"

Write-Host ""
Write-Host "Target directory: $targetDir" -ForegroundColor Yellow

# Create target directory
if (!(Test-Path $targetDir)) {
    New-Item -ItemType Directory -Path $targetDir -Force | Out-Null
    Write-Host "Created directory: $targetDir" -ForegroundColor Green
}

# Create x64 subdirectory
$x64Dir = "$targetDir\x64"
if (!(Test-Path $x64Dir)) {
    New-Item -ItemType Directory -Path $x64Dir -Force | Out-Null
}

# Create x86 subdirectory
$x86Dir = "$targetDir\x86"
if (!(Test-Path $x86Dir)) {
    New-Item -ItemType Directory -Path $x86Dir -Force | Out-Null
}

# Download 64-bit NASM
Write-Host ""
Write-Host "Downloading 64-bit NASM $nasmVersion..." -ForegroundColor Yellow
try {
    Invoke-WebRequest -Uri $nasmUrl -OutFile $tempZip64 -UseBasicParsing
    Write-Host "Download complete!" -ForegroundColor Green
} catch {
    Write-Host "Failed to download 64-bit NASM: $_" -ForegroundColor Red
    Write-Host "You may need to download manually from: $nasmUrl" -ForegroundColor Yellow
}

# Extract 64-bit NASM
if (Test-Path $tempZip64) {
    Write-Host "Extracting 64-bit NASM..." -ForegroundColor Yellow
    $tempExtract = "$env:TEMP\nasm-extract-64"
    if (Test-Path $tempExtract) { Remove-Item -Recurse -Force $tempExtract }
    Expand-Archive -Path $tempZip64 -DestinationPath $tempExtract -Force
    
    # Copy nasm.exe to x64 directory
    $nasmExe = Get-ChildItem -Path $tempExtract -Recurse -Filter "nasm.exe" | Select-Object -First 1
    if ($nasmExe) {
        Copy-Item -Path $nasmExe.FullName -Destination "$x64Dir\nasm.exe" -Force
        Write-Host "Installed nasm.exe to $x64Dir" -ForegroundColor Green
    }
    
    # Cleanup
    Remove-Item -Recurse -Force $tempExtract
    Remove-Item -Force $tempZip64
}

# Download 32-bit NASM
Write-Host ""
Write-Host "Downloading 32-bit NASM $nasmVersion..." -ForegroundColor Yellow
try {
    Invoke-WebRequest -Uri $nasmUrl32 -OutFile $tempZip32 -UseBasicParsing
    Write-Host "Download complete!" -ForegroundColor Green
} catch {
    Write-Host "Failed to download 32-bit NASM: $_" -ForegroundColor Red
    Write-Host "You may need to download manually from: $nasmUrl32" -ForegroundColor Yellow
}

# Extract 32-bit NASM
if (Test-Path $tempZip32) {
    Write-Host "Extracting 32-bit NASM..." -ForegroundColor Yellow
    $tempExtract = "$env:TEMP\nasm-extract-32"
    if (Test-Path $tempExtract) { Remove-Item -Recurse -Force $tempExtract }
    Expand-Archive -Path $tempZip32 -DestinationPath $tempExtract -Force
    
    # Copy nasm.exe to x86 directory
    $nasmExe = Get-ChildItem -Path $tempExtract -Recurse -Filter "nasm.exe" | Select-Object -First 1
    if ($nasmExe) {
        Copy-Item -Path $nasmExe.FullName -Destination "$x86Dir\nasm.exe" -Force
        Write-Host "Installed nasm.exe to $x86Dir" -ForegroundColor Green
    }
    
    # Cleanup
    Remove-Item -Recurse -Force $tempExtract
    Remove-Item -Force $tempZip32
}

# Verify installation
Write-Host ""
Write-Host "========================================" -ForegroundColor Cyan
Write-Host "Verifying installation..." -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan

$nasm64 = "$x64Dir\nasm.exe"
$nasm32 = "$x86Dir\nasm.exe"

if (Test-Path $nasm64) {
    $version = & $nasm64 -v 2>&1
    Write-Host "64-bit NASM: $version" -ForegroundColor Green
} else {
    Write-Host "64-bit NASM: NOT FOUND" -ForegroundColor Red
}

if (Test-Path $nasm32) {
    $version = & $nasm32 -v 2>&1
    Write-Host "32-bit NASM: $version" -ForegroundColor Green
} else {
    Write-Host "32-bit NASM: NOT FOUND" -ForegroundColor Red
}

Write-Host ""
Write-Host "========================================" -ForegroundColor Cyan
Write-Host "Setup complete!" -ForegroundColor Green
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""
Write-Host "You can now rebuild the solution." -ForegroundColor Yellow
Write-Host ""

# Pause if running interactively
if ($Host.Name -eq "ConsoleHost") {
    Write-Host "Press any key to exit..."
    $null = $Host.UI.RawUI.ReadKey("NoEcho,IncludeKeyDown")
}
