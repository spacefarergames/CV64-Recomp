# Configure mupen64plus projects for static linking
# This script modifies the RMG projects to build as static libraries

$ErrorActionPreference = "Stop"

Write-Host "============================================" -ForegroundColor Cyan
Write-Host " Configure mupen64plus for Static Linking" -ForegroundColor Cyan
Write-Host "============================================" -ForegroundColor Cyan
Write-Host ""

# Projects to configure
$projects = @(
    @{
        Name = "mupen64plus-core"
        Path = "RMG\Source\3rdParty\mupen64plus-core\projects\msvc\mupen64plus-core.vcxproj"
        OutputName = "mupen64plus-core-static"
    },
    @{
        Name = "mupen64plus-rsp-hle"
        Path = "RMG\Source\3rdParty\mupen64plus-rsp-hle\projects\msvc\mupen64plus-rsp-hle.vcxproj"
        OutputName = "mupen64plus-rsp-hle-static"
    }
)

function Update-ProjectForStaticLib {
    param(
        [string]$ProjectPath,
        [string]$OutputName
    )
    
    if (!(Test-Path $ProjectPath)) {
        Write-Host "  [SKIP] Project not found: $ProjectPath" -ForegroundColor Yellow
        return $false
    }
    
    Write-Host "  Modifying $ProjectPath..." -ForegroundColor Yellow
    
    # Read project file
    $xml = [xml](Get-Content $ProjectPath)
    $nsmgr = New-Object System.Xml.XmlNamespaceManager($xml.NameTable)
    $nsmgr.AddNamespace("msbuild", "http://schemas.microsoft.com/developer/msbuild/2003")
    
    # Find all PropertyGroup elements
    $propertyGroups = $xml.SelectNodes("//msbuild:PropertyGroup", $nsmgr)
    
    foreach ($group in $propertyGroups) {
        # Change Configuration Type to StaticLibrary
        $configType = $group.SelectSingleNode("msbuild:ConfigurationType", $nsmgr)
        if ($configType) {
            $configType.InnerText = "StaticLibrary"
            Write-Host "    - Set ConfigurationType to StaticLibrary" -ForegroundColor Green
        }
        
        # Change target extension
        $targetExt = $group.SelectSingleNode("msbuild:TargetExt", $nsmgr)
        if ($targetExt) {
            $targetExt.InnerText = ".lib"
        }
        
        # Update output name
        $targetName = $group.SelectSingleNode("msbuild:TargetName", $nsmgr)
        if ($targetName -and $OutputName) {
            $targetName.InnerText = $OutputName
        }
    }
    
    # Find all ItemDefinitionGroup elements (compiler/linker settings)
    $itemDefGroups = $xml.SelectNodes("//msbuild:ItemDefinitionGroup", $nsmgr)
    
    foreach ($group in $itemDefGroups) {
        $clCompile = $group.SelectSingleNode("msbuild:ClCompile", $nsmgr)
        if ($clCompile) {
            # Change runtime library to static
            $runtimeLib = $clCompile.SelectSingleNode("msbuild:RuntimeLibrary", $nsmgr)
            if ($runtimeLib) {
                # Change to /MT (MultiThreaded) for Release, /MTd for Debug
                if ($runtimeLib.InnerText -like "*Debug*" -or $runtimeLib.InnerText -eq "MultiThreadedDebugDLL") {
                    $runtimeLib.InnerText = "MultiThreadedDebug"
                    Write-Host "    - Set RuntimeLibrary to /MTd" -ForegroundColor Green
                } else {
                    $runtimeLib.InnerText = "MultiThreaded"
                    Write-Host "    - Set RuntimeLibrary to /MT" -ForegroundColor Green
                }
            } else {
                # Add runtime library node
                $newRuntimeLib = $xml.CreateElement("RuntimeLibrary", "http://schemas.microsoft.com/developer/msbuild/2003")
                $newRuntimeLib.InnerText = "MultiThreaded"
                $clCompile.AppendChild($newRuntimeLib) | Out-Null
            }
            
            # Add M64P_CORE_STATIC preprocessor define
            $preproc = $clCompile.SelectSingleNode("msbuild:PreprocessorDefinitions", $nsmgr)
            if ($preproc) {
                if ($preproc.InnerText -notlike "*M64P_CORE_STATIC*") {
                    $preproc.InnerText = "M64P_CORE_STATIC;" + $preproc.InnerText
                    Write-Host "    - Added M64P_CORE_STATIC define" -ForegroundColor Green
                }
            }
        }
    }
    
    # Save modified project
    $backupPath = $ProjectPath + ".backup"
    Copy-Item $ProjectPath $backupPath -Force
    Write-Host "    - Backup created: $backupPath" -ForegroundColor Cyan
    
    $xml.Save($ProjectPath)
    Write-Host "    [OK] Project modified successfully" -ForegroundColor Green
    
    return $true
}

# Process each project
foreach ($project in $projects) {
    Write-Host "`nProcessing $($project.Name)..." -ForegroundColor Yellow
    $success = Update-ProjectForStaticLib -ProjectPath $project.Path -OutputName $project.OutputName
    
    if (!$success) {
        Write-Host "[WARNING] Failed to process $($project.Name)" -ForegroundColor Red
    }
}

Write-Host "`n============================================" -ForegroundColor Cyan
Write-Host " Manual Steps Required" -ForegroundColor Cyan
Write-Host "============================================" -ForegroundColor Cyan
Write-Host ""
Write-Host "1. Open CV64_RMG.sln in Visual Studio" -ForegroundColor Yellow
Write-Host "2. Right-click solution ? Add ? Existing Project" -ForegroundColor Yellow
Write-Host "   Add:" -ForegroundColor Yellow
Write-Host "   - RMG\Source\3rdParty\mupen64plus-core\projects\msvc\mupen64plus-core.vcxproj" -ForegroundColor White
Write-Host "   - RMG\Source\3rdParty\mupen64plus-rsp-hle\projects\msvc\mupen64plus-rsp-hle.vcxproj" -ForegroundColor White
Write-Host ""
Write-Host "3. Right-click CV64_RMG project ? Properties" -ForegroundColor Yellow
Write-Host "   ? C/C++ ? General ? Additional Include Directories" -ForegroundColor Yellow
Write-Host "   Add:" -ForegroundColor Yellow
Write-Host "   - `$(SolutionDir)RMG\Source\3rdParty\mupen64plus-core\src\api" -ForegroundColor White
Write-Host ""
Write-Host "4. Right-click CV64_RMG project ? Properties" -ForegroundColor Yellow
Write-Host "   ? Linker ? Input ? Additional Dependencies" -ForegroundColor Yellow
Write-Host "   Add:" -ForegroundColor Yellow
Write-Host "   - mupen64plus-core-static.lib" -ForegroundColor White
Write-Host "   - mupen64plus-rsp-hle-static.lib" -ForegroundColor White
Write-Host ""
Write-Host "5. Right-click CV64_RMG project ? Add ? Reference" -ForegroundColor Yellow
Write-Host "   Check:" -ForegroundColor Yellow
Write-Host "   - mupen64plus-core" -ForegroundColor White
Write-Host "   - mupen64plus-rsp-hle" -ForegroundColor White
Write-Host ""
Write-Host "6. Build solution" -ForegroundColor Yellow
Write-Host ""
Write-Host "============================================" -ForegroundColor Cyan
Write-Host "Or run: configure_cv64_for_static.ps1" -ForegroundColor Green
Write-Host "============================================" -ForegroundColor Cyan
