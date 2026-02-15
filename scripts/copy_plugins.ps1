# ==============================================================================
# Castlevania 64 PC Recomp - Plugin Copy Script
# ==============================================================================
# This script copies mupen64plus DLLs and plugins to the output directory
# Source: https://github.com/mupen64plus/mupen64plus-core
# Run this after building: .\scripts\copy_plugins.ps1 -Configuration Release
# ==============================================================================

param(
    [string]$Configuration = "Release",
    [string]$Platform = "x64",
    [switch]$Download = $false
)

$ErrorActionPreference = "Stop"

# Paths
$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$ProjectDir = Split-Path -Parent $ScriptDir
$OutputDir = Join-Path $ProjectDir "$Platform\$Configuration"
$PluginsOutputDir = Join-Path $OutputDir "plugins"
$TempDir = Join-Path $ProjectDir "temp"

# Create output directories
Write-Host "Creating output directories..."
New-Item -ItemType Directory -Force -Path $OutputDir | Out-Null
New-Item -ItemType Directory -Force -Path $PluginsOutputDir | Out-Null
New-Item -ItemType Directory -Force -Path (Join-Path $OutputDir "assets") | Out-Null

# Required mupen64plus files
$RequiredFiles = @(
    @{ Name = "mupen64plus.dll"; Type = "Core" },
    @{ Name = "mupen64plus-video-GLideN64.dll"; Type = "Plugin" },
    @{ Name = "mupen64plus-audio-sdl.dll"; Type = "Plugin" },
    @{ Name = "mupen64plus-input-sdl.dll"; Type = "Plugin" },
    @{ Name = "mupen64plus-rsp-hle.dll"; Type = "Plugin" }
)

# Alternative plugin names to search for
$PluginAliases = @{
    "mupen64plus-video-GLideN64.dll" = @("GLideN64.dll", "mupen64plus-video-gliden64.dll")
    "mupen64plus-audio-sdl.dll" = @("mupen64plus-audio-sdl2.dll", "audio-sdl.dll", "audio-sdl2.dll")
    "mupen64plus-input-sdl.dll" = @("input-sdl.dll", "mupen64plus-input-sdl2.dll")
    "mupen64plus-rsp-hle.dll" = @("rsp-hle.dll")
}

# Search locations for mupen64plus files (official mupen64plus paths)
$SearchLocations = @(
    (Join-Path $ProjectDir "plugins"),
    (Join-Path $ProjectDir "lib"),
    (Join-Path $ProjectDir "mupen64plus"),
    (Join-Path $TempDir "m64p"),
    "C:\mupen64plus",
    "$env:USERPROFILE\mupen64plus",
    "$env:LOCALAPPDATA\mupen64plus"
)

Write-Host "============================================"
Write-Host " Castlevania 64 PC Recomp - Plugin Setup"
Write-Host " Source: https://github.com/mupen64plus"
Write-Host "============================================"
Write-Host ""
Write-Host "Output directory: $OutputDir"
Write-Host "Plugins directory: $PluginsOutputDir"
Write-Host ""

# ==============================================================================
# Download function - fetches from official mupen64plus repositories
# ==============================================================================
function Download-OfficialPlugins {
    Write-Host "Downloading plugins from official mupen64plus repositories..."
    Write-Host ""
    
    New-Item -ItemType Directory -Force -Path $TempDir | Out-Null
    $downloadDir = Join-Path $TempDir "m64p"
    New-Item -ItemType Directory -Force -Path $downloadDir | Out-Null
    
    # Try mupen64plus-win32-deps first (contains prebuilt binaries)
    Write-Host "Checking mupen64plus-win32-deps for prebuilt binaries..."
    try {
        $releases = Invoke-RestMethod -Uri "https://api.github.com/repos/mupen64plus/mupen64plus-win32-deps/releases/latest" -UseBasicParsing
        Write-Host "Latest release: $($releases.tag_name)"
        
        $asset = $releases.assets | Where-Object { $_.name -like "*64*" } | Select-Object -First 1
        if ($asset) {
            Write-Host "Downloading: $($asset.name)..."
            $zipPath = Join-Path $TempDir "deps.zip"
            Invoke-WebRequest -Uri $asset.browser_download_url -OutFile $zipPath -UseBasicParsing
            Expand-Archive -Path $zipPath -DestinationPath $downloadDir -Force
            Remove-Item $zipPath -Force
            Write-Host "[OK] Dependencies downloaded" -ForegroundColor Green
        }
    } catch {
        Write-Host "Win32-deps download failed: $($_.Exception.Message)" -ForegroundColor Yellow
    }
    
    # Download GLideN64 video plugin
    Write-Host ""
    Write-Host "Downloading GLideN64 from official repository..."
    try {
        $releases = Invoke-RestMethod -Uri "https://api.github.com/repos/gonetz/GLideN64/releases/latest" -UseBasicParsing
        Write-Host "Latest GLideN64: $($releases.tag_name)"
        
        $asset = $releases.assets | Where-Object { $_.name -like "*mupen64plus*" -and $_.name -like "*win*" } | Select-Object -First 1
        if ($asset) {
            Write-Host "Downloading: $($asset.name)..."
            $zipPath = Join-Path $TempDir "gliden64.zip"
            Invoke-WebRequest -Uri $asset.browser_download_url -OutFile $zipPath -UseBasicParsing
            $glideDir = Join-Path $downloadDir "GLideN64"
            Expand-Archive -Path $zipPath -DestinationPath $glideDir -Force
            Remove-Item $zipPath -Force
            Write-Host "[OK] GLideN64 downloaded" -ForegroundColor Green
        }
    } catch {
        Write-Host "GLideN64 download failed: $($_.Exception.Message)" -ForegroundColor Yellow
    }
    
    Write-Host ""
    Write-Host "Download complete. Files in: $downloadDir"
    Write-Host ""
}

# ==============================================================================
# Plugin search function
# ==============================================================================
function Find-PluginFile {
    param([string]$FileName)
    
    # Search primary name
    foreach ($location in $SearchLocations) {
        if (Test-Path $location) {
            $found = Get-ChildItem -Path $location -Filter $FileName -Recurse -ErrorAction SilentlyContinue | Select-Object -First 1
            if ($found) {
                return $found.FullName
            }
        }
    }
    
    # Search aliases
    if ($PluginAliases.ContainsKey($FileName)) {
        foreach ($alias in $PluginAliases[$FileName]) {
            foreach ($location in $SearchLocations) {
                if (Test-Path $location) {
                    $found = Get-ChildItem -Path $location -Filter $alias -Recurse -ErrorAction SilentlyContinue | Select-Object -First 1
                    if ($found) {
                        return $found.FullName
                    }
                }
            }
        }
    }
    
    return $null
}

# ==============================================================================
# Main execution
# ==============================================================================

# Download if requested
if ($Download) {
    Download-OfficialPlugins
}

$CopiedCount = 0
$MissingFiles = @()

foreach ($file in $RequiredFiles) {
    $sourcePath = Find-PluginFile -FileName $file.Name
    
    if ($sourcePath) {
        if ($file.Type -eq "Core") {
            $destPath = Join-Path $OutputDir $file.Name
        } else {
            $destPath = Join-Path $PluginsOutputDir $file.Name
        }
        
        Write-Host "[OK] Copying: $($file.Name)"
        Write-Host "     From: $sourcePath"
        Copy-Item -Path $sourcePath -Destination $destPath -Force
        $CopiedCount++
    } else {
        Write-Host "[MISSING] $($file.Name)" -ForegroundColor Yellow
        $MissingFiles += $file.Name
    }
}

# Copy SDL2.dll if present
$sdl2Path = Find-PluginFile -FileName "SDL2.dll"
if ($sdl2Path) {
    Write-Host "[OK] Copying: SDL2.dll"
    Copy-Item -Path $sdl2Path -Destination (Join-Path $OutputDir "SDL2.dll") -Force
    $CopiedCount++
}

# Copy GLideN64 config if present
$gliden64IniPath = Find-PluginFile -FileName "GLideN64.ini"
if ($gliden64IniPath) {
    Write-Host "[OK] Copying: GLideN64.ini"
    Copy-Item -Path $gliden64IniPath -Destination (Join-Path $PluginsOutputDir "GLideN64.ini") -Force
}

# Copy our patch configs
$PatchDir = Join-Path $ProjectDir "patches"
if (Test-Path $PatchDir) {
    Write-Host "[OK] Copying patch configuration files..."
    Copy-Item -Path "$PatchDir\*" -Destination $OutputDir -Force
}

Write-Host ""
Write-Host "============================================"
Write-Host " Summary"
Write-Host "============================================"
Write-Host "Files copied: $CopiedCount"

if ($MissingFiles.Count -gt 0) {
    Write-Host ""
    Write-Host "MISSING FILES:" -ForegroundColor Yellow
    foreach ($missing in $MissingFiles) {
        Write-Host "  - $missing" -ForegroundColor Yellow
    }
    Write-Host ""
    Write-Host "Please download mupen64plus from:" -ForegroundColor Cyan
    Write-Host "  https://github.com/Rosalie241/RMG/releases" -ForegroundColor Cyan
    Write-Host ""
    Write-Host "Or build RMG from source in the RMG subdirectory."
} else {
    Write-Host ""
    Write-Host "All plugins copied successfully!" -ForegroundColor Green
    Write-Host ""
    Write-Host "Next step: Place your Castlevania 64 ROM at:"
    Write-Host "  $OutputDir\assets\Castlevania (U) (V1.2) [!].z64"
}

Write-Host ""
