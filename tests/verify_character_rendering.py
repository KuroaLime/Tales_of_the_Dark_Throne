#!/usr/bin/env python3
"""Check character source integration and optional map/art preservation.

These are structural source and file-integrity checks, not game execution,
GDI rendering, or measurement of displayed FPS. Compile-time arithmetic and
geometry tests are run separately by run_character_checks.ps1.
"""

from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import re
import sys

from verify_graveyard import SOURCE, body_of, require, strip_comments


def check_integration() -> None:
    character = (SOURCE / 'Character.cpp').read_text(encoding='utf-8-sig')
    battle = (SOURCE / 'Battle_Stage.cpp').read_text(encoding='utf-8-sig')
    header = strip_comments((SOURCE / 'Character.h').read_text(encoding='utf-8-sig'))
    movement = body_of(character, 'AdvanceMovement')
    state = body_of(character, 'SetState')
    update = body_of(character, 'Update')
    frame_timing = body_of(character.replace('GetAnimationFrameSeconds() const', 'GetAnimationFrameSeconds()'), 'GetAnimationFrameSeconds')
    stage_update = body_of(battle, 'Update')
    stage_visuals = body_of(battle, 'AdvanceBattleVisuals')
    pending_advance = body_of(battle, 'AdvancePendingAction')
    draw = body_of(character, 'DrawAtFoot')
    draw_unit = body_of(battle, 'DrawUnit')
    preparation = body_of(character, 'PrepareSpriteLayouts')

    require(re.search(r'if\s*\(\s*!KeepWalkingOnArrival\s*\)\s*SetState\(ECharacterState::Idle\)', movement),
            'Intermediate tile arrivals must retain the walking state')
    require('return step.remainingSeconds;' in movement,
            'Movement must preserve unused time at a tile boundary')
    require(state.index('if (State == newState && !restartAnimation)') < state.index('CurFrame = 0'),
            'Repeated Walk requests must not restart the animation')
    require('keepWalkingOnArrival = false' in header,
            'Independent single-step movement must still settle to Idle by default')
    require('AnimationElapsedSeconds += elapsedSeconds;' in update and
            'CharacterMotion::AdvanceAnimation' in update,
            'Non-walking actions must accumulate real elapsed time instead of counting updates')
    require('CharacterMotion::AdvanceWalkCycle(WalkCycleDistance, step.distance, cycleLength)' in movement and
            'CharacterMotion::WalkFrame(WalkCycleDistance, MaxFrame, cycleLength)' in movement and
            'const float cycleLength = GetWalkCycleLength();' in movement,
            'Walking must advance from actual travel, including movement after a tile boundary')
    gait_length = body_of(character.replace('GetWalkCycleLength() const', 'GetWalkCycleLength()'), 'GetWalkCycleLength')
    for class_name in ('Warrior', 'Archer', 'Mage'):
        require(f'case ECharacterClass::{class_name}: return CharacterMotion::{class_name}WalkCycleDistancePixels;' in gait_length,
                f'{class_name} must use its own measured stance travel')
    walk_clock_guard = 'if ((!bUseMonsterSheetLayout || bUseAuthoredMonsterLayout) && State == ECharacterState::Walk) return remainingMovementSeconds;'
    require(walk_clock_guard in update and update.index(walk_clock_guard) < update.index('AnimationElapsedSeconds += elapsedSeconds;'),
            'Player walking must not also advance from the action animation clock')
    require('WalkCycleDistance = 0.0f;' in state and state.index('if (State == newState && !restartAnimation)') < state.index('WalkCycleDistance = 0.0f;'),
            'Consecutive path steps must preserve distance-based gait phase')
    require('L"_WalkCycle.bmp"' in body_of(character, 'LoadSprites') and 'L"_Walk.bmp"' in body_of(character, 'LoadSprites'),
            'New walk sheets must have a compatible original-art fallback')
    load_sprites = body_of(character, 'LoadSprites')
    load_action = body_of(character, 'LoadActionBitmap')
    for action_name in ('Attack', 'Defend', 'Skill', 'Hit', 'Dead'):
        require(f'LoadActionBitmap(basePath, prefix, L"{action_name}"' in load_sprites,
                f'{action_name} must load its matching approved-design action sheet')
        require(f'CharacterMotion::ActionTiming::{action_name}Seconds' in frame_timing,
                f'{action_name} must preserve its pre-replacement duration')
    require('L"Action.bmp"' in load_action and 'L".bmp"' in load_action and
            'CharacterSpriteLayout::Action::HasDimensions' in load_action,
            'Invalid or missing authored actions must retain the original-art fallback')
    require('L"_Portrait.bmp"' in load_sprites and 'L"_Illustration.bmp"' in load_sprites and
            load_sprites.index('L"_Portrait.bmp"') < load_sprites.index('L"_Illustration.bmp"'),
            'Approved portraits must take precedence while preserving fallback compatibility')
    portrait_key_assignment = 'IllustrationTransparentColor = hIllustration ? CHARACTER_TRANSPARENT_COLOR : RGB(255, 255, 255);'
    require(portrait_key_assignment in load_sprites and
            load_sprites.index('L"_Portrait.bmp"') < load_sprites.index(portrait_key_assignment) < load_sprites.index('L"_Illustration.bmp"'),
            'Authored portraits must use magenta while original illustration fallbacks retain their white key')
    require(character.count('IllustrationTransparentColor = other.IllustrationTransparentColor;') == 2 and
            character.count('other.IllustrationTransparentColor = RGB(255, 255, 255);') == 2 and
            'IllustrationTransparentColor = RGB(255, 255, 255);' in body_of(character, 'Release'),
            'Portrait transparency metadata must move and reset together with the portrait bitmap')
    require('CharacterSpriteLayout::WithActionAnchor' in preparation and
            'CharacterSpriteLayout::WithActionAnchor' in draw and
            'CharacterSpriteLayout::Action::ReferenceFrameHeight' in draw,
            'Action sprites must keep a fixed root and the idle/walk body scale despite larger cells')
    require('CharacterSpriteLayout::Action::FrameCount' in body_of(character, 'UpdateMaxFrameByState') and
            'CharacterMotion::ActionTiming::FrameSeconds(MaxFrame, durationSeconds)' in frame_timing and
            'CurFrame = MaxFrame - 1;' in update and 'IsActionEnd = true;' in update,
            'Authored actions must complete once and retain their final pose')
    require('L"_IdlePose.bmp"' in load_sprites and 'L"_Idle.bmp"' in load_sprites and
            'CharacterSpriteLayout::IdlePose::HasDimensions' in load_sprites,
            'The approved static idle must validate its single-cell format and retain original-art fallback')
    require('CharacterSpriteLayout::WithIdlePoseAnchor' in preparation and
            'CharacterSpriteLayout::WithIdlePoseAnchor' in draw and
            'bUseAuthoredIdleLayout && sprite == hSpriteIdle' in draw,
            'Static idle must share the authored body root, including missing-action sprite fallback')
    require('CharacterSpriteLayout::IdlePose::FrameCount' in body_of(character, 'UpdateMaxFrameByState') and
            'bUseAuthoredIdleLayout || !IsCharacterSixByThreeSheet' in update,
            'Missing animated idle must still fall back to the approved single pose')
    idle_visual = body_of(character, 'UpdateIdleVisual')
    idle_begin = body_of(character, 'BeginIdleRecovery')
    idle_loader = body_of(character, 'LoadIdleVisualBitmap')
    for suffix in ('IdleCycle', 'WalkStop', 'IdleReturn'):
        require(f'L"_{suffix}.bmp"' in load_sprites,
                f'{suffix} must be loaded without overwriting the approved idle/walk/action sources')
    require('CharacterSpriteLayout::IdleCycle::HasDimensions' in idle_loader and
            'CharacterSpriteLayout::IdleTransition::HasDimensions' in idle_loader and
            'DeleteObject(sheet)' in idle_loader and 'return nullptr;' in idle_loader,
            'Malformed optional idle assets must fall back safely instead of choosing an invalid grid')
    require('BeginIdleRecovery(State)' in state and
            state.index('BeginIdleRecovery(State)') < state.index('State = newState;') and
            'else IdleRecovery = {};' in state and
            state.index('if (State == newState && !restartAnimation)') < state.index('BeginIdleRecovery(State)'),
            'Recovery must capture the outgoing pose, preserve repeated Idle requests, and be interrupted by new commands')
    require('CharacterMotion::BeginWalkRecovery' in idle_begin and
            'WalkCycleDistance - CurFrame * frameDistance' in idle_begin and
            'CharacterMotion::BeginActionRecovery' in idle_begin,
            'Recovery must keep the actual arrival gait phase and the outgoing action pose')
    require('UpdateIdleVisual(wasMoving ? remainingMovementSeconds : elapsedSeconds)' in update,
            'The arrival update must not consume the movement interval twice when starting recovery')
    require('CharacterMotion::AdvanceIdleRecovery' in idle_visual and
            'CharacterMotion::IdleTiming::CycleFrameSeconds' in idle_visual and
            'EnemySpriteLayout::StateFrames : CharacterSpriteLayout::IdleCycle::FrameCount)' in idle_visual and
            'WorldPixel' not in idle_visual and 'IsActionEnd' not in idle_visual,
            'Visual recovery and breathing must leave world position and gameplay completion unchanged')
    require('recovering ? IdleRecovery.frame' in draw and '!recovering && MaxFrame' in draw,
            'Recovery rendering must use its own absolute frame even when the logical idle fallback has one pose')
    for field in ('hSpriteIdleCycle', 'hSpriteWalkStop', 'hSpriteIdleReturn'):
        require(character.count(f'{field} = other.{field};') == 2 and
                character.count(f'other.{field} = nullptr;') == 2 and field in body_of(character, 'Release'),
                f'{field} ownership must transfer once and be released with the original sprite handles')
    for panel_name in ('UProfilePanel.cpp', 'UTurnCharacterInfo.cpp'):
        panel = (SOURCE / panel_name).read_text(encoding='utf-8-sig')
        require('CharacterSpriteLayout::IdlePose::HasDimensions' in panel,
                f'{panel_name} must not split a one-cell idle portrait fallback')
        require('GetIllustrationTransparentColor()' in panel,
                f'{panel_name} must use the loaded portrait color key instead of cutting out white clothing')
    require('CharacterSpriteLayout::WithWalkCycleAnchor' in preparation and 'authoredWalk' in draw,
            'Authored gait sheets must preserve their common root rather than recentering on each pose')
    require('std::chrono::steady_clock::now()' in stage_update and
            'c->Update(elapsedSeconds)' in stage_visuals and 'AdvancePendingAction' in stage_update,
            'Both teams must receive measured elapsed time')
    require('MovingUnit->AdvanceMovement(remainingMovementSeconds)' in stage_update,
            'The battle path must consume leftover movement time on the following tile')
    require('PrepareSpriteLayouts();' in body_of(character, 'LoadSprites') and
            'PrepareSpriteLayouts();' in body_of(character, 'LoadMonsterSheet'),
            'Sprite crop analysis must be cached during loading')
    require('UpdateMaxFrameByState();' in body_of(character, 'LoadSprites'),
            'Initial player idle must use its actual frame count')
    require('GetDIBits' in preparation and 'CharacterSpriteLayout::Analyze' in preparation,
            'Crop layout must inspect the actual loaded color-key bitmap')
    require('CharacterSpriteLayout::PlaceAtFoot' in draw and 'SpriteLayouts.find(sprite)' in draw,
            'Drawing must use cached source bounds and one aspect-preserving placement operation')
    require('referenceFrameHeight' in draw_unit and 'unit->DrawAtFoot' in draw_unit,
            'Battle units must be positioned from their ground anchor')
    require(not re.search(r'drawW\s*=\s*TILE_SIZE\s*\*\s*5\s*/\s*6', draw_unit),
            'The old skinny 40x64 stretch must not return')
    require('PrepareSpriteLayouts' not in draw and 'GetDIBits' not in draw,
            'Per-frame drawing must not rescan every source bitmap')
    commit = body_of(battle, 'CommitPendingImpact')
    commit_hit = body_of(battle, 'CommitPendingHit')
    commit_cells = body_of(battle, 'CommitPendingAreaCells')
    schedule = body_of(battle, 'BeginPendingAction')
    resolution = body_of(battle, 'UpdateActionResolution')
    skill = body_of(battle, 'UseSelectedBattleSkill')
    for name in ('Input', 'ProcessEnemyTurn', 'UseSelectedBattleSkill'):
        command = body_of(battle, name)
        require('BeginPendingAction(' in command and '->GetDamage(' not in command,
                f'{name} must schedule damage instead of applying it on command entry')
    require('PlaySkillEffect(' not in skill and
            skill.index('userStat.CurMP -= info->MPCost;') < skill.index('BeginPendingAction('),
            'A confirmed cast spends MP once while its visual impact waits for contact')
    require('actor->SetState(state, true);' in schedule and
            'restartAnimation = false' in header and
            'actor->GetCurrentActionImpactSeconds()' in schedule and 'actor->GetActionSequence()' in schedule,
            'Every new command must restart its pose clock and reserve impact against that fresh state generation')
    require('BattleSkillArea::AdvanceTimeline' in pending_advance and
            pending_advance.index('AdvanceBattleVisuals(step.beforeEvent)') < pending_advance.index('CommitPendingImpact();') <
            pending_advance.index('remaining = step.afterEvent;') and 'ActionEvent::Impact' in pending_advance,
            'Crossing updates must split release/arrival and apply damage only at arrival, then consume the remainder')
    require('actor->GetActionSequence() != PendingAction.sequence' in commit and
            '!ContainsBattleUnit(actor)' in commit and '!actor->IsAlive()' in commit and
            '!ContainsBattleUnit(hit.target)' in commit_hit,
            'Pending impact must guard interrupted windups and removed targets')
    require('!PendingAction.timeline.launched' in commit and '!PendingAction.timeline.launched' in pending_advance and
            'if (!actorPresent && !PendingAction.timeline.launched && !PendingAction.timeline.impactCommitted)' in resolution,
            'An already released projectile must survive actor Idle, sequence changes, death or removal')
    require('!PendingAction.timeline.impactCommitted' in resolution and 'PendingAction.actorFinished' in resolution,
            'Actor animation completion cannot unlock a still-flying projectile')
    projectile_kind = body_of(character.replace('GetCurrentProjectileKind() const', 'GetCurrentProjectileKind()'), 'GetCurrentProjectileKind')
    require('bUseMonsterSheetLayout' in projectile_kind and 'ECharacterClass::Archer' in projectile_kind and
            'ECharacterClass::Mage' in projectile_kind and 'ProjectileKind::None' in projectile_kind,
            'Only archer and mage player sheets may introduce projectile flight')
    projectile_draw = body_of(battle, 'DrawPendingProjectile')
    require('GetCurrentProjectileKind()' in schedule and 'BattleActionTiming::FlightSeconds' in schedule and
            'actor->GetDirection() == EDirection::Left' in schedule and 'sqrtf(dx * dx + dy * dy)' in schedule,
            'Flight must start at the facing-dependent weapon and derive duration from actual world distance')
    require('ArrowTipOffsetX' in projectile_draw and 'MagicCoreOffsetX' in projectile_draw and
            'DirectionFromRadians(atan2f(dy, dx))' in projectile_draw and
            'PendingAction.startX + dx * progress' in projectile_draw,
            'The continuous path must reach the target with the rotated tip/core, not the atlas center')
    require('ContainsBattleUnit(ActingUnit)' in stage_update and 'ContainsBattleUnit(focusTarget)' in stage_update,
            'A released projectile cannot leave a removed actor dangling in camera focus')
    require('ClearPendingAction();\n            ActiveAreaEffects.clear();\n            return MAP_STAGE;' in body_of(battle, 'Input'),
            'Leaving battle with ESC must discard both queued and already flying projectiles')
    require('hpBefore - hit.target->GetHP()' in commit_hit and 'PlaySkillEffect(' in commit_hit,
            'Damage numbers must reflect actual HP loss and begin with the hit effect')
    require('GetDamage(hit.damage, PendingAction.skillID < 0)' in commit_hit and
            commit.count('SoundManager::PlaySFX') == 1 and commit_cells.count('SoundManager::PlaySFX') == 1 and
            commit.index('if (!anyTarget) return;') < commit.index('SoundManager::PlaySFX') < commit.index('CommitPendingHit(hit);') and
            commit_cells.index('if (!PendingAction.fieldStarted)') < commit_cells.index('SoundManager::PlaySFX') < commit_cells.index('CommitPendingHit(hit);') and
            'unit->GetDamage(damage);' in body_of(battle, 'ApplyTileEffect'),
            'Skill impact owns its sound once; target Hit/Dead sounds must not replace it, while normal hits stay audible')
    require('PendingAction.skillID < 0' not in commit and 'CommitPendingHit(hit)' in commit and
            commit_hit.index('PlaySkillEffect(BattleActionTiming::ContactEffectID(PendingAction.element)') <
            commit_hit.index('AddDamageText(hit.target'),
            'Every valid normal or skill target must receive one contact spark alongside its damage number')
    require('PendingAction.element = state == ECharacterState::Skill' in schedule and
            'ToElementVisual(SkillData::GetElement(skillID))' in schedule and
            'BattleActionTiming::ElementVisual::Neutral' in schedule and
            'SelectedBattleSkillID' not in schedule and 'actor->GetElement()' not in schedule,
            'Skill visual element must be captured from the selected skill metadata; ordinary attacks stay neutral')
    require('ProjectileEffectID(PendingAction.projectile, PendingAction.element)' in projectile_draw and
            'SkillData::GetElement' not in commit and 'SkillData::GetElement' not in projectile_draw,
            'Flight and contact must use one immutable pending element even after selection or actor state changes')
    effect_loader = body_of(battle, 'LoadSkillEffectBitmap')
    require('NeutralEffectID(skillID)' in effect_loader and 'SkillEffectBitmaps[skillID] = nullptr;' in effect_loader and
            'IsContactEffect(skillID)' in effect_loader and 'IsProjectileEffect(skillID)' in effect_loader,
            'Missing or malformed elemental resources must retain neutral fallbacks without double-owning bitmap handles')
    require('if (!BattleActionTiming::IsContactEffect(skillID))' in body_of(battle, 'PlaySkillEffect') and
            'BattleActionTiming::IsContactEffect(effect.skillID)' in body_of(battle, 'UpdateSkillEffects') and
            'BattleActionTiming::IsContactEffect(effect.skillID)' in body_of(battle, 'DrawSkillEffects'),
            'Element contacts must share the existing short duration, body placement and no-extra-sound policy')
    effect_paths = body_of(battle.replace('GetSkillEffectRelativePath(int skillID) const',
                                         'GetSkillEffectRelativePath(int skillID)'), 'GetSkillEffectRelativePath')
    approved_repairs = {2101: 'Skill\\Warrior\\Fire\\1103_Blaze_Break.bmp',
                        1402: 'Skill\\Warrior\\Light\\1401_Holy_Slash.bmp',
                        3401: 'Skill\\Mage\\Light\\3401_Holy_Ray.bmp',
                        3301: 'Skill\\Mage\\Grass\\3301_Vine_Whip.bmp',
                        3302: 'Skill\\Mage\\Grass\\3302_Thorn_Field.bmp',
                        1501: 'Skill\\Warrior\\Dark\\1502_Shadow_Break.bmp',
                        3501: 'Skill\\Warrior\\Dark\\1502_Shadow_Break.bmp',
                        2502: 'Skill\\Warrior\\Dark\\1502_Shadow_Break.bmp'}
    for skill_id, path in approved_repairs.items():
        escaped = path.replace('\\', '\\\\')
        require(f'case {skill_id}: return L"{escaped}";' in effect_paths and
                (SOURCE / path.replace('\\', '/')).is_file(),
                f'Skill {skill_id} must use its visually audited matching element effect')
    require('ECharacterState::Hit && c->GetIsActionEnd()' in stage_visuals and
            'c->GetState() == ECharacterState::Hit' not in stage_update,
            'Only a completed target reaction may return to Idle, independently of attacker completion')
    require('hit.reacted' in resolution and 'ECharacterState::Hit || state == ECharacterState::Dead' in resolution and
            '!hit.target->GetIsActionEnd()' in resolution and 'PendingAction.actorFinished' in resolution,
            'Next commands and turns must wait for impacted reactions without adding a fixed sleep')
    require('if (PendingAction.actor != nullptr) return;' in body_of(battle, 'CheckBattleEnd'),
            'Victory and defeat must not cover a newly triggered final hit or death')
    for name in ('Init', 'LoadBattleMap', 'ResetTurnActions'):
        require('ClearPendingAction();' in body_of(battle, name),
                f'{name} must discard pending pointers and timers before reusing battle state')
    battle_init = body_of(battle, 'Init')
    battle_reset = body_of(character, 'ResetBattleAnimation')
    require(battle_init.index('c->LoadSprites(L"Resource")') < battle_init.index('c->ResetBattleAnimation();') <
            battle_init.index('c->ResetTurnState();'),
            'Re-entering battle must reset shared-party animation after loading the current assets')
    for reset in ('++ActionSequence;', 'State = IsAlive() ? ECharacterState::Idle : ECharacterState::Dead;',
                  'IdleRecovery = {};', 'WalkCycleDistance = 0.0f;', 'CurFrame = 0;',
                  'AnimationElapsedSeconds = 0.0f;', 'IsActionEnd = false;',
                  'IsMoving = false;', 'KeepWalkingOnArrival = false;', 'UpdateMaxFrameByState();'):
        require(reset in battle_reset, f'Battle re-entry must explicitly reset stale animation data: {reset}')
    require('SetState(' not in battle_reset and 'Stat.' not in battle_reset,
            'A battle animation reset must bypass same-state no-op without altering party HP or MP')
    require('elapsedSeconds' in body_of(battle, 'UpdateDamageTexts') and
            'BattleActionTiming::BasicImpactFrameSeconds' in body_of(battle, 'UpdateSkillEffects'),
            'Split updates must preserve real-time feedback durations')
    area_geometry = body_of(battle.replace('int centerY) const', 'int centerY)'), 'BuildSkillAreaCells')
    require('BattleSkillArea::BuildCells' in area_geometry and
            'const auto cells = BuildSkillAreaCells' in skill and 'info->AreaType, cells' in skill and
            'const auto cells = BuildSkillAreaCells' in body_of(battle, 'DrawSkillAreaPreview'),
            'Preview, target membership and scheduled effects must consume the same geometry helper/snapshot')
    require('pos.x != cells[i].x || pos.y != cells[i].y' in skill and
            'hit != target' in skill and 'BattleSkillArea::SecondaryDamagePercent' in skill and
            'hitTargets.push_back(target)' not in skill,
            'Only captured area cells can take damage; selected primary and original secondary coefficients remain intact')
    require('PendingAction.areaCells.back()' in schedule and
            'BattleSkillArea::LineArrival' in schedule and 'BattleSkillArea::SplashArrival' in schedule and
            'BattleSkillArea::MeleeLineArrival' in schedule,
            'Ranged lines must fly to their clipped endpoint; splash and melee use ordered spread times')
    require('BattleSkillArea::MakeLinePath' in schedule and 'PendingAction.areaCells.front()' in schedule and
            'BattleSkillArea::TotalLength(PendingAction.linePath)' in schedule and
            'BattleSkillArea::SampleLinePath(PendingAction.linePath, progress)' in projectile_draw and
            'dx = sample.dx;' in projectile_draw and 'dy = sample.dy;' in projectile_draw,
            'Line projectiles must enter the first body center then cross every following center using the current segment direction')
    require('nextAreaCell++' in commit_cells and 'BattleSkillArea::IsDue' in commit_cells and
            'hit.areaCellIndex == cellIndex' in commit_cells and 'CommitPendingHit(hit)' in commit_cells and
            'ActiveAreaEffects.push_back' in commit_cells and 'anyTarget' not in commit_cells,
            'Each reached cell must activate once, including empty cells and all cells sharing one ring time')
    require(commit_hit.index('hit.resolved = true') < commit_hit.index('!ContainsBattleUnit(hit.target)') and
            'pos.x != cell.x || pos.y != cell.y' in commit_hit,
            'Dead, removed or moved-out targets must be consumed once without damage outside their captured cell')
    require('PendingAction.nextAreaCell < PendingAction.areaCells.size()' in resolution and
            'if (!PendingAction.areaCells.empty()) return;' in commit,
            'Action lock must survive through all area cells and skip the old single-center AoE artwork')
    stage_draw = body_of(battle, 'Draw')
    require(stage_draw.index('DrawAreaSkillEffects(') < stage_draw.index('DrawUnit(') and
            'effect.cells, effect.elapsedSeconds, options' in body_of(battle, 'DrawAreaSkillEffects') and
            'AreaSkillEffectRenderer::TailSeconds' in stage_visuals,
            'Floor wave must use the damage snapshot behind units and keep its own finite tail lifetime')
    for name in ('Init', 'LoadBattleMap', 'Input'):
        require('ActiveAreaEffects.clear();' in body_of(battle, name), f'{name} must clear old area fields')
    require('if (!preserveAreaEffects)' in body_of(battle, 'ClearPendingAction') and
            'ActiveAreaEffects.clear();' in body_of(battle, 'ClearPendingAction') and
            'ClearPendingAction(true);' in resolution and
            'if (PendingAction.actor) ClearPendingAction();' in body_of(battle, 'ResetTurnActions'),
            'Forced reset must cancel field propagation while normal completion preserves only its visual tail')
    require('BattleSkillArea::AreaDamageFontSize' in body_of(battle, 'DrawDamageTexts') and
            'dt.areaDamage && textSize.cx > TILE_SIZE - 6' in body_of(battle, 'DrawDamageTexts'),
            'AoE text must use compact type and fit large numbers inside a grid-cell width')
    require('effect.enhancedContact = enhancedContact && BattleActionTiming::IsContactEffect(skillID)' in body_of(battle, 'PlaySkillEffect') and
            '!PendingAction.areaCells.empty()' in commit_hit and
            'AreaSkillVisuals::BodyFrame(effect.elapsedSeconds)' in body_of(battle, 'UpdateSkillEffects') and
            'AreaSkillVisuals::BodySeconds' in body_of(battle, 'UpdateSkillEffects'),
            'Vivid body contacts must share frame/lifetime sampling and remain limited to AoE hits')
    require(stage_draw.index('DrawPendingProjectile(') < stage_draw.index('DrawAreaSkillEffects(hMemDC, hMemDC1, true)') <
            stage_draw.index('DrawSkillEffects(') < stage_draw.index('DrawDamageTexts(') and
            'options.element = effect.element' in body_of(battle, 'DrawAreaSkillEffects'),
            'Back/front decoration must keep the captured element and leave all damage numbers on top')
    require('SkillPresentation::Requested(skillID)' in schedule and 'SkillPresentation::Resolve(skillID, available, flightAvailable)' in schedule and
            'SkillPresentation::DeliverySeconds(PendingAction.presentation)' in schedule and
            'PendingAction.projectile = BattleActionTiming::ProjectileKind::None;' in schedule,
            'Dedicated ultimates must snapshot the skill profile and available art, suppress bolts and reserve their own delivery time')
    require('ActionEvent::Launch' in pending_advance and 'StartPendingPresentation();' in pending_advance and
            'ActiveUltimateEffects.push_back' in body_of(battle, 'StartPendingPresentation') and
            'SkillPresentation::Lifetime(effect.presentation, effect.deliverySeconds)' in stage_visuals and
            'ActiveUltimateEffects.clear();' in body_of(battle, 'ClearPendingAction'),
            'Ultimate visuals must start on release, persist independently through Idle and clear on forced reset')
    require('options.groundOnly = effect.presentation != SkillPresentation::Kind::Normal' in body_of(battle, 'DrawAreaSkillEffects') and
            stage_draw.index('DrawUltimateSkillEffects(') < stage_draw.index('DrawUnit(') and
            'SkillPresentation::IsUltimateBitmap(skillID)' in effect_loader,
            'Large ultimate art must replace repeated air plumes and validate its dedicated atlas before opting in')
    ultimate_renderer = (SOURCE / 'UltimateSkillRenderer.h').read_text(encoding='utf-8-sig')
    require(stage_draw.index('DrawUnit(') < stage_draw.index('DrawUltimateSkillEffects(hMemDC, hMemDC1, true)') <
            stage_draw.index('DrawSkillEffects(') < stage_draw.index('DrawDamageTexts(') and
            'options.front = front;' in body_of(battle, 'DrawUltimateSkillEffects') and
            'if(SkillPresentation::Front(options.kind,sample.phase)!=options.front)' in ultimate_renderer,
            'Profile-defined front phases must draw once in front of units, below contacts and damage numbers')
    rain_arrows = body_of(ultimate_renderer, 'DrawRainArrows')
    require('!options.front || !SkillPresentation::IsRain(options.kind)' in rain_arrows and
            'for(const auto& cell:cells)' in rain_arrows and
            'SkillPresentation::RainArrowAt(commandElapsed,cell.arrivalSeconds)' in rain_arrows and
            '(cell.y+options.footRatio-.5f)*options.tileSize' in rain_arrows and
            '32.0f+BattleActionTiming::ArrowTipOffsetX' in rain_arrows and
            ultimate_renderer.count('DrawRainArrows(destination,scratch,cells,commandElapsed,options);') == 2 and
            ultimate_renderer.rfind('DrawRainArrows(destination,scratch,cells,commandElapsed,options);') >
            ultimate_renderer.rfind('RestoreDC(scratch,savedScratch);'),
            'Rain must deliver a tip-aligned foreground arrow to every snapshot cell before its exact arrival, including outer rings after the central landing')
    cell_summons = body_of(ultimate_renderer, 'DrawCellSummons')
    require('!options.front || !SkillPresentation::IsCellSummon(options.kind)' in cell_summons and
            'SkillPresentation::CellSummonAt(options.kind,cell.x,cell.y,commandElapsed,cell.arrivalSeconds)' in cell_summons and
            'for(const auto& cell:cells)' in cell_summons and
            'if(sample.phase==SkillPresentation::Phase::Impact || SkillPresentation::IsCellSummon(options.kind))' in ultimate_renderer and
            'if(sample.phase==SkillPresentation::Phase::Impact && !BattleSkillArea::IsDue(commandElapsed,cell.arrivalSeconds))continue;' in ultimate_renderer and
            'options.contactBitmap = LoadSkillEffectBitmap(BattleActionTiming::ContactEffectID(effect.element)' in
            body_of(battle, 'DrawUltimateSkillEffects'),
            'New ground summons must expose the preparation footprint, then show front-layer cell eruptions at the same captured arrivals and element')
    require('SkillPresentation::StartsOnCommand(PendingAction.presentation)' in schedule and
            'PendingAction.presentationStarted' in body_of(battle, 'StartPendingPresentation') and
            'effect.presentationID == PendingAction.presentationID' in body_of(battle, 'CancelPendingPresentation') and
            'CancelPendingPresentation();' in pending_advance and 'CancelPendingPresentation();' in resolution,
            'Pre-release charge must start once and be cancelled with only its own pending action')
    require('PendingAction.presentation == SkillPresentation::Kind::Normal' in commit and
            'SkillPresentation::SnipeFlightSeconds' in schedule and 'SkillPresentation::IsFlightBitmap(skillID)' in effect_loader,
            'Unique single-target strikes must suppress duplicate legacy artwork and validate directional flight art')
    require(commit.count('Skill_Impact.wav') == 1 and commit_cells.count('Skill_Impact.wav') == 1 and
            commit.index('if (!anyTarget) return;') < commit.index('Skill_Impact.wav') and
            'if (!PendingAction.areaCells.empty()) return;' in commit,
            'A valid unique single-target impact needs one skill sound; AoE keeps its separate single field sound')
    print('PASS: distance-driven gait, walk continuation, action timing and cached root-anchored rendering are integrated')


def check_preservation(path: Path) -> None:
    baseline = json.loads(path.read_text(encoding='utf-8-sig'))
    source = (SOURCE / 'BattleMapData.cpp').read_text(encoding='utf-8-sig')
    for number, expected in baseline['stageBodySha256'].items():
        actual = hashlib.sha256(body_of(source, f'BuildStage{int(number):02d}').encode()).hexdigest()
        require(actual == expected, f'Stage {number} builder changed during character work')
    for relative, expected in baseline['files'].items():
        require(hashlib.sha256((SOURCE / relative).read_bytes()).hexdigest() == expected,
                f'Map or source art changed during character work: {relative}')
    print(f"PASS: all five map builders and {len(baseline['files'])} layout/art files match the pre-character snapshot")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--baseline', type=Path, help='compare maps and original sprite assets with a pre-edit snapshot')
    options = parser.parse_args()
    try:
        check_integration()
        if options.baseline:
            check_preservation(options.baseline)
    except (AssertionError, OSError, ValueError, KeyError) as error:
        print(f'FAIL: {error}', file=sys.stderr)
        return 1
    print('NOTE: no native game, GDI pixel assertions or actual presentation FPS were executed or measured')
    return 0


if __name__ == '__main__':
    sys.exit(main())
