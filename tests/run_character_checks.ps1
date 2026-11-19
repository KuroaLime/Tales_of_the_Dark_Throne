param([string]$BaselinePath = '', [string]$BuildDirectory = '')

# Compile-only verification: native tests and the game are never launched here.
$ErrorActionPreference = 'Stop'
if (!$BuildDirectory) { $BuildDirectory = Join-Path $PSScriptRoot 'bin/character-checks' }
$buildDirectory = [System.IO.Path]::GetFullPath($BuildDirectory)
$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio/Installer/vswhere.exe'
if (!(Test-Path -LiteralPath $vswhere)) { throw 'Visual Studio Installer/vswhere.exe is required.' }
$visualStudio = & $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath | Select-Object -First 1
if (!$visualStudio) { throw 'No Visual Studio C++ installation was found.' }
$compilerDirectory = Get-ChildItem -LiteralPath (Join-Path $visualStudio 'VC/Tools/MSVC') -Directory |
    Where-Object { $_.Name -match '^\d+\.\d+\.\d+$' -and (Test-Path -LiteralPath (Join-Path $_.FullName 'bin/Hostx64/x64/cl.exe')) } |
    Sort-Object { [version]$_.Name } -Descending | Select-Object -First 1
if (!$compilerDirectory) { throw 'No usable MSVC x64 compiler was found.' }
$sdkRoot = (Get-ItemProperty -LiteralPath 'HKLM:/SOFTWARE/Microsoft/Windows Kits/Installed Roots' -ErrorAction SilentlyContinue).KitsRoot10
if (!$sdkRoot) { $sdkRoot = Join-Path ${env:ProgramFiles(x86)} 'Windows Kits/10' }
$sdkDirectory = Get-ChildItem -LiteralPath (Join-Path $sdkRoot 'Include') -Directory |
    Where-Object { $_.Name -match '^\d+\.\d+\.\d+\.\d+$' -and (Test-Path -LiteralPath (Join-Path $_.FullName 'ucrt/stdio.h')) } |
    Sort-Object { [version]$_.Name } -Descending | Select-Object -First 1
if (!$sdkDirectory) { throw 'No complete Windows SDK was found.' }
New-Item -ItemType Directory -Path $buildDirectory -Force | Out-Null
foreach ($fixture in @(
    @('generate_turn_completion_check.py', 'turn_completion_compile'),
    @('generate_reaction_sound_check.py', 'reaction_sound_compile'),
    @('generate_skill_target_input_check.py', 'skill_target_input_compile'),
    @('generate_enemy_loader_check.py', 'enemy_loader_compile'))) {
    & python -B (Join-Path $PSScriptRoot $fixture[0]) --output (Join-Path $buildDirectory ($fixture[1] + '.cpp'))
    if ($LASTEXITCODE -ne 0) { throw "$($fixture[0]) source fixture generation failed." }
}
$compiler = Join-Path $compilerDirectory.FullName 'bin/Hostx64/x64/cl.exe'
$oldCompilerPath = $env:PATH
try {
    $env:PATH = (Split-Path -Parent $compiler) + [System.IO.Path]::PathSeparator + $oldCompilerPath
    foreach ($testName in @('character_motion_compile', 'character_sprite_layout_tests', 'battle_action_timing_compile', 'battle_skill_area_compile', 'area_skill_visuals_compile', 'area_effect_renderer_compile', 'skill_presentation_compile', 'standard_skill_compile', 'enemy_animation_compile', 'turn_completion_compile', 'reaction_sound_compile', 'skill_target_input_compile', 'enemy_loader_compile')) {
        $testSource = Join-Path $PSScriptRoot ($testName + '.cpp')
        $testLanguage = '/std:c++17'
        if ($testName -in @('turn_completion_compile', 'skill_target_input_compile', 'enemy_loader_compile')) {
            $testSource = Join-Path $buildDirectory ($testName + '.cpp')
            $testLanguage = '/std:c++20'
        }
        if ($testName -eq 'reaction_sound_compile') { $testSource = Join-Path $buildDirectory ($testName + '.cpp') }
        $arguments = @('/nologo', $testLanguage, '/utf-8', '/EHsc', '/W4', '/WX', '/c',
            ('/I' + (Join-Path $compilerDirectory.FullName 'include')),
            ('/I' + (Join-Path $sdkDirectory.FullName 'ucrt')),
            ('/I' + (Join-Path $sdkDirectory.FullName 'um')),
            ('/I' + (Join-Path $sdkDirectory.FullName 'shared')),
            ('/I' + (Join-Path $PSScriptRoot '../Tower_Triumph/Tower_Triumph')),
            ('/Fo:' + (Join-Path $buildDirectory ($testName + '.obj'))),
            ('/Fd:' + (Join-Path $buildDirectory ($testName + '.pdb'))),
            $testSource)
        & $compiler @arguments
        if ($LASTEXITCODE -ne 0) { throw "$testName compilation failed: $LASTEXITCODE" }
    }
} finally { $env:PATH = $oldCompilerPath }
Write-Output 'PASS: production motion arithmetic static_asserts and sprite-layout compile checks.'
Write-Output 'NOTE: sprite runtime fixture assertions are compiled, not executed; no native game/FPS/GDI claim.'
$pythonArguments = @('-B', (Join-Path $PSScriptRoot 'verify_character_rendering.py'))
if ($BaselinePath) { $pythonArguments += @('--baseline', $BaselinePath) }
& python @pythonArguments
if ($LASTEXITCODE -ne 0) { throw 'Character source/asset checks failed.' }
& python -B (Join-Path $PSScriptRoot 'verify_standard_skill_integration.py') --output (Join-Path $buildDirectory 'standard-skills-integration.json')
if ($LASTEXITCODE -ne 0) { throw 'Standard skill integration checks failed.' }
& python -B (Join-Path $PSScriptRoot 'verify_runtime_asset_root.py')
if ($LASTEXITCODE -ne 0) { throw 'Runtime asset root checks failed.' }
