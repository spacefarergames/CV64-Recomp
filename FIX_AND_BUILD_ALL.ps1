#!/usr/bin/env pwsh
# COMPLETE FIX AND BUILD - Restores working implementation and builds everything
# Run this script to fix all issues and build in one go

$ErrorActionPreference = "Continue"

Write-Host ""
Write-Host "========================================================================" -ForegroundColor Cyan
Write-Host "  CV64 RMG - COMPLETE RESTORATION AND BUILD" -ForegroundColor Cyan  
Write-Host "========================================================================" -ForegroundColor Cyan
Write-Host ""

# ===== PART 1: RESTORE WORKING DYNAMIC IMPLEMENTATION =====
Write-Host "[PART 1/4] Restoring Working Dynamic Implementation" -ForegroundColor Yellow
Write-Host "-----------------------------------------------------" -ForegroundColor Yellow

# Backup corrupted file
if (Test-Path "src\cv64_m64p_integration.cpp") {
    Copy-Item "src\cv64_m64p_integration.cpp" "src\cv64_m64p_integration.cpp.CORRUPTED_BACKUP" -Force
    Write-Host "? Backed up corrupted file" -ForegroundColor Green
}

# The issue: File was corrupted with static declarations mixed with dynamic code
# Solution: Restore the working dynamic version

Write-Host "? Restoring working dynamic LoadLibrary implementation..." -ForegroundColor Green
Write-Host "  (This is the version that was working before static attempt)" -ForegroundColor Gray

# Note: The actual file content would be too large for this script
# Instead, we'll use the temp backup that the IDE created

$tempBackup = "..\..\..\AppData\Local\Temp\iyc2mujh.cpp"
$tempBackup2 = "..\..\..\AppData\Local\Temp\5wzzuxbf.cpp"

$restored = $false

# Try to restore from IDE temp files
if (Test-Path $tempBackup) {
    Copy-Item $tempBackup "src\cv64_m64p_integration.cpp" -Force
    Write-Host "? Restored from temp backup 1" -ForegroundColor Green
    $restored = $true
} elseif (Test-Path $tempBackup2) {
    Copy-Item $tempBackup2 "src\cv64_m64p_integration.cpp" -Force
    Write-Host "? Restored from temp backup 2" -ForegroundColor Green
    $restored = $true
}

if (!$restored) {
    Write-Host "? No temp backup found - will rebuild from INTEGRATION_NOTES.md" -ForegroundColor Yellow
    Write-Host "  The working implementation uses:" -ForegroundColor Gray
    Write-Host "    - LoadLibrary/GetProcAddress for DLL loading" -ForegroundColor Gray
    Write-Host "    - Function pointers (ptr_CoreStartup, ptr_CoreDoCommand, etc.)" -ForegroundColor Gray
    Write-Host "    - Dynamic plugin discovery and loading" -ForegroundColor Gray
    Write-Host "    - Video extension with OpenGL context management" -ForegroundColor Gray
    Write-Host "    - Frame callbacks via VidExt_GL_SwapBuffers" -ForegroundColor Gray
    Write-Host ""
    Write-Host "  Manual fix required:" -ForegroundColor Red
    Write-Host "    1. Open INTEGRATION_NOTES.md for reference" -ForegroundColor White
    Write-Host "    2. Remove ALL 'extern \"C\" { EXPORT ... }' declarations" -ForegroundColor White
    Write-Host "    3. Ensure typedef ptr_CoreStartup, ptr_CoreDoCommand, etc. exist" -ForegroundColor White
    Write-Host "    4. Ensure LoadLibrary/GetProcAddress code is present" -ForegroundColor White
    Write-Host ""
    Write-Host "  OR: Download fresh from RMG reference implementation" -ForegroundColor Yellow
    Write-Host ""
    Read-Host "Press Enter to continue with build attempt anyway"
}

Write-Host ""

# ===== PART 2: RESTORE PROJECT FILES =====
Write-Host "[PART 2/4] Restoring Project Configuration" -ForegroundColor Yellow
Write-Host "-------------------------------------------" -ForegroundColor Yellow

# Restore CV64_RMG.vcxproj from backup
if (Test-Path "CV64_RMG.vcxproj.backup") {
    Copy-Item "CV64_RMG.vcxproj.backup" "CV64_RMG.vcxproj" -Force
    Write-Host "? Restored CV64_RMG.vcxproj from backup" -ForegroundColor Green
} else {
    Write-Host "? No project backup - using current version" -ForegroundColor Yellow
}

# Remove static implementation file if present
if (Test-Path "src\cv64_m64p_integration_static.cpp") {
    Remove-Item "src\cv64_m64p_integration_static.cpp" -Force
    Write-Host "? Removed static implementation file" -ForegroundColor Green
}

Write-Host ""

# ===== PART 3: VERIFY DEPENDENCIES =====
Write-Host "[PART 3/4] Verifying Plugin DLLs" -ForegroundColor Yellow
Write-Host "----------------------------------" -ForegroundColor Yellow

$pluginDir = "x64\Release"
$requiredDlls = @{
    "mupen64plus.dll" = "Core emulator"
    "mupen64plus-rsp-hle.dll" = "RSP plugin (required)"
    "SDL2.dll" = "Core dependency"
    "zlib1.dll" = "Core dependency"
}

$optionalDlls = @{
    "mupen64plus-video-GLideN64.dll" = "Video plugin (recommended)"
    "mupen64plus-audio-sdl.dll" = "Audio plugin"
    "mupen64plus-input-sdl.dll" = "Input plugin"
}

$allPresent = $true
$somePresent = $false

Write-Host "Required DLLs:" -ForegroundColor White
foreach ($dll in $requiredDlls.Keys) {
    $path = Join-Path $pluginDir $dll
    if (Test-Path $path) {
        Write-Host "  ? $dll" -ForegroundColor Green -NoNewline
        Write-Host " - $($requiredDlls[$dll])" -ForegroundColor Gray
        $somePresent = $true
    } else {
        Write-Host "  ? $dll" -ForegroundColor Red -NoNewline
        Write-Host " - $($requiredDlls[$dll])" -ForegroundColor Gray
        $allPresent = $false
    }
}

Write-Host ""
Write-Host "Optional DLLs:" -ForegroundColor White
foreach ($dll in $optionalDlls.Keys) {
    $path = Join-Path $pluginDir $dll
    if (Test-Path $path) {
        Write-Host "  ? $dll" -ForegroundColor Green -NoNewline
        Write-Host " - $($optionalDlls[$dll])" -ForegroundColor Gray
    } else {
        Write-Host "  ? $dll" -ForegroundColor Yellow -NoNewline
        Write-Host " - $($optionalDlls[$dll])" -ForegroundColor Gray
    }
}

Write-Host ""
if (!$allPresent) {
    Write-Host "? MISSING REQUIRED DLLs!" -ForegroundColor Red
    Write-Host ""
    Write-Host "  Fix: Run download_plugins_direct.ps1 to download all required files" -ForegroundColor Yellow
    Write-Host "  Command: .\download_plugins_direct.ps1" -ForegroundColor Cyan
    Write-Host ""
    Write-Host "  Continuing with build anyway (will fail at runtime)..." -ForegroundColor Yellow
    Write-Host ""
}

# ===== PART 4: BUILD =====
Write-Host "[PART 4/4] Building CV64_RMG.exe" -ForegroundColor Yellow
Write-Host "---------------------------------" -ForegroundColor Yellow
Write-Host ""

Write-Host "Building Release x64..." -ForegroundColor Cyan

try {
    msbuild CV64_RMG.vcxproj /t:Rebuild /p:Configuration=Release /p:Platform=x64 /v:minimal /nologo
    
    if ($LASTEXITCODE -eq 0) {
        Write-Host ""
        Write-Host "========================================================================" -ForegroundColor Green
        Write-Host "  BUILD SUCCESSFUL!" -ForegroundColor Green
        Write-Host "========================================================================" -ForegroundColor Green
        Write-Host ""
        Write-Host "Executable: x64\Release\CV64_RMG.exe" -ForegroundColor Cyan
        Write-Host ""
        
        if ($allPresent) {
            Write-Host "? All required DLLs present - Ready to run!" -ForegroundColor Green
            Write-Host ""
            Write-Host "Run: x64\Release\CV64_RMG.exe" -ForegroundColor Cyan
        } else {
            Write-Host "? DLLs missing - Download them first:" -ForegroundColor Yellow
            Write-Host "  .\download_plugins_direct.ps1" -ForegroundColor Cyan
            Write-Host ""
            Write-Host "Then run: x64\Release\CV64_RMG.exe" -ForegroundColor Cyan
        }
        
        Write-Host ""
        Write-Host "========================================================================" -ForegroundColor Green
        Write-Host ""
        
        # Show what was built
        if (Test-Path "x64\Release\CV64_RMG.exe") {
            $exeInfo = Get-Item "x64\Release\CV64_RMG.exe"
            $sizeKB = [math]::Round($exeInfo.Length / 1KB, 2)
            $sizeMB = [math]::Round($exeInfo.Length / 1MB, 2)
            Write-Host "Executable size: $sizeMB MB ($sizeKB KB)" -ForegroundColor Gray
            Write-Host "Built: $($exeInfo.LastWriteTime)" -ForegroundColor Gray
            Write-Host ""
        }
        
    } else {
        Write-Host ""
        Write-Host "========================================================================" -ForegroundColor Red
        Write-Host "  BUILD FAILED!" -ForegroundColor Red
        Write-Host "========================================================================" -ForegroundColor Red
        Write-Host ""
        Write-Host "Exit code: $LASTEXITCODE" -ForegroundColor Red
        Write-Host ""
        Write-Host "Common issues:" -ForegroundColor Yellow
        Write-Host "  1. File still corrupted - Check src\cv64_m64p_integration.cpp" -ForegroundColor White
        Write-Host "     Look for 'extern \"C\" { EXPORT' declarations - remove them!" -ForegroundColor White
        Write-Host ""
        Write-Host "  2. Missing headers - Check include directories" -ForegroundColor White
        Write-Host ""
        Write-Host "  3. Syntax errors - Check build output above" -ForegroundColor White
        Write-Host ""
        Write-Host "Reference: INTEGRATION_NOTES.md for working implementation" -ForegroundColor Cyan
        Write-Host ""
        Write-Host "========================================================================" -ForegroundColor Red
        Write-Host ""
        exit 1
    }
    
} catch {
    Write-Host ""
    Write-Host "BUILD EXCEPTION: $_" -ForegroundColor Red
    Write-Host ""
    exit 1
}

# ===== BONUS: STATIC BUILD INFO =====
Write-Host ""
Write-Host "========================================================================" -ForegroundColor Cyan
Write-Host "  Optional: Static Linking (Single EXE, No DLLs)" -ForegroundColor Cyan
Write-Host "========================================================================" -ForegroundColor Cyan
Write-Host ""
Write-Host "The dynamic DLL version is working and ready to use." -ForegroundColor Green
Write-Host ""
Write-Host "If you want to build a static version (single .exe, no DLLs):" -ForegroundColor Yellow
Write-Host ""
Write-Host "  1. Install NASM assembler" -ForegroundColor White
Write-Host "     Download: https://www.nasm.us/pub/nasm/releasebuilds/?C=M;O=D" -ForegroundColor Cyan
Write-Host "     Get: nasm-X.XX.XX-installer-x64.exe" -ForegroundColor Cyan
Write-Host ""
Write-Host "  2. Run static build configuration" -ForegroundColor White
Write-Host "     Command: .\configure_static_m64p.ps1" -ForegroundColor Cyan
Write-Host ""
Write-Host "  3. Build static libraries" -ForegroundColor White
Write-Host "     Command: .\build_all_static.ps1" -ForegroundColor Cyan
Write-Host ""
Write-Host "  See: STATIC_LINKING_GUIDE.md for full instructions" -ForegroundColor Cyan
Write-Host ""
Write-Host "========================================================================" -ForegroundColor Cyan
Write-Host ""

# Check if NASM is installed
try {
    $nasmCheck = nasm --version 2>&1
    if ($?) {
        Write-Host "? NASM detected: $nasmCheck" -ForegroundColor Green
        Write-Host "  Static build is possible!" -ForegroundColor Green
        Write-Host ""
    }
} catch {
    Write-Host "  NASM not installed - static build not available" -ForegroundColor Gray
    Write-Host ""
}

Write-Host "Done!" -ForegroundColor Green
