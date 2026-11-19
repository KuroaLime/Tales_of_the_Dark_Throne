param(
    [string]$ApprovedDirectory = 'Source/Concepts/CharacterIdle/approved',
    [string]$ResourceDirectory = 'Tower_Triumph/Tower_Triumph/Resource'
)

# Same approved source pixels as IDLE. Only crop, uniform sampling and conversion
# to the engine's RGB transparency key are performed; no character is redrawn.
$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.Drawing
if (-not ('ApprovedPortraitPixels' -as [type])) {
    Add-Type -TypeDefinition @'
using System;
public static class ApprovedPortraitPixels
{
    public static int[] Bounds(int[] pixels, int width, int height)
    {
        int l=width,t=height,r=0,b=0;
        for(int y=0;y<height;++y) for(int x=0;x<width;++x)
            if(((uint)pixels[y*width+x]>>24)>=128) {
                l=Math.Min(l,x);t=Math.Min(t,y);r=Math.Max(r,x+1);b=Math.Max(b,y+1);
            }
        if(l>=r||t>=b) throw new InvalidOperationException("Empty portrait source");
        return new[]{l,t,r,b};
    }
    public static int[] Render(int[] source,int sourceWidth,int sourceHeight,
        int left,int top,int outputWidth,int outputHeight,double scale,bool keyed)
    {
        int[] result=new int[outputWidth*outputHeight];
        for(int y=0;y<outputHeight;++y) for(int x=0;x<outputWidth;++x) {
            int sx=left+(int)Math.Floor((x+.5)/scale), sy=top+(int)Math.Floor((y+.5)/scale);
            int index=y*outputWidth+x;
            if(keyed) result[index]=unchecked((int)0xffff00ff);
            if(sx<0||sy<0||sx>=sourceWidth||sy>=sourceHeight) continue;
            int pixel=source[sy*sourceWidth+sx];
            if(((uint)pixel>>24)<128) continue;
            if((pixel&0xffffff)==0xff00ff) throw new InvalidOperationException("Opaque magenta key collision");
            result[index]=pixel|unchecked((int)0xff000000);
        }
        return result;
    }
}
'@
}

function Read-PortraitPixels([Drawing.Bitmap]$Bitmap) {
    $locked=$Bitmap.LockBits([Drawing.Rectangle]::new(0,0,$Bitmap.Width,$Bitmap.Height),[Drawing.Imaging.ImageLockMode]::ReadOnly,[Drawing.Imaging.PixelFormat]::Format32bppArgb)
    try {
        $pixels=[int[]]::new($Bitmap.Width*$Bitmap.Height)
        for($row=0;$row -lt $Bitmap.Height;$row++) {
            [Runtime.InteropServices.Marshal]::Copy([IntPtr]::Add($locked.Scan0,$row*$locked.Stride),$pixels,$row*$Bitmap.Width,$Bitmap.Width)
        }
        return ,$pixels
    } finally {$Bitmap.UnlockBits($locked)}
}
function Save-PortraitPixels([int[]]$Pixels,[int]$Width,[int]$Height,[string]$Path,[bool]$Keyed) {
    $format=if($Keyed){[Drawing.Imaging.PixelFormat]::Format24bppRgb}else{[Drawing.Imaging.PixelFormat]::Format32bppArgb}
    $bitmap=[Drawing.Bitmap]::new($Width,$Height,$format)
    try {
        $locked=$bitmap.LockBits([Drawing.Rectangle]::new(0,0,$Width,$Height),[Drawing.Imaging.ImageLockMode]::WriteOnly,[Drawing.Imaging.PixelFormat]::Format32bppArgb)
        try {
            for($row=0;$row -lt $Height;$row++) {
                [Runtime.InteropServices.Marshal]::Copy($Pixels,$row*$Width,[IntPtr]::Add($locked.Scan0,$row*$locked.Stride),$Width)
            }
        } finally {$bitmap.UnlockBits($locked)}
        $imageFormat=if($Keyed){[Drawing.Imaging.ImageFormat]::Bmp}else{[Drawing.Imaging.ImageFormat]::Png}
        $bitmap.Save($Path,$imageFormat)
    } finally {$bitmap.Dispose()}
}

New-Item -ItemType Directory -Path $ResourceDirectory -Force | Out-Null
$resourceRoot=(Resolve-Path -LiteralPath $ResourceDirectory).Path
foreach($class in @('Warrior','Archer','Mage')) {
    $sourcePath=(Resolve-Path -LiteralPath (Join-Path $ApprovedDirectory ($class+'.png'))).Path
    $source=[Drawing.Bitmap]::new($sourcePath)
    try {
        $pixels=Read-PortraitPixels $source
        $bounds=[ApprovedPortraitPixels]::Bounds($pixels,$source.Width,$source.Height)
        $margin=16
        $left=$bounds[0]-$margin;$top=$bounds[1]-$margin
        $cropWidth=$bounds[2]-$bounds[0]+2*$margin;$cropHeight=$bounds[3]-$bounds[1]+2*$margin
        $height=512;$scale=$height/[double]$cropHeight
        $width=[int][Math]::Ceiling($cropWidth*$scale)
        $name=$class+'_Portrait'
        $bitmapPath=Join-Path $resourceRoot ($name+'.bmp')
        $pngPath=Join-Path $resourceRoot ($name+'.png')
        $sourceCopy=Join-Path $resourceRoot ($name+'.source.png')
        Save-PortraitPixels ([ApprovedPortraitPixels]::Render($pixels,$source.Width,$source.Height,$left,$top,$width,$height,$scale,$true)) $width $height $bitmapPath $true
        Save-PortraitPixels ([ApprovedPortraitPixels]::Render($pixels,$source.Width,$source.Height,$left,$top,$width,$height,$scale,$false)) $width $height $pngPath $false
        Copy-Item -LiteralPath $sourcePath -Destination $sourceCopy -Force
        $sourceHash=(Get-FileHash -LiteralPath $sourcePath -Algorithm SHA256).Hash.ToLowerInvariant()
        if((Get-FileHash -LiteralPath $sourceCopy -Algorithm SHA256).Hash.ToLowerInvariant() -ne $sourceHash){throw 'Source copy changed'}
        [ordered]@{
            character=$class;approvedReference=('Source/Concepts/CharacterIdle/approved/'+$class+'.png')
            source=($name+'.source.png');sourceSha256=$sourceHash;sourceSize=@($source.Width,$source.Height)
            sourceVisibleBounds=$bounds;cropRect=@($left,$top,($left+$cropWidth),($top+$cropHeight))
            uniformScale=$scale;sheetSize=@($width,$height);columns=1;rows=1;frameCount=1
            bitmap=($name+'.bmp');bitmapSha256=(Get-FileHash -LiteralPath $bitmapPath -Algorithm SHA256).Hash.ToLowerInvariant()
            preview=($name+'.png');alphaThreshold=128;transparencyKey='#ff00ff'
            method='Exact approved reference, cropped with margin and uniformly nearest-neighbor sampled; no redraw or recoloring. Full body matches existing portrait UI composition.'
        } | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath (Join-Path $resourceRoot ($name+'.layout.json')) -Encoding utf8
        Write-Output "$class portrait: ${width}x$height from approved source $sourceHash"
    } finally {$source.Dispose()}
}
