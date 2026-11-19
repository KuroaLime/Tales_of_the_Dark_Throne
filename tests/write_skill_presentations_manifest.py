"""One provenance index for all45 skill visuals; no source art is modified."""
import json
import re
from verify_standard_skill_assets import RESOURCE, ROOT, OUT, PROFILES, digest

GAME=RESOURCE.parent
def file_info(name):
    p=RESOURCE/name
    return {'path':name,'sha256':digest(p),'bytes':p.stat().st_size}

def main():
    skills=json.loads((OUT/'skills-before.json').read_text())
    source=(GAME/'StandardSkillPresentation.h').read_text(encoding='utf-8-sig')
    standard={int(i):{'class':c,'tier':t,'motion':m,'element':e} for i,c,t,m,e in re.findall(
        r'case (\d+):return \{id,Class::(\w+),Tier::(\w+),Motion::(\w+),Element::(\w+)\};',source)}
    assert len(standard)==30
    expected={s['skillID'] for s in skills if s['presentation']=='Normal'}
    assert set(standard)==expected
    art=[]
    for stem,ids,kind in PROFILES:
        data=json.loads((RESOURCE/(stem+'.layout.json')).read_text(encoding='utf-8-sig'))
        assert data['sharedSkillIDs']==list(ids)
        art.append({'asset':stem,'skillIDs':list(ids),'generationMode':'built-in image_gen',
            'files':{key:file_info(stem+suffix) for key,suffix in (
                ('original','.source.png'),('prompt','.prompt.txt'),('runtime','.bmp'),
                ('transparentPreview','.png'),('layout','.layout.json'))},
            'sheet':{'size':[512,384],'columns':4,'rows':3,'cellSize':128,'frames':12},
            'phases':data['phases'],'preparationAnchor':data['preparationAnchor'],
            'impactAnchor':data['impactAnchor'],'groundSplit':data['groundBandStartY']})
    signature={}
    for name in ('Class_Ultimates.manifest.json','Mage_Ultimates.manifest.json'):
        data=json.loads((RESOURCE/name).read_text(encoding='utf-8-sig'))
        for item in data['assets']:
            signature[item['skillID']]=item['files']['runtime']['path']
    assert len(signature)==15
    coverage=[]
    for skill in skills:
        sid=skill['skillID'];element=skill['element'];cls=skill['class']
        common={'skillID':sid,'name':skill['name'],'class':cls,'element':element,
            'mp':skill['mp'],'area':skill['area'],'range':skill['range'],
            'power':skill['power'],'defenseRate':skill['defenseRate']}
        if sid in signature:
            coverage.append({**common,'status':'previously completed signature preserved',
                'motion':skill['presentation'],'mainBitmap':signature[sid]})
            continue
        p=standard[sid]
        assert p['element']==element
        if p['class']=='Knight':
            assets=[f'Skill_Warrior_Standard_{element}.bmp',f'Element_Impact_{element}.bmp']
            source_mode='new shared element art, distinct quick/break phase and motion'
        else:
            assets=[f'{cls}_Projectile_{element}.bmp',f'Element_Impact_{element}.bmp']
            source_mode='existing pixel-art projectile/contact with skill-specific motion'
            if p['tier']=='Middle':
                assets.insert(0,f'Skill_Element_Mid_{element}.bmp')
                source_mode='new intermediate art plus existing projectile/contact and skill-specific motion'
        coverage.append({**common,'status':'updated ordinary/intermediate','profile':p,
            'artMode':source_mode,'runtimeBitmaps':assets})
    for row in coverage:
        for name in row.get('runtimeBitmaps',[row.get('mainBitmap')]):
            assert name and (RESOURCE/name).is_file(),name
    manifest={'schemaVersion':1,'pathBase':'this manifest directory',
        'scope':'All45 skills:15 existing highest-MP signatures preserved;30 ordinary/intermediate visual presentations completed.',
        'generationMode':'built-in image_gen for10 new source sheets; existing art reused where listed',
        'newSourceSheets':10,'newRuntimeBitmaps':10,'newAuthoredFrames':120,
        'coverageCounts':{'total':45,'preservedSignature':15,'newWarriorPhaseVariants':10,
            'newSharedIntermediate':10,'existingArtWithNewMotion':10},
        'conversion':'Source RGBA bytes and RGB are preserved. One uniform scale for12poses; registered phase anchors; nearest-neighbor sampling; alpha>=128 to24-bit magenta-key BMP.',
        'preservation':'Existing maps/characters/effects, SkillData and gameplay geometry/timing are compared against435 saved file hashes.',
        'assets':art,'skills':coverage,
        'previousAssetManifests':[file_info(name) for name in ('Class_Ultimates.manifest.json','Mage_Ultimates.manifest.json','Elemental_Effects.manifest.json')],
        'rejectedCandidate':{'reason':'First Warrior Fire retained an opaque beige halo after alpha thresholding; replaced by a newly generated source. No manual repainting or recoloring.',
            'preservedFiles':[{'path':'../../../'+str(p.relative_to(ROOT)).replace('\\','/'),'sha256':digest(p)} for p in sorted((OUT/'rejected').rglob('*')) if p.is_file()]},
        'verification':{'assets':'../../../tests/verify_standard_skill_assets.py',
            'report':'../../../tests/bin/all-skills-audit/asset-verification.json',
            'originalAudit':'../../../tests/bin/all-skills-audit/audit-summary.md',
            'coverage':'../../../tests/bin/all-skills-audit/coverage-after.json',
            'finalAudit':'../../../tests/bin/all-skills-audit/final-art-audit.json',
            'mapSizeProof':'../../../tests/bin/all-skills-audit/all-ten-map-size-proof.png'}}
    (RESOURCE/'Skill_Presentations.manifest.json').write_text(json.dumps(manifest,indent=2,ensure_ascii=False)+'\n',encoding='utf-8')
    (OUT/'coverage-after.json').write_text(json.dumps({'passed':True,**manifest['coverageCounts'],'skills':coverage},indent=2)+'\n',encoding='utf-8')
    print('Wrote unified45-skill provenance manifest and coverage report.')

if __name__=='__main__':main()
