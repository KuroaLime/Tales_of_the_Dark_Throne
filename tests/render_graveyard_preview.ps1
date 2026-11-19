param([string]$OutputDirectory = (Join-Path $PSScriptRoot 'bin/graveyard-preview'))

# Design preview, NOT a game capture or native runtime verification. Authored
# scenes are shown at their exact grid dimensions. The legacy C# fallback
# mirrors GraveyardRenderer.cpp's CPU compositor using production map data/art.
$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.Drawing
$repositoryRoot = Split-Path -Parent $PSScriptRoot
$sourceDirectory = Join-Path $repositoryRoot 'Tower_Triumph/Tower_Triumph'
$OutputDirectory = [System.IO.Path]::GetFullPath($OutputDirectory)
New-Item -ItemType Directory -Path $OutputDirectory -Force | Out-Null
$readMap = @'
import json,re,sys
sys.path.insert(0,sys.argv[1])
from verify_graveyard import body_of,load_graveyard,SOURCE
source=(SOURCE/'BattleMapData.cpp').read_text(encoding='utf-8-sig')
w,h,t,rows,tiles,props,players,enemies=load_graveyard(source)
stage=body_of(source,'BuildStage01')
aligned=re.search(r'map\.gridAlignedBackground\s*=\s*(true|false)\s*;',stage)
background=re.search(r'map\.backgroundFilePath\s*=\s*L("(?:\\.|[^"\\])*")\s*;',stage)
print(json.dumps(dict(width=w,height=h,tile=t,tiles=tiles,props=props,players=players,enemies=enemies,
    gridAlignedBackground=bool(aligned and aligned[1]=='true'),
    backgroundFilePath=json.loads(background[1]) if background else '')))
'@
$mapJson = & python -B -c $readMap $PSScriptRoot
if ($LASTEXITCODE -ne 0) { throw 'Cannot parse the production graveyard map' }
$map = $mapJson | ConvertFrom-Json

$rasterSource = @'
using System;
using System.Drawing;
using System.Drawing.Imaging;
using System.Runtime.InteropServices;

public static class GraveyardDiagnosticRasterV4
{
    static float Clamp(float v) { return Math.Max(0.0f, Math.Min(1.0f, v)); }
    static float Smooth(float low, float high, float value)
    {
        float t = Clamp((value-low)/(high-low));
        return t*t*(3.0f-2.0f*t);
    }
    static int Mix(int a, int b, float amount)
    {
        amount = Clamp(amount);
        int result = 0;
        for(int shift=0;shift<=16;shift+=8)
        {
            float first=(a>>shift)&255, second=(b>>shift)&255;
            result |= ((int)(first+(second-first)*amount+0.5f))<<shift;
        }
        return result;
    }
    static float Weathering(int x, int y)
    {
        return (float)Math.Sin(x*0.105f+(float)Math.Sin(y*0.071f)*1.7f)*0.55f+
            (float)Math.Sin(y*0.19f+x*0.047f)*0.30f+
            (float)Math.Sin(x*0.39f-y*0.31f)*0.15f;
    }
    sealed class Atlas
    {
        public int Width,Height;
        int[] pixels;
        public Atlas(Bitmap source)
        {
            Width=source.Width; Height=source.Height;
            pixels=new int[Width*Height];
            using(Bitmap rgba=new Bitmap(Width,Height,PixelFormat.Format32bppArgb))
            {
                using(Graphics g=Graphics.FromImage(rgba)) g.DrawImageUnscaled(source,0,0);
                BitmapData data=rgba.LockBits(new Rectangle(0,0,Width,Height),ImageLockMode.ReadOnly,PixelFormat.Format32bppArgb);
                try { Marshal.Copy(data.Scan0,pixels,0,pixels.Length); }
                finally { rgba.UnlockBits(data); }
            }
        }
        public int Sample(int slot,float u,float v,bool deck=false)
        {
            int col=slot%4,row=slot/4;
            int left=Width*col/4+2,top=Height*row/4+2;
            int right=Width*(col+1)/4-2,bottom=Height*(row+1)/4-2;
            if(deck)
            {
                int h=bottom-top;
                top+=h*40/100; bottom=top+h*24/100;
                int center=(left+right)/2;
                left=center-(bottom-top)/2; right=left+(bottom-top);
            }
            float sx=left+Clamp(u)*(right-left-1),sy=top+Clamp(v)*(bottom-top-1);
            int x=(int)sx,y=(int)sy,nx=Math.Min(x+1,right-1),ny=Math.Min(y+1,bottom-1);
            int upper=Mix(pixels[y*Width+x],pixels[y*Width+nx],sx-x);
            int lower=Mix(pixels[ny*Width+x],pixels[ny*Width+nx],sx-x);
            return Mix(upper,lower,sy-y);
        }
        public int World(int slot,int x,int y,int span=192,bool deck=false)
        {
            int mx=x%(span*2),my=y%(span*2);
            if(mx>=span) mx=span*2-1-mx;
            if(my>=span) my=span*2-1-my;
            return Sample(slot,mx/(float)(span-1),my/(float)(span-1),deck);
        }
        public int Reduced(int slot,float u,float v,int drawWidth,int drawHeight)
        {
            int sx=Math.Min(4,Math.Max(1,(Width/4+drawWidth-1)/drawWidth));
            int sy=Math.Min(4,Math.Max(1,(Height/4+drawHeight-1)/drawHeight));
            int red=0,green=0,blue=0;
            for(int y=0;y<sy;y++) for(int x=0;x<sx;x++)
            {
                int sample=Sample(slot,u+((x+0.5f)/sx-0.5f)/drawWidth,
                    v+((y+0.5f)/sy-0.5f)/drawHeight);
                red+=(sample>>16)&255; green+=(sample>>8)&255; blue+=sample&255;
            }
            int count=sx*sy;
            return ((red/count)<<16)|((green/count)<<8)|(blue/count);
        }
    }
    static int LowWallPixel(Atlas atlas,int ground,int x,int y,
        int dx,int dy,int width,int height)
    {
        // Diagnostic mirror of the native raised-wall compositor. Stones,
        // moss and shadow stay inside this prop's reserved collision box.
        bool horizontal=width>=height;
        float along=horizontal?dx:dy;
        float across=horizontal?dy:dx;
        float length=horizontal?width:height;
        float depth=horizontal?height:width;
        int course=(int)along/22;
        int wear=(course*37+(x-dx)*3+(y-dy)*5)%13;
        float endDistance=Math.Min(along,length-1.0f-along);
        float collapsed=1.0f-Smooth(5.0f,27.0f,endDistance);
        float crest=depth*0.22f+(wear%3)*1.3f+
            (wear==0?depth*0.17f:0.0f)+collapsed*depth*0.28f;
        float foot=depth*0.78f+Weathering(x,y)*1.3f;
        float ends=Smooth(3.0f,8.0f,endDistance);
        float shadow=ends*Smooth(crest,crest+6.0f,across)*
            (1.0f-Smooth(foot,depth-2.0f,across));
        int color=Mix(ground,0x0d141c,shadow*0.48f);
        float body=ends*Smooth(crest-0.7f,crest+0.7f,across)*
            (1.0f-Smooth(foot-1.0f,foot+1.0f,across));
        int stone=atlas.World(3,x,y,144);
        float cap=1.0f-Smooth(crest+6.0f,crest+8.5f,across);
        int masonry=Mix(Mix(stone,0x131d29,0.24f),
            Mix(stone,0xa2a9b0,0.27f),cap);
        int joint=((int)along+wear%3)%22;
        if(joint<2) masonry=Mix(masonry,0x1b2530,cap*0.55f);
        masonry=Mix(masonry,0x2a392e,
            Smooth(foot-7.0f,foot,across)*(0.30f+Weathering(x,y)*0.15f));
        color=Mix(color,masonry,body);
        int rubblePhase=(x-dx+y-dy)%11;
        float rx=((int)along+rubblePhase)%29-14.0f;
        float ry=across-depth*0.88f;
        float rubble=ends*(1.0f-Smooth(0.6f,1.0f,
            rx*rx/22.0f+ry*ry/7.0f));
        return Mix(color,Mix(stone,0x75808b,0.12f),rubble*0.80f);
    }
    static bool Same(string[] types,int width,int height,string type,int x,int y)
    { return x>=0&&y>=0&&x<width&&y<height&&types[y*width+x]==type; }
    static float EdgeDistance(string[] types,int width,int height,int tile,string type,int x,int y)
    {
        int gx=x/tile,gy=y/tile;
        float left=x%tile+0.5f,top=y%tile+0.5f,right=tile-left,bottom=tile-top,distance=tile;
        if(!Same(types,width,height,type,gx-1,gy)) distance=Math.Min(distance,left);
        if(!Same(types,width,height,type,gx+1,gy)) distance=Math.Min(distance,right);
        if(!Same(types,width,height,type,gx,gy-1)) distance=Math.Min(distance,top);
        if(!Same(types,width,height,type,gx,gy+1)) distance=Math.Min(distance,bottom);
        if(!Same(types,width,height,type,gx-1,gy-1)) distance=Math.Min(distance,(float)Math.Sqrt(left*left+top*top));
        if(!Same(types,width,height,type,gx+1,gy-1)) distance=Math.Min(distance,(float)Math.Sqrt(right*right+top*top));
        if(!Same(types,width,height,type,gx-1,gy+1)) distance=Math.Min(distance,(float)Math.Sqrt(left*left+bottom*bottom));
        if(!Same(types,width,height,type,gx+1,gy+1)) distance=Math.Min(distance,(float)Math.Sqrt(right*right+bottom*bottom));
        return distance;
    }
    public static Bitmap Render(Bitmap input,int mapWidth,int mapHeight,int tile,string[] types,int[] props)
    {
        if(types.Length!=mapWidth*mapHeight||props.Length%5!=0) throw new ArgumentException("Invalid map data");
        int width=mapWidth*tile,height=mapHeight*tile;
        Atlas atlas=new Atlas(input);
        int[] output=new int[width*height];
        bool[] propCells=new bool[mapWidth*mapHeight];
        for(int p=0;p<props.Length;p+=5)
        {
            int px=props[p],py=props[p+1],pw=props[p+2],ph=props[p+3],slot=props[p+4];
            if(px<0||py<0||pw<=0||ph<=0||px+pw>mapWidth||py+ph>mapHeight||slot<0||slot>=16) continue;
            for(int y=py;y<py+ph;y++) for(int x=px;x<px+pw;x++) propCells[y*mapWidth+x]=true;
        }
        for(int y=0;y<height;y++)
        {
            for(int x=0;x<width;x++)
            {
                int gx=x/tile,gy=y/tile;
                string type=types[gy*mapWidth+gx];
                int ground=atlas.World(0,x,y),color=ground;
                float noise=(type=="Road"||type=="Water"||type=="CursedGround") ? Weathering(x,y) : 0.0f;
                if(!propCells[gy*mapWidth+gx])
                {
                    switch(type)
                    {
                    case "Road":
                        color=Mix(ground,atlas.World(1,x,y),0.68f*Smooth(1.5f,18.0f+noise*4.0f,EdgeDistance(types,mapWidth,mapHeight,tile,type,x,y)));
                        break;
                    case "Bridge":
                        color=Mix(Mix(ground,atlas.World(1,x,y),0.68f),
                            Mix(atlas.World(12,x,y,96,true),ground,0.10f),
                            Smooth(0.0f,7.0f,EdgeDistance(types,mapWidth,mapHeight,tile,type,x,y))); break;
                    case "Water":
                        int dampEarth=Mix(ground,0x151e20,0.45f);
                        float bank=6.0f+noise*3.5f;
                        color=Mix(dampEarth,atlas.World(2,x,y),Smooth(bank-2.0f,bank+3.0f,EdgeDistance(types,mapWidth,mapHeight,tile,type,x,y))); break;
                    case "Wall": case "Fence": color=atlas.World(3,x,y,144); break;
                    case "Building": color=Mix(atlas.World(3,x,y,144),0x182435,0.45f); break;
                    case "Grave": color=atlas.Sample(4,(x%tile)/(float)(tile-1),(y%tile)/(float)(tile-1)); break;
                    case "Tree": color=atlas.Sample(5,(x%tile)/(float)(tile-1),(y%tile)/(float)(tile-1)); break;
                    case "CursedGround":
                        color=Mix(ground,atlas.World(6,x,y,144),Smooth(1.0f,15.0f+noise*4.0f,EdgeDistance(types,mapWidth,mapHeight,tile,type,x,y))); break;
                    case "MagicHazard":
                        float edge=Math.Min(Math.Min(x%tile,y%tile),Math.Min(tile-1-x%tile,tile-1-y%tile));
                        color=Mix(ground,atlas.Reduced(7,(x%tile)/(float)(tile-1),(y%tile)/(float)(tile-1),tile,tile),Smooth(0.0f,7.0f,edge)); break;
                    }
                }
                output[y*width+x]=color;
            }
        }
        for(int p=0;p<props.Length;p+=5)
        {
            int px=props[p],py=props[p+1],pw=props[p+2],ph=props[p+3],slot=props[p+4];
            if(px<0||py<0||pw<=0||ph<=0||px+pw>mapWidth||py+ph>mapHeight||slot<0||slot>=16) continue;
            int left=px*tile,top=py*tile,w=pw*tile,h=ph*tile;
            float feather=Math.Min(34.0f,Math.Min(w,h)*0.22f);
            for(int dy=0;dy<h;dy++)
            {
                for(int dx=0;dx<w;dx++)
                {
                    int x=left+dx,y=top+dy;
                    if(slot==3)
                    {
                        output[y*width+x]=LowWallPixel(atlas,output[y*width+x],x,y,dx,dy,w,h);
                        continue;
                    }
                    float soften=feather+Weathering(x,y)*feather*0.20f;
                    float alpha=Smooth(0.0f,soften,dx)*Smooth(0.0f,soften,dy)*
                        Smooth(0.0f,soften,w-1-dx)*Smooth(0.0f,soften,h-1-dy);
                    int artwork=atlas.Reduced(slot,dx/(float)(w-1),dy/(float)(h-1),w,h);
                    output[y*width+x]=Mix(output[y*width+x],artwork,alpha);
                }
            }
        }
        for(int i=0;i<output.Length;i++) output[i]|=unchecked((int)0xff000000);
        Bitmap result=new Bitmap(width,height,PixelFormat.Format32bppArgb);
        BitmapData bits=result.LockBits(new Rectangle(0,0,width,height),ImageLockMode.WriteOnly,PixelFormat.Format32bppArgb);
        try { Marshal.Copy(output,0,bits.Scan0,output.Length); }
        finally { result.UnlockBits(bits); }
        return result;
    }
}
'@
if (-not $map.gridAlignedBackground -and !('GraveyardDiagnosticRasterV4' -as [type])) {
    $drawingReferences = @(
        [System.Drawing.Bitmap].Assembly.Location,
        [System.Drawing.Rectangle].Assembly.Location,
        [System.ComponentModel.Component].Assembly.Location
    )
    # Newer .NET versions split GDI+ into these assemblies; Windows PowerShell
    # and earlier PowerShell versions instead expose everything in Drawing.
    foreach ($name in @('System.Private.Windows.GdiPlus.dll', 'System.Private.Windows.Core.dll')) {
        $assemblyPath = Join-Path $PSHOME $name
        if (Test-Path -LiteralPath $assemblyPath) { $drawingReferences += $assemblyPath }
    }
    Add-Type -TypeDefinition $rasterSource -ReferencedAssemblies @($drawingReferences | Select-Object -Unique)
}
$atlas = $null
if ($map.gridAlignedBackground) {
    if ([string]::IsNullOrWhiteSpace($map.backgroundFilePath)) {
        throw 'The grid-aligned scene has no backgroundFilePath'
    }
    $authored = [System.Drawing.Bitmap]::new((Join-Path $sourceDirectory $map.backgroundFilePath))
    try {
        $expectedWidth = $map.width * $map.tile
        $expectedHeight = $map.height * $map.tile
        if ($authored.Width -ne $expectedWidth -or $authored.Height -ne $expectedHeight) {
            throw "Authored scene must be exactly ${expectedWidth}x${expectedHeight}; found $($authored.Width)x$($authored.Height)"
        }
        $bounds = [System.Drawing.Rectangle]::new(0, 0, $expectedWidth, $expectedHeight)
        $canvas = $authored.Clone($bounds, [System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
    }
    finally { $authored.Dispose() }
}
else {
    [string[]]$terrain = @($map.tiles | ForEach-Object { foreach ($value in $_) { [string]$value } })
    [int[]]$props = @($map.props | ForEach-Object { $_[0]; $_[1]; $_[2]; $_[3]; $_[5] })
    $atlas = [System.Drawing.Bitmap]::new((Join-Path $sourceDirectory 'Source/SMap/background/GraveyardAtlas.bmp'))
    $canvas = [GraveyardDiagnosticRasterV4]::Render($atlas, $map.width, $map.height, $map.tile, $terrain, $props)
}
$graphics = [System.Drawing.Graphics]::FromImage($canvas)
try {
    $backgroundPath = Join-Path $OutputDirectory 'graveyard-preview.png'
    $canvas.Save($backgroundPath, [System.Drawing.Imaging.ImageFormat]::Png)
    $gridPen = [System.Drawing.Pen]::new([System.Drawing.Color]::FromArgb(110, 150, 160, 180), 1)
    $blockedPen = [System.Drawing.Pen]::new([System.Drawing.Color]::FromArgb(240, 80, 100), 1)
    $playerPen = [System.Drawing.Pen]::new([System.Drawing.Color]::FromArgb(70, 230, 255), 3)
    $enemyPen = [System.Drawing.Pen]::new([System.Drawing.Color]::FromArgb(255, 120, 100), 3)
    try {
        for ($x = 0; $x -le $map.width; $x++) {
            $graphics.DrawLine($gridPen, ($x * $map.tile), 0, ($x * $map.tile), $canvas.Height)
        }
        for ($y = 0; $y -le $map.height; $y++) {
            $graphics.DrawLine($gridPen, 0, ($y * $map.tile), $canvas.Width, ($y * $map.tile))
        }
        for ($y = 0; $y -lt $map.height; $y++) {
            for ($x = 0; $x -lt $map.width; $x++) {
                if ($map.tiles[$y][$x] -in @('Water', 'Wall', 'Building', 'Tree', 'Grave', 'Fence', 'SkyVoid')) {
                    $cx = $x * $map.tile + $map.tile / 2
                    $cy = $y * $map.tile + $map.tile / 2
                    $graphics.DrawLine($blockedPen, ($cx - 5), ($cy - 5), ($cx + 5), ($cy + 5))
                    $graphics.DrawLine($blockedPen, ($cx + 5), ($cy - 5), ($cx - 5), ($cy + 5))
                }
            }
        }
        foreach ($start in $map.players) {
            $graphics.DrawRectangle($playerPen, ($start[0] * $map.tile + 16), ($start[1] * $map.tile + 30), 16, 12)
        }
        foreach ($start in $map.enemies) {
            $graphics.DrawRectangle($enemyPen, ($start[0] * $map.tile + 16), ($start[1] * $map.tile + 30), 16, 12)
        }
        $debugPath = Join-Path $OutputDirectory 'graveyard-debug-preview.png'
        $canvas.Save($debugPath, [System.Drawing.Imaging.ImageFormat]::Png)
    }
    finally {
        $gridPen.Dispose(); $blockedPen.Dispose(); $playerPen.Dispose(); $enemyPen.Dispose()
    }
    if ($map.gridAlignedBackground) {
        Write-Output "Authored scene asset preview (not a game capture): $backgroundPath"
    }
    else {
        Write-Output "Diagnostic reconstruction (not a game capture): $backgroundPath"
    }
    Write-Output "Collision grid/spawn overlay: $debugPath"
}
finally {
    $graphics.Dispose(); $canvas.Dispose()
    if ($null -ne $atlas) { $atlas.Dispose() }
}
