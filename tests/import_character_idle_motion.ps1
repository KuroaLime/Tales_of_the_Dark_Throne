param(
    [Parameter(Mandatory=$true)][string]$InputPath,
    [Parameter(Mandatory=$true)][string]$OutputDirectory,
    [Parameter(Mandatory=$true)][ValidateSet('Warrior','Archer','Mage')][string]$Character,
    [Parameter(Mandatory=$true)][ValidateSet('IdleCycle','WalkStop','IdleReturn')][string]$Kind,
    [Parameter(Mandatory=$true)][string]$AlignmentPath,
    [string]$BoundaryResourceDirectory='Tower_Triumph/Tower_Triumph/Resource'
)

# Uniform import of authored poses, followed by exact source-pixel endpoint
# packing. No intermediate pose is synthesized, warped or individually fitted.
$ErrorActionPreference='Stop'
Add-Type -AssemblyName System.Drawing
$name=$Character+'_'+$Kind
$cellSize=if($Kind -eq 'IdleCycle'){96}else{128}
$root=if($Kind -eq 'IdleCycle'){@(48,88)}else{@(64,112)}
$importer=if($Kind -eq 'IdleCycle'){'import_walk_cycle.ps1'}else{'import_character_action.ps1'}
New-Item -ItemType Directory -Path $OutputDirectory -Force | Out-Null
$finalOut=(Resolve-Path -LiteralPath $OutputDirectory).Path
$staging=Join-Path $PSScriptRoot ('bin/idle-motion-staging/'+$name+'/'+[Guid]::NewGuid().ToString('N'))
& (Join-Path $PSScriptRoot $importer) -InputPath $InputPath -OutputDirectory $staging -Name $name -AlignmentPath $AlignmentPath | Out-Null
$out=(Resolve-Path -LiteralPath $staging).Path
$bitmapPath=Join-Path $out ($name+'.bmp')
$layoutPath=Join-Path $out ($name+'.layout.json')
$layout=Get-Content -LiteralPath $layoutPath -Raw | ConvertFrom-Json
$bitmap=[Drawing.Bitmap]::new($bitmapPath)
$boundaryRecords=@()

function Copy-Endpoint([int]$Destination, [string]$SourcePath, [int]$SourceFrame,
    [int]$SourceCellSize, [int]$SourceColumns, [int[]]$Offset) {
    $source=[Drawing.Bitmap]::new($SourcePath)
    try {
        $sx=($SourceFrame%$SourceColumns)*$SourceCellSize
        $sy=[int][Math]::Floor($SourceFrame/$SourceColumns)*$SourceCellSize
        if($sx+$SourceCellSize -gt $source.Width -or $sy+$SourceCellSize -gt $source.Height){throw 'Endpoint source cell is outside its sheet.'}
        $dx=($Destination%4)*$cellSize; $dy=[int][Math]::Floor($Destination/4)*$cellSize
        for($y=0;$y -lt $cellSize;$y++){for($x=0;$x -lt $cellSize;$x++){$bitmap.SetPixel($dx+$x,$dy+$y,[Drawing.Color]::Magenta)}}
        for($y=0;$y -lt $SourceCellSize;$y++){for($x=0;$x -lt $SourceCellSize;$x++){
            $bitmap.SetPixel($dx+$Offset[0]+$x,$dy+$Offset[1]+$y,$source.GetPixel($sx+$x,$sy+$y))
        }}
        return [pscustomobject]@{destinationFrame=$Destination;source=[IO.Path]::GetFileName($SourcePath)
            sourceSha256=(Get-FileHash -LiteralPath $SourcePath -Algorithm SHA256).Hash.ToLowerInvariant()
            sourceFrame=$SourceFrame;sourceCellSize=$SourceCellSize;sourceColumns=$SourceColumns;offset=$Offset
            method='Exact unchanged RGB cell pixels, with magenta padding only.'}
    }finally{$source.Dispose()}
}

try {
    if($Kind -eq 'IdleCycle') {
        # Preserve an exact copy of the generated normalized neutral frame before
        # replacing the final loop cell. The source PNG remains untouched.
        $neutralPath=Join-Path $out ($name+'.neutral.bmp')
        $neutral=$bitmap.Clone([Drawing.Rectangle]::new(0,0,96,96),[Drawing.Imaging.PixelFormat]::Format24bppRgb)
        try{$neutral.Save($neutralPath,[Drawing.Imaging.ImageFormat]::Bmp)}finally{$neutral.Dispose()}
        $boundaryRecords+=Copy-Endpoint 11 $neutralPath 0 96 1 @(0,0)
    } elseif($Kind -eq 'WalkStop') {
        $walk=Join-Path $BoundaryResourceDirectory ($Character+'_WalkCycle.bmp')
        $idle=Join-Path $BoundaryResourceDirectory ($Character+'_IdleCycle.bmp')
        $boundaryRecords+=Copy-Endpoint 0 $walk 0 96 4 @(16,24)
        $boundaryRecords+=Copy-Endpoint 6 $walk 6 96 4 @(16,24)
        foreach($frame in @(5,11)){$boundaryRecords+=Copy-Endpoint $frame $idle 0 96 4 @(16,24)}
    } else {
        $idle=Join-Path $BoundaryResourceDirectory ($Character+'_IdleCycle.bmp')
        $states=@('Attack','Skill','Hit')
        for($row=0;$row -lt 3;$row++) {
            $action=Join-Path $BoundaryResourceDirectory ($Character+'_'+$states[$row]+'Action.bmp')
            $boundaryRecords+=Copy-Endpoint ($row*4) $action 11 128 4 @(0,0)
            $boundaryRecords+=Copy-Endpoint ($row*4+3) $idle 0 96 4 @(16,24)
        }
    }
    for($index=0;$index -lt 12;$index++) {
        $dx=($index%4)*$cellSize; $dy=[int][Math]::Floor($index/4)*$cellSize
        $l=$cellSize;$t=$cellSize;$r=0;$b=0
        for($y=0;$y -lt $cellSize;$y++){for($x=0;$x -lt $cellSize;$x++){
            if($bitmap.GetPixel($dx+$x,$dy+$y).ToArgb() -eq [Drawing.Color]::Magenta.ToArgb()){continue}
            if($x -eq 0 -or $y -eq 0 -or $x -eq $cellSize-1 -or $y -eq $cellSize-1){throw "Pose $index touches its cell boundary after endpoint packing."}
            $l=[Math]::Min($l,$x);$t=[Math]::Min($t,$y);$r=[Math]::Max($r,$x+1);$b=[Math]::Max($b,$y+1)
        }}
        if($r -le $l){throw "Empty final pose $index"}
        $layout.frames[$index].outputBounds=@($l,$t,$r,$b)
        $boundary=$boundaryRecords | Where-Object {$_.destinationFrame -eq $index}
        if($boundary){$layout.frames[$index] | Add-Member -Force -NotePropertyName boundarySource -NotePropertyValue $boundary}
    }
    # GDI+ cannot overwrite a bitmap while the original file is open.
    $packedPath=Join-Path $out ($name+'.packed.bmp')
    $packed24=[Drawing.Bitmap]::new($bitmap.Width,$bitmap.Height,[Drawing.Imaging.PixelFormat]::Format24bppRgb)
    try{
        $copyGraphics=[Drawing.Graphics]::FromImage($packed24)
        try{$copyGraphics.DrawImageUnscaled($bitmap,0,0)}finally{$copyGraphics.Dispose()}
        $packed24.Save($packedPath,[Drawing.Imaging.ImageFormat]::Bmp)
    }finally{$packed24.Dispose()}
}finally{$bitmap.Dispose()}
Move-Item -LiteralPath $packedPath -Destination $bitmapPath -Force
$layout.bitmapSha256=(Get-FileHash -LiteralPath $bitmapPath -Algorithm SHA256).Hash.ToLowerInvariant()
$layout | Add-Member -Force -NotePropertyName nominalFrameHeight -NotePropertyValue 96
$layout | Add-Member -Force -NotePropertyName boundarySources -NotePropertyValue @($boundaryRecords)
$animation=if($Kind -eq 'IdleCycle'){@{kind=$Kind;seconds=2.4;loop=$true;framesPerBranch=12;branches=1}}
    elseif($Kind -eq 'WalkStop'){@{kind=$Kind;seconds=0.24;loop=$false;framesPerBranch=6;branches=2}}
    else{@{kind=$Kind;seconds=0.20;loop=$false;framesPerBranch=4;branches=3}}
$layout | Add-Member -Force -NotePropertyName animation -NotePropertyValue $animation
$layout.method='Uniform nearest-neighbor import of generated poses, then exact RGB endpoint cell copies. Generated source/root records remain for audit; boundarySource takes precedence for replaced cells.'
$layout | ConvertTo-Json -Depth 12 | Set-Content -LiteralPath $layoutPath -Encoding utf8
$copyAlignment=Join-Path $out ($name+'.alignment.json')
if((Resolve-Path -LiteralPath $AlignmentPath).Path -ine $copyAlignment){Copy-Item -LiteralPath $AlignmentPath -Destination $copyAlignment -Force}

if($Kind -eq 'IdleCycle') {
    $neutral=[Drawing.Bitmap]::new((Join-Path $out ($name+'.neutral.bmp')))
    $png=[Drawing.Bitmap]::new(96,96,[Drawing.Imaging.PixelFormat]::Format32bppArgb)
    $large=[Drawing.Bitmap]::new(384,384,[Drawing.Imaging.PixelFormat]::Format32bppArgb)
    try {
        for($y=0;$y -lt 96;$y++){for($x=0;$x -lt 96;$x++){
            $color=$neutral.GetPixel($x,$y)
            if($color.ToArgb() -eq [Drawing.Color]::Magenta.ToArgb()){continue}
            $png.SetPixel($x,$y,$color)
            for($dy=0;$dy -lt 4;$dy++){for($dx=0;$dx -lt 4;$dx++){$large.SetPixel($x*4+$dx,$y*4+$dy,$color)}}
        }}
        $png.Save((Join-Path $out ($name+'.frame0.png')),[Drawing.Imaging.ImageFormat]::Png)
        $large.Save((Join-Path $out ($name+'.frame0-reference.png')),[Drawing.Imaging.ImageFormat]::Png)
    }finally{$neutral.Dispose();$png.Dispose();$large.Dispose()}
    $source=[Drawing.Bitmap]::new((Resolve-Path -LiteralPath $InputPath).Path)
    try {
        $cell=$layout.frames[0].sourceCell
        $crop=$source.Clone([Drawing.Rectangle]::new($cell[0],$cell[1],($cell[2]-$cell[0]),($cell[3]-$cell[1])),[Drawing.Imaging.PixelFormat]::Format32bppArgb)
        try{$crop.Save((Join-Path $out ($name+'.frame0-source.png')),[Drawing.Imaging.ImageFormat]::Png)}finally{$crop.Dispose()}
    }finally{$source.Dispose()}
}
# Publish only after the full sheet and boundary checks completed. Prompt files
# live beside these assets and are never changed by the importer.
Get-ChildItem -LiteralPath $out -File | Where-Object {$_.Name.StartsWith($name+'.')} | ForEach-Object {
    Copy-Item -LiteralPath $_.FullName -Destination (Join-Path $finalOut $_.Name) -Force
}
Write-Output (Join-Path $finalOut ($name+'.bmp'))
Write-Output (Join-Path $finalOut ($name+'.layout.json'))
