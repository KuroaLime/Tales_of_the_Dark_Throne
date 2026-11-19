#pragma once

// Time-based movement arithmetic shared by the character and battle update.
// Keep this independent of Win32 so its edge cases can be checked at compile time.
namespace CharacterMotion
{
    constexpr float MaximumDeltaSeconds = 0.1f;
    // Fallback gait length; authored sheets use their measured stance travel.
    constexpr float WalkCycleDistancePixels = 64.0f;
    constexpr float WarriorWalkCycleDistancePixels = 64.0f;
    constexpr float ArcherWalkCycleDistancePixels = 64.0f;
    constexpr float MageWalkCycleDistancePixels = 60.0f;

    namespace ActionTiming
    {
        // Preserve the original eighteen-frame action durations when the
        // authored twelve-pose sheets replace the original artwork.
        constexpr float AttackSeconds = 0.6f;
        constexpr float SkillSeconds = 0.6f;
        constexpr float HitSeconds = 0.6f;
        constexpr float DeadSeconds = 0.9f;
        constexpr float DefendSeconds = 2.25f;

        constexpr float FrameSeconds(int frameCount, float durationSeconds)
        {
            return frameCount > 0 && durationSeconds > 0.0f ? durationSeconds / frameCount : 0.0f;
        }
    }

    namespace IdleTiming
    {
        constexpr int CycleFrames = 12;
        constexpr float CycleSeconds = 2.4f;
        constexpr float CycleFrameSeconds = CycleSeconds / CycleFrames;
        constexpr int StopBranchFrames = 6;
        constexpr float StopSeconds = 0.24f;
        constexpr float StopFrameSeconds = StopSeconds / StopBranchFrames;
        constexpr int ReturnBranchFrames = 4;
        constexpr float ReturnSeconds = 0.20f;
        constexpr float ReturnFrameSeconds = ReturnSeconds / ReturnBranchFrames;
    }

    // Rendering-only recovery. The gameplay state is already Idle throughout;
    // a new command can discard this value without waiting or moving the unit.
    enum class IdleRecoveryKind { None, WalkContact, WalkStop, ActionTail, ActionReturn };

    struct IdleRecovery
    {
        IdleRecoveryKind kind = IdleRecoveryKind::None;
        int frame = 0; // Absolute index in the currently displayed sheet.
        int branch = 0; // Stop: 0/1; action return: Attack/Skill/Hit = 0/1/2.
        float elapsed = 0.0f;
        float sourceFrameSeconds = 0.0f;
    };

    constexpr IdleRecovery BeginWalkRecovery(int frame, float frameSeconds, float elapsed = 0.0f)
    {
        if (frame < 0 || frame >= 12 || !(frameSeconds > 0.0f)) return {};
        const bool contact = frame == 0 || frame == 6;
        const int branch = contact ? frame / 6 : (frame < 6 ? 1 : 0);
        return { contact ? IdleRecoveryKind::WalkStop : IdleRecoveryKind::WalkContact,
            frame, branch, contact ? 0.0f : elapsed, frameSeconds };
    }

    constexpr IdleRecovery BeginActionRecovery(int frame, int branch, float frameSeconds, float elapsed = 0.0f)
    {
        if (frame < 0 || frame >= 12 || branch < 0 || branch > 2 || !(frameSeconds > 0.0f)) return {};
        return { frame < 11 ? IdleRecoveryKind::ActionTail : IdleRecoveryKind::ActionReturn,
            frame < 11 ? frame : branch * IdleTiming::ReturnBranchFrames,
            branch, frame < 11 ? elapsed : 0.0f, frameSeconds };
    }

    constexpr float AdvanceIdleRecovery(IdleRecovery& recovery, float seconds)
    {
        if (!(seconds > 0.0f)) return 0.0f;
        if (recovery.kind == IdleRecoveryKind::None) return seconds;
        recovery.elapsed += seconds;
        while (recovery.kind != IdleRecoveryKind::None)
        {
            const float interval = recovery.kind == IdleRecoveryKind::WalkStop ? IdleTiming::StopFrameSeconds :
                recovery.kind == IdleRecoveryKind::ActionReturn ? IdleTiming::ReturnFrameSeconds : recovery.sourceFrameSeconds;
            if (!(interval > 0.0f) || recovery.elapsed + 0.000001f < interval) return 0.0f;
            recovery.elapsed -= interval;
            if (recovery.elapsed < 0.0f) recovery.elapsed = 0.0f;
            ++recovery.frame;
            if (recovery.kind == IdleRecoveryKind::WalkContact)
            {
                recovery.frame %= 12;
                if (recovery.frame == 0 || recovery.frame == 6)
                {
                    recovery.kind = IdleRecoveryKind::WalkStop;
                    recovery.frame = recovery.branch * IdleTiming::StopBranchFrames;
                }
            }
            else if (recovery.kind == IdleRecoveryKind::ActionTail)
            {
                if (recovery.frame >= 11)
                {
                    recovery.kind = IdleRecoveryKind::ActionReturn;
                    recovery.frame = recovery.branch * IdleTiming::ReturnBranchFrames;
                }
            }
            else
            {
                const int count = recovery.kind == IdleRecoveryKind::WalkStop ?
                    IdleTiming::StopBranchFrames : IdleTiming::ReturnBranchFrames;
                if (recovery.frame >= (recovery.branch + 1) * count)
                {
                    const float remaining = recovery.elapsed;
                    recovery = {};
                    return remaining;
                }
            }
        }
        return 0.0f;
    }

    constexpr float ClampDelta(float seconds)
    {
        return !(seconds > 0.0f) ? 0.0f :
            (seconds > MaximumDeltaSeconds ? MaximumDeltaSeconds : seconds);
    }

    struct MovementStep
    {
        float distance;
        float remainingSeconds;
        bool arrived;
    };

    constexpr MovementStep Advance(float distance, float speed, float seconds)
    {
        if (!(seconds > 0.0f) || !(speed > 0.0f)) return { 0.0f, 0.0f, false };
        if (!(distance > 0.0f)) return { 0.0f, seconds, true };
        const float availableDistance = speed * seconds;
        if (availableDistance < distance) return { availableDistance, 0.0f, false };
        const float remaining = seconds - distance / speed;
        return { distance, remaining > 0.0f ? remaining : 0.0f, true };
    }

    struct AnimationStep
    {
        int frames;
        float remainingSeconds;
    };

    constexpr AnimationStep AdvanceAnimation(float accumulatedSeconds, float frameSeconds)
    {
        if (!(accumulatedSeconds > 0.0f) || !(frameSeconds > 0.0f)) return { 0, 0.0f };
        // Small tolerance avoids losing a frame to repeated float additions.
        const int frames = static_cast<int>((accumulatedSeconds + 0.000001f) / frameSeconds);
        const float remaining = accumulatedSeconds - frames * frameSeconds;
        return { frames, remaining > 0.0f ? remaining : 0.0f };
    }

    constexpr float AdvanceWalkCycle(float cycleDistance, float travelledDistance,
        float cycleLength = WalkCycleDistancePixels)
    {
        if (!(cycleLength > 0.0f)) return 0.0f;
        if (!(travelledDistance > 0.0f)) return cycleDistance;
        const float distance = cycleDistance + travelledDistance;
        const int cycles = static_cast<int>(distance / cycleLength);
        return distance - cycles * cycleLength;
    }

    constexpr int WalkFrame(float cycleDistance, int frameCount,
        float cycleLength = WalkCycleDistancePixels)
    {
        if (!(cycleLength > 0.0f) || frameCount <= 0 || !(cycleDistance > 0.0f)) return 0;
        const int frame = static_cast<int>(cycleDistance * frameCount / cycleLength);
        return frame % frameCount;
    }
}
