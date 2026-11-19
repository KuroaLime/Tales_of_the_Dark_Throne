param(
    [string]$OutputDirectory = (Join-Path $PSScriptRoot 'bin/sea-ruins-preview'),
    [string]$CandidatePath = '',
    [switch]$Footprints,
    [switch]$Sweeps
)

# Authored asset / collision preview, not a game screenshot or runtime test.
$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.Drawing
$OutputDirectory = [System.IO.Path]::GetFullPath($OutputDirectory)
New-Item -ItemType Directory -Path $OutputDirectory -Force | Out-Null
$readMap = @'
import json,sys
sys.path.insert(0,sys.argv[1])
from pathlib import Path
from verify_sea_ruins import SOURCE,load_sea_ruins,load_candidate,background_path
source=(SOURCE/'BattleMapData.cpp').read_text(encoding='utf-8-sig')
w,h,t,rows,tiles,_,players,enemies=load_candidate(Path(sys.argv[2])) if len(sys.argv)>2 and sys.argv[2] else load_sea_ruins(source)
print(json.dumps(dict(width=w,height=h,tile=t,rows=rows,tiles=tiles,players=players,enemies=enemies,background=str(background_path(source)))))
'@
$mapJson = & python -B -c $readMap $PSScriptRoot $CandidatePath
if ($LASTEXITCODE -ne 0) { throw 'Cannot parse the production sea-ruins map' }
$map = $mapJson | ConvertFrom-Json
$authored = [System.Drawing.Bitmap]::new([string]$map.background)
try {
    $expectedWidth = $map.width * $map.tile
    $expectedHeight = $map.height * $map.tile
    if ($authored.Width -ne $expectedWidth -or $authored.Height -ne $expectedHeight) {
        throw "Sea ruins scene must be exactly ${expectedWidth}x${expectedHeight}; found $($authored.Width)x$($authored.Height)"
    }
    $bounds = [System.Drawing.Rectangle]::new(0, 0, $expectedWidth, $expectedHeight)
    $canvas = $authored.Clone($bounds, [System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
}
finally { $authored.Dispose() }

$graphics = [System.Drawing.Graphics]::FromImage($canvas)
$gridPen = [System.Drawing.Pen]::new([System.Drawing.Color]::FromArgb(120, 80, 90, 110), 1)
$blockedPen = [System.Drawing.Pen]::new([System.Drawing.Color]::FromArgb(230, 60, 85), 1)
$playerPen = [System.Drawing.Pen]::new([System.Drawing.Color]::FromArgb(0, 170, 230), 3)
$enemyPen = [System.Drawing.Pen]::new([System.Drawing.Color]::FromArgb(245, 90, 65), 3)
$footPen = [System.Drawing.Pen]::new([System.Drawing.Color]::FromArgb(180, 20, 125, 55), 1)
$blockedTypes = @('SkyVoid', 'Building', 'Wall', 'Tree', 'Fence', 'Grave', 'Water')
try {
    $previewPath = Join-Path $OutputDirectory 'sea-ruins-preview.png'
    $canvas.Save($previewPath, [System.Drawing.Imaging.ImageFormat]::Png)
    for ($x = 0; $x -le $map.width; $x++) {
        $graphics.DrawLine($gridPen, ($x * $map.tile), 0, ($x * $map.tile), $canvas.Height)
    }
    for ($y = 0; $y -le $map.height; $y++) {
        $graphics.DrawLine($gridPen, 0, ($y * $map.tile), $canvas.Width, ($y * $map.tile))
    }
    for ($y = 0; $y -lt $map.height; $y++) {
        for ($x = 0; $x -lt $map.width; $x++) {
            if ($map.tiles[$y][$x] -in $blockedTypes) {
                $cx = $x * $map.tile + $map.tile / 2
                $cy = $y * $map.tile + $map.tile / 2
                $graphics.DrawLine($blockedPen, ($cx - 5), ($cy - 5), ($cx + 5), ($cy + 5))
                $graphics.DrawLine($blockedPen, ($cx - 5), ($cy + 5), ($cx + 5), ($cy - 5))
            }
        }
    }
    foreach ($start in $map.players) {
        $graphics.DrawRectangle($playerPen, ($start[0] * $map.tile + 16), ($start[1] * $map.tile + 30), 16, 12)
    }
    foreach ($start in $map.enemies) {
        $graphics.DrawRectangle($enemyPen, ($start[0] * $map.tile + 16), ($start[1] * $map.tile + 30), 16, 12)
    }
    $debugPath = Join-Path $OutputDirectory 'sea-ruins-debug-preview.png'
    $canvas.Save($debugPath, [System.Drawing.Imaging.ImageFormat]::Png)
    Write-Output "Authored sea-ruins asset preview (not a game capture): $previewPath"
    if ($CandidatePath) { Write-Output "Overlay uses proposed geometry: $CandidatePath" }
    Write-Output "Collision grid / spawn overlay: $debugPath"
    if ($Footprints) {
        for ($y = 0; $y -lt $map.height; $y++) {
            for ($x = 0; $x -lt $map.width; $x++) {
                if ($map.tiles[$y][$x] -notin $blockedTypes) {
                    $graphics.DrawRectangle($footPen, ($x * $map.tile + 16), ($y * $map.tile + 30), 16, 12)
                }
            }
        }
        $footPath = Join-Path $OutputDirectory 'sea-ruins-foot-preview.png'
        $canvas.Save($footPath, [System.Drawing.Imaging.ImageFormat]::Png)
        Write-Output "Every walkable cell's conservative foot rectangle: $footPath"
    }
    if ($Sweeps) {
        # Start from the clean painted image so the faint movement overlays do
        # not obscure the physical artwork with blocked-cell crosses or a grid.
        $sweepCanvas = [System.Drawing.Bitmap]::new($previewPath)
        $sweepGraphics = [System.Drawing.Graphics]::FromImage($sweepCanvas)
        $sweepBrush = [System.Drawing.SolidBrush]::new([System.Drawing.Color]::FromArgb(25, 20, 160, 70))
        try {
            for ($y = 0; $y -lt $map.height; $y++) {
                for ($x = 0; $x -lt $map.width; $x++) {
                    if ($map.tiles[$y][$x] -in $blockedTypes) { continue }
                    $left = $x * $map.tile + 16
                    $top = $y * $map.tile + 30
                    # The foot is 16x12 centered at (24,36). A cardinal sweep
                    # lies entirely in its two walkable endpoint cells; blocked
                    # cells are never painted. Each edge is drawn only once.
                    if (($x + 1) -lt $map.width -and $map.tiles[$y][$x + 1] -notin $blockedTypes) {
                        $sweepGraphics.FillRectangle($sweepBrush, $left, $top, ($map.tile + 16), 12)
                    }
                    if (($y + 1) -lt $map.height -and $map.tiles[$y + 1][$x] -notin $blockedTypes) {
                        $sweepGraphics.FillRectangle($sweepBrush, $left, $top, 16, ($map.tile + 12))
                    }
                }
            }
            for ($y = 0; $y -lt $map.height; $y++) {
                for ($x = 0; $x -lt $map.width; $x++) {
                    if ($map.tiles[$y][$x] -notin $blockedTypes) {
                        $sweepGraphics.DrawRectangle($footPen, ($x * $map.tile + 16), ($y * $map.tile + 30), 16, 12)
                    }
                }
            }
            foreach ($start in $map.players) {
                $sweepGraphics.DrawRectangle($playerPen, ($start[0] * $map.tile + 16), ($start[1] * $map.tile + 30), 16, 12)
            }
            foreach ($start in $map.enemies) {
                $sweepGraphics.DrawRectangle($enemyPen, ($start[0] * $map.tile + 16), ($start[1] * $map.tile + 30), 16, 12)
            }
            $sweepPath = Join-Path $OutputDirectory 'sea-ruins-sweep-preview.png'
            $sweepCanvas.Save($sweepPath, [System.Drawing.Imaging.ImageFormat]::Png)
            Write-Output "Continuous foot-swept areas for visual artwork audit: $sweepPath"
        }
        finally {
            $sweepBrush.Dispose(); $sweepGraphics.Dispose(); $sweepCanvas.Dispose()
        }
    }
}
finally {
    $gridPen.Dispose(); $blockedPen.Dispose(); $playerPen.Dispose()
    $enemyPen.Dispose(); $footPen.Dispose(); $graphics.Dispose(); $canvas.Dispose()
}
