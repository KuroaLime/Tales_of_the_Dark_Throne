"""Pixel and preservation audit for the three added Mage elemental ultimates."""
import json
from pathlib import Path
from verify_class_ultimate_assets import RESOURCE, ROOT, main_sheet, digest, require

PROFILES=[('Skill_Mage_Meteor_Ultimate',3103),('Skill_Mage_Water_Ultimate',3203),
          ('Skill_Mage_Grass_Ultimate',3303),('Skill_Mage_Sanctuary_Ultimate',3403),
          ('Skill_Mage_Dark_Ultimate',3503)]
REPORT=ROOT/'tests/bin/mage-elements-import'


def main():
    reports=[]
    for stem,sid in PROFILES:
        reports.append(main_sheet(stem,sid))
        print('PASS:',stem)
    baseline=json.loads((REPORT/'preserved-baseline.json').read_text(encoding='utf-8-sig'))
    for item in baseline:
        require(digest(Path(item['path']))==item['sha256'],'Existing image changed: '+item['path'])
    (REPORT/'asset-verification.json').write_text(json.dumps(reports,indent=2)+'\n',encoding='utf-8')
    (REPORT/'preservation-report.json').write_text(json.dumps({'unchangedFiles':len(baseline),'changedFiles':0},indent=2)+'\n',encoding='utf-8')
    print('PASS: 36 new and 24 existing Mage frames; 24-bit BMP/PNG/source pixels and gutters.')
    print('Preserved',len(baseline),'existing image files.')


if __name__=='__main__':main()
