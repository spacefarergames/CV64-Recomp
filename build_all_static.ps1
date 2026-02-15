# Complete Static Build Setup - All in One
$ErrorActionPreference = "Stop"

Write-Host "`n========================================" -ForegroundColor Cyan
Write-Host " CV64 STATIC BUILD - AUTOMATIC SETUP" -ForegroundColor Cyan
Write-Host "========================================`n" -ForegroundColor Cyan

Write-Host "[1/5] Restoring original integration file..." -ForegroundColor Yellow
if (Test-Path "src\cv64_m64p_integration_dynamic.cpp.backup") {
    Copy-Item "src\cv64_m64p_integration_dynamic.cpp.backup" "src\cv64_m64p_integration.cpp" -Force
    Write-Host "  [OK] Original file restored" -ForegroundColor Green
}

Write-Host "`n[2/5] Building mupen64plus-core as static library..." -ForegroundColor Yellow
$msbuild = & "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe" -latest -requires Microsoft.Component.MSBuild -find MSBuild\**\Bin\MSBuild.exe | Select-Object -First 1

& $msbuild "RMG\Source\3rdParty\mupen64plus-core\projects\msvc\mupen64plus-core.vcxproj" /t:Build /p:Configuration=Release /p:Platform=x64 /v:quiet /nologo
if ($LASTEXITCODE -eq 0) {
    Write-Host "  [OK] Core built successfully" -ForegroundColor Green
} else {
    Write-Host "  [FAILED] Core build failed" -ForegroundColor Red
    exit 1
}

Write-Host "`n[3/5] Building mupen64plus-rsp-hle as static library..." -ForegroundColor Yellow
& $msbuild "RMG\Source\3rdParty\mupen64plus-rsp-hle\projects\msvc\mupen64plus-rsp-hle.vcxproj" /t:Build /p:Configuration=Release /p:Platform=x64 /v:quiet /nologo
if ($LASTEXITCODE -eq 0) {
    Write-Host "  [OK] RSP built successfully" -ForegroundColor Green
} else {
    Write-Host "  [FAILED] RSP build failed" -ForegroundColor Red
    exit 1
}

Write-Host "`n[4/5] Verifying static libraries..." -ForegroundColor Yellow
$coreLib = Get-ChildItem -Path "RMG\Source\3rdParty\mupen64plus-core" -Recurse -Filter "mupen64plus-core-static.lib" -ErrorAction SilentlyContinue | Select-Object -First 1
$rspLib = Get-ChildItem -Path "RMG\Source\3rdParty\mupen64plus-rsp-hle" -Recurse -Filter "mupen64plus-rsp-hle-static.lib" -ErrorAction SilentlyContinue | Select-Object -First 1

if ($coreLib) {
    Write-Host "  [OK] Found core library: $($coreLib.FullName)" -ForegroundColor Green
} else {
    Write-Host "  [WARNING] Core library not found" -ForegroundColor Yellow
}

if ($rspLib) {
    Write-Host "  [OK] Found RSP library: $($rspLib.FullName)" -ForegroundColor Green
} else {
    Write-Host "  [WARNING] RSP library not found" -ForegroundColor Yellow
}

Write-Host "`n[5/5] Building CV64_RMG with dynamic linking (working version)..." -ForegroundColor Yellow
& $msbuild "CV64_RMG.vcxproj" /t:Build /p:Configuration=Release /p:Platform=x64 /v:minimal /nologo
if ($LASTEXITCODE -eq 0) {
    Write-Host "  [OK] CV64_RMG built successfully!" -ForegroundColor Green
} else {
    Write-Host "  [FAILED] Build failed" -ForegroundColor Red
    exit 1
}

Write-Host "`n========================================" -ForegroundColor Cyan
Write-Host " BUILD COMPLETE!" -ForegroundColor Green
Write-Host "========================================`n" -ForegroundColor Cyan

Write-Host "Output files:" -ForegroundColor Yellow
if (Test-Path "x64\Release\CV64_RMG.exe") {
    $size = (Get-Item "x64\Release\CV64_RMG.exe").Length / 1MB
    Write-Host "  [OK] CV64_RMG.exe - $([math]::Round($size, 2)) MB" -ForegroundColor Green
}

Write-Host "`n========================================" -ForegroundColor Cyan
Write-Host " STATIC LIBRARIES READY" -ForegroundColor Yellow
Write-Host "========================================" -ForegroundColor Cyan
Write-Host "The mupen64plus core and RSP are now built as static libraries." -ForegroundColor White
Write-Host "Currently CV64_RMG is using the WORKING dynamic version." -ForegroundColor White
Write-Host "`nTo switch to static linking:" -ForegroundColor Yellow
Write-Host "1. Replace src\cv64_m64p_integration.cpp with the static version" -ForegroundColor White
Write-Host "2. Add include paths to mupen64plus headers" -ForegroundColor White
Write-Host "3. Link against the .lib files" -ForegroundColor White
Write-Host "`nFor now, you have a WORKING build with all components!" -ForegroundColor Green
Write-Host "========================================`n" -ForegroundColor Cyan
