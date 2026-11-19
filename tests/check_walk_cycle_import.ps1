$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.Drawing
$checkRoot = Join-Path $PSScriptRoot 'bin/gait-import-check'
New-Item -ItemType Directory -Path $checkRoot -Force | Out-Null
$fixturePath = Join-Path $checkRoot 'fixture.png'
$alignmentPath = Join-Path $checkRoot 'alignment.json'
$outputRoot = Join-Path $checkRoot 'out'
$importer = Join-Path $PSScriptRoot 'import_walk_cycle.ps1'

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
        }
    } finally { $graphics.Dispose() }
    $fixture.Save($fixturePath, [System.Drawing.Imaging.ImageFormat]::Png)
} finally { $fixture.Dispose() }
$alignment = [ordered]@{ scale = 2; frames = @(1..12 | ForEach-Object { @{ rootX = 20; groundY = 35 } }) }
$alignment | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath $alignmentPath -Encoding utf8
& $importer -InputPath $fixturePath -OutputDirectory $outputRoot -Name TestWalk -AlignmentPath $alignmentPath | Out-Null
$layout = Get-Content -LiteralPath (Join-Path $outputRoot 'TestWalk.layout.json') -Raw | ConvertFrom-Json
$inspection = Get-Content -LiteralPath (Join-Path $outputRoot 'TestWalk.inspection.json') -Raw | ConvertFrom-Json
Assert-Check ($layout.frames.Count -eq 12) 'Expected 12 output frames.'
Assert-Check ($inspection.frames.Count -eq 12) 'Expected 12 inspected frames.'
Assert-Check ((Get-FileHash -LiteralPath $fixturePath).Hash -eq (Get-FileHash -LiteralPath (Join-Path $outputRoot 'TestWalk.source.png')).Hash) 'The source copy changed.'
$bitmap = [System.Drawing.Bitmap]::new((Join-Path $outputRoot 'TestWalk.bmp'))
try {
    Assert-Check ($bitmap.Width -eq 384 -and $bitmap.Height -eq 288) 'Incorrect output atlas dimensions.'
    Assert-Check ($bitmap.PixelFormat -eq [System.Drawing.Imaging.PixelFormat]::Format24bppRgb) 'Expected 24-bit BMP.'
    $footStarts = @()
    for ($index = 0; $index -lt 12; $index++) {
        $x = ($index % 4) * 96
        $y = [int][Math]::Floor($index / 4) * 96
        for ($edge = 0; $edge -lt 96; $edge++) {
            foreach ($point in @(@(($x + $edge), $y), @(($x + $edge), ($y + 95)), @($x, ($y + $edge)), @(($x + 95), ($y + $edge)))) {
                Assert-Check ($bitmap.GetPixel($point[0], $point[1]).ToArgb() -eq [System.Drawing.Color]::Magenta.ToArgb()) 'A frame leaked into its gutter.'
            }
        }
        Assert-Check ($bitmap.GetPixel($x + 48, $y + 40).ToArgb() -eq [System.Drawing.Color]::Blue.ToArgb()) 'The stable torso root moved.'
        $firstGreen = -1
        for ($column = 0; $column -lt 96; $column++) {
            if ($bitmap.GetPixel($x + $column, $y + 84).ToArgb() -eq [System.Drawing.Color]::Green.ToArgb()) {
                $firstGreen = $column
                break
            }
        }
        Assert-Check ($firstGreen -eq (28 + ($index % 3) * 6)) 'Authored foot travel was changed.'
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
Write-Output 'Walk cycle importer passed: unchanged source, 12 poses, fixed torso root, preserved foot travel, keyed BMP gutters, and rejection checks.'
