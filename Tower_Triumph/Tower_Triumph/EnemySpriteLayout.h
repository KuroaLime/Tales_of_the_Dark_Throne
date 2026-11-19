#pragma once

#include "CharacterMotion.h"
#include "CharacterSpriteLayout.h"

// The approved enemy atlas owns its timing and anchors independently of the
// legacy 6x3 sheets. All poses share the same body scale as the player sprites.
namespace EnemySpriteLayout
{
    constexpr int Columns = 8;
    constexpr int Rows = 6;
    constexpr int CellSize = 128;
    constexpr int FrameCount = Columns * Rows;
    constexpr int StateFrames = 8;
    constexpr int TransitionFrames = 4;
    constexpr int PortraitSize = 128;
    constexpr float RootX = 64.0f;
    constexpr float GroundY = 112.0f;
    constexpr int ReferenceFrameHeight = 96;
    constexpr float IdleFrameSeconds = 0.14f;
    constexpr float AttackFrameSeconds = 0.08f;
    constexpr float HitFrameSeconds = 0.06f;
    constexpr float DeadFrameSeconds = 0.10f;
    constexpr float TransitionFrameSeconds = 0.04f;
    constexpr int AttackImpactFrame = 4;

    // This atlas releases the bow one pose later than the other enemy attacks.
    // Resolve by the explicit asset identity, never by the gameplay class.
    constexpr int AttackImpactFrameFor(const wchar_t* stem)
    {
        if (!stem) return AttackImpactFrame;
        const wchar_t* bowAngel = L"S2_BAngel";
        while (*stem && *stem == *bowAngel) { ++stem; ++bowAngel; }
        return *stem == *bowAngel ? 5 : AttackImpactFrame;
    }

    enum class Pose { Idle, Walk, Attack, Hit, Dead };

    constexpr bool HasDimensions(int width, int height)
    {
        return width == Columns * CellSize && height == Rows * CellSize;
    }

    constexpr int FrameIndex(Pose pose, int frame)
    {
        const int localFrame = frame < 0 ? 0 : frame >= StateFrames ? StateFrames - 1 : frame;
        return static_cast<int>(pose) * Columns + localFrame;
    }

    constexpr CharacterSpriteLayout::Frame WithAnchor(CharacterSpriteLayout::Frame frame, int index)
    {
        frame.anchorX = (index % Columns) * CellSize + RootX;
        frame.anchorY = (index / Columns) * CellSize + GroundY;
        return frame;
    }

    constexpr CharacterMotion::IdleRecovery BeginWalkRecovery(int frame, float frameSeconds, float elapsed)
    {
        if (frame < 0 || frame >= StateFrames || !(frameSeconds > 0.0f)) return {};
        const bool contact = frame == 0 || frame == StateFrames / 2;
        return { contact ? CharacterMotion::IdleRecoveryKind::WalkStop :
            CharacterMotion::IdleRecoveryKind::WalkContact,
            contact ? 0 : frame, 0, contact ? 0.0f : elapsed, frameSeconds };
    }

    constexpr CharacterMotion::IdleRecovery BeginActionRecovery(int frame, int branch,
        float frameSeconds, float elapsed)
    {
        if (frame < 0 || frame >= StateFrames || branch < 0 || branch > 2 || !(frameSeconds > 0.0f)) return {};
        const bool finished = frame == StateFrames - 1;
        return { finished ? CharacterMotion::IdleRecoveryKind::ActionReturn :
            CharacterMotion::IdleRecoveryKind::ActionTail,
            finished ? TransitionFrames : frame, branch, finished ? 0.0f : elapsed, frameSeconds };
    }

    constexpr int RecoveryFrameIndex(const CharacterMotion::IdleRecovery& recovery)
    {
        switch (recovery.kind)
        {
        case CharacterMotion::IdleRecoveryKind::WalkContact:
            return FrameIndex(Pose::Walk, recovery.frame);
        case CharacterMotion::IdleRecoveryKind::ActionTail:
            return FrameIndex(recovery.branch == 2 ? Pose::Hit : Pose::Attack, recovery.frame);
        case CharacterMotion::IdleRecoveryKind::WalkStop:
            return 5 * Columns + (recovery.frame < 0 ? 0 : recovery.frame >= TransitionFrames ? TransitionFrames - 1 : recovery.frame);
        case CharacterMotion::IdleRecoveryKind::ActionReturn:
            return 5 * Columns + (recovery.frame < TransitionFrames ? TransitionFrames :
                recovery.frame >= StateFrames ? StateFrames - 1 : recovery.frame);
        default:
            return FrameIndex(Pose::Idle, 0);
        }
    }

    // Consumes only visual recovery time. A new command can interrupt it at
    // once, and no step here moves the unit or changes its gameplay state.
    constexpr float AdvanceRecovery(CharacterMotion::IdleRecovery& recovery, float seconds)
    {
        using Kind = CharacterMotion::IdleRecoveryKind;
        if (!(seconds > 0.0f)) return 0.0f;
        if (recovery.kind == Kind::None) return seconds;
        recovery.elapsed += seconds;
        while (recovery.kind != Kind::None)
        {
            const float interval = recovery.kind == Kind::WalkContact || recovery.kind == Kind::ActionTail ?
                recovery.sourceFrameSeconds : TransitionFrameSeconds;
            if (!(interval > 0.0f) || recovery.elapsed + 0.000001f < interval) return 0.0f;
            recovery.elapsed -= interval;
            if (recovery.elapsed < 0.0f) recovery.elapsed = 0.0f;
            ++recovery.frame;
            if (recovery.kind == Kind::WalkContact)
            {
                recovery.frame %= StateFrames;
                if (recovery.frame == 0 || recovery.frame == StateFrames / 2)
                {
                    recovery.kind = Kind::WalkStop;
                    recovery.frame = 0;
                }
            }
            else if (recovery.kind == Kind::ActionTail)
            {
                if (recovery.frame >= StateFrames)
                {
                    recovery.kind = Kind::ActionReturn;
                    recovery.frame = TransitionFrames;
                }
            }
            else if (recovery.frame >= (recovery.kind == Kind::WalkStop ? TransitionFrames : StateFrames))
            {
                const float remaining = recovery.elapsed;
                recovery = {};
                return remaining;
            }
        }
        return 0.0f;
    }
}
