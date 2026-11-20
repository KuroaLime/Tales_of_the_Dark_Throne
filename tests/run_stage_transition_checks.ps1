param([switch]$BuildOnly)

# Header-only production compositor, linked to real GDI. No game launch or save access.
$ErrorActionPreference = 'Stop'
$repositoryRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$gameSource = Join-Path $repositoryRoot 'Tower_Triumph/Tower_Triumph'
$checkRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot 'bin/stage-transition-checks'))
$outputPrefix = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot 'bin')) + [IO.Path]::DirectorySeparatorChar
if (!$checkRoot.StartsWith($outputPrefix, [StringComparison]::OrdinalIgnoreCase)) { throw 'Test output escaped tests/bin.' }
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
$exe = Join-Path $checkRoot 'stage_transition_checks.exe'
$arguments = @('/nologo', '/std:c++latest', '/utf-8', '/EHsc', '/W4', '/MDd', '/D_DEBUG', '/DUNICODE', '/D_UNICODE',
    ('/I' + $gameSource), ('/I' + (Join-Path $toolDirectory.FullName 'include')),
    ('/I' + (Join-Path $sdkDirectory.FullName 'ucrt')), ('/I' + (Join-Path $sdkDirectory.FullName 'um')),
    ('/I' + (Join-Path $sdkDirectory.FullName 'shared')),
    ('/Fo:' + (Join-Path $checkRoot 'stage_transition_checks.obj')),
    ('/Fd:' + (Join-Path $checkRoot 'compiler.pdb')), ('/Fe:' + $exe),
    (Join-Path $PSScriptRoot 'stage_transition_checks.cpp'), '/link', '/SUBSYSTEM:CONSOLE',
    ('/PDB:' + (Join-Path $checkRoot 'stage_transition_checks.pdb')),
    ('/LIBPATH:' + (Join-Path $toolDirectory.FullName 'lib/x64')),
    ('/LIBPATH:' + (Join-Path $sdkRoot ('Lib/' + $sdkDirectory.Name + '/ucrt/x64'))),
    ('/LIBPATH:' + (Join-Path $sdkRoot ('Lib/' + $sdkDirectory.Name + '/um/x64'))),
    'user32.lib', 'gdi32.lib', 'msimg32.lib')
$originalCompilerPath = $env:PATH
try {
    $env:PATH = (Split-Path -Parent $compiler) + [IO.Path]::PathSeparator + $originalCompilerPath
    & $compiler @arguments
    if ($LASTEXITCODE -ne 0) { throw "Stage transition check compilation failed: $LASTEXITCODE" }
    if ($BuildOnly) { Write-Output 'PASS: stage transition check compiled; not executed.'; return }
    & $exe (Join-Path $checkRoot 'checks.json')
    if ($LASTEXITCODE -ne 0) { throw "Stage transition checks failed: $LASTEXITCODE" }
} finally { $env:PATH = $originalCompilerPath }
