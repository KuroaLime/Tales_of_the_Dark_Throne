param(
    [string]$OutputDirectory = (Join-Path $PSScriptRoot 'bin/graveyard-preview'),
    [switch]$BuildOnly
)

$ErrorActionPreference = 'Stop'
$repositoryRoot = Split-Path -Parent $PSScriptRoot
$gameSource = Join-Path $repositoryRoot 'Tower_Triumph/Tower_Triumph'
$buildDirectory = Join-Path $PSScriptRoot 'bin'
$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio/Installer/vswhere.exe'
if (!(Test-Path -LiteralPath $vswhere)) {
    throw 'Visual Studio Installer/vswhere.exe is required to discover the C++ toolchain.'
}
$visualStudio = & $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath | Select-Object -First 1
if (!$visualStudio) { throw 'No Visual Studio installation with C++ x64 tools was found.' }
$compilerDirectory = Get-ChildItem -LiteralPath (Join-Path $visualStudio 'VC/Tools/MSVC') -Directory |
    Where-Object { $_.Name -match '^\d+\.\d+\.\d+$' -and (Test-Path -LiteralPath (Join-Path $_.FullName 'bin/Hostx64/x64/cl.exe')) } |
    Sort-Object { [version]$_.Name } -Descending | Select-Object -First 1
if (!$compilerDirectory) { throw 'No usable MSVC x64 compiler was found.' }
$compilerRoot = $compilerDirectory.FullName
$sdkRoot = (Get-ItemProperty -LiteralPath 'HKLM:/SOFTWARE/Microsoft/Windows Kits/Installed Roots' -ErrorAction SilentlyContinue).KitsRoot10
if (!$sdkRoot) { $sdkRoot = Join-Path ${env:ProgramFiles(x86)} 'Windows Kits/10' }
$sdkDirectory = Get-ChildItem -LiteralPath (Join-Path $sdkRoot 'Include') -Directory |
    Where-Object {
        $_.Name -match '^\d+\.\d+\.\d+\.\d+$' -and
        (Test-Path -LiteralPath (Join-Path $_.FullName 'um/windows.h')) -and
        (Test-Path -LiteralPath (Join-Path $_.FullName 'ucrt/stdio.h')) -and
        (Test-Path -LiteralPath (Join-Path $sdkRoot ('Lib/' + $_.Name + '/um/x64/gdi32.lib')))
    } | Sort-Object { [version]$_.Name } -Descending | Select-Object -First 1
if (!$sdkDirectory) { throw 'No complete Windows SDK with x64 libraries was found.' }
$sdkVersion = $sdkDirectory.Name
$compiler = Join-Path $compilerRoot 'bin/Hostx64/x64/cl.exe'
if (!(Test-Path -LiteralPath $compiler)) {
    throw "Missing MSVC compiler: $compiler"
}
New-Item -ItemType Directory -Path $buildDirectory -Force | Out-Null
$OutputDirectory = [System.IO.Path]::GetFullPath($OutputDirectory)
New-Item -ItemType Directory -Path $OutputDirectory -Force | Out-Null
$smokeExe = Join-Path $buildDirectory 'graveyard_render_smoke.exe'
$compilerArguments = @(
    '/nologo', '/std:c++17', '/utf-8', '/EHsc', '/W4', '/DNOMINMAX',
    '/DUNICODE', '/D_UNICODE',
    ('/I' + $gameSource),
    ('/I' + (Join-Path $compilerRoot 'include')),
    ('/I' + (Join-Path $sdkRoot ('Include/' + $sdkVersion + '/ucrt'))),
    ('/I' + (Join-Path $sdkRoot ('Include/' + $sdkVersion + '/um'))),
    ('/I' + (Join-Path $sdkRoot ('Include/' + $sdkVersion + '/shared'))),
    ('/Fe:' + $smokeExe),
    (Join-Path $PSScriptRoot 'graveyard_render_smoke.cpp'),
    (Join-Path $gameSource 'BattleMapData.cpp'),
    (Join-Path $gameSource 'GraveyardRenderer.cpp'),
    '/link',
    ('/LIBPATH:' + (Join-Path $compilerRoot 'lib/x64')),
    ('/LIBPATH:' + (Join-Path $sdkRoot ('Lib/' + $sdkVersion + '/ucrt/x64'))),
    ('/LIBPATH:' + (Join-Path $sdkRoot ('Lib/' + $sdkVersion + '/um/x64'))),
    'gdi32.lib', 'user32.lib'
)
$originalCompilerPath = $env:PATH
$env:PATH = (Split-Path -Parent $compiler) + [System.IO.Path]::PathSeparator + $originalCompilerPath
Push-Location -LiteralPath $buildDirectory
try {
    Write-Output "MSVC $($compilerDirectory.Name); Windows SDK $sdkVersion"
    & $compiler @compilerArguments
    if ($LASTEXITCODE -ne 0) { throw "Native smoke-test compilation failed: $LASTEXITCODE" }
    if ($BuildOnly) { return }
    & $smokeExe $gameSource $OutputDirectory
    if ($LASTEXITCODE -ne 0) { throw "Native smoke test failed: $LASTEXITCODE" }
}
finally {
    Pop-Location
    $env:PATH = $originalCompilerPath
}
