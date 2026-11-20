param([switch]$BuildOnly)

# Uses the isolated game's actual object files. Never overwrites VS outputs,
# launches the game, creates a window, or reads/writes a saved game.
$ErrorActionPreference = 'Stop'
$repositoryRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$gameSource = Join-Path $repositoryRoot 'Tower_Triumph/Tower_Triumph'
$objectRoot = Join-Path $PSScriptRoot 'bin/main-stage-build/obj'
$checkRoot = Join-Path $PSScriptRoot 'bin/maintenance-map-ui-checks'
$testOutputPrefix = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot 'bin')) + [IO.Path]::DirectorySeparatorChar
foreach ($candidate in @($objectRoot, $checkRoot)) {
    if (![IO.Path]::GetFullPath($candidate).StartsWith($testOutputPrefix, [StringComparison]::OrdinalIgnoreCase)) {
        throw "Native test path escaped tests/bin: $candidate"
    }
}
New-Item -ItemType Directory -Path $checkRoot -Force | Out-Null
$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio/Installer/vswhere.exe'
$visualStudio = & $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath | Select-Object -First 1
if (!$visualStudio) { throw 'Visual Studio C++ tools are required.' }
$toolDirectory = Get-ChildItem -LiteralPath (Join-Path $visualStudio 'VC/Tools/MSVC') -Directory |
    Where-Object { $_.Name -match '^\d+\.\d+\.\d+$' } |
    Sort-Object { [version]$_.Name } -Descending | Select-Object -First 1
$sdkRoot = (Get-ItemProperty -LiteralPath 'HKLM:/SOFTWARE/Microsoft/Windows Kits/Installed Roots').KitsRoot10
$sdkDirectory = Get-ChildItem -LiteralPath (Join-Path $sdkRoot 'Include') -Directory |
    Where-Object { Test-Path -LiteralPath (Join-Path $_.FullName 'um/windows.h') } |
    Sort-Object { [version]$_.Name } -Descending | Select-Object -First 1
$compiler = Join-Path $toolDirectory.FullName 'bin/Hostx64/x64/cl.exe'
$exe = Join-Path $checkRoot 'maintenance_map_ui_checks.exe'
$objects = @(Get-ChildItem -LiteralPath $objectRoot -Filter '*.obj' |
    Where-Object { $_.BaseName -ne 'Framework' } | ForEach-Object { $_.FullName })
if ($objects.Count -lt 30) { throw 'Run tests/build_main_stage_isolated.ps1 before the UI check.' }
$arguments = @('/nologo', '/std:c++latest', '/utf-8', '/EHsc', '/W3', '/MDd', '/D_DEBUG', '/DUNICODE', '/D_UNICODE',
    ('/I' + $gameSource), ('/I' + (Join-Path $toolDirectory.FullName 'include')),
    ('/I' + (Join-Path $sdkDirectory.FullName 'ucrt')), ('/I' + (Join-Path $sdkDirectory.FullName 'um')),
    ('/I' + (Join-Path $sdkDirectory.FullName 'shared')),
    ('/Fo:' + (Join-Path $checkRoot 'maintenance_map_ui_checks.obj')),
    ('/Fd:' + (Join-Path $checkRoot 'compiler.pdb')), ('/Fe:' + $exe),
    (Join-Path $PSScriptRoot 'maintenance_map_ui_checks.cpp')) + $objects + @(
    (Join-Path $objectRoot 'Resource.res'), '/link', '/SUBSYSTEM:CONSOLE', '/DEBUG', '/OPT:NOICF', '/OPT:NOREF',
    ('/PDB:' + (Join-Path $checkRoot 'maintenance_map_ui_checks.pdb')),
    ('/LIBPATH:' + (Join-Path $toolDirectory.FullName 'lib/x64')),
    ('/LIBPATH:' + (Join-Path $sdkRoot ('Lib/' + $sdkDirectory.Name + '/ucrt/x64'))),
    ('/LIBPATH:' + (Join-Path $sdkRoot ('Lib/' + $sdkDirectory.Name + '/um/x64'))),
    'user32.lib', 'gdi32.lib', 'msimg32.lib', 'winmm.lib', 'shell32.lib', 'ole32.lib')
$originalCompilerPath = $env:PATH
$executionStatus = [ordered]@{
    timestampUtc = [DateTime]::UtcNow.ToString('o')
    compilePassed = $false
    runAttempted = $false
    runPassed = $false
    originalReportIsCurrent = $false
    sourceSha256 = (Get-FileHash -LiteralPath (Join-Path $PSScriptRoot 'maintenance_map_ui_checks.cpp') -Algorithm SHA256).Hash.ToLowerInvariant()
    executableSha256 = $null
    error = $null
    scope = 'Actual offscreen production objects; no game, audio, or saved-game access.'
}
try {
    $env:PATH = (Split-Path -Parent $compiler) + [IO.Path]::PathSeparator + $originalCompilerPath
    & $compiler @arguments
    if ($LASTEXITCODE -ne 0) { throw "Maintenance/map harness compilation/link failed: $LASTEXITCODE" }
    $executionStatus.compilePassed = $true
    $executionStatus.executableSha256 = (Get-FileHash -LiteralPath $exe -Algorithm SHA256).Hash.ToLowerInvariant()
    if ($BuildOnly) { Write-Output 'PASS: native Maintenance/map harness compiled; not executed.'; return }
    $executionStatus.runAttempted = $true
    & $exe $gameSource $checkRoot
    if ($LASTEXITCODE -ne 0) { throw "Native maintenance/map checks failed: $LASTEXITCODE" }
    $executionStatus.runPassed = $true
    $executionStatus.originalReportIsCurrent = $true
} catch {
    $executionStatus.error = $_.Exception.Message
    throw
} finally {
    $env:PATH = $originalCompilerPath
    $executionStatus | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath (Join-Path $checkRoot 'execution-status.json') -Encoding UTF8
}
