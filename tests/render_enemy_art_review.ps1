param([string]$OutputDirectory = 'tests/bin/enemy-art-audit')

# Diagnostic composition of unchanged production BMP pixels. This is an asset
# scale study, not a native game screenshot or a redraw of the character art.
$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.Drawing
$repo = Split-Path -Parent $PSScriptRoot
$resource = Join-Path $repo 'Tower_Triumph/Tower_Triumph/Resource'
$approved = Join-Path $resource 'Monster/Approved'
$backgroundDirectory = Join-Path $repo 'Tower_Triumph/Tower_Triumph/Source/SMap/background'
if (![IO.Path]::IsPathRooted($OutputDirectory)) { $OutputDirectory = Join-Path $repo $OutputDirectory }
New-Item -ItemType Directory -Path $OutputDirectory -Force | Out-Null
$records = @(
    @{name='Warrior'; file='Warrior_IdleCycle.bmp'; hero=$true; grid=@(4,7)},
    @{name='Archer'; file='Archer_IdleCycle.bmp'; hero=$true; grid=@(6,8)},
    @{name='Mage'; file='Mage_IdleCycle.bmp'; hero=$true; grid=@(8,7)},
    @{name='Skeleton'; file='S1_Skeleton_Animated.bmp'; hero=$false; grid=@(4,9)},
    @{name='Lich'; file='S1_Leach_Animated.bmp'; hero=$false; grid=@(7,10)},
    @{name='Angel Guard'; file='S2_Angel_Animated.bmp'; hero=$false; grid=@(13,6)},
    @{name='Baby Angel'; file='S2_BAngel_Animated.bmp'; hero=$false; grid=@(15,6)},
    @{name='Sky Falcon'; file='S2_Falcon_Animated.bmp'; hero=$false; grid=@(18,7)},
    @{name='Wing Angel'; file='S2_WAngel_Animated.bmp'; hero=$false; grid=@(20,8)},
    @{name='Sea Slime'; file='S4_Slime_Animated.bmp'; hero=$false; grid=@(13,9)},
    @{name='Mermaid'; file='S4_Snale_Animated.bmp'; hero=$false; grid=@(18,9)},
    @{name='Spear Crab'; file='S4_Spear_Animated.bmp'; hero=$false; grid=@(25,8)},
    @{name='Jellyfish'; file='S4_WFly_Animated.bmp'; hero=$false; grid=@(28,9)},
    @{name='Fire Demon'; file='S5_Dragon_Animated.bmp'; hero=$false; grid=@(25,11)},
    @{name='Lava Golem'; file='S5_Golem_Animated.bmp'; hero=$false; grid=@(28,12)},
    @{name='Lava Hound'; file='S5_Hound_Animated.bmp'; hero=$false; grid=@(19,14)},
    @{name='Lava Larva'; file='S5_Larva_Animated.bmp'; hero=$false; grid=@(25,14)}
)

function Read-ReviewSprite($Record) {
    $directory = if ($Record.hero) { $resource } else { $approved }
    $cell = if ($Record.hero) { 96 } else { 128 }
    $bitmap = [Drawing.Bitmap]::new((Join-Path $directory $Record.file))
    try {
        $sprite = $bitmap.Clone([Drawing.Rectangle]::new(0,0,$cell,$cell),[Drawing.Imaging.PixelFormat]::Format32bppArgb)
        $sprite.MakeTransparent([Drawing.Color]::Magenta)
        return $sprite
    } finally { $bitmap.Dispose() }
}

function Set-ReviewSampling([Drawing.Graphics]$Graphics) {
    $Graphics.InterpolationMode = [Drawing.Drawing2D.InterpolationMode]::NearestNeighbor
    $Graphics.PixelOffsetMode = [Drawing.Drawing2D.PixelOffsetMode]::Half
}

function Draw-ReviewSprite([Drawing.Graphics]$Graphics,$Record,[single]$FootX,[single]$FootY,[single]$Scale) {
    $sprite = Read-ReviewSprite $Record
    try {
        $rootX = if ($Record.hero) { 48 } else { 64 }
        $groundY = if ($Record.hero) { 88 } else { 112 }
        $target = [Drawing.RectangleF]::new(($FootX-$rootX*$Scale),($FootY-$groundY*$Scale),($sprite.Width*$Scale),($sprite.Height*$Scale))
        $Graphics.DrawImage($sprite,$target,[Drawing.RectangleF]::new(0,0,$sprite.Width,$sprite.Height),[Drawing.GraphicsUnit]::Pixel)
    } finally { $sprite.Dispose() }
}

$font = [Drawing.Font]::new('Segoe UI',16,[Drawing.FontStyle]::Bold)
$smallFont = [Drawing.Font]::new('Segoe UI',10,[Drawing.FontStyle]::Bold)
$titleFont = [Drawing.Font]::new('Segoe UI',18,[Drawing.FontStyle]::Bold)
$white = [Drawing.SolidBrush]::new([Drawing.Color]::FromArgb(240,239,231))
$blue = [Drawing.SolidBrush]::new([Drawing.Color]::FromArgb(155,197,244))
$gold = [Drawing.SolidBrush]::new([Drawing.Color]::FromArgb(238,202,140))
$center = [Drawing.StringFormat]::new()
$center.Alignment = [Drawing.StringAlignment]::Center
try {
    $lineup = [Drawing.Bitmap]::new(1280,1460,[Drawing.Imaging.PixelFormat]::Format32bppArgb)
    $g = [Drawing.Graphics]::FromImage($lineup)
    try {
        $g.Clear([Drawing.Color]::FromArgb(29,33,41)); Set-ReviewSampling $g
        $g.DrawString('APPROVED ALLIES + REDESIGNED ENEMIES',$titleFont,$white,[single]24,[single]14)
        for ($i=0;$i -lt $records.Count;$i++) {
            $record = $records[$i]
            $slot = if ($i -lt 3) { $i } else { $i+1 }
            $tileX = ($slot%4)*320; $tileY = 60+[int][Math]::Floor($slot/4)*280
            Draw-ReviewSprite $g $record ($tileX+160) ($tileY+224) 2.0
            $brush = if ($record.hero) { $blue } else { $gold }
            $g.DrawString($record.name,$font,$brush,[Drawing.RectangleF]::new($tileX,($tileY+243),320,30),$center)
        }
        $lineupPath = Join-Path $OutputDirectory 'enemy-identity-lineup.png'
        $lineup.Save($lineupPath,[Drawing.Imaging.ImageFormat]::Png)
        Write-Output $lineupPath
    } finally { $g.Dispose(); $lineup.Dispose() }

    $layout = Get-Content -LiteralPath (Join-Path $backgroundDirectory 'CelestialScene.layout.json') -Raw | ConvertFrom-Json
    $map = [Drawing.Bitmap]::new((Join-Path $backgroundDirectory 'CelestialScene.bmp'))
    $proof = [Drawing.Bitmap]::new($map.Width,($map.Height+56),[Drawing.Imaging.PixelFormat]::Format32bppArgb)
    $g = [Drawing.Graphics]::FromImage($proof)
    try {
        $g.Clear([Drawing.Color]::FromArgb(22,25,31));Set-ReviewSampling $g
        $g.DrawImageUnscaled($map,0,56)
        $g.DrawString('ENEMY / ALLY SCALE STUDY  -  PRODUCTION ART, NOT A GAME CAPTURE',$titleFont,$white,[single]22,[single]14)
        foreach ($record in ($records | Sort-Object { $_.grid[1] })) {
            $x=$record.grid[0];$y=$record.grid[1]
            if ($layout.rows[$y][$x] -notin @('.','=')) { throw "Review character placed on blocked tile: $($record.name) $x,$y" }
            $fx=$x*48+24;$fy=$y*48+36+56
            Draw-ReviewSprite $g $record $fx $fy (64.0/96.0)
            $size=$g.MeasureString($record.name,$smallFont)
            $labelX=$fx-$size.Width/2; $labelY=$fy+5
            $g.FillRectangle([Drawing.Brushes]::Black,[Drawing.RectangleF]::new(($labelX-3),$labelY,($size.Width+6),($size.Height+2)))
            $g.DrawString($record.name,$smallFont,$white,[single]$labelX,[single]($labelY+1))
        }
        $mapPath = Join-Path $OutputDirectory 'enemy-map-scale-proof.png'
        $proof.Save($mapPath,[Drawing.Imaging.ImageFormat]::Png)
        Write-Output $mapPath
    } finally { $g.Dispose();$proof.Dispose();$map.Dispose() }
} finally {
    $center.Dispose();$gold.Dispose();$blue.Dispose();$white.Dispose();$titleFont.Dispose();$smallFont.Dispose();$font.Dispose()
}
