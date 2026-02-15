# Build WORKING CV64_RMG with Dynamic DLLs
$ErrorActionPreference = "Continue"

Write-Host "`n========================================" -ForegroundColor Cyan
Write-Host " CV64_RMG - WORKING BUILD" -ForegroundColor Cyan
Write-Host "========================================`n" -ForegroundColor Cyan

Write-Host "Building CV64_RMG.exe with full RMG integration..." -ForegroundColor Yellow

$msbuild = & "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe" -latest -requires Microsoft.Component.MSBuild -find MSBuild\**\Bin\MSBuild.exe | Select-Object -First 1

& $msbuild "CV64_RMG.vcxproj" /t:Rebuild /p:Configuration=Release /p:Platform=x64 /v:minimal /nologo

if ($LASTEXITCODE -eq 0) {
    Write-Host "`n[OK] BUILD SUCCESSFUL!" -ForegroundColor Green
    
    Write-Host "`n========================================" -ForegroundColor Cyan
    Write-Host " OUTPUT FILES" -ForegroundColor Green  
    Write-Host "========================================" -ForegroundColor Cyan
    
    if (Test-Path "x64\Release\CV64_RMG.exe") {
        $exe = Get-Item "x64\Release\CV64_RMG.exe"
        $size = $exe.Length / 1MB
        Write-Host "  CV64_RMG.exe - $([math]::Round($size, 2)) MB" -ForegroundColor Green
    }
    
    Write-Host "`n  Required DLLs (in x64\Release):" -ForegroundColor Yellow
    $dlls = @("mupen64plus.dll", "mupen64plus-rsp-hle.dll", "mupen64plus-video-glide64mk2.dll", "SDL2.dll", "zlib1.dll")
    foreach ($dll in $dlls) {
        if (Test-Path "x64\Release\$dll") {
            Write-Host "    [OK] $dll" -ForegroundColor Green
        } else {
            Write-Host "    [MISSING] $dll" -ForegroundColor Red
        }
    }
    
    Write-Host "`n========================================" -ForegroundColor Cyan
    Write-Host " READY TO RUN!" -ForegroundColor Green
    Write-Host "========================================" -ForegroundColor Cyan
    Write-Host "Run: x64\Release\CV64_RMG.exe" -ForegroundColor White
    Write-Host "`nFeatures:" -ForegroundColor Yellow
    Write-Host "  [+] Full N64 emulation via mupen64plus" -ForegroundColor White
    Write-Host "  [+] OpenGL rendering with video extension" -ForegroundColor White
    Write-Host "  [+] Frame callbacks for patches" -ForegroundColor White
    Write-Host "  [+] Custom D-PAD camera control" -ForegroundColor White
    Write-Host "  [+] Save states (F5/F9)" -ForegroundColor White
    Write-Host "  [+] Memory access for game modding" -ForegroundColor White
    Write-Host "`n========================================`n" -ForegroundColor Cyan
    
} else {
    Write-Host "`n[FAILED] Build encountered errors" -ForegroundColor Red
    Write-Host "Check the error messages above" -ForegroundColor Yellow
}
