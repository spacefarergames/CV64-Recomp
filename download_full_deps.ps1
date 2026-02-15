# download_full_deps.ps1 - Downloads complete mupen64plus-win32-deps from official source
# This downloads the pre-packaged dependencies that the project expects

$ErrorActionPreference = "Stop"

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "mupen64plus Win32 Dependencies Setup" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan

$depsDir = "$PSScriptRoot\RMG\Source\3rdParty\mupen64plus-win32-deps"

# The official mupen64plus-win32-deps repository
$depsRepoUrl = "https://github.com/mupen64plus/mupen64plus-win32-deps/archive/refs/heads/master.zip"
$tempZip = "$env:TEMP\mupen64plus-win32-deps.zip"
$tempExtract = "$env:TEMP\mupen64plus-deps-extract"

Write-Host "`nDownloading mupen64plus-win32-deps..." -ForegroundColor Yellow

try {
    # Download the deps repository
    Invoke-WebRequest -Uri $depsRepoUrl -OutFile $tempZip -UseBasicParsing
    Write-Host "Download complete!" -ForegroundColor Green
    
    # Extract
    Write-Host "Extracting..." -ForegroundColor Yellow
    if (Test-Path $tempExtract) { Remove-Item -Recurse -Force $tempExtract }
    Expand-Archive -Path $tempZip -DestinationPath $tempExtract -Force
    
    # Find extracted folder
    $extractedDir = Get-ChildItem -Path $tempExtract -Directory | Select-Object -First 1
    
    if ($extractedDir) {
        # Backup existing deps if any
        if (Test-Path $depsDir) {
            $backupName = "$depsDir-backup-$(Get-Date -Format 'yyyyMMdd-HHmmss')"
            Write-Host "Backing up existing deps to: $backupName" -ForegroundColor Yellow
            Rename-Item -Path $depsDir -NewName $backupName
        }
        
        # Move extracted contents to target
        Write-Host "Installing dependencies..." -ForegroundColor Yellow
        Move-Item -Path $extractedDir.FullName -Destination $depsDir -Force
        
        Write-Host "`nDependencies installed to: $depsDir" -ForegroundColor Green
    }
    
    # Cleanup
    Remove-Item -Recurse -Force $tempExtract
    Remove-Item -Force $tempZip
    
} catch {
    Write-Host "Failed to download dependencies: $_" -ForegroundColor Red
    Write-Host "`nAlternative: Download manually from:" -ForegroundColor Yellow
    Write-Host "  $depsRepoUrl" -ForegroundColor White
    Write-Host "And extract to: $depsDir" -ForegroundColor White
}

# Verify key dependencies
Write-Host "`n========================================" -ForegroundColor Cyan
Write-Host "Verifying Dependencies" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan

$checks = @(
    @{Name="SDL2"; Path="$depsDir\SDL2-*\include\SDL.h"},
    @{Name="SDL2_net"; Path="$depsDir\SDL2_net-*\include\SDL_net.h"},
    @{Name="zlib"; Path="$depsDir\zlib-*\include\zlib.h"},
    @{Name="libpng"; Path="$depsDir\libpng-*\include\png.h"},
    @{Name="freetype"; Path="$depsDir\freetype-*\include\ft2build.h"},
    @{Name="NASM"; Path="$depsDir\nasm-*\x64\nasm.exe"}
)

foreach ($check in $checks) {
    $found = Get-ChildItem -Path $check.Path -ErrorAction SilentlyContinue
    Write-Host "$($check.Name): " -NoNewline
    if ($found) {
        Write-Host "OK" -ForegroundColor Green
    } else {
        Write-Host "MISSING" -ForegroundColor Red
    }
}

Write-Host "`n========================================" -ForegroundColor Cyan
Write-Host "Setup complete!" -ForegroundColor Green
Write-Host "========================================" -ForegroundColor Cyan
