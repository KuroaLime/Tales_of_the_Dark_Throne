// Compile with /c; these checks do not require launching a native executable.
#include "../Tower_Triumph/Tower_Triumph/CharacterMotion.h"

constexpr bool Near(float a, float b)
{
    return a > b - 0.0001f && a < b + 0.0001f;
}

using namespace CharacterMotion;
static_assert(Near(ClampDelta(10.0f), 0.1f), "Long pauses must not teleport a unit");
static_assert(ClampDelta(-0.1f) == 0.0f, "Negative time must not reverse movement");
static_assert(Near(Advance(48.0f, 240.0f, 0.05f).distance, 12.0f), "Speed is pixels per second");
static_assert(!Advance(48.0f, 240.0f, 0.05f).arrived, "Partial steps must remain in motion");
static_assert(Advance(48.0f, 240.0f, 0.2f).arrived, "Exact arrivals must settle on the target");
constexpr auto first = Advance(6.0f, 240.0f, 0.05f);
constexpr auto second = Advance(48.0f, 240.0f, first.remainingSeconds);
static_assert(first.arrived && Near(first.remainingSeconds, 0.025f), "Preserve time after a tile boundary");
static_assert(Near(first.distance + second.distance, 12.0f), "Crossing a tile boundary must not reduce travel");
static_assert(!second.arrived, "Remainder must not skip the next target");
static_assert(Advance(48.0f, 0.0f, 1.0f).distance == 0.0f, "Zero speed must not divide by zero");
static_assert(AdvanceAnimation(0.1f, 1.0f / 30.0f).frames == 3, "Slow updates must catch up all animation frames");
static_assert(AdvanceAnimation(0.025f, 1.0f / 30.0f).frames == 0, "Do not advance a pose early");
static_assert(Near(AdvanceAnimation(0.05f, 1.0f / 30.0f).remainingSeconds, 1.0f / 60.0f),
    "Retain partial-frame time instead of restarting the animation clock");

constexpr int AnimationFramesAfter(int updates, float delta)
{
    float elapsed = 0.0f;
    int frames = 0;
    for (int update = 0; update < updates; ++update)
    {
        const auto result = AdvanceAnimation(elapsed + delta, 1.0f / 30.0f);
        elapsed = result.remainingSeconds;
        frames += result.frames;
    }
    return frames;
}

static_assert(AnimationFramesAfter(60, 1.0f / 60.0f) == 30,
    "Animation cadence must not depend on a sixty-Hz update rate");
static_assert(AnimationFramesAfter(30, 1.0f / 30.0f) == 30,
    "Animation cadence must not depend on a thirty-Hz update rate");
static_assert(AnimationFramesAfter(36, 1.0f / 60.0f) == 18,
    "Time-based actions can still visit the complete eighteen-pose cycle");
static_assert(Near(60.0f * Advance(1000.0f, 240.0f, 1.0f / 60.0f).distance,
                   30.0f * Advance(1000.0f, 240.0f, 1.0f / 30.0f).distance),
    "Movement speed must be independent of the update frequency");

static_assert(Near(AdvanceWalkCycle(60.0f, 8.0f), 4.0f),
    "A gait cycle must retain distance beyond the loop boundary");
static_assert(AdvanceWalkCycle(16.0f, 0.0f) == 16.0f,
    "A stationary character must retain its current pose");
static_assert(AdvanceWalkCycle(16.0f, -1.0f) == 16.0f,
    "Invalid negative travel must not reverse the gait");
static_assert(AdvanceWalkCycle(16.0f, 4.0f, 0.0f) == 0.0f,
    "An invalid gait length must not divide by zero");
static_assert(WalkFrame(16.0f, 12) == 3 && WalkFrame(32.0f, 12) == 6 && WalkFrame(48.0f, 12) == 9,
    "Crossing and opposite-contact poses must follow quarter-cycle travel");
static_assert(WalkFrame(64.0f, 12) == 0 && WalkFrame(8.0f, 0) == 0,
    "Gait frames must wrap safely");
static_assert(Near(AdvanceWalkCycle(AdvanceWalkCycle(0.0f, first.distance), second.distance), 12.0f),
    "Travel on both sides of a tile boundary must advance the gait exactly once");

constexpr float WalkDistanceAfter(int updates, float delta, float speed, float cycleLength = WalkCycleDistancePixels)
{
    float phase = 0.0f;
    for (int update = 0; update < updates; ++update)
        phase = AdvanceWalkCycle(phase, Advance(1000.0f, speed, delta).distance, cycleLength);
    return phase;
}
static_assert(Near(WalkDistanceAfter(60, 1.0f / 60.0f, 240.0f), 48.0f) &&
    Near(WalkDistanceAfter(30, 1.0f / 30.0f, 240.0f), 48.0f),
    "Identical travel must choose the same walk phase at different update rates");
static_assert(Near(WalkDistanceAfter(60, 1.0f / 60.0f, 240.0f),
    WalkDistanceAfter(120, 1.0f / 60.0f, 120.0f)),
    "Changing speed must not change the gait phase at an identical world position");

constexpr bool CalibratedGait(float cycleLength, float speed)
{
    const float sixtyHz = WalkDistanceAfter(60, 1.0f / 60.0f, speed, cycleLength);
    const float thirtyHz = WalkDistanceAfter(30, 1.0f / 30.0f, speed, cycleLength);
    const float halfSpeed = WalkDistanceAfter(120, 1.0f / 60.0f, speed * 0.5f, cycleLength);
    return Near(sixtyHz, thirtyHz) && Near(sixtyHz, halfSpeed) &&
        WalkFrame(sixtyHz, 12, cycleLength) == WalkFrame(thirtyHz, 12, cycleLength) &&
        WalkFrame(sixtyHz, 12, cycleLength) == WalkFrame(halfSpeed, 12, cycleLength) &&
        WalkFrame(cycleLength * 0.25f, 12, cycleLength) == 3 &&
        WalkFrame(cycleLength * 0.5f, 12, cycleLength) == 6 &&
        WalkFrame(AdvanceWalkCycle(0.0f, cycleLength, cycleLength), 12, cycleLength) == 0;
}
static_assert(CalibratedGait(WarriorWalkCycleDistancePixels, 225.0f),
    "The warrior's measured gait must preserve phase across frame rates and movement speeds");
static_assert(CalibratedGait(ArcherWalkCycleDistancePixels, 270.0f),
    "The archer's measured gait must preserve phase across frame rates and movement speeds");
static_assert(CalibratedGait(MageWalkCycleDistancePixels, 240.0f),
    "The mage's measured gait must preserve phase across frame rates and movement speeds");

constexpr bool AuthoredActionCompletesAt(float durationSeconds, int updates)
{
    const float frameSeconds = ActionTiming::FrameSeconds(12, durationSeconds);
    float remainder = 0.0f;
    int frames = 0;
    for (int update = 0; update < updates; ++update)
    {
        const auto step = AdvanceAnimation(remainder + durationSeconds / updates, frameSeconds);
        frames += step.frames;
        remainder = step.remainingSeconds;
        if (update + 1 < updates && frames >= 12) return false;
    }
    return frames == 12;
}
constexpr bool PreservedActionDuration(float durationSeconds, float oldFps)
{
    return Near(durationSeconds, 18.0f / oldFps) &&
        AdvanceAnimation(durationSeconds - 0.001f, ActionTiming::FrameSeconds(12, durationSeconds)).frames == 11 &&
        AdvanceAnimation(durationSeconds, ActionTiming::FrameSeconds(12, durationSeconds)).frames == 12 &&
        AuthoredActionCompletesAt(durationSeconds, 36) && AuthoredActionCompletesAt(durationSeconds, 18);
}
static_assert(PreservedActionDuration(ActionTiming::AttackSeconds, 30.0f) &&
    PreservedActionDuration(ActionTiming::SkillSeconds, 30.0f) &&
    PreservedActionDuration(ActionTiming::HitSeconds, 30.0f),
    "Attack, skill and hit replacement artwork must not advance the gameplay completion boundary");
static_assert(PreservedActionDuration(ActionTiming::DeadSeconds, 20.0f) &&
    PreservedActionDuration(ActionTiming::DefendSeconds, 8.0f),
    "Guard and death must retain their original duration and final-pose hold timing");
static_assert(ActionTiming::FrameSeconds(0, 0.6f) == 0.0f && ActionTiming::FrameSeconds(12, 0.0f) == 0.0f,
    "Invalid action timing must not divide by zero");

constexpr bool CheckWalkRecoveryEveryPhase()
{
    for (int frame = 0; frame < 12; ++frame)
    {
        auto recovery = BeginWalkRecovery(frame, 0.02f, 0.005f);
        if (recovery.frame != frame) return false;
        const bool startsAtContact = frame == 0 || frame == 6;
        const int expectedBranch = startsAtContact ? frame / 6 : (frame < 6 ? 1 : 0);
        const int contactSteps = startsAtContact ? 0 : (frame < 6 ? 6 - frame : 12 - frame);
        if (recovery.branch != expectedBranch || contactSteps > 5) return false;
        if (!startsAtContact)
        {
            const float secondsToContact = contactSteps * 0.02f - 0.005f;
            AdvanceIdleRecovery(recovery, secondsToContact - 0.001f);
            if (recovery.kind != IdleRecoveryKind::WalkContact) return false;
            AdvanceIdleRecovery(recovery, 0.001f);
        }
        if (recovery.kind != IdleRecoveryKind::WalkStop || recovery.frame != expectedBranch * 6) return false;
        AdvanceIdleRecovery(recovery, IdleTiming::StopSeconds - 0.001f);
        if (recovery.kind != IdleRecoveryKind::WalkStop || recovery.frame != expectedBranch * 6 + 5) return false;
        const float remainder = AdvanceIdleRecovery(recovery, 0.011f);
        if (recovery.kind != IdleRecoveryKind::None || !Near(remainder, 0.01f)) return false;
    }
    return true;
}
static_assert(CheckWalkRecoveryEveryPhase(),
    "Every walk phase must finish at the next contact, play only its own stop branch, and preserve leftover time");

constexpr bool CheckActionRecovery()
{
    for (int branch = 0; branch < 3; ++branch)
    {
        auto recovery = BeginActionRecovery(11, branch, 0.05f);
        if (recovery.kind != IdleRecoveryKind::ActionReturn || recovery.frame != branch * 4) return false;
        AdvanceIdleRecovery(recovery, 0.199f);
        if (recovery.kind != IdleRecoveryKind::ActionReturn || recovery.frame != branch * 4 + 3) return false;
        if (!Near(AdvanceIdleRecovery(recovery, 0.021f), 0.02f) || recovery.kind != IdleRecoveryKind::None) return false;
    }
    auto shortenedHit = BeginActionRecovery(9, 2, 0.05f, 0.01f);
    AdvanceIdleRecovery(shortenedHit, 0.04f);
    if (shortenedHit.kind != IdleRecoveryKind::ActionTail || shortenedHit.frame != 10) return false;
    AdvanceIdleRecovery(shortenedHit, 0.05f);
    if (shortenedHit.kind != IdleRecoveryKind::ActionReturn || shortenedHit.frame != 8) return false;
    AdvanceIdleRecovery(shortenedHit, 0.20f);
    return shortenedHit.kind == IdleRecoveryKind::None;
}
static_assert(CheckActionRecovery(),
    "Each action has a separate return row, and a shortened hit finishes its outgoing recoil before returning");

constexpr bool RecoveryIsIndependentOfUpdateRate(int updates)
{
    auto recovery = BeginWalkRecovery(1, 0.02f);
    float idleSeconds = 0.0f;
    for (int step = 0; step < updates; ++step)
        idleSeconds += AdvanceIdleRecovery(recovery, 0.5f / updates);
    return recovery.kind == IdleRecoveryKind::None && Near(idleSeconds, 0.16f);
}
static_assert(RecoveryIsIndependentOfUpdateRate(15) && RecoveryIsIndependentOfUpdateRate(30) &&
    RecoveryIsIndependentOfUpdateRate(60), "Visual recovery must consume equal time at 30, 60 and 120 Hz");
static_assert(BeginWalkRecovery(-1, 0.02f).kind == IdleRecoveryKind::None &&
    BeginWalkRecovery(3, 0.0f).kind == IdleRecoveryKind::None &&
    BeginActionRecovery(12, 2, 0.05f).kind == IdleRecoveryKind::None &&
    BeginActionRecovery(11, 3, 0.05f).kind == IdleRecoveryKind::None,
    "Invalid recovery inputs cannot select a pose outside an authored branch");
static_assert(AdvanceAnimation(IdleTiming::CycleSeconds, IdleTiming::CycleFrameSeconds).frames == 12 &&
    AdvanceAnimation(IdleTiming::CycleSeconds - 0.001f, IdleTiming::CycleFrameSeconds).frames == 11,
    "Idle breathing must complete one twelve-pose loop in 2.4 seconds");
