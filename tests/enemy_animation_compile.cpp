#include "../Tower_Triumph/Tower_Triumph/EnemySpriteLayout.h"
#include "../Tower_Triumph/Tower_Triumph/BattleActionTiming.h"

namespace
{
namespace E = EnemySpriteLayout;
namespace M = CharacterMotion;
using RecoveryKind = M::IdleRecoveryKind;

constexpr bool Near(float a, float b) { return a - b >= -0.00001f && a - b <= 0.00001f; }

constexpr bool EveryPoseHasOneCell()
{
    for (int row = 0; row < 5; ++row)
    {
        const auto pose = static_cast<E::Pose>(row);
        if (E::FrameIndex(pose, -1) != row * 8 || E::FrameIndex(pose, 8) != row * 8 + 7) return false;
        for (int frame = 0; frame < 8; ++frame)
            if (E::FrameIndex(pose, frame) != row * 8 + frame) return false;
    }
    return true;
}

constexpr bool FixedAnchorEveryFrame()
{
    for (int index = 0; index < 48; ++index)
    {
        CharacterSpriteLayout::Frame frame{{0, 0, 128, 128}, 0, 0};
        const auto anchored = E::WithAnchor(frame, index);
        if (!Near(anchored.anchorX, index % 8 * 128.0f + 64) ||
            !Near(anchored.anchorY, index / 8 * 128.0f + 112) ||
            anchored.source.left != frame.source.left || anchored.source.bottom != frame.source.bottom) return false;
    }
    return true;
}

constexpr bool WalkRecoveryForEveryPhase()
{
    for (int phase = 0; phase < 8; ++phase)
    {
        auto recovery = E::BeginWalkRecovery(phase, .05f, .01f);
        if (phase == 0 || phase == 4)
        {
            if (recovery.kind != RecoveryKind::WalkStop || E::RecoveryFrameIndex(recovery) != 40) return false;
        }
        else if (recovery.kind != RecoveryKind::WalkContact || E::RecoveryFrameIndex(recovery) != 8 + phase) return false;
        const int posesToContact = phase == 0 || phase == 4 ? 0 : phase < 4 ? 4 - phase : 8 - phase;
        const float expectedRecovery = posesToContact * .05f - (posesToContact > 0 ? .01f : 0.0f) + .16f;
        const float remaining = E::AdvanceRecovery(recovery, .8f);
        if (recovery.kind != RecoveryKind::None || !Near(remaining, .8f - expectedRecovery)) return false;
    }
    return true;
}

constexpr bool ActionRecoveryForEveryPhase()
{
    for (int branch = 0; branch < 3; ++branch)
        for (int phase = 0; phase < 8; ++phase)
        {
            const float interval = branch == 2 ? E::HitFrameSeconds : E::AttackFrameSeconds;
            auto recovery = E::BeginActionRecovery(phase, branch, interval, .01f);
            if (phase == 7)
            {
                if (recovery.kind != RecoveryKind::ActionReturn || E::RecoveryFrameIndex(recovery) != 44) return false;
            }
            else if (recovery.kind != RecoveryKind::ActionTail ||
                E::RecoveryFrameIndex(recovery) != (branch == 2 ? 24 : 16) + phase) return false;
            const float tailSeconds = phase == 7 ? 0.0f : (8 - phase) * interval - .01f;
            const float remaining = E::AdvanceRecovery(recovery, 1.0f);
            if (recovery.kind != RecoveryKind::None || !Near(remaining, 1.0f - tailSeconds - .16f)) return false;
        }
    return true;
}

constexpr bool RecoveryRateIndependent(int fps)
{
    auto recovery = E::BeginWalkRecovery(6, .05f, .01f);
    float returned = 0;
    for (int i = 0; i < fps; ++i)
    {
        if (recovery.kind != RecoveryKind::None)
        {
            const int frame = E::RecoveryFrameIndex(recovery);
            if (!((frame >= 8 && frame < 16) || (frame >= 40 && frame < 44))) return false;
        }
        returned += E::AdvanceRecovery(recovery, 1.0f / fps);
    }
    return recovery.kind == RecoveryKind::None && Near(returned, .75f);
}

constexpr bool AttackContactOnce(int fps, int impactFrame)
{
    BattleActionTiming::ImpactClock clock;
    clock.impactSeconds = impactFrame * E::AttackFrameSeconds;
    int contacts = 0;
    for (int i = 0; i < fps; ++i)
    {
        const auto step = BattleActionTiming::Advance(clock, 1.0f / fps);
        if (step.commit)
        {
            ++contacts;
            if (!Near(i * (1.0f / fps) + step.beforeImpact, impactFrame * E::AttackFrameSeconds)) return false;
        }
    }
    return contacts == 1 && clock.consumed;
}

constexpr bool ContactProfilesMatchReviewedArt()
{
    const wchar_t* ordinary[] = { L"S1_Skeleton", L"S1_Leach", L"S2_Angel", L"S2_Falcon", L"S2_WAngel",
        L"S4_Slime", L"S4_Snale", L"S4_Spear", L"S4_WFly", L"S5_Dragon", L"S5_Golem", L"S5_Hound", L"S5_Larva" };
    for (const auto* stem : ordinary)
        if (E::AttackImpactFrameFor(stem) != 4) return false;
    return E::AttackImpactFrameFor(L"S2_BAngel") == 5 && E::AttackImpactFrameFor(nullptr) == 4 &&
        E::AttackImpactFrameFor(L"") == 4 && E::AttackImpactFrameFor(L"S2_BAngelExtra") == 4 &&
        E::AttackImpactFrameFor(L"S2_BAng") == 4;
}

constexpr bool InvalidRecoveryDoesNotStart()
{
    return E::BeginWalkRecovery(-1, .05f, 0).kind == RecoveryKind::None &&
        E::BeginWalkRecovery(8, .05f, 0).kind == RecoveryKind::None &&
        E::BeginWalkRecovery(2, 0, 0).kind == RecoveryKind::None &&
        E::BeginActionRecovery(-1, 0, .08f, 0).kind == RecoveryKind::None &&
        E::BeginActionRecovery(8, 0, .08f, 0).kind == RecoveryKind::None &&
        E::BeginActionRecovery(2, -1, .08f, 0).kind == RecoveryKind::None &&
        E::BeginActionRecovery(2, 3, .08f, 0).kind == RecoveryKind::None &&
        E::BeginActionRecovery(2, 0, 0, 0).kind == RecoveryKind::None;
}

constexpr bool GaitDependsOnlyOnDistance()
{
    constexpr float stride = 64.0f;
    float cycle = M::AdvanceWalkCycle(0, 12, stride);
    const int before = M::WalkFrame(cycle, E::StateFrames, stride);
    cycle = M::AdvanceWalkCycle(cycle, 0, stride);
    if (M::WalkFrame(cycle, E::StateFrames, stride) != before) return false;
    for (int i = 0; i < 8; ++i)
        if (M::WalkFrame(i * 8.0f, E::StateFrames, stride) != i) return false;
    return Near(M::AdvanceWalkCycle(60, 8, stride), 4.0f);
}

static_assert(E::HasDimensions(1024, 768) && !E::HasDimensions(1536, 768) &&
    !E::HasDimensions(1024, 767), "An approved atlas cannot silently use the legacy grid");
static_assert(E::Columns == 8 && E::Rows == 6 && E::FrameCount == 48 &&
    E::CellSize == 128 && E::PortraitSize == 128 && E::ReferenceFrameHeight == 96,
    "Enemy source padding must not change the established player body scale");
static_assert(EveryPoseHasOneCell() && FixedAnchorEveryFrame(), "State rows and fixed anchors cannot bleed between poses");
static_assert(Near(8 * E::IdleFrameSeconds, 1.12f) && Near(8 * E::AttackFrameSeconds, .64f) &&
    Near(8 * E::HitFrameSeconds, .48f) && Near(8 * E::DeadFrameSeconds, .8f), "Authored enemy state durations");
static_assert(WalkRecoveryForEveryPhase(), "Walking settles through its next contact and preserves leftover time");
static_assert(ActionRecoveryForEveryPhase(), "Every action phase completes the proper attack/hit tail before returning to idle");
static_assert(RecoveryRateIndependent(15) && RecoveryRateIndependent(30) &&
    RecoveryRateIndependent(60) && RecoveryRateIndependent(120), "Recovery has the same duration at each update rate");
static_assert(AttackContactOnce(15, 4) && AttackContactOnce(30, 4) && AttackContactOnce(60, 4) && AttackContactOnce(120, 4) &&
    AttackContactOnce(15, 5) && AttackContactOnce(30, 5) && AttackContactOnce(60, 5) && AttackContactOnce(120, 5),
    "Normal .32-second and bow angel .40-second contacts must each trigger once across update rates");
static_assert(ContactProfilesMatchReviewedArt(), "Every enemy identity must use the contact pose from its final atlas");
static_assert(InvalidRecoveryDoesNotStart() && GaitDependsOnlyOnDistance(), "Malformed recovery and stationary movement cannot create motion");
static_assert(BattleActionTiming::MonsterAttackFrame(L"S1_Skeleton.bmp") == 1 &&
    BattleActionTiming::MonsterAttackFrame(L"S1_Leach.bmp") == 3 &&
    BattleActionTiming::MonsterAttackFrame(L"S5_Golem.bmp") == 2,
    "Missing approved art must retain the original monster contact profiles");
}
