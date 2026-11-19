param([string]$OutputDirectory = 'tests/bin/area-renderer')

# Mechanical diagnostic: the production ground formulas + unchanged map/sprite
# rasters. GDI+ stroke rasterization approximates native GDI; no game is launched.
$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.Drawing
$output = [IO.Path]::GetFullPath((Join-Path (Get-Location) $OutputDirectory))
New-Item -ItemType Directory -Path $output -Force | Out-Null
$assets = Join-Path (Get-Location) 'tests/bin/area-skills'
$contract = Get-Content -LiteralPath (Join-Path $assets 'contract.json') -Raw -Encoding UTF8 | ConvertFrom-Json
$manifest = Get-Content -LiteralPath (Join-Path $assets 'sprite-assets.json') -Raw -Encoding UTF8 | ConvertFrom-Json
$map = [Drawing.Bitmap]::new((Join-Path $assets 'hall.png'))
$sprites = [Drawing.Bitmap]::new((Join-Path $assets 'sprites.png'))
$font = [Drawing.Font]::new('Segoe UI',14,[Drawing.FontStyle]::Regular,[Drawing.GraphicsUnit]::Pixel)
$white = [Drawing.SolidBrush]::new([Drawing.Color]::White)
$effects = @{}
foreach($element in @('Fire','Water','Light')) { $effects[$element] = [Drawing.Bitmap]::new((Join-Path $assets ('contact-'+$element+'.png'))) }
$source = @'
using System;
using System.Drawing;
using System.Drawing.Drawing2D;
public static class AreaGroundProof {
    static int R(float v) { return (int)Math.Round(v,MidpointRounding.AwayFromZero); }
    public static Bitmap Ground(float[] values,float elapsed,int ox,int oy,bool line,Color color,Bitmap contacts) {
        Bitmap image=new Bitmap(624,384,System.Drawing.Imaging.PixelFormat.Format32bppArgb);
        using(Graphics g=Graphics.FromImage(image)) using(Region clip=new Region()) {
            clip.MakeEmpty(); float first=float.MaxValue,last=0,youngest=.32f; int near=int.MaxValue,far=0,dx=0,dy=0,active=0;
            for(int i=0;i<values.Length;i+=3) {
                int x=(int)values[i],y=(int)values[i+1],dist=Math.Abs(x-ox)+Math.Abs(y-oy); float arrival=values[i+2],age=elapsed-arrival;
                first=Math.Min(first,arrival);last=Math.Max(last,arrival);near=Math.Min(near,dist);
                if(dist>far){far=dist;dx=Math.Sign(x-ox);dy=Math.Sign(y-oy);}
                if(elapsed+1e-6f<arrival || age>=.32f)continue;
                clip.Union(new Rectangle(x*48-384,y*48-96,48,48));youngest=Math.Min(youngest,Math.Max(0,age));active++;
            }
            if(active==0)return image;
            g.SetClip(clip,CombineMode.Intersect);g.SmoothingMode=SmoothingMode.None;
            float cx=(ox+.5f)*48-384,cy=(oy+.75f)*48-96;
            float step=far>near && last>first?(last-first)/(far-near):.09f;
            float front=near+(elapsed-first)/step,strength=.35f+.65f*(1-youngest/.32f);
            if(elapsed>last) {
                float t=Math.Min(1,(elapsed-last)/.10f);
                front=far+.10f*(2*t-t*t);strength*=(1-t)*(1-t);
            }
            Color main=color,light=Color.FromArgb((color.R*4+255)/5,(color.G*4+245)/5,(color.B*4+230)/5);
            using(Bitmap wave=new Bitmap(624,384,System.Drawing.Imaging.PixelFormat.Format32bppArgb))
            using(Graphics waveG=Graphics.FromImage(wave)) {
             waveG.SmoothingMode=SmoothingMode.None;
             for(int trail=0;trail<2 && strength>.02f;trail++) {
                float radius=(front-trail*.20f)*48;if(!(radius>48*.06f))continue;Point[] points;
                if(line) {
                    float[] across={-.43f,-.22f,0,.22f,.43f},behind={.13f,.03f,0,.03f,.13f};points=new Point[5];
                    for(int n=0;n<5;n++)points[n]=new Point(R(cx+dx*radius+(-dy*across[n]-dx*behind[n])*48),R(cy+dy*radius+(dx*across[n]-dy*behind[n])*48));
                } else {
                    int[,] directions={{1,0},{0,1},{-1,0},{0,-1}};points=new Point[37];int n=0;
                    for(int corner=0;corner<4;corner++)for(int sample=0;sample<=8;sample++) {
                        float t=sample/8f,u=1-t;float[] p=new float[2];
                        for(int axis=0;axis<2;axis++) {
                            float a=directions[corner,axis]*.88f+directions[(corner+3)%4,axis]*.12f;
                            float b=directions[corner,axis]*.88f+directions[(corner+1)%4,axis]*.12f;
                            p[axis]=(u*u*a+2*u*t*directions[corner,axis]+t*t*b)*radius;
                        }
                        points[n++]=new Point(R(cx+p[0]),R(cy+p[1]));
                    }
                    points[36]=points[0];
                }
                using(Pen p=new Pen(main,trail==0?3:1))waveG.DrawLines(p,points);
                if(trail==0)using(Pen p=new Pen(light,1))waveG.DrawLines(p,points);
             }
             using(System.Drawing.Imaging.ImageAttributes opacity=new System.Drawing.Imaging.ImageAttributes()) {
                var matrix=new System.Drawing.Imaging.ColorMatrix();matrix.Matrix33=R(Math.Min(1,strength)*255)/255f;
                opacity.SetColorMatrix(matrix);
                g.DrawImage(wave,new Rectangle(0,0,624,384),0,0,624,384,GraphicsUnit.Pixel,opacity);
             }
            }
            g.InterpolationMode=InterpolationMode.NearestNeighbor;g.PixelOffsetMode=PixelOffsetMode.Half;
            for(int i=0;i<values.Length;i+=3) {
                float age=Math.Max(0,elapsed-values[i+2]);if(elapsed+1e-6f<values[i+2]||age>=.2f)continue;
                int frame=Math.Min(7,(int)((age+1e-6f)/.025f));float x=(values[i]+.5f)*48-384,y=(values[i+1]+.75f)*48-96;
                g.DrawImage(contacts,new Rectangle(R(x-21.5f),R(y-12),43,24),frame*64,0,64,64,GraphicsUnit.Pixel);
            }
        }
        return image;
    }
    public static int CheckPixels(Bitmap image,float[] cells,float elapsed) {
        int painted=0;
        for(int y=0;y<image.Height;y++)for(int x=0;x<image.Width;x++) {
            if(image.GetPixel(x,y).A==0)continue;painted++;bool valid=false;
            for(int i=0;i<cells.Length;i+=3) {
                if(elapsed+1e-6f<cells[i+2]||elapsed-cells[i+2]>=.32f)continue;
                int left=(int)cells[i]*48-384,top=(int)cells[i+1]*48-96;
                if(x>=left&&x<left+48&&y>=top&&y<top+48){valid=true;break;}
            }
            if(!valid)throw new Exception("Ground pixel outside reached snapshot at "+x+","+y);
        }
        return painted;
    }
}
'@
Add-Type -TypeDefinition $source -ReferencedAssemblies System.Drawing

function Draw-Actor([Drawing.Graphics]$g,[string]$Class,[string]$State,[int]$Frame,[int]$GridX,[int]$GridY,[bool]$Flip) {
    $logical = ([Array]::IndexOf(@('Archer','Mage'),$Class)*7+[Array]::IndexOf($manifest.states,$State))*12+$Frame
    $stored=[int]$manifest.frameMap[$logical]
    if($stored -lt 0){throw "Missing pose $Class/$State/$Frame"}
    $columns=[int]$manifest.atlasColumns
    if(!$columns){$columns=[int]$manifest.columns}
    $pose=$sprites.Clone([Drawing.Rectangle]::new(($stored%$columns)*96,[Math]::Floor($stored/$columns)*96,96,96),[Drawing.Imaging.PixelFormat]::Format32bppArgb)
    try {
        if($Flip){$pose.RotateFlip([Drawing.RotateFlipType]::RotateNoneFlipX)}
        $g.DrawImage($pose,[Drawing.Rectangle]::new($GridX*48-408,$GridY*48-144,96,96),0,0,96,96,[Drawing.GraphicsUnit]::Pixel)
    } finally {$pose.Dispose()}
}

$report=@()
try {
 foreach($name in @('small','wide','line')) {
    $scenario=$contract.scenarios.$name;$skill=$contract.skills.([string]$scenario.skillID)
    $line=$name -eq 'line';$radius=if($name -eq 'wide'){2}else{1}
    $ox=if($line){[int]$scenario.heroGrid.x}else{[int]$scenario.selected.x}
    $oy=if($line){[int]$scenario.heroGrid.y}else{[int]$scenario.selected.y}
    $cells=[Collections.Generic.List[single]]::new()
    $step=if($line){48.0/420}else{.09}
    if($line) {
        for($n=1;$n -le $skill.range;$n++){$cells.Add($ox+$n);$cells.Add($oy);$cells.Add([single](($n-1)*$step))}
    } else {
        for($ring=0;$ring -le $radius;$ring++) {
            for($y=$oy-$ring;$y -le $oy+$ring;$y++){for($x=$ox-$ring;$x -le $ox+$ring;$x++){
                if([Math]::Abs($x-$ox)+[Math]::Abs($y-$oy) -eq $ring){$cells.Add($x);$cells.Add($y);$cells.Add([single]($ring*$step))}
            }}
        }
    }
    $values=$cells.ToArray();$color=[Drawing.ColorTranslator]::FromHtml($skill.color)
    $times=if($line){@(.025,.14,.255,.48)}elseif($name -eq 'wide'){@(.025,.115,.205,.255)}else{@(.025,.06,.115,.22)}
    $sheet=[Drawing.Bitmap]::new(1268,852,[Drawing.Imaging.PixelFormat]::Format24bppRgb)
    $sg=[Drawing.Graphics]::FromImage($sheet)
    try {
      $sg.Clear([Drawing.Color]::FromArgb(24,27,32))
      $sg.DrawString(($name+' | actual map/sprites + ground formula | seconds since first arrival'),$font,$white,10,7)
      for($ti=0;$ti -lt $times.Count;$ti++) {
        $time=[single]$times[$ti];$ground=[AreaGroundProof]::Ground($values,$time,$ox,$oy,$line,$color,$effects[$scenario.element])
        $painted=[AreaGroundProof]::CheckPixels($ground,$values,$time)
        $scene=[Drawing.Bitmap]::new(624,384,[Drawing.Imaging.PixelFormat]::Format24bppRgb);$g=[Drawing.Graphics]::FromImage($scene)
        try {
            $g.InterpolationMode=[Drawing.Drawing2D.InterpolationMode]::NearestNeighbor;$g.PixelOffsetMode=[Drawing.Drawing2D.PixelOffsetMode]::Half
            $g.DrawImageUnscaled($map,0,0);$g.DrawImageUnscaled($ground,0,0)
            $units=@([pscustomobject]@{x=[int]$scenario.heroGrid.x;y=[int]$scenario.heroGrid.y;class=$scenario.character;state='Skill';frame=6;flip=$false})
            foreach($target in $scenario.targets) {
                $arrival=$null
                for($ci=0;$ci -lt $values.Length;$ci+=3){if($values[$ci]-eq $target[0] -and $values[$ci+1]-eq $target[1]){$arrival=$values[$ci+2];break}}
                $hit=$null -ne $arrival -and $time+1e-6 -ge $arrival
                $units+=[pscustomobject]@{x=[int]$target[0];y=[int]$target[1];class='Archer';state=$(if($hit){'Hit'}else{'Idle'});frame=$(if($hit){[Math]::Min(11,[Math]::Floor(($time-$arrival+1e-6)/.05))}else{0});flip=$true}
            }
            foreach($unit in ($units|Sort-Object y,x)){Draw-Actor $g $unit.class $unit.state $unit.frame $unit.x $unit.y $unit.flip}
            # Ground-only visual proof deliberately excludes front body contacts,
            # damage and projectiles, so occlusion/cell-boundary issues remain visible.
            $scene.Save((Join-Path $output ($name+'-'+$ti+'.png')),[Drawing.Imaging.ImageFormat]::Png)
            $ground.Save((Join-Path $output ($name+'-'+$ti+'-ground.png')),[Drawing.Imaging.ImageFormat]::Png)
            $sx=10+($ti%2)*634;$sy=36+[Math]::Floor($ti/2)*406
            $sg.DrawString(('t + '+$time.ToString('0.000')+' s'),$font,$white,$sx,$sy)
            $sg.DrawImageUnscaled($scene,$sx,$sy+18)
            $report+=[pscustomobject]@{scenario=$name;time=$time;cells=$values.Length/3;groundPixels=$painted;outsideReachedCells=0}
        }finally{$g.Dispose();$scene.Dispose();$ground.Dispose()}
      }
      $sheet.Save((Join-Path $output ($name+'-ground-proof.png')),[Drawing.Imaging.ImageFormat]::Png)
    }finally{$sg.Dispose();$sheet.Dispose()}
 }
 # Reverse/vertical lines, pre-arrival and fully expired fields must obey the
 # same pixel mask. These are geometry checks, separate from the three scenes.
 foreach($direction in @(@(1,0),@(-1,0),@(0,1),@(0,-1))) {
    $values=[single[]]@()
    for($n=1;$n -le 4;$n++){$values+=@((14+$direction[0]*$n),(6+$direction[1]*$n),(($n-1)*.09))}
    foreach($time in @(-.01,.025,.14,.255,.4,.6)) {
        $ground=[AreaGroundProof]::Ground($values,[single]$time,14,6,$true,[Drawing.Color]::Cyan,$effects.Water)
        try {
            $painted=[AreaGroundProof]::CheckPixels($ground,$values,[single]$time)
            if(($time -lt 0 -or $time -ge .59) -and $painted -ne 0){throw 'Inactive line painted pixels.'}
            $report+=[pscustomobject]@{scenario=('line-direction-'+$direction[0]+'-'+$direction[1]);time=$time;cells=4;groundPixels=$painted;outsideReachedCells=0}
        }finally{$ground.Dispose()}
    }
 }
 $report | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $output 'clipping-report.json') -Encoding utf8
 Write-Output ('PASS: '+$report.Count+' snapshots; all painted ground pixels inside reached cells. GDI+ stroke approximation; no native game execution.')
}finally{
 $map.Dispose();$sprites.Dispose();$font.Dispose();$white.Dispose()
 foreach($effect in $effects.Values){$effect.Dispose()}
}
