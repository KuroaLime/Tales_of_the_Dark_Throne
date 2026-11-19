param(
    [Parameter(Mandatory=$true)][string]$InputPath,
    [Parameter(Mandatory=$true)][string]$RegistrationPath,
    [ValidateSet(2,4)][int]$SourceRows=2,
    [string]$Element='',
    [string]$OutputDirectory='Tower_Triumph/Tower_Triumph/Resource',
    [string]$ProofDirectory='tests/bin/projectile-import'
)

# Mechanical import of generated art: common sheet scaling, explicit measured
# tip/core registration, and nearest-neighbor rotation. No redrawing/recoloring.
$ErrorActionPreference='Stop'
Add-Type -AssemblyName System.Drawing
$source=[Drawing.Bitmap]::new((Resolve-Path -LiteralPath $InputPath).Path)
$registration=Get-Content -LiteralPath $RegistrationPath -Raw | ConvertFrom-Json
$normalized=[Drawing.Bitmap]::new(256,$SourceRows*64,[Drawing.Imaging.PixelFormat]::Format32bppArgb)
$classes=@('Archer','Mage')
$atlases=@();$previews=@();$bases=@();$reports=@()
$impactBmp=$null;$impactPng=$null;$impactReport=$null
try {
    if($Element -and $Element -notin @('Fire','Water','Grass','Light','Dark')){throw 'Unknown element.'}
    if(($SourceRows -eq 4) -ne [bool]$Element){throw 'Elemental packs require four rows; neutral pack requires two.'}
    if($source.Width*$SourceRows -ne 4*$source.Height){throw 'Expected equal square cells in the source grid.'}
    $suffix=if($Element){'_'+$Element}else{''}
    $sourceName='Projectiles'+$suffix+'.source.png'
    for($y=0;$y -lt $normalized.Height;$y++){for($x=0;$x -lt 256;$x++){
        $sx=[int][Math]::Floor(($x+.5)*$source.Width/256)
        $sy=[int][Math]::Floor(($y+.5)*$source.Height/$normalized.Height)
        $color=$source.GetPixel($sx,$sy)
        if($color.A -lt 128){continue}
        if($color.R -eq 255 -and $color.G -eq 0 -and $color.B -eq 255){throw 'Opaque foreground collides with magenta key.'}
        $normalized.SetPixel($x,$y,[Drawing.Color]::FromArgb(255,$color.R,$color.G,$color.B))
    }}
    for($ci=0;$ci -lt 2;$ci++){
        $class=$classes[$ci];$reg=$registration.$class
        if($reg.sourceAnchors.Count -ne 4 -or $reg.targetAnchor.Count -ne 2){throw "Missing measured registration for $class"}
        $groupScale=if($reg.groupScale){[double]$reg.groupScale}else{1.0}
        if($groupScale -le 0 -or $groupScale -gt 1){throw 'Common group scale must be in (0,1].'}
        $base=[Drawing.Bitmap]::new(256,64,[Drawing.Imaging.PixelFormat]::Format32bppArgb)
        $bases+=,$base
        for($frame=0;$frame -lt 4;$frame++){
            $shiftX=[double]$reg.targetAnchor[0]-[double]$reg.sourceAnchors[$frame][0]
            $shiftY=[double]$reg.targetAnchor[1]-[double]$reg.sourceAnchors[$frame][1]
            if($shiftX -ne [Math]::Truncate($shiftX) -or $shiftY -ne [Math]::Truncate($shiftY)){throw 'Registration must translate whole pixels.'}
            $shiftX=[int]$shiftX;$shiftY=[int]$shiftY
            for($y=0;$y -lt 64;$y++){for($x=0;$x -lt 64;$x++){
                $color=$normalized.GetPixel($frame*64+$x,$ci*64+$y)
                if($color.A -eq 0){continue}
                $tx=($x+.5-$reg.sourceAnchors[$frame][0])*$groupScale+$reg.targetAnchor[0]
                $ty=($y+.5-$reg.sourceAnchors[$frame][1])*$groupScale+$reg.targetAnchor[1]
                if($tx -lt 0 -or $ty -lt 0 -or $tx -ge 64 -or $ty -ge 64){throw "$class frame $frame registration clips pixels"}
            }}
            for($y=0;$y -lt 64;$y++){for($x=0;$x -lt 64;$x++){
                $sx=[int][Math]::Floor(($x+.5-$reg.targetAnchor[0])/$groupScale+$reg.sourceAnchors[$frame][0]+1e-9)
                $sy=[int][Math]::Floor(($y+.5-$reg.targetAnchor[1])/$groupScale+$reg.sourceAnchors[$frame][1]+1e-9)
                if($sx -lt 0 -or $sy -lt 0 -or $sx -ge 64 -or $sy -ge 64){continue}
                $color=$normalized.GetPixel($frame*64+$sx,$ci*64+$sy)
                if($color.A -gt 0){$base.SetPixel($frame*64+$x,$y,$color)}
            }}
        }
        $bmp=[Drawing.Bitmap]::new(1024,256,[Drawing.Imaging.PixelFormat]::Format24bppRgb)
        $png=[Drawing.Bitmap]::new(1024,256,[Drawing.Imaging.PixelFormat]::Format32bppArgb)
        $atlases+=,$bmp;$previews+=,$png
        $frameReports=@();$anchors=@()
        for($direction=0;$direction -lt 16;$direction++){
            $angle=$direction*[Math]::PI/8
            $cos=[Math]::Round([Math]::Cos($angle),12);$sin=[Math]::Round([Math]::Sin($angle),12)
            $ax=[double]$reg.targetAnchor[0]-32;$ay=[double]$reg.targetAnchor[1]-32
            $anchors+=,@([Math]::Round(32+$cos*$ax-$sin*$ay,6),[Math]::Round(32+$sin*$ax+$cos*$ay,6))
            for($frame=0;$frame -lt 4;$frame++){
                $l=64;$t=64;$r=0;$b=0;$count=0
                for($y=0;$y -lt 64;$y++){for($x=0;$x -lt 64;$x++){
                    # Pixel centers rotate about the shared geometric cell center.
                    $dx=$x+.5-32;$dy=$y+.5-32
                    $sx=[int][Math]::Floor($cos*$dx+$sin*$dy+32+1e-9)
                    $sy=[int][Math]::Floor(-$sin*$dx+$cos*$dy+32+1e-9)
                    $bmp.SetPixel($direction*64+$x,$frame*64+$y,[Drawing.Color]::Magenta)
                    if($sx -lt 0 -or $sy -lt 0 -or $sx -ge 64 -or $sy -ge 64){continue}
                    $color=$base.GetPixel($frame*64+$sx,$sy)
                    if($color.A -eq 0){continue}
                    $bmp.SetPixel($direction*64+$x,$frame*64+$y,$color)
                    $png.SetPixel($direction*64+$x,$frame*64+$y,$color)
                    $count++;$l=[Math]::Min($l,$x);$t=[Math]::Min($t,$y);$r=[Math]::Max($r,$x+1);$b=[Math]::Max($b,$y+1)
                }}
                if($count -eq 0){throw "$class frame $frame direction $direction is empty"}
                if($l -le 0 -or $t -le 0 -or $r -ge 64 -or $b -ge 64){throw "$class frame $frame direction $direction touches edge"}
                $frameReports += [ordered]@{frame=$frame;direction=$direction;bounds=@($l,$t,$r,$b);opaquePixels=$count}
            }
        }
        $assetName=$class+'_Projectile'+$suffix
        $reports+=,[ordered]@{class=$class;element=$Element;source=$sourceName;sourceSize=@($source.Width,$source.Height);sourceRows=$SourceRows
            sourceSha256=(Get-FileHash -LiteralPath $InputPath -Algorithm SHA256).Hash.ToLowerInvariant()
            bitmap=($assetName+'.bmp');preview=($assetName+'.png');base=($assetName+'.base.png')
            columns=16;rows=4;frameCount=4;directions=16;cellSize=64;sheetSize=@(1024,256)
            rotationCenter=@(32,32);clockwiseDegreesPerColumn=22.5;columnZero='right';recommendedDrawSize=32
            rotationCoefficientDecimals=12;samplingEpsilon=1e-9
            alphaThreshold=128;transparencyKey='#ff00ff';uniformScale=256.0/$source.Width
            sourceAnchors=$reg.sourceAnchors;registeredAnchor=$reg.targetAnchor;directionAnchors=$anchors
            registrationGroupScale=$groupScale
            anchorKind=$reg.anchorKind;registrationNote=$reg.note;animationFrameSeconds=.06
            method='Uniform whole-sheet nearest-neighbor scale; explicitly recorded anchor registration and one common scale for all four poses; 16 clockwise nearest-neighbor rotations around (32,32).'
            frames=$frameReports}
    }
    if($Element){
        $impactBmp=[Drawing.Bitmap]::new(256,128,[Drawing.Imaging.PixelFormat]::Format24bppRgb)
        $impactPng=[Drawing.Bitmap]::new(256,128,[Drawing.Imaging.PixelFormat]::Format32bppArgb)
        $clear=[Drawing.Graphics]::FromImage($impactBmp);$clear.Clear([Drawing.Color]::Magenta);$clear.Dispose()
        $impactShift=if($registration.Impact){$registration.Impact.translation}else{@(0,0)}
        $impactFrames=@()
        for($frame=0;$frame -lt 8;$frame++){
            $x0=($frame%4)*64;$y0=[int][Math]::Floor($frame/4)*64
            $l=64;$t=64;$r=0;$b=0;$count=0
            for($y=0;$y -lt 64;$y++){for($x=0;$x -lt 64;$x++){
                $color=$normalized.GetPixel($x0+$x,128+$y0+$y)
                if($color.A -eq 0){continue}
                $tx=$x+[int]$impactShift[0];$ty=$y+[int]$impactShift[1]
                if($tx -lt 0 -or $ty -lt 0 -or $tx -ge 64 -or $ty -ge 64){throw "$Element contact $frame registration clips pixels"}
                $impactBmp.SetPixel($x0+$tx,$y0+$ty,$color);$impactPng.SetPixel($x0+$tx,$y0+$ty,$color)
                $count++;$l=[Math]::Min($l,$tx);$t=[Math]::Min($t,$ty);$r=[Math]::Max($r,$tx+1);$b=[Math]::Max($b,$ty+1)
            }}
            if($count -eq 0 -or $l -le 0 -or $t -le 0 -or $r -ge 64 -or $b -ge 64){throw "$Element contact $frame empty or clipped"}
            $impactFrames+=,[ordered]@{frame=$frame;bounds=@($l,$t,$r,$b);opaquePixels=$count}
        }
        $impactReport=[ordered]@{element=$Element;source=$sourceName;sourceSize=@($source.Width,$source.Height);sourceRows=$SourceRows
            sourceSha256=$reports[0].sourceSha256;bitmap=('Element_Impact'+$suffix+'.bmp');preview=('Element_Impact'+$suffix+'.png')
            columns=4;rows=2;frameCount=8;cellSize=64;sheetSize=@(256,128);center=@(32,32)
            recommendedDrawSize=48;alphaThreshold=128;transparencyKey='#ff00ff';uniformScale=256.0/$source.Width
            sourceStartRow=2;animationFrameSeconds=.025;sourceSharedTranslation=$impactShift
            method='Uniform whole-sheet nearest-neighbor scale; source rows 2 and 3 packed with one recorded shared translation for all eight poses. No per-frame fitting or redrawing.'
            frames=$impactFrames}
    }
    # Validate both complete atlases before replacing any production output.
    New-Item -ItemType Directory -Path $OutputDirectory -Force | Out-Null
    New-Item -ItemType Directory -Path $ProofDirectory -Force | Out-Null
    $out=(Resolve-Path -LiteralPath $OutputDirectory).Path
    Copy-Item -LiteralPath $InputPath -Destination (Join-Path $out $sourceName) -Force
    for($ci=0;$ci -lt 2;$ci++){
        $report=$reports[$ci]
        $bmpPath=Join-Path $out $report.bitmap
        $atlases[$ci].Save($bmpPath,[Drawing.Imaging.ImageFormat]::Bmp)
        $previews[$ci].Save((Join-Path $out $report.preview),[Drawing.Imaging.ImageFormat]::Png)
        $bases[$ci].Save((Join-Path $out $report.base),[Drawing.Imaging.ImageFormat]::Png)
        $report.bitmapSha256=(Get-FileHash -LiteralPath $bmpPath -Algorithm SHA256).Hash.ToLowerInvariant()
        $report | ConvertTo-Json -Depth 10 | Set-Content -LiteralPath (Join-Path $out ($classes[$ci]+'_Projectile'+$suffix+'.layout.json')) -Encoding utf8
        $proof=[Drawing.Bitmap]::new(800,240,[Drawing.Imaging.PixelFormat]::Format24bppRgb)
        $g=[Drawing.Graphics]::FromImage($proof)
        $font=[Drawing.Font]::new('Segoe UI',11,[Drawing.FontStyle]::Regular,[Drawing.GraphicsUnit]::Pixel)
        try {
            $g.Clear([Drawing.Color]::FromArgb(45,48,57));$g.InterpolationMode='NearestNeighbor';$g.PixelOffsetMode='Half'
            for($direction=0;$direction -lt 16;$direction++){
                $g.DrawString([string]$direction,$font,[Drawing.Brushes]::White,$direction*50+15,2)
                for($frame=0;$frame -lt 4;$frame++){
                    $g.DrawImage($previews[$ci],[Drawing.Rectangle]::new($direction*50+9,24+$frame*52,32,32),
                        $direction*64,$frame*64,64,64,[Drawing.GraphicsUnit]::Pixel)
                }
            }
            $proof.Save((Join-Path (Resolve-Path -LiteralPath $ProofDirectory).Path ($classes[$ci]+$suffix+'-projectile-32px-proof.png')),[Drawing.Imaging.ImageFormat]::Png)
        }finally{$g.Dispose();$font.Dispose();$proof.Dispose()}
        Write-Output "$($classes[$ci]): 4 frames x 16 directions; all keyed gutters clear."
    }
    if($impactReport){
        $path=Join-Path $out $impactReport.bitmap
        $impactBmp.Save($path,[Drawing.Imaging.ImageFormat]::Bmp)
        $impactPng.Save((Join-Path $out $impactReport.preview),[Drawing.Imaging.ImageFormat]::Png)
        $impactReport.bitmapSha256=(Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash.ToLowerInvariant()
        $impactReport | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath (Join-Path $out ('Element_Impact'+$suffix+'.layout.json')) -Encoding utf8
        Write-Output "$Element contact: 8 frames, shared center (32,32); all keyed gutters clear."
    }
}finally{
    $source.Dispose();$normalized.Dispose()
    foreach($image in ($atlases+$previews+$bases)){$image.Dispose()}
    if($impactBmp){$impactBmp.Dispose()};if($impactPng){$impactPng.Dispose()}
}
