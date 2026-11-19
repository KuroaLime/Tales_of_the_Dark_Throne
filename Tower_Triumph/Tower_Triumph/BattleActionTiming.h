#pragma once

// Shared with the preview and compile-only checks. Zero-based pose indices
// start melee contact or ranged release. Ranged damage waits for flight arrival.
namespace BattleActionTiming
{
    constexpr int WarriorAttackFrame = 5;
    constexpr int WarriorSkillFrame = 6;
    constexpr int ArcherAttackFrame = 6;
    constexpr int ArcherSkillFrame = 6;
    constexpr int MageAttackFrame = 6;
    constexpr int MageSkillFrame = 6;
    constexpr int DefaultMonsterAttackFrame = 1;
    constexpr int BasicImpactEffectID = 0;
    constexpr float BasicImpactFrameSeconds = 0.025f;
    constexpr float SkillEffectFrameSeconds = 4.0f / 60.0f;
    constexpr float DamageTextSeconds = 1.0f;

    enum class ProjectileKind { None, Arrow, Magic };
    enum class ElementVisual { Neutral, Fire, Water, Grass, Light, Dark };
    constexpr int ArrowProjectileEffectID = -2;
    constexpr int MagicProjectileEffectID = -3;
    // Reserved negative families cannot collide with positive gameplay skill IDs.
    constexpr int FirstElementContactID = -100;
    constexpr int FirstElementArrowID = -200;
    constexpr int FirstElementMagicID = -300;
    constexpr int ElementVisualCount = 5;

    constexpr int ElementIndex(ElementVisual element)
    {
        switch (element)
        {
        case ElementVisual::Fire: return 0;
        case ElementVisual::Water: return 1;
        case ElementVisual::Grass: return 2;
        case ElementVisual::Light: return 3;
        case ElementVisual::Dark: return 4;
        default: return -1;
        }
    }
    constexpr bool InEffectFamily(int effectID, int first)
    {
        return effectID <= first && effectID > first - ElementVisualCount;
    }
    constexpr bool IsElementContact(int effectID) { return InEffectFamily(effectID, FirstElementContactID); }
    constexpr bool IsElementArrow(int effectID) { return InEffectFamily(effectID, FirstElementArrowID); }
    constexpr bool IsElementMagic(int effectID) { return InEffectFamily(effectID, FirstElementMagicID); }
    constexpr bool IsContactEffect(int effectID) { return effectID == BasicImpactEffectID || IsElementContact(effectID); }
    constexpr bool IsProjectileEffect(int effectID)
    {
        return effectID == ArrowProjectileEffectID || effectID == MagicProjectileEffectID ||
            IsElementArrow(effectID) || IsElementMagic(effectID);
    }
    constexpr int ContactEffectID(ElementVisual element)
    {
        const int index = ElementIndex(element);
        return index < 0 ? BasicImpactEffectID : FirstElementContactID - index;
    }
    constexpr int ProjectileEffectID(ProjectileKind kind, ElementVisual element)
    {
        if (kind == ProjectileKind::None) return BasicImpactEffectID;
        const bool arrow = kind == ProjectileKind::Arrow;
        const int index = ElementIndex(element);
        return index < 0 ? (arrow ? ArrowProjectileEffectID : MagicProjectileEffectID) :
            (arrow ? FirstElementArrowID : FirstElementMagicID) - index;
    }
    constexpr int NeutralEffectID(int effectID)
    {
        return IsElementContact(effectID) ? BasicImpactEffectID : IsElementArrow(effectID) ? ArrowProjectileEffectID :
            IsElementMagic(effectID) ? MagicProjectileEffectID : effectID;
    }
    constexpr ElementVisual EffectElement(int effectID)
    {
        const int first = IsElementContact(effectID) ? FirstElementContactID : IsElementArrow(effectID) ? FirstElementArrowID :
            IsElementMagic(effectID) ? FirstElementMagicID : 0;
        return first == 0 ? ElementVisual::Neutral : static_cast<ElementVisual>(first - effectID + 1);
    }
    constexpr const wchar_t* ElementSuffix(ElementVisual element)
    {
        switch (element)
        {
        case ElementVisual::Fire: return L"Fire";
        case ElementVisual::Water: return L"Water";
        case ElementVisual::Grass: return L"Grass";
        case ElementVisual::Light: return L"Light";
        case ElementVisual::Dark: return L"Dark";
        default: return L"";
        }
    }
    constexpr int ProjectileDirections = 16;
    constexpr int ProjectileFrames = 4;
    constexpr int ProjectileCellSize = 64;
    constexpr int ProjectileDrawSize = 32;
    constexpr float ProjectileFrameSeconds = 0.06f;
    constexpr float ArrowPixelsPerSecond = 420.0f;
    constexpr float MagicPixelsPerSecond = 320.0f;
    constexpr float ArrowMinimumSeconds = 0.16f;
    constexpr float MagicMinimumSeconds = 0.20f;
    constexpr float ProjectileTargetHeight = 24.0f;
    constexpr float ArrowLaunchX = 16.0f;
    constexpr float ArrowLaunchY = -21.0f;
    constexpr float MagicLaunchX = 27.0f;
    constexpr float MagicLaunchY = -31.0f;
    // Source-cell anchor relative to the atlas's rotation center (32,32).
    constexpr float ArrowTipOffsetX = 22.0f;
    constexpr float ArrowTipOffsetY = 1.5f;
    constexpr float MagicCoreOffsetX = 13.0f;
    constexpr float MagicCoreOffsetY = 0.0f;
    constexpr float DirectionCosines[16] = { 1.0f, 0.9238795f, 0.7071068f, 0.3826834f,
        0.0f, -0.3826834f, -0.7071068f, -0.9238795f, -1.0f, -0.9238795f,
        -0.7071068f, -0.3826834f, 0.0f, 0.3826834f, 0.7071068f, 0.9238795f };

    constexpr float FlightSeconds(ProjectileKind kind, float distance)
    {
        if (kind == ProjectileKind::None) return 0.0f;
        const float minimum = kind == ProjectileKind::Arrow ? ArrowMinimumSeconds : MagicMinimumSeconds;
        const float speed = kind == ProjectileKind::Arrow ? ArrowPixelsPerSecond : MagicPixelsPerSecond;
        const float duration = distance > 0.0f ? distance / speed : 0.0f;
        return duration > minimum ? duration : minimum;
    }

    constexpr int DirectionFromRadians(float radians)
    {
        const float scaled = radians * ProjectileDirections / 6.28318530718f;
        const int nearest = static_cast<int>(scaled + (scaled < 0.0f ? -0.5f : 0.5f));
        return (nearest % ProjectileDirections + ProjectileDirections) % ProjectileDirections;
    }

    constexpr bool SameName(const wchar_t* a, const wchar_t* b)
    {
        if (!a || !b) return false;
        while (*a && *a == *b) { ++a; ++b; }
        return *a == *b;
    }

    constexpr int MonsterAttackFrame(const wchar_t* sheet)
    {
        if (SameName(sheet, L"S1_Leach.bmp") || SameName(sheet, L"S2_BAngel.bmp") ||
            SameName(sheet, L"S2_Falcon.bmp") || SameName(sheet, L"S4_Slime.bmp") ||
            SameName(sheet, L"S4_Snale.bmp") || SameName(sheet, L"S4_WFly.bmp")) return 3;
        if (SameName(sheet, L"S4_Spear.bmp") || SameName(sheet, L"S5_Dragon.bmp") ||
            SameName(sheet, L"S5_Golem.bmp") || SameName(sheet, L"S5_Larva.bmp")) return 2;
        // Skeleton, Angel, Wing Angel and Hound first contact on pose one.
        return DefaultMonsterAttackFrame;
    }

    constexpr int MapAuthoredFrame(int authoredFrame, int actualFrameCount)
    {
        return actualFrameCount > 0 ? authoredFrame * actualFrameCount / 12 : 0;
    }

    struct ImpactClock
    {
        float elapsed = 0.0f;
        float impactSeconds = 0.0f;
        bool consumed = false;
    };

    struct SplitStep
    {
        float beforeImpact;
        float afterImpact;
        bool commit;
    };

    constexpr SplitStep Advance(ImpactClock& clock, float seconds)
    {
        if (!(seconds > 0.0f)) return { 0.0f, 0.0f, false };
        if (clock.consumed) return { seconds, 0.0f, false };
        const float until = clock.impactSeconds > clock.elapsed ? clock.impactSeconds - clock.elapsed : 0.0f;
        clock.elapsed += seconds;
        if (seconds + 0.000001f < until) return { seconds, 0.0f, false };
        clock.consumed = true; // Latch before any damage or callback is applied.
        const float before = until < seconds ? until : seconds;
        return { before, seconds - before, true };
    }

    enum class ActionEvent { None, Launch, Impact };
    struct ActionTimeline
    {
        ImpactClock clock;
        float flightSeconds = 0.0f;
        bool hasProjectile = false;
        bool launched = false;
        bool impactCommitted = false;
    };
    struct ActionStep
    {
        float beforeEvent;
        float afterEvent;
        ActionEvent event;
    };

    // Consume one boundary at a time. The caller applies the event between
    // updating the scene for beforeEvent and advancing the returned remainder.
    constexpr ActionStep AdvanceTimeline(ActionTimeline& timeline, float seconds)
    {
        if (timeline.impactCommitted) return { seconds > 0.0f ? seconds : 0.0f, 0.0f, ActionEvent::None };
        const SplitStep step = Advance(timeline.clock, seconds);
        if (!step.commit) return { step.beforeImpact, step.afterImpact, ActionEvent::None };
        if (timeline.hasProjectile && !timeline.launched)
        {
            timeline.launched = true;
            timeline.clock = { 0.0f, timeline.flightSeconds, false };
            return { step.beforeImpact, step.afterImpact, ActionEvent::Launch };
        }
        timeline.impactCommitted = true;
        return { step.beforeImpact, step.afterImpact, ActionEvent::Impact };
    }
}
