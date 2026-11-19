"""Verify approved enemy sprite wiring and optional pre-change preservation.

Source assertions and file hashes are evidence of integration, not a substitute
for observing real gameplay, rendering performance or animation anatomy.
"""
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import re

from verify_graveyard import require, strip_comments

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / 'Tower_Triumph/Tower_Triumph'


def member(text: str, owner: str, name: str, clean: bool = True) -> str:
    if clean:
        text = strip_comments(text)
    match = re.search(r'\b' + owner + r'::' + name + r'\([^;{}]*\)\s*(?:const\s*)?\{', text)
    require(match is not None, f'Missing {owner}::{name}')
    start = match.start()
    cursor = match.end()
    depth = 1
    # The snapshotted bodies have no braces in their string literals.
    while depth:
        depth += (text[cursor] == '{') - (text[cursor] == '}')
        cursor += 1
    return text[start:cursor]


def integration() -> dict:
    char = (SOURCE / 'Character.cpp').read_text(encoding='utf-8-sig')
    header = (SOURCE / 'Character.h').read_text(encoding='utf-8-sig')
    battle = (SOURCE / 'Battle_Stage.cpp').read_text(encoding='utf-8-sig')
    portrait = (SOURCE / 'UTurnCharacterInfo.cpp').read_text(encoding='utf-8-sig')
    source = lambda name: member(char, 'Character', name)
    loader = source('LoadMonsterSheet')
    require('bool bUseAuthoredMonsterLayout = false;' in header, 'New sprite mode defaults to legacy/off')
    require('L"Approved\\\\" + stem' in loader and 'L"_Animated.bmp"' in loader and 'L"_Portrait.bmp"' in loader,
            'Live enemy loader must resolve the approved atlas/portrait pair by original identity')
    require(loader.count('EnemySpriteLayout::HasDimensions') == 2 and
            'portraitInfo.bmWidth == EnemySpriteLayout::PortraitSize' in loader and
            'portraitInfo.bmHeight == EnemySpriteLayout::PortraitSize' in loader,
            'Both atlas and portrait formats must be validated before enabling new mode')
    approved = loader.index('bUseAuthoredMonsterLayout = true;')
    legacy = loader.index('hSheet = LoadBitmapFromFile(basePath + fileName, &sheetInfo);')
    require(loader.index('portrait &&') < approved < legacy and
            'if (hSheet) DeleteObject(hSheet);' in loader and 'if (portrait) DeleteObject(portrait);' in loader,
            'A missing or malformed pair must clean up and use the original whole sprite set')
    require('bUseMonsterSheetLayout = true;' in loader and 'bSpriteFacesRight = true;' in loader and
            'hIllustration = portrait;' in loader and 'SpriteInfoIllustration = portraitInfo;' in loader and
            'IllustrationTransparentColor = CHARACTER_TRANSPARENT_COLOR;' in loader,
            'Enemy identity, facing and portrait transparency must travel with the selected pair')
    require('LoadMonsterSheet(folderPath, start.monsterSheetFile, start.monsterSourceFacesRight)' in battle,
            'Battle spawns must keep calling the upgraded real loader')
    require('if (isEnemy && owner->GetIllustration() == NULL)' in portrait and
            portrait.index('if (isEnemy && owner->GetIllustration() == NULL)') <
            portrait.index('GetEnemyTurnIconFileName(owner->GetName())'),
            'Legacy enemy turn icons must not hide a newly loaded approved portrait')
    require(char.count('bUseAuthoredMonsterLayout = other.bUseAuthoredMonsterLayout;') == 2 and
            char.count('other.bUseAuthoredMonsterLayout = false;') == 2 and
            'bUseAuthoredMonsterLayout = false;' in source('Release') and
            'bUseAuthoredMonsterLayout = false;' in source('Init'),
            'Mode ownership must transfer and reset alongside the image handles')
    require('hIllustration, hSpriteIdleCycle' in source('Release') and 'alreadyDeleted' in source('Release'),
            'A shared atlas must be deleted only once, together with its separate portrait')
    require('if (bUseAuthoredMonsterLayout) return hSpriteIdle;' in source('GetCurrentSprite'),
            'Every authored enemy state must use the matching shared atlas')
    maximum = source('UpdateMaxFrameByState')
    require(maximum.index('if (bUseAuthoredMonsterLayout)') < maximum.index('else if (bUseMonsterSheetLayout)') and
            'MaxFrame = EnemySpriteLayout::StateFrames;' in maximum,
            'Eight-pose rows must take precedence over the old six/three-pose rows')
    for name in ('PrepareSpriteLayouts', 'DrawAtFoot'):
        require('EnemySpriteLayout::WithAnchor' in source(name) and 'bUseAuthoredMonsterLayout' in source(name),
                name + ': new art must use its fixed root without changing hero anchors')
    draw = source('DrawAtFoot')
    require('const int authoredIndex = GetAuthoredMonsterFrameIndex();' in draw and
            'row = authoredIndex / columns;' in draw and
            'bUseAuthoredMonsterLayout ? EnemySpriteLayout::ReferenceFrameHeight' in draw,
            'New atlas rows and padded source scale must match the authored contract')
    frame = source('GetAuthoredMonsterFrameIndex')
    for pose in ('Walk', 'Attack', 'Hit', 'Dead'):
        require('EnemySpriteLayout::Pose::' + pose in frame, pose + ': missing enemy state row')
    require('EnemySpriteLayout::RecoveryFrameIndex(IdleRecovery)' in frame and
            'EnemySpriteLayout::FrameIndex(pose, CurFrame)' in frame,
            'Settling must select its own transition cells, not the idle frame number')
    update = source('Update')
    walking = source('AdvanceMovement')
    for body in (update, walking):
        require('(!bUseMonsterSheetLayout || bUseAuthoredMonsterLayout) && State == ECharacterState::Walk' in body,
                'Approved enemy gait must advance only from actual distance')
    require('CharacterMotion::AdvanceWalkCycle(WalkCycleDistance, step.distance, cycleLength)' in walking and
            'return remainingMovementSeconds;' in update and 'CurFrame = MaxFrame - 1;' in update,
            'Movement continuity and final action/death poses must remain intact')
    idle = source('UpdateIdleVisual')
    require('EnemySpriteLayout::AdvanceRecovery(IdleRecovery, elapsedSeconds)' in idle and
            'EnemySpriteLayout::IdleFrameSeconds' in idle and 'EnemySpriteLayout::StateFrames' in idle,
            'Idle must keep breathing after recovery without advancing the world position')
    begin = source('BeginIdleRecovery')
    require('EnemySpriteLayout::BeginWalkRecovery' in begin and 'EnemySpriteLayout::BeginActionRecovery' in begin and
            begin.index('if (bUseAuthoredMonsterLayout)') < begin.index('if (bUseMonsterSheetLayout) return;'),
            'Walking and combat returns must reach the new recovery path before legacy short-circuit')
    timing = source('GetCurrentActionImpactSeconds')
    require('MonsterImpactFrame * EnemySpriteLayout::AttackFrameSeconds' in timing and
            'MonsterImpactFrame = EnemySpriteLayout::AttackImpactFrameFor(stem.c_str());' in loader and
            timing.index('if (bUseAuthoredMonsterLayout)') < timing.index('if (bUseMonsterSheetLayout)'),
            'The new contact time must come from the loaded identity, including bow angel pose five')
    require('MonsterImpactFrame * GetAnimationFrameSeconds()' in timing and
            'BattleActionTiming::MonsterAttackFrame(fileName.c_str())' in loader,
            'Original fallback monsters retain their own contact profiles')
    projectile = source('GetCurrentProjectileKind')
    require('if (bUseMonsterSheetLayout ||' in projectile and 'ProjectileKind::None;' in projectile,
            'Enemy art replacement must not introduce projectile delays or a new attack type')
    return {'passed': True, 'enemyCount': 14, 'frameCount': 672,
            'scope': 'Production source wiring; no native renderer or game execution'}


def preservation(path: Path) -> dict:
    data = json.loads(path.read_text(encoding='utf-8-sig'))
    file_sets = (data['files'], data['originalEnemyFiles'])
    changed = [name for files in file_sets for name, expected in files.items()
               if not (ROOT / name).exists() or hashlib.sha256((ROOT / name).read_bytes()).hexdigest() != expected]
    changed_bodies = []
    for qualified, expected in data['functionBodies'].items():
        owner, name = qualified.split('::')
        text = (SOURCE / (owner + '.cpp')).read_text(encoding='utf-8-sig')
        if member(text, owner, name, clean=False) != expected:
            changed_bodies.append(qualified)
    require(not changed, 'Protected files changed: ' + ', '.join(changed))
    require(not changed_bodies, 'Battle/AI/damage behavior changed: ' + ', '.join(changed_bodies))
    return {'passed': True, 'protectedFiles': sum(len(files) for files in file_sets),
            'functionBodies': len(data['functionBodies']), 'changedFiles': changed, 'changedBodies': changed_bodies}


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--baseline', type=Path)
    parser.add_argument('--output', type=Path, default=ROOT / 'tests/bin/enemy-style-audit/integration-verification.json')
    args = parser.parse_args()
    result = integration()
    if args.baseline:
        result['preservation'] = preservation(args.baseline)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(result, indent=2) + '\n', encoding='utf-8')
    print('PASS: approved enemy loading, state rows, recovery, facing, anchors, portraits and direct-contact timing.')
    if args.baseline:
        print('PASS:', result['preservation']['protectedFiles'], 'protected files and',
              result['preservation']['functionBodies'], 'battle/AI/damage function bodies unchanged.')


if __name__ == '__main__':
    main()
