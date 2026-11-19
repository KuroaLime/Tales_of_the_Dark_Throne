param([switch]$EvaluateOnly)

# All compiler, linker, PDB and tracking outputs stay out of Visual Studio's
# live Debug/Release folders. This script never launches native executables.
$ErrorActionPreference = 'Stop'
$repositoryRoot = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$buildRoot = Join-Path $PSScriptRoot 'bin/skill-wiring-build'
$objectDirectory = (Join-Path $buildRoot 'obj') + [System.IO.Path]::DirectorySeparatorChar
$outputDirectory = (Join-Path $buildRoot 'bin') + [System.IO.Path]::DirectorySeparatorChar
$trackingDirectory = (Join-Path $objectDirectory 'tlog') + [System.IO.Path]::DirectorySeparatorChar
$checkDirectory = Join-Path $buildRoot 'checks'
New-Item -ItemType Directory -Path $objectDirectory,$outputDirectory,$trackingDirectory,$checkDirectory -Force | Out-Null
$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio/Installer/vswhere.exe'
$visualStudio = & $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath | Select-Object -First 1
if (!$visualStudio) { throw 'No Visual Studio C++ installation was found.' }
$msbuild = Join-Path $visualStudio 'MSBuild/Current/Bin/MSBuild.exe'
$project = Join-Path $repositoryRoot 'Tower_Triumph/Tower_Triumph/Tower_Triumph.vcxproj'
$properties = @($project, '/nologo', '/m:1', '/nr:false', '/p:Configuration=Debug', '/p:Platform=x64',
    '/p:PlatformToolset=v145', '/p:PreferredToolArchitecture=x64', '/p:BuildProjectReferences=false',
    '/p:TargetName=Tower_Triumph_SkillWiringCheck', ('/p:IntDir=' + $objectDirectory),
    ('/p:IntermediateOutputPath=' + $objectDirectory), ('/p:OutDir=' + $outputDirectory),
    ('/p:TLogLocation=' + $trackingDirectory))

# Evaluation performs no build targets. Confirm the actual inherited /Fo /Fd
# and resource output metadata before a compiler or linker can write anything.
$evaluationText = & $msbuild @properties '-getProperty:OutDir,IntDir,TargetPath,TargetName,TLogLocation' '-getItem:ClCompile,ResourceCompile'
if ($LASTEXITCODE -ne 0) { throw 'Isolated build path evaluation failed.' }
$evaluationText | Set-Content -LiteralPath (Join-Path $buildRoot 'evaluated-paths.json') -Encoding UTF8
$evaluation = ($evaluationText -join [Environment]::NewLine) | ConvertFrom-Json
$rootPrefix = [System.IO.Path]::GetFullPath($buildRoot).TrimEnd('\','/') + [System.IO.Path]::DirectorySeparatorChar
function Assert-IsolatedPath([string]$candidate, [string]$description) {
    if (!$candidate -or ![System.IO.Path]::IsPathRooted($candidate) -or
        ![System.IO.Path]::GetFullPath($candidate).StartsWith($rootPrefix, [StringComparison]::OrdinalIgnoreCase)) {
        throw "Output escaped isolated build folder: $description = $candidate"
    }
}
foreach ($name in @('OutDir','IntDir','TargetPath','TLogLocation')) {
    Assert-IsolatedPath $evaluation.Properties.$name $name
}
foreach ($item in $evaluation.Items.ClCompile) {
    Assert-IsolatedPath $item.ObjectFileName ($item.Identity + ' object')
    Assert-IsolatedPath $item.ProgramDataBaseFileName ($item.Identity + ' compiler PDB')
}
foreach ($item in $evaluation.Items.ResourceCompile) {
    Assert-IsolatedPath $item.ResourceOutputFileName ($item.Identity + ' resources')
}
# Microsoft.Link.Common.props derives Link.ProgramDatabaseFile from the same
# absolute OutDir and TargetName; the project contains no overriding Link path.
Assert-IsolatedPath (Join-Path $evaluation.Properties.OutDir ($evaluation.Properties.TargetName + '.pdb')) 'linker PDB'
if ($EvaluateOnly) {
    Write-Output 'PASS: evaluated compiler/resource/linker paths stay in the isolated build tree.'
    return
}

& (Join-Path $PSScriptRoot 'run_character_checks.ps1') -BaselinePath (Join-Path $PSScriptRoot 'bin/character-map-baseline.json') -BuildDirectory $checkDirectory *> (Join-Path $buildRoot 'checks.log')
if ($LASTEXITCODE -ne 0) { throw 'Isolated compile/source checks failed.' }
Get-Content -LiteralPath (Join-Path $buildRoot 'checks.log')

& $msbuild @properties '/t:Build' '/v:minimal' '/fl' ('/flp:logfile=' + (Join-Path $buildRoot 'build.log') + ';verbosity=normal')
if ($LASTEXITCODE -ne 0) { throw "Isolated project build failed: $LASTEXITCODE" }
Write-Output ('PASS: isolated build output: ' + $evaluation.Properties.TargetPath)
