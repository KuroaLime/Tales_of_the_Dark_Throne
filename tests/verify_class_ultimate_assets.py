"""Independent pixel/geometry audit for generated Warrior and Archer effects."""
import argparse
import hashlib
import json
import math
import struct
from pathlib import Path
from verify_battle_projectile_assets import read_png
from verify_character_action_assets import read_bitmap, require

ROOT = Path(__file__).resolve().parents[1]
RESOURCE = ROOT/'Tower_Triumph/Tower_Triumph/Resource'
REPORT = ROOT/'tests/bin/class-ultimate-import'
KEY = (255, 0, 255)
PROFILES = [(f'Skill_Warrior_{e}_Ultimate', sid) for e,sid in
            [('Fire',1102),('Water',1202),('Grass',1302),('Light',1403),('Dark',1503)]] + [
            (f'Skill_Archer_{e}_Snipe_Ultimate',sid) for e,sid in
            [('Fire',2103),('Water',2203),('Grass',2303),('Dark',2503)]] + [('Skill_Archer_Light_Rain_Ultimate',2403)]


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def load(stem):
    data=json.loads((RESOURCE/(stem+'.layout.json')).read_text(encoding='utf-8-sig'))
    for field in ('source','bitmap','preview'):
        require(digest(RESOURCE/data[field])==data[field+'Sha256'],f'{stem}: {field} hash')
    require(struct.unpack_from('<H',(RESOURCE/data['bitmap']).read_bytes(),28)[0]==24,'Runtime must be 24-bit BMP')
    return data


def bounds(points):
    require(points,'Empty frame')
    result=[min(x for x,y in points),min(y for x,y in points),max(x for x,y in points)+1,max(y for x,y in points)+1]
    require(min(result[0],result[1],128-result[2],128-result[3])>=1,'Transparent gutter lost')
    return result


def main_sheet(stem, sid):
    data=load(stem)
    require(data['skillID']==sid and data['frameCount']==12 and data['cellSize']==128,'Wrong skill/layout')
    sw,sh,source=read_png(RESOURCE/data['source'])
    bw,bh,bitmap=read_bitmap(RESOURCE/data['bitmap'])
    pw,ph,preview=read_png(RESOURCE/data['preview'])
    require((bw,bh)==(pw,ph)==(512,384) and sw*3==sh*4,'Wrong sheet dimensions')
    scale=data['uniformScale']; reports=[]
    for frame in range(12):
        reg=data['registration']['frames'][frame]
        ax,ay=reg['sourceAnchor'];tx,ty=reg['targetAnchor']
        x0,y0=(frame%4)*128,(frame//4)*128
        left,top=(frame%4)*sw//4,(frame//4)*sh//3
        points=[];red=0
        for y in range(128):
            for x in range(128):
                sx=math.floor((x+.5-tx)/scale+ax+1e-9);sy=math.floor((y+.5-ty)/scale+ay+1e-9)
                rgba=source(left+sx,top+sy) if 0<=sx<sw//4 and 0<=sy<sh//3 else (0,0,0,0)
                expected=rgba[:3] if rgba[3]>=128 else KEY
                actual=bitmap(x0+x,y0+y);p=preview(x0+x,y0+y)
                require(actual==expected and p[3]==(255 if rgba[3]>=128 else 0),'Source sample/RGB/alpha mismatch')
                if rgba[3]>=128:
                    require(p[:3]==expected,'PNG RGB changed');points.append((x,y))
                    red+=int(rgba[0]>180 and rgba[1]<100 and rgba[2]<100)
        b=bounds(points)
        require(b==data['frames'][frame]['RuntimeBounds'] and data['frames'][frame]['SourceEdgePixels']==0,'Bounds or source edge mismatch')
        reports.append({'frame':frame,'bounds':b,'opaquePixels':len(points),'redPixels':red})
    return {'asset':stem,'kind':'main','skillID':sid,'frames':reports,'sourceSha256':data['sourceSha256']}


def flight_sheet(stem):
    data=load(stem+'_Flight')
    require(data['cellSize']==128 and data['columns']==16 and data['rows']==4,'Flight contract')
    require(digest(RESOURCE/data['sourceMainPreview'])==data['sourceMainPreviewSha256'],'Flight input hash')
    sw,sh,source=read_png(RESOURCE/data['sourceMainPreview'])
    bw,bh,bitmap=read_bitmap(RESOURCE/data['bitmap']);pw,ph,preview=read_png(RESOURCE/data['preview'])
    require((sw,sh)==(512,384) and (bw,bh)==(pw,ph)==(2048,512),'Flight sizes')
    scale=data['registrationGroupScale'];reports=[]
    ax,ay=data['registeredAnchor']
    require(abs(ax-(64+(data['sourceTip'][0]-64)*scale))<1e-8 and
            abs(ay-(64+(data['sourceTip'][1]-64)*scale))<1e-8,'Measured scaled tip')
    for direction in range(16):
        angle=direction*math.pi/8;c=round(math.cos(angle),12);s=round(math.sin(angle),12)
        expected_anchor=[round(64+c*(ax-64)-s*(ay-64),6),round(64+s*(ax-64)+c*(ay-64),6)]
        require(data['directionAnchors'][direction]==expected_anchor,'Direction tip anchor')
        for frame in range(4):
            points=[]
            for y in range(128):
                for x in range(128):
                    dx,dy=x+.5-64,y+.5-64
                    sx=math.floor((c*dx+s*dy)/scale+64+1e-9);sy=math.floor((-s*dx+c*dy)/scale+64+1e-9)
                    rgba=source(frame*128+sx,sy) if 0<=sx<128 and 0<=sy<128 else (0,0,0,0)
                    expected=rgba[:3] if rgba[3]>=128 else KEY
                    p=preview(direction*128+x,frame*128+y)
                    require(bitmap(direction*128+x,frame*128+y)==expected and p[3]==(255 if rgba[3]>=128 else 0),'Flight rotation/sample mismatch')
                    if rgba[3]>=128:
                        require(p[:3]==expected,'Flight PNG RGB changed');points.append((x,y))
            b=bounds(points);record=data['frames'][direction*4+frame]
            require(b==record['bounds'] and len(points)==record['opaquePixels'],'Flight bound metadata')
            tip_distance=min(math.hypot(x+.5-expected_anchor[0],y+.5-expected_anchor[1]) for x,y in points)
            require(tip_distance<=1.5,'Rotated measured tip has drifted away from visible arrow')
            reports.append({'direction':direction,'frame':frame,'bounds':b,'opaquePixels':len(points),
                            'tipNearestOpaqueCenterDistance':round(tip_distance,6)})
    return {'asset':stem+'_Flight','kind':'flight','frames':reports}


def main():
    parser=argparse.ArgumentParser();parser.add_argument('--partial',action='store_true');args=parser.parse_args()
    reports=[]
    for stem,sid in PROFILES:
        if not (RESOURCE/(stem+'.layout.json')).exists() and args.partial:continue
        reports.append(main_sheet(stem,sid))
        if '_Snipe_' in stem:
            reports.append(flight_sheet(stem))
        print('PASS:',stem)
    require(args.partial or len(reports)==14,'Expected ten main sheets and four derived flight atlases')
    (REPORT/'asset-verification.json').write_text(json.dumps(reports,indent=2)+'\n',encoding='utf-8')
    baseline=json.loads((REPORT/'preserved-baseline.json').read_text(encoding='utf-8-sig'))
    for original in baseline:
        require(digest(Path(original['path']))==original['sha256'],'Pre-existing art was modified: '+original['path'])
    (REPORT/'preservation-report.json').write_text(json.dumps({'unchangedFiles':len(baseline),'changedFiles':0},indent=2)+'\n',encoding='utf-8')
    print('Verified',len(reports),'assets and',sum(len(r['frames']) for r in reports),'frames.')
    print('Preserved',len(baseline),'pre-existing character, map, Mage and effect image files.')


if __name__=='__main__':main()
