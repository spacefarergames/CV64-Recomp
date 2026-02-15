# apply_static_integration.ps1
# Run this script to complete the static mupen64plus integration

$ErrorActionPreference = "Stop"
$projectRoot = Split-Path -Parent $MyInvocation.MyCommand.Path

Write-Host "=== Applying Static mupen64plus Integration ===" -ForegroundColor Cyan

# Step 1: Update CV64_RMG.slnx
Write-Host "`n[1/3] Updating solution file..." -ForegroundColor Yellow

$slnxContent = @'
<Solution>
  <Configurations>
    <Platform Name="x64" />
    <Platform Name="x86" />
  </Configurations>
  
  <Folder Name="/3rdParty/">
    <Project Path="mupen64plus-core-static/mupen64plus-core-static.vcxproj" Id="a1b2c3d4-e5f6-7890-abcd-ef1234567890" />
  </Folder>
  
  <Project Path="CV64_RMG.vcxproj" Id="16ef6c11-b177-4dcb-b3d4-1bf1f68eb923">
    <Dependency Project="mupen64plus-core-static/mupen64plus-core-static.vcxproj" />
  </Project>
</Solution>
'@

Set-Content -Path "$projectRoot\CV64_RMG.slnx" -Value $slnxContent -Encoding UTF8
Write-Host "  Solution file updated." -ForegroundColor Green

# Step 2: Update CV64_RMG.vcxproj
Write-Host "`n[2/3] Updating project file..." -ForegroundColor Yellow

$vcxprojPath = "$projectRoot\CV64_RMG.vcxproj"
$vcxprojContent = Get-Content $vcxprojPath -Raw

# Check if already modified
if ($vcxprojContent -match "mupen64plus-static\.props") {
    Write-Host "  Project already configured for static linking." -ForegroundColor Green
} else {
    # Add props import and project reference before Microsoft.Cpp.targets
    $replacement = @'
<Import Project="mupen64plus-static.props" />
  <ItemGroup>
    <ProjectReference Include="mupen64plus-core-static\mupen64plus-core-static.vcxproj">
      <Project>{a1b2c3d4-e5f6-7890-abcd-ef1234567890}</Project>
    </ProjectReference>
  </ItemGroup>
  <Import Project="$(VCTargetsPath)\Microsoft.Cpp.targets" />
'@
    
    $vcxprojContent = $vcxprojContent -replace '<Import Project="\$\(VCTargetsPath\)\\Microsoft\.Cpp\.targets" />', $replacement
    Set-Content -Path $vcxprojPath -Value $vcxprojContent -NoNewline
    Write-Host "  Project file updated." -ForegroundColor Green
}

# Step 3: Verify files exist
Write-Host "`n[3/3] Verifying files..." -ForegroundColor Yellow

$requiredFiles = @(
    "mupen64plus-core-static\mupen64plus-core-static.vcxproj",
    "mupen64plus-static.props",
    "include\cv64_m64p_static_wrapper.h",
    "src\cv64_m64p_integration_static.cpp"
)

$allGood = $true
foreach ($file in $requiredFiles) {
    $fullPath = Join-Path $projectRoot $file
    if (Test-Path $fullPath) {
        Write-Host "  [OK] $file" -ForegroundColor Green
    } else {
        Write-Host "  [MISSING] $file" -ForegroundColor Red
        $allGood = $false
    }
}

if ($allGood) {
    Write-Host "`n=== Integration Complete! ===" -ForegroundColor Cyan
    Write-Host @"

Next steps:
1. Open CV64_RMG.slnx in Visual Studio
2. Build the solution (mupen64plus-core-static will build first)
3. The static library will be linked into your executable

To enable static linking in your code, define CV64_STATIC_MUPEN64PLUS
in your preprocessor definitions.
"@
} else {
    Write-Host "`n=== Some files are missing! ===" -ForegroundColor Red
}
