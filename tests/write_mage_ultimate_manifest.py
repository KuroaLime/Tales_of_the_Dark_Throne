"""One provenance manifest for all five completed Mage elemental presentations."""
import json
from verify_mage_elemental_ultimates import PROFILES, RESOURCE, digest


def file_info(name):
    path=RESOURCE/name
    return {'path':name,'sha256':digest(path),'bytes':path.stat().st_size}


def main():
    entries=[]
    for stem,sid in PROFILES:
        layout=json.loads((RESOURCE/(stem+'.layout.json')).read_text(encoding='utf-8-sig'))
        entries.append({'skillID':sid,'element':{3103:'Fire',3203:'Water',3303:'Grass',3403:'Light',3503:'Dark'}[sid],
            'characterClass':'Mage','generationMode':'built-in image_gen',
            'files':{key:file_info(stem+suffix) for key,suffix in (
                ('original','.source.png'),('prompt','.prompt.txt'),('runtime','.bmp'),
                ('transparentPreview','.png'),('layout','.layout.json'))},
            'sheet':{'columns':4,'rows':3,'cellSize':128,'frames':12,'size':[512,384]},
            'phases':layout['phases'],'groundAnchor':layout['groundAnchor'],
            'groundBandStartY':layout['groundBandStartY'],
            'preparationAnchor':layout.get('preparationAnchor',layout.get('flightCoreAnchor') or layout['groundAnchor']),
            'impactAnchor':layout.get('impactAnchor',layout['groundAnchor']),
            'uniformSourceScale':layout['uniformScale'],
            'newInThisBatch':sid in (3203,3303,3503)})
    data={'schemaVersion':1,'generationMode':'built-in image_gen','pathBase':'this manifest directory',
        'sourceSheets':5,'runtimeBitmaps':5,'frames':60,'newSourceSheets':3,'newFrames':36,
        'description':'Complete highest-MP Mage elemental presentations; original gameplay statistics, character art, maps and earlier presentations preserved.',
        'conversion':'Uniform nearest-neighbor scaling and explicitly recorded anchor registration; original RGB unchanged; alpha>=128; 24-bit BMP magenta transparency key.',
        'assets':entries,'verification':{'script':'../../../tests/verify_mage_elemental_ultimates.py',
            'pixelReport':'../../../tests/bin/mage-elements-import/asset-verification.json',
            'preservationReport':'../../../tests/bin/mage-elements-import/preservation-report.json',
            'groundBandReport':'../../../tests/bin/mage-elements-import/ground-band-verification.json',
            'visualAudit':'../../../tests/bin/mage-elements-import/audit-report.json',
            'newThreeMapProof':'../../../tests/bin/mage-elements-import/all-three-map-size-proof.png',
            'allFiveMageMapProof':'../../../tests/bin/mage-elements-import/all-five-mage-map-size-proof.png'}}
    path=RESOURCE/'Mage_Ultimates.manifest.json'
    path.write_text(json.dumps(data,indent=2,ensure_ascii=False)+'\n',encoding='utf-8')
    print(path)


if __name__=='__main__':main()
