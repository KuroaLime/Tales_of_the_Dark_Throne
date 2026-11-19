"""Consolidated provenance for ten generated sheets and four rotated derivatives."""
import json
from verify_class_ultimate_assets import RESOURCE, REPORT, PROFILES, digest


def file_info(name):
    path=RESOURCE/name
    return {'path':name,'sha256':digest(path),'bytes':path.stat().st_size}


def main():
    entries=[]
    for stem,sid in PROFILES:
        layout=json.loads((RESOURCE/(stem+'.layout.json')).read_text(encoding='utf-8-sig'))
        entry={'skillID':sid,'characterClass':layout['characterClass'],'element':layout['element'],
               'generationMode':'built-in image_gen',
               'files':{key:file_info(stem+suffix) for key,suffix in (
                   ('original','.source.png'),('prompt','.prompt.txt'),('runtime','.bmp'),
                   ('transparentPreview','.png'),('layout','.layout.json'))},
               'sheet':{'columns':4,'rows':3,'cellSize':128,'frames':12,'size':[512,384]},
               'phases':layout['phases'],'preparationAnchor':layout['preparationAnchor'],
               'impactAnchor':layout['impactAnchor'],'groundAnchor':layout['groundAnchor'],
               'groundBandStartY':layout['groundBandStartY'],
               'displaySize':{'preparation':layout['recommendedPreparationDrawSize'],
                              'impact':layout['recommendedImpactDrawSize']}}
        if '_Snipe_' in stem:
            name=stem+'_Flight';flight=json.loads((RESOURCE/(name+'.layout.json')).read_text(encoding='utf-8-sig'))
            entry['flight']={'generationMode':'mechanical derivative, no new artwork',
                'files':{key:file_info(name+suffix) for key,suffix in (
                    ('runtime','.bmp'),('transparentPreview','.png'),('layout','.layout.json'))},
                'sheet':{'columns':16,'rows':4,'cellSize':128,'frames':4,'size':[2048,512]},
                'rotationCenter':[64,64],'clockwiseDegreesPerColumn':22.5,'registeredAnchor':flight['registeredAnchor'],
                'directionAnchors':flight['directionAnchors'],'groupScale':flight['registrationGroupScale']}
        entries.append(entry)
    manifest={'schemaVersion':1,'generationMode':'built-in image_gen','pathBase':'this manifest directory',
              'sourceSheets':10,'runtimeBitmaps':14,'mainFrames':120,'rotatedFlightCells':256,
              'preservation':'Existing Mage effects, characters, maps and gameplay statistics remain unchanged.',
              'conversion':'Original RGB preserved; uniform nearest-neighbor normalization; alpha>=128 opaque; 24-bit BMP magenta key.',
              'assets':entries,'verification':{'script':'../../../tests/verify_class_ultimate_assets.py',
                    'report':'../../../tests/bin/class-ultimate-import/asset-verification.json',
                    'mapProof':'../../../tests/bin/class-ultimate-import/all-ten-map-size-proof.png'}}
    path=RESOURCE/'Class_Ultimates.manifest.json'
    path.write_text(json.dumps(manifest,indent=2,ensure_ascii=False)+'\n',encoding='utf-8')
    print(path)


if __name__=='__main__':main()
