"""Read production data and check the standard/ signature dispatch boundary without launching game code."""
from pathlib import Path
import json
import re
import argparse
from verify_character_rendering import body_of, require

ROOT=Path(__file__).resolve().parents[1]
SOURCE=ROOT/'Tower_Triumph/Tower_Triumph'
standard=(SOURCE/'StandardSkillPresentation.h').read_text(encoding='utf-8-sig')
signature=(SOURCE/'SkillPresentation.h').read_text(encoding='utf-8-sig')
battle=(SOURCE/'Battle_Stage.cpp').read_text(encoding='utf-8-sig')
renderer=(SOURCE/'StandardSkillRenderer.h').read_text(encoding='utf-8-sig')
data=(SOURCE/'SkillData.cpp').read_text(encoding='utf-8-sig')
profiles={int(i):(owner,tier,motion,element) for i,owner,tier,motion,element in re.findall(
    r'case (\d+):return \{id,Class::(\w+),Tier::(\w+),Motion::(\w+),Element::(\w+)\};',standard)}
signatures={int(i) for i in re.findall(r'case (\d+):return Kind::',signature)}
skills={int(i):(owner,element,area,int(distance),int(mp),int(power),int(defense))
    for i,owner,element,area,distance,mp,power,defense in re.findall(
        r'v.push_back\(\{ (\d+),.*?ECharacterClass::(\w+), EElement::(\w+), ESkillAreaType::(\w+), '
        r'ESkillStatType::\w+, (\d+), (\d+), (\d+), (\d+),',data)}
require(len(profiles)==30 and len(signatures)==15 and not(profiles.keys()&signatures) and
        set(skills)==profiles.keys()|signatures,'Every actual gameplay skill must resolve exactly once')
for skill_id,(owner,tier,motion,element) in profiles.items():
    actual=skills[skill_id]
    require(actual[0]==('Warrior' if owner=='Knight' else owner) and actual[1]==element,
            f'{skill_id}: class/element metadata mismatch')
    if motion in ('PiercingArrow','ForwardWave'):require(actual[2]=='Line',f'{skill_id}: line shape mismatch')
    if motion in ('FanVolley','BurstArrow','FlameBurst','ThornField'):require(actual[2]=='Splash1',f'{skill_id}: splash shape mismatch')
    if motion in ('Break','Smash','Smite','ShadowSpike','RadiantArrow'):require(actual[2]=='Single',f'{skill_id}: single action must not become an area')

begin=body_of(battle,'BeginPendingAction')
start=body_of(battle,'StartPendingStandardPresentation')
commit=body_of(battle,'CommitPendingImpact')
draw=body_of(battle,'Draw')
require('PendingAction.standard.element = PendingAction.element;' in begin and
        'StandardSkillPresentation::ProfileFor(skillID)' in begin,
        'The skill ID and authoritative element must be captured at command time')
require('PendingAction.standardStarted' in start and 'effect.cells = PendingAction.areaCells;' in start and
        'effect.linePath = PendingAction.linePath;' in start and 'effect.release = PendingAction.releaseSeconds;' in start and
        'effect.arrival = effect.release + effect.flight;' in start,
        'Standard visuals must capture the existing geometry/timing once, without recomputing damage scheduling')
require('!StandardSkillPresentation::Valid(PendingAction.standard)' in commit and
        'if (StandardSkillPresentation::Valid(PendingAction.standard)) return;' in body_of(battle,'DrawPendingProjectile'),
        'Standard skills must suppress old single FX and duplicate generic projectiles')
require('ActiveStandardEffects.clear();' in body_of(battle,'ClearPendingAction') and
        'ActiveStandardEffects.clear();' in body_of(battle,'ReleaseSkillEffectBitmaps') and
        'effect.id == PendingAction.presentationID' in body_of(battle,'CancelPendingPresentation') and
        'StandardSkillPresentation::EndSeconds(effect)' in body_of(battle,'AdvanceBattleVisuals'),
        'Independent effects must finish after center impact and clear on cancel/reset/release')
require(draw.index('DrawStandardSkillEffects(hMemDC, hMemDC1);') < draw.index('DrawUnit(') <
        draw.index('DrawStandardSkillEffects(hMemDC, hMemDC1, true);') < draw.index('DrawSkillEffects(') < draw.index('DrawDamageTexts('),
        'Field effects, airborne objects, contacts and numbers must retain their layer order')
require('Model::Fan(action.profile)' in renderer and 'cell.arrivalSeconds,false,options' in renderer and
        'action.area==BattleSkillArea::Kind::Line' in renderer and 'action.linePath' in standard and
        'if(!BattleSkillArea::IsDue(action.elapsed,reachedCell.arrivalSeconds))continue;' in renderer,
        'Fan arrows and line projectiles must use exact cell clocks and ground pixels only in reached cells')
require('const bool standard = StandardSkillPresentation::IsBitmap(skillID);' in body_of(battle,'LoadSkillEffectBitmap') and
        'StandardSkillPresentation::Rows * StandardSkillPresentation::CellSize' in body_of(battle,'LoadSkillEffectBitmap') and
        'if(!options.bitmap)' in renderer,
        'Missing or invalid new art must retain safe contact fallback, not expose old numbered atlases')
report={'passed':True,'standardCount':len(profiles),'signatureCount':len(signatures),
    'profiles':[{'skillID':i,'class':p[0],'tier':p[1],'motion':p[2],'element':p[3],
                 'area':skills[i][2],'range':skills[i][3],'mp':skills[i][4],
                 'powerPercent':skills[i][5],'defensePercent':skills[i][6]} for i,p in profiles.items()]}
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--output',type=Path,default=ROOT/'tests/bin/standard-skills-integration.json')
output=parser.parse_args().output
output.parent.mkdir(parents=True,exist_ok=True)
output.write_text(json.dumps(report,indent=2),encoding='utf-8')
print('PASS: 45/45 actual skills covered once; 30 standard motions, captured arrival/element, cancellation, fallback and render ordering verified.')
