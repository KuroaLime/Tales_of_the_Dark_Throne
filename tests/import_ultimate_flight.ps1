param(
 [Parameter(Mandatory=$true)][string]$InputPath,
 [Parameter(Mandatory=$true)][double]$TipX,
 [Parameter(Mandatory=$true)][double]$TipY,
 [double]$GroupScale=1,
 [string]$OutputDirectory='Tower_Triumph/Tower_Triumph/Resource',
 [string]$ProofDirectory='tests/bin/class-ultimate-import'
)
$ErrorActionPreference='Stop'
Add-Type -AssemblyName System.Drawing
Add-Type -ReferencedAssemblies System.Drawing -TypeDefinition @'
using System;
using System.Drawing;
using System.Drawing.Imaging;
using System.Collections.Generic;
public sealed class UltimateFlightFrame {public int frame,direction,opaquePixels;public int[] bounds;}
public static class UltimateFlightPacking {
 public static UltimateFlightFrame[] Run(Bitmap source,Bitmap keyed,Bitmap alpha,double scale) {
  if(source.Width!=512||source.Height!=384)throw new Exception("Expected normalized 4x3/128 main sheet.");
  using(var g=Graphics.FromImage(keyed))g.Clear(Color.Magenta);
  var reports=new List<UltimateFlightFrame>();
  for(int direction=0;direction<16;direction++) {
   double a=direction*Math.PI/8,c=Math.Round(Math.Cos(a),12),s=Math.Round(Math.Sin(a),12);
   for(int frame=0;frame<4;frame++) {
    for(int y=0;y<128;y++)for(int x=0;x<128;x++) {
     if(source.GetPixel(frame*128+x,y).A<128)continue;
     double dx=(x+.5-64)*scale,dy=(y+.5-64)*scale;
     double tx=64+c*dx-s*dy,ty=64+s*dx+c*dy;
     if(tx<1||ty<1||tx>=127||ty>=127)throw new Exception("Flight foreground leaves safe rotation gutter: "+frame+"/"+direction);
    }
    int l=128,t=128,r=0,b=0,count=0;
    for(int y=0;y<128;y++)for(int x=0;x<128;x++) {
     double dx=x+.5-64,dy=y+.5-64;
     int sx=(int)Math.Floor((c*dx+s*dy)/scale+64+1e-9),sy=(int)Math.Floor((-s*dx+c*dy)/scale+64+1e-9);
     if(sx<0||sy<0||sx>=128||sy>=128)continue;
     Color pixel=source.GetPixel(frame*128+sx,sy);if(pixel.A<128)continue;
     keyed.SetPixel(direction*128+x,frame*128+y,pixel);alpha.SetPixel(direction*128+x,frame*128+y,pixel);
     count++;l=Math.Min(l,x);t=Math.Min(t,y);r=Math.Max(r,x+1);b=Math.Max(b,y+1);
    }
    if(count==0||l<=0||t<=0||r>=128||b>=128)throw new Exception("Empty or clipped output frame.");
    reports.Add(new UltimateFlightFrame{frame=frame,direction=direction,opaquePixels=count,bounds=new[]{l,t,r,b}});
   }
  }
  return reports.ToArray();
 }
}
'@
if($GroupScale -le 0 -or $GroupScale -gt 1){throw 'Common group scale must be in (0,1].'}
$sourcePath=(Resolve-Path -LiteralPath $InputPath).Path
$source=[Drawing.Bitmap]::new($sourcePath)
$keyed=[Drawing.Bitmap]::new(2048,512,[Drawing.Imaging.PixelFormat]::Format24bppRgb)
$alpha=[Drawing.Bitmap]::new(2048,512,[Drawing.Imaging.PixelFormat]::Format32bppArgb)
try {
 $frames=[UltimateFlightPacking]::Run($source,$keyed,$alpha,$GroupScale)
 $baseName=[IO.Path]::GetFileNameWithoutExtension($InputPath)
 $name=$baseName+'_Flight'
 $baseLayout=Get-Content -Raw -LiteralPath ([IO.Path]::ChangeExtension($sourcePath,'layout.json'))|ConvertFrom-Json
 New-Item -ItemType Directory -Path $OutputDirectory,$ProofDirectory -Force|Out-Null
 $out=(Resolve-Path -LiteralPath $OutputDirectory).Path
 $bmp=Join-Path $out ($name+'.bmp');$png=Join-Path $out ($name+'.png')
 $keyed.Save($bmp,[Drawing.Imaging.ImageFormat]::Bmp);$alpha.Save($png,[Drawing.Imaging.ImageFormat]::Png)
 $ax=64+($TipX-64)*$GroupScale;$ay=64+($TipY-64)*$GroupScale;$anchors=@()
 foreach($d in 0..15){$angle=$d*[Math]::PI/8;$c=[Math]::Round([Math]::Cos($angle),12);$s=[Math]::Round([Math]::Sin($angle),12);$anchors+=,@([Math]::Round(64+$c*($ax-64)-$s*($ay-64),6),[Math]::Round(64+$s*($ax-64)+$c*($ay-64),6))}
 $layout=[ordered]@{schemaVersion=1;generationMode='mechanical derivative of built-in image_gen';skillID=$baseLayout.skillID;characterClass='Archer';element=$baseLayout.element;source=$baseLayout.source;sourceSha256=$baseLayout.sourceSha256;sourceMainPreview=([IO.Path]::GetFileName($sourcePath));sourceMainPreviewSha256=(Get-FileHash -LiteralPath $sourcePath -Algorithm SHA256).Hash.ToLowerInvariant();bitmap=($name+'.bmp');preview=($name+'.png');bitmapSha256=(Get-FileHash -LiteralPath $bmp -Algorithm SHA256).Hash.ToLowerInvariant();previewSha256=(Get-FileHash -LiteralPath $png -Algorithm SHA256).Hash.ToLowerInvariant();columns=16;rows=4;frameCount=4;directions=16;cellSize=128;sheetSize=@(2048,512);rotationCenter=@(64,64);clockwiseDegreesPerColumn=22.5;columnZero='right';sourceTip=@($TipX,$TipY);registeredAnchor=@($ax,$ay);directionAnchors=$anchors;registrationGroupScale=$GroupScale;rotationCoefficientDecimals=12;samplingEpsilon=1e-9;recommendedDrawSize=80;alphaThreshold=128;transparencyKey='#ff00ff';frames=$frames;method='First four normalized source frames only. One common scale about cell center, composed with 16 nearest-neighbor rotations in one sample. No per-frame fitting, RGB change, or painted edits.'}
 $layout|ConvertTo-Json -Depth 10|Set-Content -LiteralPath (Join-Path $out ($name+'.layout.json')) -Encoding UTF8
 $proof=[Drawing.Bitmap]::new(1344,388,[Drawing.Imaging.PixelFormat]::Format24bppRgb);$g=[Drawing.Graphics]::FromImage($proof);$font=[Drawing.Font]::new('Segoe UI',11)
 try {
  $g.Clear([Drawing.Color]::FromArgb(31,35,43));$g.InterpolationMode='NearestNeighbor';$g.PixelOffsetMode='Half'
  for($d=0;$d -lt 16;$d++){$g.DrawString([string]$d,$font,[Drawing.Brushes]::White,($d*84+34),2);for($f=0;$f -lt 4;$f++){$g.DrawImage($alpha,[Drawing.Rectangle]::new(($d*84+2),(28+$f*90),80,80),$d*128,$f*128,128,128,[Drawing.GraphicsUnit]::Pixel)}}
  $proof.Save((Join-Path (Resolve-Path -LiteralPath $ProofDirectory).Path ($name+'-80px-proof.png')),[Drawing.Imaging.ImageFormat]::Png)
 }finally{$g.Dispose();$font.Dispose();$proof.Dispose()}
 Write-Output ($name+': 64 rotated cells, full foreground and transparent gutters preserved; measured tip '+$ax+','+$ay)
}finally{$source.Dispose();$keyed.Dispose();$alpha.Dispose()}
