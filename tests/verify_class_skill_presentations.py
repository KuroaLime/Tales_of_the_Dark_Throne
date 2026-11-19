#!/usr/bin/env python3
"""Validate production profile anchors against authored metadata and unchanged skill balance."""
from pathlib import Path
import hashlib
import json
import re
import struct

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / 'Tower_Triumph/Tower_Triumph'
RESOURCE = SOURCE / 'Resource'
header = (SOURCE / 'SkillPresentation.h').read_text(encoding='utf-8-sig')
skills = (SOURCE / 'SkillData.cpp').read_text(encoding='utf-8-sig')
profiles = {
    'Meteor': ('Skill_Mage_Meteor_Ultimate', 3103, 'Splash2', 4, 16),
    'Sanctuary': ('Skill_Mage_Sanctuary_Ultimate', 3403, 'Splash2', 4, 16),
    'KnightFire': ('Skill_Warrior_Fire_Ultimate', 1102, 'Splash1', 1, 7),
    'KnightWater': ('Skill_Warrior_Water_Ultimate', 1202, 'Line', 3, 7),
    'KnightGrass': ('Skill_Warrior_Grass_Ultimate', 1302, 'Splash1', 1, 7),
    'KnightLight': ('Skill_Warrior_Light_Ultimate', 1403, 'Splash1', 1, 8),
    'KnightDark': ('Skill_Warrior_Dark_Ultimate', 1503, 'Splash1', 1, 8),
    'SnipeFire': ('Skill_Archer_Fire_Snipe_Ultimate', 2103, 'Single', 5, 9),
    'SnipeWater': ('Skill_Archer_Water_Snipe_Ultimate', 2203, 'Single', 5, 9),
    'SnipeGrass': ('Skill_Archer_Grass_Snipe_Ultimate', 2303, 'Single', 5, 9),
    'SnipeDark': ('Skill_Archer_Dark_Snipe_Ultimate', 2503, 'Single', 5, 10),
    'LightRain': ('Skill_Archer_Light_Rain_Ultimate', 2403, 'Splash2', 4, 12),
    'TidalBlast': ('Skill_Mage_Water_Ultimate', 3203, 'Splash1', 3, 13),
    'RootStorm': ('Skill_Mage_Grass_Ultimate', 3303, 'Splash2', 4, 16),
    'AbyssBlast': ('Skill_Mage_Dark_Ultimate', 3503, 'Splash2', 4, 16),
}
anchors = {kind: [float(n.strip().rstrip('f')) for n in values.split(',')]
           for kind, values in re.findall(r'if\(kind==Kind::(\w+)\)return \{([^}]+)\};', header)}

def metadata(base, dimensions):
    data = json.loads((RESOURCE / (base + '.layout.json')).read_text(encoding='utf-8-sig'))
    payload = (RESOURCE / data['bitmap']).read_bytes()
    assert payload[:2] == b'BM' and struct.unpack_from('<ii', payload, 18) == dimensions, base
    assert hashlib.sha256(payload).hexdigest() == data['bitmapSha256'], base + ' bitmap hash'
    assert hashlib.sha256((RESOURCE / data['preview']).read_bytes()).hexdigest() == data['previewSha256'], base + ' preview hash'
    assert data['transparencyKey'] == '#ff00ff', base + ' key'
    return data

def close(actual, expected):
    return len(actual) == len(expected) and all(abs(a - b) < 0.0001 for a, b in zip(actual, expected))

report = []
for kind, (base, skill_id, area, distance, mp) in profiles.items():
    meta = metadata(base, (512, 384))
    values = anchors[kind]
    assert close(values[:2], meta.get('groundAnchor') or meta['impactAnchor']), kind + ' impact anchor'
    assert values[2] == meta['groundBandStartY'], kind + ' ground split'
    if kind.startswith('Knight'):
        assert close(values[5:7], meta['preparationAnchor']), kind + ' weapon charge anchor'
    elif kind.startswith('Snipe'):
        flight = metadata(base + '_Flight', (2048, 512))
        assert close(values[3:5], flight['registeredAnchor']), kind + ' actual directional tip'
        assert flight['rotationCenter'] == [64, 64] and len(flight['directionAnchors']) == 16, kind
    elif kind == 'LightRain':
        assert close(values[3:5], meta['preparationAnchor']), 'Rain leading arrow plane'
    elif kind == 'Meteor':
        assert close(values[3:5], meta['flightCoreAnchor']), 'Meteor core'
    elif kind in ('TidalBlast', 'RootStorm', 'AbyssBlast'):
        assert close(values[:2], meta['preparationAnchor']), kind + ' fixed ground preparation'
    match = re.search(r'\{\s*' + str(skill_id) + r',[^\n]+?ESkillAreaType::(\w+),\s*ESkillStatType::\w+,\s*(\d+),\s*(\d+),', skills)
    assert match and (match[1], int(match[2]), int(match[3])) == (area, distance, mp), kind + ' skill balance'
    report.append({'kind': kind, 'skillID': skill_id, 'area': area, 'range': distance, 'mp': mp,
                   'bitmap': meta['bitmap'], 'bitmapSha256': meta['bitmapSha256'], 'anchors': values})
out = ROOT / 'tests/bin/class-skill-profile-validation.json'
out.write_text(json.dumps({'passed': True, 'profiles': report, 'mainAtlases': len(profiles), 'flightAtlases': 4}, indent=2), encoding='utf-8')
print(f'PASS: all {len(profiles)} main / 4 directional atlases match runtime dimensions, hashes and measured anchors; MP/range/area preserved.')
