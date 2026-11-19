"""Independent pixel/provenance checks for standard and intermediate skill sheets.

No game or compiled test process is launched. The established source-to-PNG/BMP
pixel sampler checks every normalized pixel and the accepted source's gutters.
"""
import argparse
import json
from pathlib import Path
from verify_class_ultimate_assets import RESOURCE, ROOT, main_sheet, digest
from verify_character_action_assets import require

OUT=ROOT/'tests/bin/all-skills-audit'
ELEMENTS=('Fire','Water','Grass','Light','Dark')
WARRIOR_IDS=((1101,1103),(1201,1203),(1301,1303),(1401,1402),(1501,1502))
MID_IDS=((2102,3102),(2202,3202),(2302,3302),(2402,3402),(2502,3502))
PROFILES=[(f'Skill_Warrior_Standard_{e}',ids,'Warrior') for e,ids in zip(ELEMENTS,WARRIOR_IDS)]+[
    (f'Skill_Element_Mid_{e}',ids,'Shared') for e,ids in zip(ELEMENTS,MID_IDS)]

def audit_assets(partial=False):
    reports=[]
    for stem,ids,kind in PROFILES:
        path=RESOURCE/(stem+'.layout.json')
        if partial and not path.exists():continue
        meta=json.loads(path.read_text(encoding='utf-8-sig'))
        report=main_sheet(stem,ids[0])
        require(meta['sharedSkillIDs']==list(ids),stem+' participating skills')
        phase_frames=[p['frames'] for p in meta['phases']]
        expected=[list(range(6)),list(range(6,12))] if kind=='Warrior' else [list(range(4)),list(range(4,12))]
        require(phase_frames==expected,stem+' phase boundaries')
        require(len(meta['preparationAnchor'])==len(meta['impactAnchor'])==2,stem+' explicit anchors')
        require((RESOURCE/(stem+'.prompt.txt')).stat().st_size>0,stem+' generation prompt')
        report['sharedSkillIDs']=list(ids)
        report['anchors']={'preparation':meta['preparationAnchor'],'impact':meta['impactAnchor'],'groundSplit':meta['groundBandStartY']}
        reports.append(report)
        print('PASS:',stem)
    require(partial or len(reports)==10,'Expected five Warrior and five intermediate sheets')
    baseline=json.loads((OUT/'preserved-baseline.json').read_text(encoding='utf-8-sig'))['files']
    changed=[path for path,h in baseline.items() if digest(ROOT/path)!=h]
    require(not changed,'Existing art or gameplay file changed: '+', '.join(changed))
    result={'assetCount':len(reports),'frameCount':sum(len(r['frames']) for r in reports),
            'unchangedExistingFiles':len(baseline),'changedExistingFiles':changed,'assets':reports}
    (OUT/'asset-verification.json').write_text(json.dumps(result,indent=2)+'\n',encoding='utf-8')
    print(f'Verified {len(reports)} new sheets; preserved {len(baseline)} existing image/gameplay files.')
    return result

if __name__=='__main__':
    parser=argparse.ArgumentParser();parser.add_argument('--partial',action='store_true')
    audit_assets(parser.parse_args().partial)
