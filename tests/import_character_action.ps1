param(
    [Parameter(Mandatory = $true)][string]$InputPath,
    [Parameter(Mandatory = $true)][string]$OutputDirectory,
    [Parameter(Mandatory = $true)][ValidatePattern('^[A-Za-z][A-Za-z0-9_-]*$')][string]$Name,
    [string]$AlignmentPath,
    [switch]$InspectOnly
)

# Imports 12 authored combat poses into padded 128px cells. The renderer uses a
# nominal body height of 96px; padding protects weapon, effect and death extents.
# Alignment JSON: { "scale": 0.4, "frames": [
#   { "rootX": 128, "groundY": 234 }, ... exactly 12 entries ... ] }
# rootX/groundY are coordinates relative to that frame's source cell, not its
# visible bounds. Keep the pelvis and common ground line stable across poses.
# Optional cellRects: 12 [left, top, right, bottom] source-pixel rectangles.
# One uniform scale applies to every frame; no foot-median re-anchoring occurs.

$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.Drawing
if (-not ('ActionImportPixels' -as [type])) {
    Add-Type -TypeDefinition @'
using System;
using System.Collections.Generic;

public sealed class ActionFrameInspection
{
    public int[] Cell;
    public int[] Bounds;
    public int OpaquePixels;
    public int TransparentPixels;
    public int[] ComponentSizes;
    public bool TouchesCellEdge;
}

public static class ActionImportPixels
{
    public const int AlphaThreshold = 128;

    private static bool Opaque(int pixel)
    {
        return ((uint)pixel >> 24) >= AlphaThreshold;
    }

    public static ActionFrameInspection Inspect(int[] pixels, int sheetWidth, int[] cell)
    {
        int width = cell[2] - cell[0], height = cell[3] - cell[1];
        bool[] mask = new bool[width * height];
        int left = width, top = height, right = 0, bottom = 0, count = 0;
        for (int y = 0; y < height; ++y)
            for (int x = 0; x < width; ++x)
            {
                if (!Opaque(pixels[(cell[1] + y) * sheetWidth + cell[0] + x])) continue;
                mask[y * width + x] = true;
                ++count;
                left = Math.Min(left, x); top = Math.Min(top, y);
                right = Math.Max(right, x + 1); bottom = Math.Max(bottom, y + 1);
            }
        var sizes = new List<int>();
        var pending = new Queue<int>();
        for (int index = 0; index < mask.Length; ++index)
        {
            if (!mask[index]) continue;
            mask[index] = false;
            pending.Enqueue(index);
            int size = 0;
            while (pending.Count != 0)
            {
                int point = pending.Dequeue();
                ++size;
                int x = point % width, y = point / width;
                for (int dy = -1; dy <= 1; ++dy)
                    for (int dx = -1; dx <= 1; ++dx)
                    {
                        int nx = x + dx, ny = y + dy;
                        if (nx < 0 || ny < 0 || nx >= width || ny >= height) continue;
                        int neighbor = ny * width + nx;
                        if (!mask[neighbor]) continue;
                        mask[neighbor] = false;
                        pending.Enqueue(neighbor);
                    }
            }
            sizes.Add(size);
        }
        sizes.Sort((a, b) => b.CompareTo(a));
        return new ActionFrameInspection {
            Cell = cell, Bounds = count == 0 ? new int[4] : new[] {left, top, right, bottom},
            OpaquePixels = count, TransparentPixels = width * height - count,
            ComponentSizes = sizes.ToArray(),
            TouchesCellEdge = count != 0 && (left == 0 || top == 0 || right == width || bottom == height)
        };
    }

    public static int[] RenderFrame(int[] destination, int[] sourcePixels, int sourceWidth,
        int[] cell, double rootX, double groundY, double scale, int index)
    {
        int width = cell[2] - cell[0], height = cell[3] - cell[1];
        int left = 128, top = 128, right = 0, bottom = 0, count = 0;
        int targetX = index % 4 * 128, targetY = index / 4 * 128;
        // Inverse sampling keeps one exact uniform transform for every source
        // point, including planted feet. Transparent edges become the exact
        // magenta key rather than a halo composited against magenta.
        for (int y = 0; y < 128; ++y)
            for (int x = 0; x < 128; ++x)
            {
                int sx = (int)Math.Floor(rootX + (x + 0.5 - 64) / scale);
                int sy = (int)Math.Floor(groundY + (y + 0.5 - 112) / scale);
                if (sx < 0 || sy < 0 || sx >= width || sy >= height) continue;
                int pixel = sourcePixels[(cell[1] + sy) * sourceWidth + cell[0] + sx];
                if (!Opaque(pixel)) continue;
                if ((pixel & 0x00ffffff) == 0x00ff00ff)
                    throw new InvalidOperationException("Opaque magenta conflicts with the game transparency key.");
                destination[(targetY + y) * 512 + targetX + x] = pixel | unchecked((int)0xff000000);
                ++count;
                left = Math.Min(left, x); top = Math.Min(top, y);
                right = Math.Max(right, x + 1); bottom = Math.Max(bottom, y + 1);
            }
        if (count == 0) throw new InvalidOperationException("Imported frame " + index + " is empty.");
        if (left < 1 || top < 1 || right > 127 || bottom > 127)
            throw new InvalidOperationException("Imported frame " + index + " touches its cell gutter.");
        return new[] { left, top, right, bottom };
    }
}
'@
}

$resolvedInput = (Resolve-Path -LiteralPath $InputPath).Path
if ([IO.Path]::GetExtension($resolvedInput) -ine '.png') {
    throw 'Use the original generated PNG as input so its source copy stays unchanged.'
}
$source = [System.Drawing.Bitmap]::new($resolvedInput)
try {
    $alignment = if ($AlignmentPath) {
        Get-Content -LiteralPath $AlignmentPath -Raw | ConvertFrom-Json
    } else { $null }
    $cells = @()
    if ($alignment -and $alignment.cellRects) {
        if ($alignment.cellRects.Count -ne 12) { throw 'cellRects must contain exactly 12 source rectangles.' }
        foreach ($cell in $alignment.cellRects) { $cells += ,([int[]]$cell) }
    } else {
        for ($index = 0; $index -lt 12; $index++) {
            $column = $index % 4
            $row = [int][Math]::Floor($index / 4)
            $cells += ,([int[]]@(
                [int][Math]::Floor($column * $source.Width / 4),
                [int][Math]::Floor($row * $source.Height / 3),
                [int][Math]::Floor(($column + 1) * $source.Width / 4),
                [int][Math]::Floor(($row + 1) * $source.Height / 3)))
        }
    }
    foreach ($cell in $cells) {
        if ($cell.Count -ne 4 -or $cell[0] -lt 0 -or $cell[1] -lt 0 -or
            $cell[2] -gt $source.Width -or $cell[3] -gt $source.Height -or
            $cell[0] -ge $cell[2] -or $cell[1] -ge $cell[3]) { throw 'Invalid source cell rectangle.' }
    }
    $decoded = [System.Drawing.Bitmap]::new($source.Width, $source.Height, [System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
    try {
        $graphics = [System.Drawing.Graphics]::FromImage($decoded)
        try { $graphics.DrawImageUnscaled($source, 0, 0) } finally { $graphics.Dispose() }
        $locked = $decoded.LockBits([System.Drawing.Rectangle]::new(0, 0, $decoded.Width, $decoded.Height),
            [System.Drawing.Imaging.ImageLockMode]::ReadOnly, [System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
        try {
            $pixels = [int[]]::new($decoded.Width * $decoded.Height)
            for ($row = 0; $row -lt $decoded.Height; $row++) {
                [Runtime.InteropServices.Marshal]::Copy([IntPtr]::Add($locked.Scan0, $row * $locked.Stride),
                    $pixels, $row * $decoded.Width, $decoded.Width)
            }
        } finally { $decoded.UnlockBits($locked) }
    } finally { $decoded.Dispose() }
    $inspections = @()
    for ($index = 0; $index -lt 12; $index++) {
        $inspection = [ActionImportPixels]::Inspect($pixels, $source.Width, $cells[$index])
        if ($inspection.OpaquePixels -eq 0) { throw "Source frame $index is empty." }
        if ($inspection.TransparentPixels -eq 0) { throw "Source frame $index has no transparent background." }
        $inspections += [ordered]@{
            index = $index; cell = $cells[$index]; bounds = $inspection.Bounds
            opaquePixels = $inspection.OpaquePixels; transparentPixels = $inspection.TransparentPixels
            componentSizes = $inspection.ComponentSizes; touchesCellEdge = $inspection.TouchesCellEdge
        }
    }
    New-Item -ItemType Directory -Path $OutputDirectory -Force | Out-Null
    $outputRoot = (Resolve-Path -LiteralPath $OutputDirectory).Path
    $inspectionPath = Join-Path $outputRoot ($Name + '.inspection.json')
    [ordered]@{ input = $resolvedInput; width = $source.Width; height = $source.Height
        columns = 4; rows = 3; alphaThreshold = 128; frames = $inspections
    } | ConvertTo-Json -Depth 10 | Set-Content -LiteralPath $inspectionPath -Encoding utf8
    if ($InspectOnly) { Write-Output $inspectionPath; return }

    foreach ($inspection in $inspections) {
        if ($inspection.touchesCellEdge) {
            throw "Source pose $($inspection.index) reaches its cell edge; inspect the authored gutter before importing a clipped pose."
        }
    }

    if (-not $alignment -or -not $alignment.frames -or $alignment.frames.Count -ne 12) {
        throw 'An alignment JSON with a single scale and exactly 12 frame roots is required for import.'
    }
    $scale = [double]$alignment.scale
    if ([double]::IsNaN($scale) -or [double]::IsInfinity($scale) -or $scale -le 0 -or $scale -gt 8) {
        throw 'scale must be finite, greater than zero and no greater than 8.'
    }
    $atlas = [System.Drawing.Bitmap]::new(512, 384, [System.Drawing.Imaging.PixelFormat]::Format24bppRgb)
    try {
        $atlasPixels = [int[]]::new(512 * 384)
        [Array]::Fill[int]($atlasPixels, [System.Drawing.Color]::Magenta.ToArgb())
        $frameRecords = @()
        for ($index = 0; $index -lt 12; $index++) {
            $frame = $alignment.frames[$index]
            if ($null -eq $frame.rootX -or $null -eq $frame.groundY) { throw "Frame $index is missing rootX or groundY." }
            $rootX = [double]$frame.rootX
            $groundY = [double]$frame.groundY
            if ([double]::IsNaN($rootX) -or [double]::IsInfinity($rootX) -or
                [double]::IsNaN($groundY) -or [double]::IsInfinity($groundY)) { throw "Invalid root in frame $index." }
            $bounds = $inspections[$index].bounds
            $projected = @(
                (64 + ($bounds[0] - $rootX) * $scale), (112 + ($bounds[1] - $groundY) * $scale),
                (64 + ($bounds[2] - $rootX) * $scale), (112 + ($bounds[3] - $groundY) * $scale))
            if ($projected[0] -lt 1 -or $projected[1] -lt 1 -or $projected[2] -gt 127 -or $projected[3] -gt 127) {
                throw "Source frame $index would clip or touch a gutter. Change the common scale or explicit root alignment."
            }
            $drawBounds = [ActionImportPixels]::RenderFrame($atlasPixels, $pixels, $source.Width,
                $cells[$index], $rootX, $groundY, $scale, $index)
            $frameRecords += [ordered]@{
                index = $index; sourceCell = $cells[$index]; sourceBounds = $bounds
                sourceRoot = @($rootX, $groundY); outputBounds = $drawBounds; outputRoot = @(64, 112)
                projectedSourceBounds = $projected
            }
        }
        $locked = $atlas.LockBits([System.Drawing.Rectangle]::new(0, 0, 512, 384),
            [System.Drawing.Imaging.ImageLockMode]::WriteOnly, [System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
        try {
            for ($row = 0; $row -lt 384; $row++) {
                [Runtime.InteropServices.Marshal]::Copy($atlasPixels, $row * 512,
                    [IntPtr]::Add($locked.Scan0, $row * $locked.Stride), 512)
            }
        } finally { $atlas.UnlockBits($locked) }
        $bitmapPath = Join-Path $outputRoot ($Name + '.bmp')
        $atlas.Save($bitmapPath, [System.Drawing.Imaging.ImageFormat]::Bmp)
    } finally { $atlas.Dispose() }

    $sourceCopy = Join-Path $outputRoot ($Name + '.source.png')
    if ($resolvedInput -ine $sourceCopy) { Copy-Item -LiteralPath $resolvedInput -Destination $sourceCopy -Force }
    $layoutPath = Join-Path $outputRoot ($Name + '.layout.json')
    [ordered]@{
        source = [IO.Path]::GetFileName($sourceCopy); bitmap = [IO.Path]::GetFileName($bitmapPath)
        sourceSha256 = (Get-FileHash -LiteralPath $sourceCopy -Algorithm SHA256).Hash.ToLowerInvariant()
        bitmapSha256 = (Get-FileHash -LiteralPath $bitmapPath -Algorithm SHA256).Hash.ToLowerInvariant()
        columns = 4; rows = 3; frameCount = 12; cellSize = 128; sheetSize = @(512, 384); nominalFrameHeight = 96
        root = @(64, 112); uniformScale = $scale; alphaThreshold = 128; transparencyKey = '#ff00ff'
        frameOrder = 'row-major, left to right, top to bottom'; frames = $frameRecords
        method = 'Nearest-neighbor uniform scaling and explicit root translation only; no pose warping or foot-median anchors.'
    } | ConvertTo-Json -Depth 10 | Set-Content -LiteralPath $layoutPath -Encoding utf8
    Write-Output $bitmapPath
    Write-Output $sourceCopy
    Write-Output $layoutPath
} finally { $source.Dispose() }
