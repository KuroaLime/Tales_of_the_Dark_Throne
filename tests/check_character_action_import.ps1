$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.Drawing
$checkRoot = Join-Path $PSScriptRoot 'bin/action-import-check'
New-Item -ItemType Directory -Path $checkRoot -Force | Out-Null
$fixturePath = Join-Path $checkRoot 'fixture.png'
$alignmentPath = Join-Path $checkRoot 'alignment.json'
$outputRoot = Join-Path $checkRoot 'out'
$importer = Join-Path $PSScriptRoot 'import_character_action.ps1'

function Assert-Check([bool]$Condition, [string]$Message) {
    if (-not $Condition) { throw $Message }
}
function Assert-Rejected([scriptblock]$Action, [string]$ExpectedMessage) {
    $rejected = $false
    try { & $Action | Out-Null }
    catch {
        if ($_.Exception.Message -notlike ('*' + $ExpectedMessage + '*')) { throw }
        $rejected = $true
    }
    Assert-Check $rejected ('Expected rejection: ' + $ExpectedMessage)
}

# Deliberately move only the green foot. Its travel must survive importing;
# automatic per-frame centering would accidentally erase this test signal.
$fixture = [System.Drawing.Bitmap]::new(160, 120, [System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
try {
    $graphics = [System.Drawing.Graphics]::FromImage($fixture)
    try {
        $graphics.Clear([System.Drawing.Color]::Transparent)
        for ($index = 0; $index -lt 12; $index++) {
            $x = ($index % 4) * 40
            $y = [int][Math]::Floor($index / 4) * 40
            $graphics.FillRectangle([System.Drawing.Brushes]::Blue, $x + 14, $y + 5, 12, 22)
            $graphics.FillRectangle([System.Drawing.Brushes]::Green, $x + 10 + ($index % 3) * 3, $y + 27, 6, 8)
            # A weapon/effect extends past the old 96px cell, within new padding.
            $graphics.FillRectangle([System.Drawing.Brushes]::Yellow, $x + 35, $y + 10, 3, 8)
        }
    } finally { $graphics.Dispose() }
    $fixture.Save($fixturePath, [System.Drawing.Imaging.ImageFormat]::Png)
} finally { $fixture.Dispose() }
$alignment = [ordered]@{ scale = 2; frames = @(1..12 | ForEach-Object { @{ rootX = 20; groundY = 35 } }) }
$alignment | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath $alignmentPath -Encoding utf8
& $importer -InputPath $fixturePath -OutputDirectory $outputRoot -Name TestAction -AlignmentPath $alignmentPath | Out-Null
$layout = Get-Content -LiteralPath (Join-Path $outputRoot 'TestAction.layout.json') -Raw | ConvertFrom-Json
$inspection = Get-Content -LiteralPath (Join-Path $outputRoot 'TestAction.inspection.json') -Raw | ConvertFrom-Json
Assert-Check ($layout.frames.Count -eq 12) 'Expected 12 output frames.'
Assert-Check ($inspection.frames.Count -eq 12) 'Expected 12 inspected frames.'
Assert-Check ((Get-FileHash -LiteralPath $fixturePath).Hash -eq (Get-FileHash -LiteralPath (Join-Path $outputRoot 'TestAction.source.png')).Hash) 'The source copy changed.'
$bitmap = [System.Drawing.Bitmap]::new((Join-Path $outputRoot 'TestAction.bmp'))
try {
    Assert-Check ($bitmap.Width -eq 512 -and $bitmap.Height -eq 384) 'Incorrect output atlas dimensions.'
    Assert-Check ($bitmap.PixelFormat -eq [System.Drawing.Imaging.PixelFormat]::Format24bppRgb) 'Expected 24-bit BMP.'
    $footStarts = @()
    for ($index = 0; $index -lt 12; $index++) {
        $x = ($index % 4) * 128
        $y = [int][Math]::Floor($index / 4) * 128
        for ($edge = 0; $edge -lt 128; $edge++) {
            foreach ($point in @(@(($x + $edge), $y), @(($x + $edge), ($y + 127)), @($x, ($y + $edge)), @(($x + 127), ($y + $edge)))) {
                Assert-Check ($bitmap.GetPixel($point[0], $point[1]).ToArgb() -eq [System.Drawing.Color]::Magenta.ToArgb()) 'A frame leaked into its gutter.'
            }
        }
        Assert-Check ($bitmap.GetPixel($x + 64, $y + 64).ToArgb() -eq [System.Drawing.Color]::Blue.ToArgb()) 'The stable torso root moved.'
        Assert-Check ($bitmap.GetPixel($x + 98, $y + 64).ToArgb() -eq [System.Drawing.Color]::Yellow.ToArgb()) 'Extended weapon/effect was cropped by the old cell boundary.'
        $firstGreen = -1
        for ($column = 0; $column -lt 128; $column++) {
            if ($bitmap.GetPixel($x + $column, $y + 108).ToArgb() -eq [System.Drawing.Color]::Green.ToArgb()) {
                $firstGreen = $column
                break
            }
        }
        Assert-Check ($firstGreen -eq (44 + ($index % 3) * 6)) 'Authored foot travel was changed.'
        $footStarts += $firstGreen
    }
} finally { $bitmap.Dispose() }

$badAlignmentPath = Join-Path $checkRoot 'invalid-alignment.json'
$alignment.scale = 8
$alignment | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath $badAlignmentPath -Encoding utf8
Assert-Rejected { & $importer -InputPath $fixturePath -OutputDirectory $outputRoot -Name Invalid -AlignmentPath $badAlignmentPath } 'would clip or touch a gutter'
$alignment.scale = 2
$alignment.frames[3].Remove('rootX')
$alignment | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath $badAlignmentPath -Encoding utf8
Assert-Rejected { & $importer -InputPath $fixturePath -OutputDirectory $outputRoot -Name Invalid -AlignmentPath $badAlignmentPath } 'missing rootX or groundY'

$opaquePath = Join-Path $checkRoot 'opaque.png'
$opaque = [System.Drawing.Bitmap]::new(160, 120)
try {
    $graphics = [System.Drawing.Graphics]::FromImage($opaque)
    try { $graphics.Clear([System.Drawing.Color]::White) } finally { $graphics.Dispose() }
    $opaque.Save($opaquePath, [System.Drawing.Imaging.ImageFormat]::Png)
} finally { $opaque.Dispose() }
Assert-Rejected { & $importer -InputPath $opaquePath -OutputDirectory $outputRoot -Name Invalid -InspectOnly } 'no transparent background'

$emptyPath = Join-Path $checkRoot 'empty.png'
$empty = [System.Drawing.Bitmap]::new(160, 120, [System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
try { $empty.Save($emptyPath, [System.Drawing.Imaging.ImageFormat]::Png) } finally { $empty.Dispose() }
Assert-Rejected { & $importer -InputPath $emptyPath -OutputDirectory $outputRoot -Name Invalid -InspectOnly } 'is empty'
Write-Output 'Combat action importer passed: unchanged source, 12 poses, fixed torso root, preserved foot travel, keyed BMP gutters, and rejection checks.'
