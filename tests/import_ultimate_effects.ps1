param(
 [Parameter(Mandatory=$true)][string]$InputPath,
 [Parameter(Mandatory=$true)][string]$RegistrationPath,
 [string]$Effect,
 [string]$AssetName='',
 [int]$SkillID=0,
 [string]$CharacterClass='Mage',
 [string]$Element='',
 [ValidateSet('charge','flight','rain','quick')][string]$PreparationPhase='charge',
 [ValidateRange(1,11)][int]$PhaseSplit=4,
 [string]$SecondPhase='impact',
 [int[]]$SharedSkillIDs=@(),
 [int]$PreparationDrawSize=0,
 [int]$ImpactDrawSize=240,
 [string]$OutputDirectory='Tower_Triumph/Tower_Triumph/Resource',
 [string]$ProofDirectory='tests/bin/ultimate-import'
)

# Mechanical import only: one isotropic scale for the entire 12-pose sheet,
# measured anchor registration, nearest-neighbor samples, and a binary alpha key.
# Original PNG bytes are retained; no redrawing, recoloring or per-pose fitting.
$ErrorActionPreference='Stop'
Add-Type -AssemblyName System.Drawing
Add-Type -ReferencedAssemblies System.Drawing -TypeDefinition @'
using System;
using System.Drawing;
using System.Drawing.Imaging;
public sealed class UltimateFrameAudit {
 public int Frame,SourceOpaquePixels,RuntimeOpaquePixels,SourceEdgePixels;
 public int[] SourceRect,SourceBounds,RuntimeBounds;
}
public sealed class UltimateImportResult : IDisposable {
 public Bitmap Alpha,Keyed;
 public UltimateFrameAudit[] Frames;
 public void Dispose(){if(Alpha!=null)Alpha.Dispose();if(Keyed!=null)Keyed.Dispose();}
}
public static class UltimateSheetImport {
 public static UltimateImportResult Run(Bitmap source,double[,] sourceAnchors,double[,] targetAnchors,double scale) {
  var result=new UltimateImportResult{Alpha=new Bitmap(512,384,PixelFormat.Format32bppArgb),Keyed=new Bitmap(512,384,PixelFormat.Format24bppRgb),Frames=new UltimateFrameAudit[12]};
  try {
   using(Graphics g=Graphics.FromImage(result.Keyed))g.Clear(Color.Magenta);
   for(int frame=0;frame<12;frame++) {
    int column=frame%4,row=frame/4,left=column*source.Width/4,top=row*source.Height/3,right=(column+1)*source.Width/4,bottom=(row+1)*source.Height/3;
    int width=right-left,height=bottom-top,sl=width,st=height,sr=0,sb=0,dl=128,dt=128,dr=0,db=0,count=0,drawn=0,edge=0;
    double ax=sourceAnchors[frame,0],ay=sourceAnchors[frame,1],tx=targetAnchors[frame,0],ty=targetAnchors[frame,1];
    for(int y=0;y<height;y++)for(int x=0;x<width;x++) {
     Color pixel=source.GetPixel(left+x,top+y);if(pixel.A<128)continue;
     count++;sl=Math.Min(sl,x);st=Math.Min(st,y);sr=Math.Max(sr,x+1);sb=Math.Max(sb,y+1);
     if(x==0||y==0||x==width-1||y==height-1)edge++;
     if(pixel.R==255&&pixel.G==0&&pixel.B==255)throw new Exception("Opaque magenta key collision in source frame "+frame);
     double mappedX=(x+.5-ax)*scale+tx,mappedY=(y+.5-ay)*scale+ty;
     if(mappedX<0||mappedY<0||mappedX>=128||mappedY>=128)throw new Exception("Measured registration clips foreground in frame "+frame);
    }
    if(count==0)throw new Exception("Empty source frame "+frame);
    if(edge>0)throw new Exception("Opaque source crosses/touches a cell boundary in frame "+frame+" ("+edge+" pixels)");
    for(int y=0;y<128;y++)for(int x=0;x<128;x++) {
     int sx=(int)Math.Floor((x+.5-tx)/scale+ax+1e-9),sy=(int)Math.Floor((y+.5-ty)/scale+ay+1e-9);
     if(sx<0||sy<0||sx>=width||sy>=height)continue;
     Color pixel=source.GetPixel(left+sx,top+sy);if(pixel.A<128)continue;
     Color opaque=Color.FromArgb(255,pixel.R,pixel.G,pixel.B);
     result.Alpha.SetPixel(column*128+x,row*128+y,opaque);result.Keyed.SetPixel(column*128+x,row*128+y,opaque);
     drawn++;dl=Math.Min(dl,x);dt=Math.Min(dt,y);dr=Math.Max(dr,x+1);db=Math.Max(db,y+1);
    }
    if(drawn==0||dl<=0||dt<=0||dr>=128||db>=128)throw new Exception("Empty runtime frame or missing transparent gutter in frame "+frame);
    result.Frames[frame]=new UltimateFrameAudit{Frame=frame,SourceRect=new[]{left,top,right,bottom},SourceBounds=new[]{sl,st,sr,sb},RuntimeBounds=new[]{dl,dt,dr,db},SourceOpaquePixels=count,RuntimeOpaquePixels=drawn,SourceEdgePixels=edge};
   }
   return result;
  }catch{result.Dispose();throw;}
 }
}
'@
$sourcePath=(Resolve-Path -LiteralPath $InputPath).Path
$registration=Get-Content -LiteralPath $RegistrationPath -Raw -Encoding UTF8|ConvertFrom-Json
if($registration.frames.Count -ne 12){throw 'All 12 frame registrations are required.'}
if(!$AssetName){$AssetName='Skill_Mage_'+$Effect+'_Ultimate'}
if(!$SkillID){$SkillID=if($Effect -eq 'Meteor'){3103}elseif($Effect -eq 'Sanctuary'){3403}else{throw 'Explicit SkillID required for a new presentation.'}}
if($Effect -eq 'Meteor'){$PreparationPhase='flight'}
if(!$PreparationDrawSize){$PreparationDrawSize=if($Effect -eq 'Meteor'){192}else{$ImpactDrawSize}}
$source=[Drawing.Bitmap]::new($sourcePath)
$result=$null
try {
 $scale=if($registration.uniformScale){[double]$registration.uniformScale}else{128.0/[Math]::Max($source.Width/4.0,$source.Height/3.0)}
 if($scale -le 0 -or $scale -gt 1){throw 'Expected a positive common downscale <=1.'}
 $sourceAnchors=[double[,]]::new(12,2);$targetAnchors=[double[,]]::new(12,2)
 for($i=0;$i -lt 12;$i++) {
  $frame=$registration.frames[$i]
  if([int]$frame.frame -ne $i -or $frame.sourceAnchor.Count -ne 2 -or $frame.targetAnchor.Count -ne 2){throw 'Frames must be row-major and explicitly anchored.'}
  for($axis=0;$axis -lt 2;$axis++){$sourceAnchors[$i,$axis]=[double]$frame.sourceAnchor[$axis];$targetAnchors[$i,$axis]=[double]$frame.targetAnchor[$axis]}
 }
 $result=[UltimateSheetImport]::Run($source,$sourceAnchors,$targetAnchors,$scale)
 New-Item -ItemType Directory -Path $OutputDirectory,$ProofDirectory -Force|Out-Null
 $out=(Resolve-Path -LiteralPath $OutputDirectory).Path;$proof=(Resolve-Path -LiteralPath $ProofDirectory).Path
 $name=$AssetName
 $original=Join-Path $out ($name+'.source.png')
 if((Test-Path -LiteralPath $original) -and (Get-FileHash -LiteralPath $original).Hash -ne (Get-FileHash -LiteralPath $sourcePath).Hash){throw 'A different preserved source already exists; use a new versioned name.'}
 if($sourcePath -ne $original){Copy-Item -LiteralPath $sourcePath -Destination $original -Force}
 $bitmap=Join-Path $out ($name+'.bmp');$png=Join-Path $out ($name+'.png')
 $result.Keyed.Save($bitmap,[Drawing.Imaging.ImageFormat]::Bmp);$result.Alpha.Save($png,[Drawing.Imaging.ImageFormat]::Png)
 $layout=[ordered]@{schemaVersion=1;effect=$Effect;skillID=$SkillID;characterClass=$CharacterClass;element=$Element;generationMode='built-in image_gen';source=($name+'.source.png');sourceSize=@($source.Width,$source.Height);sourceSha256=(Get-FileHash -LiteralPath $sourcePath -Algorithm SHA256).Hash.ToLowerInvariant();bitmap=($name+'.bmp');preview=($name+'.png');bitmapSha256=(Get-FileHash -LiteralPath $bitmap -Algorithm SHA256).Hash.ToLowerInvariant();previewSha256=(Get-FileHash -LiteralPath $png -Algorithm SHA256).Hash.ToLowerInvariant();columns=4;rows=3;frameCount=12;cellSize=128;sheetSize=@(512,384);alphaThreshold=128;transparencyKey='#ff00ff';uniformScale=$scale;groundAnchor=$registration.groundAnchor;flightCoreAnchor=$registration.flightCoreAnchor;preparationAnchor=$registration.preparationAnchor;impactAnchor=$registration.impactAnchor;groundBandStartY=$registration.groundBandStartY;recommendedPreparationDrawSize=$PreparationDrawSize;recommendedImpactDrawSize=$ImpactDrawSize;phases=@(@{name=$PreparationPhase;frames=@(0..($PhaseSplit-1))},@{name=$SecondPhase;frames=@($PhaseSplit..11)});registration=$registration;frames=$result.Frames;method='One common isotropic scale; explicit measured anchor translations; nearest-neighbor pixel-center sampling; alpha>=128 becomes opaque; no pose fitting or painted edits.'}
 if($SharedSkillIDs.Count){$layout.sharedSkillIDs=$SharedSkillIDs}
 $layout|ConvertTo-Json -Depth 12|Set-Content -LiteralPath (Join-Path $out ($name+'.layout.json')) -Encoding UTF8
 $result.Alpha.Save((Join-Path $proof ($Effect+'-128px-atlas.png')),[Drawing.Imaging.ImageFormat]::Png)
 $sheet=[Drawing.Bitmap]::new(1000,826,[Drawing.Imaging.PixelFormat]::Format24bppRgb);$g=[Drawing.Graphics]::FromImage($sheet)
 $font=[Drawing.Font]::new('Segoe UI',13,[Drawing.FontStyle]::Regular,[Drawing.GraphicsUnit]::Pixel)
 try {
  $g.Clear([Drawing.Color]::FromArgb(31,35,43));$g.InterpolationMode=[Drawing.Drawing2D.InterpolationMode]::NearestNeighbor;$g.PixelOffsetMode=[Drawing.Drawing2D.PixelOffsetMode]::Half
  for($i=0;$i -lt 12;$i++) {
   $x=8+($i%4)*248;$y=8+[int][Math]::Floor($i/4)*272
   $size=if($i -lt $PhaseSplit){$PreparationDrawSize}else{$ImpactDrawSize}
   $g.DrawString(($Effect+' #'+$i+' / '+$size+'px'),$font,[Drawing.Brushes]::White,$x,$y)
   $g.DrawImage($result.Alpha,[Drawing.Rectangle]::new($x+(240-$size)/2,$y+24+(240-$size)/2,$size,$size),($i%4)*128,[int][Math]::Floor($i/4)*128,128,128,[Drawing.GraphicsUnit]::Pixel)
  }
  $sheet.Save((Join-Path $proof ($Effect+'-runtime-size-proof.png')),[Drawing.Imaging.ImageFormat]::Png)
 }finally{$g.Dispose();$sheet.Dispose();$font.Dispose()}
 Write-Output ($name+': twelve registered frames; source/runtime gutters clear; original source preserved.')
}finally{$source.Dispose();if($result){$result.Dispose()}}
