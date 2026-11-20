#pragma once

#include "Character.h"
#include "UIFrameTransition.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <string>
#include <utility>

// Presentation consumes completed game results; it never mutates a character,
// party, inventory or currency. The stage supplies the shared elapsed time.
namespace MaintenanceFeedback
{
inline double SmoothStep(double value)
{
    value = (std::max)(0.0, (std::min)(1.0, value));
    return value * value * (3.0 - 2.0 * value);
}

inline double Fraction(int current, int maximum)
{
    return maximum > 0 ? (std::max)(0.0, (std::min)(1.0, double(current) / maximum)) : 0.0;
}

struct NoticeTiming
{
    double Enter;
    double Duration;
    double HoldEnd;
    double Exit;
    double PulseHoldEnd;
    double PulseExit;
};

class TimedNotice
{
private:
    NoticeTiming Timing;
    UIRegionTransition Transition;
    std::wstring Text;
    double Elapsed;
    double PulseFrom = 0.0;

public:
    explicit TimedNotice(NoticeTiming timing) : Timing(timing), Elapsed(timing.Duration) {}
    void Record(const UIFrameSnapshot& displayed, const RECT& body, std::wstring text, double pulseFrom)
    {
        Transition.Begin(displayed, body, Timing.Enter);
        Text = std::move(text);
        Elapsed = 0.0;
        PulseFrom = pulseFrom;
    }
    void Advance(double seconds)
    {
        if (!std::isfinite(seconds) || seconds <= 0.0) return;
        Elapsed = (std::min)(Timing.Duration, Elapsed + seconds);
        Transition.Advance(seconds);
        if (Elapsed >= Timing.Duration) Text.clear();
    }
    void Reset()
    {
        Text.clear(); Elapsed = Timing.Duration; PulseFrom = 0.0; Transition.Reset();
    }
    void CancelTransition() { Transition.Reset(); }
    bool IsActive() const { return !Text.empty() && Elapsed < Timing.Duration; }
    double Progress() const { return IsActive() ? Elapsed / Timing.Duration : 1.0; }
    double Age() const { return Elapsed; }
    const std::wstring& Message() const { return Text; }
    double ExitOpacity() const
    {
        return IsActive() ? 1.0 - SmoothStep((Elapsed - Timing.HoldEnd) / Timing.Exit) : 0.0;
    }
    double Opacity() const { return SmoothStep(Elapsed / Timing.Enter) * ExitOpacity(); }
    double DrawOpacity(HDC destination, const RECT& body) const
    {
        return Transition.CanComposite(destination, body) ? ExitOpacity() : Opacity();
    }
    double Pulse() const
    {
        return IsActive() ? (PulseFrom + (1.0 - PulseFrom) * SmoothStep(Elapsed / Timing.Enter)) *
            (1.0 - SmoothStep((Elapsed - Timing.PulseHoldEnd) / Timing.PulseExit)) : 0.0;
    }
    bool IsTransitioning() const { return Transition.IsActive(); }
    double TransitionProgress() const { return Transition.Progress(); }
    bool Composite(HDC destination, const RECT& body, const RECT& region)
    {
        return Transition.Composite(destination, body, region);
    }
};

class BodyTransition
{
public:
    enum class Kind { Tab, Character };
private:
    UIRegionTransition Transition;
    Kind CurrentKind = Kind::Tab;
public:
    void Begin(const UIFrameSnapshot& displayed, const RECT& body, Kind kind, double duration)
    {
        CurrentKind = kind; Transition.Begin(displayed, body, duration);
    }
    void Advance(double seconds) { Transition.Advance(seconds); }
    void Reset() { Transition.Reset(); CurrentKind = Kind::Tab; }
    bool IsActive(Kind kind) const { return CurrentKind == kind && Transition.IsActive(); }
    double Progress(Kind kind) const { return IsActive(kind) ? Transition.Progress() : 1.0; }
    bool Composite(HDC destination, const RECT& body)
    {
        RECT region = body;
        if (CurrentKind == Kind::Character) region.top += 52;
        if (UIFrameSnapshot::IsRasterTarget(destination) && Transition.IsActive() && region.bottom <= region.top)
        { Reset(); return false; }
        return Transition.Composite(destination, body, region);
    }
};

class Equipment
{
private:
    const Character* Target = nullptr; // Borrowed identity, valid only for the current stage session.
    CharacterStat Before;
    CharacterStat After;
    double Duration;
    double Elapsed;
    int Part = -1;
    static int Value(const CharacterStat& stat, int index)
    {
        const int values[] = { stat.MaxHP, stat.MaxMP, stat.STR, stat.INT, stat.DEF, stat.SPD };
        return index >= 0 && index < 6 ? values[index] : 0;
    }
    static bool Same(const CharacterStat& before, const CharacterStat& after)
    {
        if (before.CurHP != after.CurHP || before.CurMP != after.CurMP) return false;
        for (int i = 0; i < 6; ++i) if (Value(before, i) != Value(after, i)) return false;
        return true;
    }
public:
    explicit Equipment(double duration) : Duration(duration), Elapsed(duration) {}
    bool IsActiveFor(const Character* target) const { return Target && Target == target && Elapsed < Duration; }
    bool Record(const Character* target, const CharacterStat& before, const CharacterStat& after, int part, bool equipmentChanged)
    {
        if (!equipmentChanged && Same(before, after)) return false;
        if (!IsActiveFor(target)) Before = before;
        After = after; Target = target; Part = part; Elapsed = 0.0;
        return true;
    }
    void Advance(double seconds)
    {
        if (std::isfinite(seconds) && seconds > 0.0) Elapsed = (std::min)(Duration, Elapsed + seconds);
    }
    void Reset() { Target = nullptr; Part = -1; Elapsed = Duration; }
    double Progress() const { return Elapsed / Duration; }
    int StatDelta(int index) const { return Value(After, index) - Value(Before, index); }
    int CurrentDelta(int index) const { return index == 0 ? After.CurHP - Before.CurHP : After.CurMP - Before.CurMP; }
    int ChangedPart() const { return Part; }
    double Strength() const { return SmoothStep(Elapsed / 0.12) * (1.0 - SmoothStep((Elapsed - 1.1) / 0.5)); }
};

class Skill
{
private:
    UIRegionTransition Transition;
    double PulseDuration;
    double PulseElapsed;
    int PulseSlot = -1;
    int PulseID = -1;
    std::wstring Text;
    int MessageFrames = 0;
public:
    explicit Skill(double pulseDuration) : PulseDuration(pulseDuration), PulseElapsed(pulseDuration) {}
    void Begin(const UIFrameSnapshot& displayed, const RECT& body, double duration) { Transition.Begin(displayed, body, duration); }
    void RecordSuccess(int slot, int skillID) { PulseSlot = slot; PulseID = skillID; PulseElapsed = 0.0; }
    void RecordMessage(std::wstring message, int frames) { Text = std::move(message); MessageFrames = frames; }
    void Advance(double seconds)
    {
        if (!std::isfinite(seconds) || seconds <= 0.0) return;
        Transition.Advance(seconds); PulseElapsed = (std::min)(PulseDuration, PulseElapsed + seconds);
    }
    // Preserve the existing skill message's frame-count lifetime during this refactor.
    void AdvanceMessageFrame() { if (MessageFrames > 0) --MessageFrames; }
    void ResetPresentation() { Transition.Reset(); PulseSlot = PulseID = -1; PulseElapsed = PulseDuration; }
    void CancelTransition() { Transition.Reset(); }
    void Reset() { ResetPresentation(); Text.clear(); MessageFrames = 0; }
    bool IsTransitioning() const { return Transition.IsActive(); }
    double Progress() const { return Transition.Progress(); }
    double PulseForSlot(int slot) const { return slot == PulseSlot ? 1.0 - SmoothStep(PulseElapsed / PulseDuration) : 0.0; }
    double PulseForSkill(int skillID) const { return skillID == PulseID ? 1.0 - SmoothStep(PulseElapsed / PulseDuration) : 0.0; }
    bool HasMessage() const { return !Text.empty() && MessageFrames > 0; }
    const std::wstring& Message() const { return Text; }
    bool Composite(HDC destination, const RECT& body, const RECT& region) { return Transition.Composite(destination, body, region); }
};

class Shop
{
private:
    TimedNotice Notice;
    int ItemID = -1;
    int GoldChange = 0;
    int QuantityChange = 0;
    int Shortfall = 0;
    bool Succeeded = false;
public:
    explicit Shop(NoticeTiming timing) : Notice(timing) {}
    void Record(const UIFrameSnapshot& displayed, const RECT& body, int itemID, bool succeeded,
        int goldChange, int quantityChange, int shortfall, std::wstring message)
    {
        const double previous = ItemID == itemID ? Notice.Pulse() : 0.0;
        ItemID = itemID; Succeeded = succeeded; GoldChange = goldChange;
        QuantityChange = quantityChange; Shortfall = shortfall;
        Notice.Record(displayed, body, std::move(message), previous);
    }
    void Advance(double seconds) { Notice.Advance(seconds); }
    void CancelTransition() { Notice.CancelTransition(); }
    void Reset() { Notice.Reset(); ItemID = -1; GoldChange = QuantityChange = Shortfall = 0; Succeeded = false; }
    bool IsActive() const { return Notice.IsActive(); }
    double Progress() const { return Notice.Progress(); }
    double Opacity() const { return Notice.Opacity(); }
    double Pulse() const { return Notice.Pulse(); }
    double DrawOpacity(HDC dc, const RECT& body) const { return Notice.DrawOpacity(dc, body); }
    int FeedbackItemID() const { return ItemID; }
    int GoldDelta() const { return GoldChange; }
    int QuantityDelta() const { return QuantityChange; }
    int MissingGold() const { return Shortfall; }
    bool WasSuccessful() const { return Succeeded; }
    const std::wstring& Message() const { return Notice.Message(); }
    bool Composite(HDC dc, const RECT& body, const RECT& region) { return Notice.Composite(dc, body, region); }
};

class Inventory
{
private:
    TimedNotice Notice;
    const Character* Target = nullptr;
    int ItemID = -1;
    int HPChange = 0;
    int MPChange = 0;
    int QuantityChange = 0;
    int Remaining = 0;
    bool Succeeded = false;
    double BarDuration;
    bool HasBarFrame = false;
    const Character* BarTarget = nullptr;
    std::array<double, 2> Displayed{};
    std::array<double, 2> From{};
public:
    Inventory(NoticeTiming timing, double barDuration) : Notice(timing), BarDuration(barDuration) {}
    void Record(const UIFrameSnapshot& displayed, const RECT& body, const Character* target, int itemID,
        const CharacterStat& before, const CharacterStat& after, int oldCount, int remaining, bool succeeded, std::wstring message)
    {
        const double previous = Target == target && ItemID == itemID ? Notice.Pulse() : 0.0;
        From = HasBarFrame && BarTarget == target ? Displayed :
            std::array<double, 2>{ Fraction(before.CurHP, before.MaxHP), Fraction(before.CurMP, before.MaxMP) };
        Target = target; ItemID = itemID; Succeeded = succeeded;
        HPChange = succeeded ? after.CurHP - before.CurHP : 0;
        MPChange = succeeded ? after.CurMP - before.CurMP : 0;
        QuantityChange = succeeded ? remaining - oldCount : 0; Remaining = remaining;
        Notice.Record(displayed, body, std::move(message), previous);
    }
    void Advance(double seconds) { Notice.Advance(seconds); }
    void CancelTransition() { Notice.CancelTransition(); }
    void Reset()
    {
        Notice.Reset(); Target = BarTarget = nullptr; ItemID = -1;
        HPChange = MPChange = QuantityChange = Remaining = 0; Succeeded = false; HasBarFrame = false;
    }
    bool IsActiveFor(const Character* target) const { return Target == target && Notice.IsActive(); }
    double Progress() const { return Notice.Progress(); }
    double Opacity() const { return Notice.Opacity(); }
    double Pulse() const { return Notice.Pulse(); }
    double DrawOpacity(HDC dc, const RECT& body) const { return Notice.DrawOpacity(dc, body); }
    int FeedbackItemID() const { return ItemID; }
    int HPDelta() const { return HPChange; }
    int MPDelta() const { return MPChange; }
    int QuantityDelta() const { return QuantityChange; }
    int RemainingCount() const { return Remaining; }
    bool WasSuccessful() const { return Succeeded; }
    bool HadNoRecovery() const { return Succeeded && HPChange == 0 && MPChange == 0; }
    const std::wstring& Message() const { return Notice.Message(); }
    std::array<double, 2> BarFractions(const Character* target, const CharacterStat& actual) const
    {
        const double progress = IsActiveFor(target) ? SmoothStep(Notice.Age() / BarDuration) : 1.0;
        return { From[0] + (Fraction(actual.CurHP, actual.MaxHP) - From[0]) * progress,
            From[1] + (Fraction(actual.CurMP, actual.MaxMP) - From[1]) * progress };
    }
    void RecordDisplayedBars(const Character* target, const std::array<double, 2>& fractions)
    {
        Displayed = fractions; BarTarget = target; HasBarFrame = true;
    }
    double DisplayedHPFraction() const { return Displayed[0]; }
    double DisplayedMPFraction() const { return Displayed[1]; }
    bool Composite(HDC dc, const RECT& body, const RECT& region) { return Notice.Composite(dc, body, region); }
};

class Party
{
private:
    TimedNotice Notice;
    int CharacterID = -1;
    int CountChange = 0;
    bool Succeeded = false;
public:
    explicit Party(NoticeTiming timing) : Notice(timing) {}
    void Record(const UIFrameSnapshot& displayed, const RECT& body, int characterID, int countChange, bool succeeded, std::wstring message)
    {
        const double previous = CharacterID == characterID ? Notice.Pulse() : 0.0;
        CharacterID = characterID; CountChange = countChange; Succeeded = succeeded;
        Notice.Record(displayed, body, std::move(message), previous);
    }
    void Advance(double seconds) { Notice.Advance(seconds); }
    void CancelTransition() { Notice.CancelTransition(); }
    void Reset() { Notice.Reset(); CharacterID = -1; CountChange = 0; Succeeded = false; }
    bool IsActive() const { return Notice.IsActive(); }
    double Progress() const { return Notice.Progress(); }
    double Opacity() const { return Notice.Opacity(); }
    double Pulse() const { return Notice.Pulse(); }
    double DrawOpacity(HDC dc, const RECT& body) const { return Notice.DrawOpacity(dc, body); }
    int FeedbackCharacterID() const { return CharacterID; }
    int CountDelta() const { return CountChange; }
    bool WasSuccessful() const { return Succeeded; }
    const std::wstring& Message() const { return Notice.Message(); }
    bool IsTransitioning() const { return Notice.IsTransitioning(); }
    double TransitionProgress() const { return Notice.TransitionProgress(); }
    bool Composite(HDC dc, const RECT& body, const RECT& region) { return Notice.Composite(dc, body, region); }
};
}
