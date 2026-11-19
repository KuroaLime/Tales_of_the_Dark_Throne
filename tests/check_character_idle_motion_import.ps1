$ErrorActionPreference='Stop'
$resource=Join-Path (Split-Path $PSScriptRoot -Parent) 'Tower_Triumph/Tower_Triumph/Resource'
$out=Join-Path $PSScriptRoot 'bin/idle-motion-import-check'
$importer=Join-Path $PSScriptRoot 'import_character_idle_motion.ps1'
New-Item -ItemType Directory -Path $out -Force | Out-Null

# Reproduce all three packing modes from their preserved sources. This catches
# GDI+ format changes, endpoint conversion and dependence on prior output files.
foreach($kind in @('IdleCycle','WalkStop','IdleReturn')) {
    $name='Warrior_'+$kind
    $source=Join-Path $resource ($name+'.source.png')
    $alignment=Join-Path $resource ($name+'.alignment.json')
    $sourceHash=(Get-FileHash -LiteralPath $source -Algorithm SHA256).Hash
    & $importer -InputPath $source -OutputDirectory $out -Character Warrior -Kind $kind -AlignmentPath $alignment -BoundaryResourceDirectory $resource | Out-Null
    if((Get-FileHash -LiteralPath $source -Algorithm SHA256).Hash -ne $sourceHash){throw "$kind changed its original source"}
    if((Get-FileHash -LiteralPath (Join-Path $out ($name+'.bmp')) -Algorithm SHA256).Hash -ne
       (Get-FileHash -LiteralPath (Join-Path $resource ($name+'.bmp')) -Algorithm SHA256).Hash){throw "$kind is not reproducible"}
    $bytes=[IO.File]::ReadAllBytes((Join-Path $out ($name+'.bmp')))
    if([BitConverter]::ToUInt16($bytes,28) -ne 24){throw "$kind lost its 24-bit BMP format"}
}

# A missing endpoint must fail before publishing any replacement to a known-good
# destination. This is a real failure mode when transitions arrive before IDLE.
$knownGood=Join-Path $out 'Warrior_WalkStop.bmp'
$before=(Get-FileHash -LiteralPath $knownGood -Algorithm SHA256).Hash
$rejected=$false
try {
    & $importer -InputPath (Join-Path $resource 'Warrior_WalkStop.source.png') -OutputDirectory $out -Character Warrior -Kind WalkStop -AlignmentPath (Join-Path $resource 'Warrior_WalkStop.alignment.json') -BoundaryResourceDirectory (Join-Path $out 'missing-endpoints') | Out-Null
}catch{$rejected=$true}
if(!$rejected){throw 'A missing boundary source was accepted'}
if((Get-FileHash -LiteralPath $knownGood -Algorithm SHA256).Hash -ne $before){throw 'Failed import replaced the known-good destination'}
Write-Output 'PASS: idle, stop and return reproducible; source files unchanged; 24-bit BMP retained; missing endpoints cannot replace good output.'
