# Automatically configure CV64_RMG.vcxproj for static linking with mupen64plus
# This script modifies the project file to add includes, libraries, and references

$ErrorActionPreference = "Stop"

Write-Host "============================================" -ForegroundColor Cyan
Write-Host " Configure CV64_RMG for Static Linking" -ForegroundColor Cyan
Write-Host "============================================" -ForegroundColor Cyan
Write-Host ""

$projectFile = "CV64_RMG.vcxproj"

if (!(Test-Path $projectFile)) {
    Write-Host "[ERROR] CV64_RMG.vcxproj not found!" -ForegroundColor Red
    exit 1
}

Write-Host "Backing up project file..." -ForegroundColor Yellow
Copy-Item $projectFile "$projectFile.backup" -Force
Write-Host "[OK] Backup created: $projectFile.backup" -ForegroundColor Green

Write-Host "`nModifying project file..." -ForegroundColor Yellow

# Load project XML
$xml = [xml](Get-Content $projectFile)
$nsmgr = New-Object System.Xml.XmlNamespaceManager($xml.NameTable)
$nsmgr.AddNamespace("msbuild", "http://schemas.microsoft.com/developer/msbuild/2003")

# Add include directories
$itemDefGroups = $xml.SelectNodes("//msbuild:ItemDefinitionGroup", $nsmgr)
foreach ($group in $itemDefGroups) {
$clCompile = $group.SelectSingleNode("msbuild:ClCompile", $nsmgr)
if ($clCompile) {
    $addIncDirs = $clCompile.SelectSingleNode("msbuild:AdditionalIncludeDirectories", $nsmgr)
        if ($addIncDirs) {
            $includes = @(
                "`$(SolutionDir)RMG\Source\3rdParty\mupen64plus-core\src\api",
                "`$(SolutionDir)RMG\Source\3rdParty\mupen64plus-core\src",
                "`$(SolutionDir)RMG\Source\3rdParty\mupen64plus-rsp-hle\src"
            )
            
            foreach ($inc in $includes) {
                if ($addIncDirs.InnerText -notlike "*$inc*") {
                    $addIncDirs.InnerText = $inc + ";" + $addIncDirs.InnerText
                }
            }
            Write-Host "  [OK] Added include directories" -ForegroundColor Green
        }
        
        
        # Add preprocessor definitions
        $preproc = $clCompile.SelectSingleNode("msbuild:PreprocessorDefinitions", $nsmgr)
        if ($preproc -and $preproc.InnerText -notlike "*M64P_CORE_STATIC*") {
            $preproc.InnerText = "M64P_CORE_STATIC;" + $preproc.InnerText
            Write-Host "  [OK] Added M64P_CORE_STATIC definition" -ForegroundColor Green
        }
        
        # Set runtime library to /MT
        $runtimeLib = $clCompile.SelectSingleNode("msbuild:RuntimeLibrary", $nsmgr)
        if ($runtimeLib) {
            if ($runtimeLib.InnerText -eq "MultiThreadedDLL") {
                $runtimeLib.InnerText = "MultiThreaded"
                Write-Host "  [OK] Changed runtime to /MT" -ForegroundColor Green
            } elseif ($runtimeLib.InnerText -eq "MultiThreadedDebugDLL") {
                $runtimeLib.InnerText = "MultiThreadedDebug"
                Write-Host "  [OK] Changed runtime to /MTd" -ForegroundColor Green
            }
        }
    }
    
    
    # Add library dependencies
    $link = $group.SelectSingleNode("msbuild:Link", $nsmgr)
    if ($link) {
        $addDeps = $link.SelectSingleNode("msbuild:AdditionalDependencies", $nsmgr)
        if ($addDeps) {
            $libs = @(
                "mupen64plus-core-static.lib",
                "mupen64plus-rsp-hle-static.lib"
            )
            
            foreach ($lib in $libs) {
                if ($addDeps.InnerText -notlike "*$lib*") {
                    $addDeps.InnerText = $lib + ";" + $addDeps.InnerText
                }
            }
            Write-Host "  [OK] Added library dependencies" -ForegroundColor Green
        }
    }
}

# Add project references
$projectNode = $xml.SelectSingleNode("//msbuild:Project", $nsmgr)
if ($projectNode) {
    # Check if ItemGroup for ProjectReference exists
    $projRefGroup = $xml.SelectSingleNode("//msbuild:ItemGroup[msbuild:ProjectReference]", $nsmgr)
    if (!$projRefGroup) {
        $projRefGroup = $xml.CreateElement("ItemGroup", "http://schemas.microsoft.com/developer/msbuild/2003")
        $projectNode.AppendChild($projRefGroup) | Out-Null
    }
    
    # Add mupen64plus-core reference
    $coreRef = @{
        Path = "RMG\Source\3rdParty\mupen64plus-core\projects\msvc\mupen64plus-core.vcxproj"
        GUID = "{5A3B21CB-BA18-4CA6-AF99-85CE2DF9C1AD}"
        Name = "mupen64plus-core"
    }
    
    # Add RSP reference
    $rspRef = @{
        Path = "RMG\Source\3rdParty\mupen64plus-rsp-hle\projects\msvc\mupen64plus-rsp-hle.vcxproj"
        GUID = "{6F0B8F06-2977-4BAE-B3CE-AE349379F923}"
        Name = "mupen64plus-rsp-hle"
    }
    
    foreach ($ref in @($coreRef, $rspRef)) {
        # Check if reference already exists
        $existingRef = $projRefGroup.SelectSingleNode("msbuild:ProjectReference[@Include='$($ref.Path)']", $nsmgr)
        if (!$existingRef) {
            $projRef = $xml.CreateElement("ProjectReference", "http://schemas.microsoft.com/developer/msbuild/2003")
            $projRef.SetAttribute("Include", $ref.Path)
            
            $proj = $xml.CreateElement("Project", "http://schemas.microsoft.com/developer/msbuild/2003")
            $proj.InnerText = $ref.GUID
            $projRef.AppendChild($proj) | Out-Null
            
            $name = $xml.CreateElement("Name", "http://schemas.microsoft.com/developer/msbuild/2003")
            $name.InnerText = $ref.Name
            $projRef.AppendChild($name) | Out-Null
            
            $projRefGroup.AppendChild($projRef) | Out-Null
            Write-Host "  [OK] Added reference to $($ref.Name)" -ForegroundColor Green
        }
    }
}

# Save modified project
$xml.Save((Resolve-Path $projectFile))
Write-Host "`n[OK] Project file updated successfully" -ForegroundColor Green

Write-Host "`n============================================" -ForegroundColor Cyan
Write-Host " Next Steps" -ForegroundColor Cyan
Write-Host "============================================" -ForegroundColor Cyan
Write-Host ""
Write-Host "1. Run: .\configure_static_m64p.ps1" -ForegroundColor Yellow
Write-Host "   (Configures mupen64plus projects for static linking)" -ForegroundColor Gray
Write-Host ""
Write-Host "2. Open CV64_RMG.sln in Visual Studio" -ForegroundColor Yellow
Write-Host ""
Write-Host "3. Manually add projects to solution:" -ForegroundColor Yellow
Write-Host "   Right-click solution ? Add ? Existing Project" -ForegroundColor Gray
Write-Host "   - RMG\Source\3rdParty\mupen64plus-core\projects\msvc\mupen64plus-core.vcxproj" -ForegroundColor White
Write-Host "   - RMG\Source\3rdParty\mupen64plus-rsp-hle\projects\msvc\mupen64plus-rsp-hle.vcxproj" -ForegroundColor White
Write-Host ""
Write-Host "4. Update integration code:" -ForegroundColor Yellow
Write-Host "   Replace LoadLibrary calls with static function calls" -ForegroundColor Gray
Write-Host "   See: include\cv64_m64p_static.h" -ForegroundColor White
Write-Host ""
Write-Host "5. Build solution (Ctrl+Shift+B)" -ForegroundColor Yellow
Write-Host ""
Write-Host "============================================" -ForegroundColor Cyan
Write-Host " Benefits" -ForegroundColor Cyan
Write-Host "============================================" -ForegroundColor Cyan
Write-Host ""
Write-Host "[+] Single EXE file - no external DLLs needed" -ForegroundColor Green
Write-Host "[+] Faster - no LoadLibrary overhead" -ForegroundColor Green
Write-Host "[+] Better optimization - Link-Time Code Generation" -ForegroundColor Green
Write-Host "[+] Easier debugging - step into mupen64plus code" -ForegroundColor Green
Write-Host "[+] No DLL version conflicts" -ForegroundColor Green
Write-Host ""
