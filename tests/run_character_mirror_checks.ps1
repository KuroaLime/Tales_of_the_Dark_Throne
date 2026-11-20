param([switch]$BuildOnly)

# Link the isolated build's actual production Character/SkillData objects.
# -BuildOnly performs no native execution; all outputs remain in tests/bin.
$ErrorActionPreference = 'Stop'
$repositoryRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$objectRoot = Join-Path $PSScriptRoot 'bin/main-stage-build/obj'
$checkRoot = Join-Path $PSScriptRoot 'bin/character-mirror-checks'
$objects = @('Character.obj', 'SkillData.obj') | ForEach-Object {
    $candidate = Join-Path $objectRoot $_
    if (!(Test-Path -LiteralPath $candidate)) { throw 'Run tests/build_main_stage_isolated.ps1 first.' }
    $candidate
}
New-Item -ItemType Directory -Path $checkRoot -Force | Out-Null
$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio/Installer/vswhere.exe'
$visualStudio = & $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath | Select-Object -First 1
if (!$visualStudio) { throw 'Visual Studio C++ tools are required.' }
$toolDirectory = Get-ChildItem -LiteralPath (Join-Path $visualStudio 'VC/Tools/MSVC') -Directory |
    Where-Object { $_.Name -match '^\d+\.\d+\.\d+$' } |
    Sort-Object { [version]$_.Name } -Descending | Select-Object -First 1
$sdkRoot = (Get-ItemProperty -LiteralPath 'HKLM:/SOFTWARE/Microsoft/Windows Kits/Installed Roots' -ErrorAction SilentlyContinue).KitsRoot10
if (!$sdkRoot) { $sdkRoot = Join-Path ${env:ProgramFiles(x86)} 'Windows Kits/10' }
$sdkDirectory = Get-ChildItem -LiteralPath (Join-Path $sdkRoot 'Include') -Directory |
    Where-Object { $_.Name -match '^\d+\.\d+\.\d+\.\d+$' -and (Test-Path -LiteralPath (Join-Path $_.FullName 'um/windows.h')) } |
    Sort-Object { [version]$_.Name } -Descending | Select-Object -First 1
if (!$toolDirectory -or !$sdkDirectory) { throw 'A complete MSVC/Windows SDK installation is required.' }
$compiler = Join-Path $toolDirectory.FullName 'bin/Hostx64/x64/cl.exe'
$exe = Join-Path $checkRoot 'character_mirror_render_tests.exe'
$arguments = @('/nologo', '/std:c++17', '/utf-8', '/EHsc', '/W4', '/WX', '/MDd', '/D_DEBUG', '/DUNICODE', '/D_UNICODE',
    ('/I' + (Join-Path $toolDirectory.FullName 'include')),
    ('/I' + (Join-Path $sdkDirectory.FullName 'ucrt')), ('/I' + (Join-Path $sdkDirectory.FullName 'um')),
    ('/I' + (Join-Path $sdkDirectory.FullName 'shared')),
    ('/Fo:' + (Join-Path $checkRoot 'character_mirror_render_tests.obj')),
    ('/Fd:' + (Join-Path $checkRoot 'compiler.pdb')), ('/Fe:' + $exe),
    (Join-Path $PSScriptRoot 'character_mirror_render_tests.cpp')) + $objects + @(
    '/link', '/SUBSYSTEM:CONSOLE', '/DEBUG', ('/PDB:' + (Join-Path $checkRoot 'character_mirror_render_tests.pdb')),
    ('/LIBPATH:' + (Join-Path $toolDirectory.FullName 'lib/x64')),
    ('/LIBPATH:' + (Join-Path $sdkRoot ('Lib/' + $sdkDirectory.Name + '/ucrt/x64'))),
    ('/LIBPATH:' + (Join-Path $sdkRoot ('Lib/' + $sdkDirectory.Name + '/um/x64'))),
    'user32.lib', 'gdi32.lib', 'msimg32.lib', 'winmm.lib')
$originalCompilerPath = $env:PATH
try {
    $env:PATH = (Split-Path -Parent $compiler) + [IO.Path]::PathSeparator + $originalCompilerPath
    & $compiler @arguments
    if ($LASTEXITCODE -ne 0) { throw "Character mirror fixture compilation/link failed: $LASTEXITCODE" }
    if ($BuildOnly) {
        Write-Output 'PASS: actual Character mirror fixture compiled/linked; runtime pixel assertions were not executed.'
        return
    }
    & $exe (Join-Path $checkRoot 'synthetic-sprites')
    if ($LASTEXITCODE -ne 0) { throw "Character mirror pixel checks failed: $LASTEXITCODE" }
} finally { $env:PATH = $originalCompilerPath }
