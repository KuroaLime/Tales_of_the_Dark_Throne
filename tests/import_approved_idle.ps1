param(
    [string]$InputPath = 'Source/Concepts/CharacterIdle/idle-style-v1.png',
    [string]$ApprovedDirectory = 'Source/Concepts/CharacterIdle/approved',
    [string]$ResourceDirectory = 'Tower_Triumph/Tower_Triumph/Resource'
)

# Mechanical import of the user-approved master. Cropped reference PNG pixels
# remain unchanged; the game bitmap uses one uniform nearest-neighbor transform.
$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.Drawing
if (-not ('ApprovedIdlePixels' -as [type])) {
    Add-Type -TypeDefinition @'
using System;
public static class ApprovedIdlePixels
{
    public static int[] Bounds(int[] pixels, int width, int[] region, int threshold)
    {
        int l=region[2], t=region[3], r=region[0], b=region[1];
        for(int y=region[1]; y<region[3]; ++y)
            for(int x=region[0]; x<region[2]; ++x)
                if(((uint)pixels[y*width+x]>>24)>=threshold) {
                    l=Math.Min(l,x); t=Math.Min(t,y); r=Math.Max(r,x+1); b=Math.Max(b,y+1);
                }
        if(l>=r || t>=b) throw new InvalidOperationException("Empty approved sprite");
        return new[]{l,t,r,b};
    }
    public static int[] Crop(int[] pixels,int width,int[] rect)
    {
        int w=rect[2]-rect[0], h=rect[3]-rect[1];
        int[] result=new int[w*h];
        for(int y=0;y<h;++y) Array.Copy(pixels,(rect[1]+y)*width+rect[0],result,y*w,w);
        return result;
    }
    public static void AssertEqual(int[] expected,int[] actual)
    {
        if(expected.Length!=actual.Length) throw new InvalidOperationException("Reference dimensions changed");
        for(int i=0;i<expected.Length;++i)
            if(expected[i]!=actual[i]) throw new InvalidOperationException("Reference RGBA pixel changed at "+i);
    }
    public static int[] Render(int[] pixels,int width,int height,double rootX,double groundY,double scale)
    {
        int[] output=new int[96*96];
        for(int i=0;i<output.Length;++i) output[i]=unchecked((int)0xffff00ff);
        for(int y=0;y<96;++y) for(int x=0;x<96;++x) {
            int sx=(int)Math.Floor(rootX+(x+.5-48)/scale);
            int sy=(int)Math.Floor(groundY+(y+.5-88)/scale);
            if(sx<0 || sy<0 || sx>=width || sy>=height) continue;
            int p=pixels[sy*width+sx];
            if(((uint)p>>24)<128) continue;
            if((p&0xffffff)==0xff00ff) throw new InvalidOperationException("Opaque magenta conflicts with transparency key");
            if(x==0 || y==0 || x==95 || y==95) throw new InvalidOperationException("Sprite touches the 96px cell edge");
            output[y*96+x]=p|unchecked((int)0xff000000);
        }
        return output;
    }
}
'@
}

function Read-IdlePixels([System.Drawing.Bitmap]$Bitmap) {
    $rectangle = [System.Drawing.Rectangle]::new(0,0,$Bitmap.Width,$Bitmap.Height)
    $locked = $Bitmap.LockBits($rectangle,[System.Drawing.Imaging.ImageLockMode]::ReadOnly,[System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
    try {
        $data = [int[]]::new($Bitmap.Width*$Bitmap.Height)
        for($y=0;$y -lt $Bitmap.Height;$y++) {
            [Runtime.InteropServices.Marshal]::Copy([IntPtr]::Add($locked.Scan0,$y*$locked.Stride),$data,$y*$Bitmap.Width,$Bitmap.Width)
        }
        return ,$data
    } finally {$Bitmap.UnlockBits($locked)}
}
function Write-IdleBitmap([int[]]$Pixels,[int]$Width,[int]$Height,[string]$Path,[bool]$Keyed) {
    $format = if($Keyed) {[System.Drawing.Imaging.PixelFormat]::Format24bppRgb} else {[System.Drawing.Imaging.PixelFormat]::Format32bppArgb}
    $bitmap = [System.Drawing.Bitmap]::new($Width,$Height,$format)
    try {
        $locked = $bitmap.LockBits([System.Drawing.Rectangle]::new(0,0,$Width,$Height),[System.Drawing.Imaging.ImageLockMode]::WriteOnly,[System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
        try {
            for($y=0;$y -lt $Height;$y++) {
                [Runtime.InteropServices.Marshal]::Copy($Pixels,$y*$Width,[IntPtr]::Add($locked.Scan0,$y*$locked.Stride),$Width)
            }
        } finally {$bitmap.UnlockBits($locked)}
        $imageFormat = if($Keyed) {[System.Drawing.Imaging.ImageFormat]::Bmp} else {[System.Drawing.Imaging.ImageFormat]::Png}
        $bitmap.Save($Path,$imageFormat)
    } finally {$bitmap.Dispose()}
}

$inputFile = (Resolve-Path -LiteralPath $InputPath).Path
$source = [System.Drawing.Bitmap]::new($inputFile)
try {
    if($source.Width -ne 2172 -or $source.Height -ne 724) {throw 'The approved master must be the original 2172x724 v1 lineup.'}
    $pixels = Read-IdlePixels $source
    New-Item -ItemType Directory -Path $ApprovedDirectory -Force | Out-Null
    New-Item -ItemType Directory -Path $ResourceDirectory -Force | Out-Null
    $approvedRoot = (Resolve-Path -LiteralPath $ApprovedDirectory).Path
    $resourceRoot = (Resolve-Path -LiteralPath $ResourceDirectory).Path
    $masterCopy = Join-Path $approvedRoot 'master.png'
    Copy-Item -LiteralPath $inputFile -Destination $masterCopy -Force
    $masterHash = (Get-FileHash -LiteralPath $masterCopy -Algorithm SHA256).Hash.ToLowerInvariant()
    $definitions = @(
        @{name='Warrior';region=@(0,0,724,724);rootX=420;groundY=681},
        @{name='Archer';region=@(724,0,1448,724);rootX=1120;groundY=681},
        @{name='Mage';region=@(1448,0,2172,724);rootX=1800;groundY=680}
    )
    $records=@()
    foreach($definition in $definitions) {
        $visibleBounds = [ApprovedIdlePixels]::Bounds($pixels,$source.Width,[int[]]$definition.region,128)
        $alphaBounds = [ApprovedIdlePixels]::Bounds($pixels,$source.Width,[int[]]$definition.region,1)
        $cropRect = [int[]]@(
            [Math]::Max($definition.region[0],$alphaBounds[0]-12),
            [Math]::Max(0,$alphaBounds[1]-12),
            [Math]::Min($definition.region[2],$alphaBounds[2]+12),
            [Math]::Min($source.Height,$alphaBounds[3]+12))
        $cropWidth=$cropRect[2]-$cropRect[0];$cropHeight=$cropRect[3]-$cropRect[1]
        $cropPixels=[ApprovedIdlePixels]::Crop($pixels,$source.Width,$cropRect)
        $referencePath=Join-Path $approvedRoot ($definition.name+'.png')
        Write-IdleBitmap $cropPixels $cropWidth $cropHeight $referencePath $false
        $reloaded=[System.Drawing.Bitmap]::new($referencePath)
        try {[ApprovedIdlePixels]::AssertEqual($cropPixels,(Read-IdlePixels $reloaded))} finally {$reloaded.Dispose()}

        $rootX=[double]$definition.rootX-$cropRect[0]
        $groundY=[double]$definition.groundY-$cropRect[1]
        $scale=81.0/($visibleBounds[3]-$visibleBounds[1])
        $rendered=[ApprovedIdlePixels]::Render($cropPixels,$cropWidth,$cropHeight,$rootX,$groundY,$scale)
        $name=$definition.name+'_IdlePose'
        $bitmapPath=Join-Path $resourceRoot ($name+'.bmp')
        Write-IdleBitmap $rendered 96 96 $bitmapPath $true
        $sourceCopy=Join-Path $resourceRoot ($name+'.source.png')
        Copy-Item -LiteralPath $referencePath -Destination $sourceCopy -Force
        $record=[ordered]@{
            character=$definition.name;master='master.png';masterSha256=$masterHash
            reference=($definition.name+'.png');referenceSha256=(Get-FileHash -LiteralPath $referencePath -Algorithm SHA256).Hash.ToLowerInvariant()
            cropRect=$cropRect;sourceSize=@($cropWidth,$cropHeight);masterVisibleBounds=$visibleBounds
            sourceVisibleBounds=@(($visibleBounds[0]-$cropRect[0]),($visibleBounds[1]-$cropRect[1]),($visibleBounds[2]-$cropRect[0]),($visibleBounds[3]-$cropRect[1]))
            sourceRoot=@($rootX,$groundY);masterRoot=@($definition.rootX,$definition.groundY)
            uniformScale=$scale;targetVisibleHeight=81;gameVisibleHeight=54
            sheetSize=@(96,96);columns=1;rows=1;frameCount=1;cellSize=96;root=@(48,88)
            source=($name+'.source.png');bitmap=($name+'.bmp')
            bitmapSha256=(Get-FileHash -LiteralPath $bitmapPath -Algorithm SHA256).Hash.ToLowerInvariant()
            alphaThreshold=128;transparencyKey='#ff00ff'
            method='Exact RGBA crop for reference; uniform nearest-neighbor scale and fixed body-root translation for game BMP. No redraw, recolor or pixel deformation.'
        }
        $record | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath (Join-Path $resourceRoot ($name+'.layout.json')) -Encoding utf8
        [ordered]@{scale=$scale;rootX=$rootX;groundY=$groundY;source=$record.source;root=@(48,88)} | ConvertTo-Json -Depth 4 |
            Set-Content -LiteralPath (Join-Path $resourceRoot ($name+'.alignment.json')) -Encoding utf8
        $records += $record
        Write-Output "$($definition.name): crop $($cropRect -join ','); root $rootX,$groundY; scale $scale; visible 54px"
    }
    [ordered]@{
        status='User approved original v1 IDLE design on 2026-11-18 by attaching the exact master image.'
        selectedConcept='../idle-style-v1.png';master='master.png';sha256=$masterHash
        originalGeneratedFilename='exec-cce0bd7e-a23b-4e9b-93ff-cf073221e84e.png'
        pixelPreservation='All pixels in each cropped reference were round-trip checked equal to the master RGBA region.'
        characters=$records
    } | ConvertTo-Json -Depth 10 | Set-Content -LiteralPath (Join-Path $approvedRoot 'selection.json') -Encoding utf8
} finally {$source.Dispose()}
