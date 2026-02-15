# Direct download script for mupen64plus plugins
# Downloads from working URLs

$ErrorActionPreference = "Stop"
$ProgressPreference = 'SilentlyContinue'

Write-Host "============================================" -ForegroundColor Cyan
Write-Host " Downloading mupen64plus Plugins" -ForegroundColor Cyan
Write-Host "============================================" -ForegroundColor Cyan
Write-Host ""

# Create directories
$pluginsDir = "plugins"
$outputDir = "x64\Release"
New-Item -ItemType Directory -Force -Path $pluginsDir | Out-Null
New-Item -ItemType Directory -Force -Path $outputDir | Out-Null

# Download GLideN64 from releases
Write-Host "Downloading GLideN64..." -ForegroundColor Yellow
try {
    $gliden64Url = "https://github.com/gonetz/GLideN64/releases/download/Public_Release_4_0/GLideN64_Public_Release_4_0.zip"
    $gliden64Zip = "temp\gliden64.zip"
    New-Item -ItemType Directory -Force -Path "temp" | Out-Null
    Invoke-WebRequest -Uri $gliden64Url -OutFile $gliden64Zip -UseBasicParsing
    Expand-Archive -Path $gliden64Zip -DestinationPath "temp\gliden64" -Force
    
    # Copy plugin DLL
    Get-ChildItem -Path "temp\gliden64" -Recurse -Filter "mupen64plus-video-GLideN64.dll" | ForEach-Object {
        Copy-Item $_.FullName -Destination $pluginsDir -Force
        Write-Host "  [OK] GLideN64 plugin copied" -ForegroundColor Green
    }
    
    # Copy config files
    Get-ChildItem -Path "temp\gliden64" -Recurse -Filter "*.ini" | ForEach-Object {
        Copy-Item $_.FullName -Destination $pluginsDir -Force
    }
} catch {
    Write-Host "  [FAILED] GLideN64 download failed: $_" -ForegroundColor Red
}

# Download mupen64plus bundle with all plugins
Write-Host "`nDownloading mupen64plus bundle..." -ForegroundColor Yellow
try {
    # Try the mupen64plus.org release
    $bundleUrl = "https://github.com/mupen64plus/mupen64plus-core/releases/download/2.5.9/mupen64plus-bundle-win32-2.5.9.zip"
    $bundleZip = "temp\m64p-bundle.zip"
    Invoke-WebRequest -Uri $bundleUrl -OutFile $bundleZip -UseBasicParsing
    Expand-Archive -Path $bundleZip -DestinationPath "temp\m64p-bundle" -Force
    
    # Copy all DLLs
    Get-ChildItem -Path "temp\m64p-bundle" -Recurse -Filter "*.dll" | ForEach-Object {
        $destPath = if ($_.Name -like "mupen64plus-*") { $pluginsDir } else { $outputDir }
        Copy-Item $_.FullName -Destination $destPath -Force
        Write-Host "  [OK] $($_.Name)" -ForegroundColor Green
    }
} catch {
    Write-Host "  [FAILED] Bundle download failed: $_" -ForegroundColor Red
    Write-Host "  Trying alternative source..." -ForegroundColor Yellow
    
    # Try individual plugin downloads
    $plugins = @(
        @{Name="RSP-HLE"; Url="https://github.com/mupen64plus/mupen64plus-rsp-hle/releases/download/2.5/mupen64plus-rsp-hle-win32-2.5.zip"},
        @{Name="Audio-SDL"; Url="https://github.com/mupen64plus/mupen64plus-audio-sdl/releases/download/2.5/mupen64plus-audio-sdl-win32-2.5.zip"},
        @{Name="Input-SDL"; Url="https://github.com/mupen64plus/mupen64plus-input-sdl/releases/download/2.5/mupen64plus-input-sdl-win32-2.5.zip"}
    )
    
    foreach ($plugin in $plugins) {
        try {
            Write-Host "  Downloading $($plugin.Name)..." -ForegroundColor Yellow
            $zipPath = "temp\$($plugin.Name).zip"
            Invoke-WebRequest -Uri $plugin.Url -OutFile $zipPath -UseBasicParsing
            Expand-Archive -Path $zipPath -DestinationPath "temp\$($plugin.Name)" -Force
            
            Get-ChildItem -Path "temp\$($plugin.Name)" -Recurse -Filter "*.dll" | ForEach-Object {
                Copy-Item $_.FullName -Destination $pluginsDir -Force
                Write-Host "    [OK] $($_.Name)" -ForegroundColor Green
            }
        } catch {
            Write-Host "    [FAILED] $($plugin.Name): $_" -ForegroundColor Red
        }
    }
}

# Summary
Write-Host "`n============================================" -ForegroundColor Cyan
Write-Host " Download Summary" -ForegroundColor Cyan
Write-Host "============================================" -ForegroundColor Cyan

Write-Host "`nPlugins directory:" -ForegroundColor Yellow
Get-ChildItem -Path $pluginsDir -Filter "*.dll" | ForEach-Object {
    Write-Host "  [OK] $($_.Name)" -ForegroundColor Green
}

Write-Host "`nOutput directory (x64\Release):" -ForegroundColor Yellow
Get-ChildItem -Path $outputDir -Filter "*.dll" | ForEach-Object {
    Write-Host "  [OK] $($_.Name)" -ForegroundColor Green
}

Write-Host "`n============================================" -ForegroundColor Cyan
Write-Host "Next steps:" -ForegroundColor Yellow
Write-Host "1. Build the project (if not already built)"
Write-Host "2. Place your ROM at: assets\Castlevania (U) (V1.2) [!].z64"
Write-Host "3. Run CV64_RMG.exe"
Write-Host "============================================" -ForegroundColor Cyan
