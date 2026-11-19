"""Record generated originals, runtime artifacts and their measured contracts."""
import hashlib
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
RESOURCE = ROOT / 'Tower_Triumph/Tower_Triumph/Resource'


def item(name):
    path = RESOURCE / name
    return {'path': name, 'sha256': hashlib.sha256(path.read_bytes()).hexdigest(),
            'bytes': path.stat().st_size}


def main():
    entries = []
    for effect in ('Meteor', 'Sanctuary'):
        stem = f'Skill_Mage_{effect}_Ultimate'
        layout = json.loads((RESOURCE / (stem+'.layout.json')).read_text(encoding='utf-8-sig'))
        entries.append({
            'effect': effect, 'skillID': layout['skillID'],
            'generationMode': 'built-in image_gen',
            'files': {key:item(stem+suffix) for key,suffix in (
                ('original','.source.png'), ('prompt','.prompt.txt'),
                ('runtime','.bmp'), ('transparentPreview','.png'), ('layout','.layout.json'))},
            'sheet': {'columns':4,'rows':3,'cellSize':128,'frames':12,'size':[512,384]},
            'phases': layout['phases'],
            'groundAnchor': layout['groundAnchor'],
            'flightCoreAnchor': layout['flightCoreAnchor'],
            'groundBandStartY': layout['groundBandStartY'],
            'registration': 'Single uniform 128/362 nearest-neighbor scale; no per-frame translation or scale; original RGB preserved.',
            'alphaThreshold':128, 'runtimeColorKey':'#ff00ff',
        })
    manifest = {
        'schemaVersion':1, 'generationMode':'built-in image_gen',
        'description':'Two dedicated high-cost Mage presentations. Existing artwork and skill statistics are unchanged.',
        'pathBase':'this manifest directory',
        'assets':entries,
        'verification': {
            'pixelAudit':'../../../tests/verify_ultimate_effects.py',
            'pixelReport':'../../../tests/bin/ultimate-import/asset-verification.json',
            'groundClipReport':'../../../tests/bin/ultimate-import/ground-band-verification.json',
            'sourceFrames':24, 'emptyFrames':0, 'sourceBorderOpaquePixels':0,
            'runtimeSanctuaryRedPixels':0,
        },
    }
    target = RESOURCE/'Skill_Mage_Ultimates.manifest.json'
    target.write_text(json.dumps(manifest,indent=2,ensure_ascii=False)+'\n',encoding='utf-8')
    print(target)


if __name__=='__main__':
    main()
