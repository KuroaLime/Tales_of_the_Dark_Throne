#include "../Tower_Triumph/Tower_Triumph/BattleActionTiming.h"
#include "../Tower_Triumph/Tower_Triumph/CharacterMotion.h"

using namespace BattleActionTiming;
constexpr bool NearImpact(float a, float b) { return a > b - 0.00001f && a < b + 0.00001f; }

constexpr bool OneShotAtEveryRate(float impact, int hz)
{
    ImpactClock clock{0.0f, impact, false};
    float before = 0.0f;
    float after = 0.0f;
    int commits = 0;
    bool impacted = false;
    for (int i = 0; i < hz; ++i)
    {
        const auto step = Advance(clock, 1.0f / hz);
        if (!NearImpact(step.beforeImpact + step.afterImpact, 1.0f / hz)) return false;
        if (!impacted) before += step.beforeImpact;
        else after += step.beforeImpact;
        after += step.afterImpact;
        if (step.commit)
        {
            if (impacted || !NearImpact(before, impact)) return false;
            impacted = true;
            ++commits;
        }
    }
    return commits == 1 && NearImpact(before, impact) && NearImpact(after, 1.0f - impact);
}
static_assert(OneShotAtEveryRate(0.25f, 30) && OneShotAtEveryRate(0.25f, 60) && OneShotAtEveryRate(0.25f, 120),
    "Warrior contact must commit once at the same boundary at all update rates");
static_assert(OneShotAtEveryRate(0.30f, 30) && OneShotAtEveryRate(0.30f, 60) && OneShotAtEveryRate(0.30f, 120),
    "Bow/staff release and skills must not deal early or duplicate damage");
static_assert(OneShotAtEveryRate(1.0f / 12, 30) && OneShotAtEveryRate(2.0f / 12, 60) &&
    OneShotAtEveryRate(3.0f / 12, 120), "Monster contact profiles use their own six-pose cadence");

constexpr bool SplitAndCancel()
{
    ImpactClock clock{0.20f, 0.25f, false};
    auto step = Advance(clock, 0.10f);
    if (!step.commit || !NearImpact(step.beforeImpact, 0.05f) || !NearImpact(step.afterImpact, 0.05f)) return false;
    step = Advance(clock, 1.0f);
    if (step.commit || !NearImpact(step.beforeImpact, 1.0f)) return false;
    clock = {}; // The production reset discards pending payload and clock together.
    clock.consumed = true;
    return !Advance(clock, 10.0f).commit;
}
static_assert(SplitAndCancel(), "Crossing updates preserve post-impact time and clearing cannot replay an impact");
static_assert(WarriorAttackFrame * (CharacterMotion::ActionTiming::AttackSeconds / 12) > 0.249f &&
    ArcherAttackFrame == 6 && MageSkillFrame == 6 && WarriorSkillFrame == 6,
    "Authored action contact indices must remain aligned to the inspected artwork");
static_assert(MapAuthoredFrame(6, 18) == 9 && MapAuthoredFrame(5, 12) == 5 && MapAuthoredFrame(5, 0) == 0,
    "Legacy action fallbacks retain a valid corresponding contact frame");
static_assert(MonsterAttackFrame(L"S1_Skeleton.bmp") == 1 && MonsterAttackFrame(L"S1_Leach.bmp") == 3 &&
    MonsterAttackFrame(L"S2_BAngel.bmp") == 3 && MonsterAttackFrame(L"S4_Spear.bmp") == 2 &&
    MonsterAttackFrame(L"S5_Golem.bmp") == 2, "Distinct monster attacks must not all share the same contact pose");
static_assert(NearImpact(8 * BasicImpactFrameSeconds, 0.2f) &&
    NearImpact(8 * SkillEffectFrameSeconds, 8.0f / 15.0f), "Basic and skill effects preserve independent real-time durations");

constexpr bool FlightEventsAtEveryRate(float duration, int hz)
{
    ActionTimeline timeline{{0.0f, 0.30f, false}, duration, true, false, false};
    float elapsed = 0.0f;
    float reactionTime = 0.0f;
    int launches = 0;
    int impacts = 0;
    for (int i = 0; i < hz * 2; ++i)
    {
        float remaining = 1.0f / hz;
        while (remaining > 0.0f)
        {
            const bool alreadyImpacted = timeline.impactCommitted;
            const auto step = AdvanceTimeline(timeline, remaining);
            if (!NearImpact(step.beforeEvent + step.afterEvent, remaining)) return false;
            elapsed += step.beforeEvent;
            if (alreadyImpacted) reactionTime += step.beforeEvent;
            if (step.event == ActionEvent::Launch)
            {
                ++launches;
                if (!NearImpact(elapsed, 0.30f) || timeline.impactCommitted) return false;
            }
            if (step.event == ActionEvent::Impact)
            {
                ++impacts;
                if (!NearImpact(elapsed, 0.30f + duration)) return false;
            }
            remaining = step.afterEvent;
        }
    }
    return launches == 1 && impacts == 1 && NearImpact(reactionTime, 2.0f - 0.30f - duration);
}
// A normal three-tile shot reaches contact after the actor has already finished
// its 0.6s attack. The independent timeline must still deliver the impact once.
constexpr float threeTileArrow = FlightSeconds(ProjectileKind::Arrow, 128.03515f);
static_assert(0.30f + threeTileArrow > CharacterMotion::ActionTiming::AttackSeconds &&
    FlightEventsAtEveryRate(threeTileArrow, 30) && FlightEventsAtEveryRate(threeTileArrow, 60) &&
    FlightEventsAtEveryRate(threeTileArrow, 120),
    "Three-tile arrows must arrive after actor recovery without cancellation or early target reaction");
static_assert(FlightEventsAtEveryRate(FlightSeconds(ProjectileKind::Magic, 300.0f), 30) &&
    FlightEventsAtEveryRate(FlightSeconds(ProjectileKind::Magic, 300.0f), 60),
    "Longer spell flights must retain their action lock until arrival and reaction");
static_assert(NearImpact(FlightSeconds(ProjectileKind::Arrow, 1.0f), 0.16f) &&
    NearImpact(FlightSeconds(ProjectileKind::Magic, 1.0f), 0.20f) &&
    FlightSeconds(ProjectileKind::None, 100.0f) == 0.0f,
    "Close targets retain a visible flight while melee and monsters retain direct contact");

constexpr bool CrossBothBoundaries()
{
    ActionTimeline ranged{{0.0f, 0.30f, false}, 0.20f, true, false, false};
    const auto release = AdvanceTimeline(ranged, 0.80f);
    if (release.event != ActionEvent::Launch || !NearImpact(release.beforeEvent, 0.30f)) return false;
    const auto arrival = AdvanceTimeline(ranged, release.afterEvent);
    if (arrival.event != ActionEvent::Impact || !NearImpact(arrival.beforeEvent, 0.20f) ||
        !NearImpact(arrival.afterEvent, 0.30f)) return false;
    return AdvanceTimeline(ranged, arrival.afterEvent).event == ActionEvent::None;
}
static_assert(CrossBothBoundaries(), "A delayed update must deliver release and arrival once each in the correct order");
constexpr bool DirectContactHasNoLaunch()
{
    ActionTimeline melee{{0.0f, 0.25f, false}, 0.0f, false, false, false};
    const auto event = AdvanceTimeline(melee, 0.3f);
    return event.event == ActionEvent::Impact && !melee.launched && NearImpact(event.beforeEvent, 0.25f);
}
static_assert(DirectContactHasNoLaunch(), "Warrior and monster contact must not acquire projectile delay");
static_assert(DirectionFromRadians(0.0f) == 0 && DirectionFromRadians(1.57079632679f) == 4 &&
    DirectionFromRadians(3.14159265359f) == 8 && DirectionFromRadians(-1.57079632679f) == 12,
    "Atlas columns must use clockwise screen-space directions with east at zero");

constexpr bool ElementVisualSelection()
{
    int seen[15]{};
    int count = 0;
    for (int index = 0; index < ElementVisualCount; ++index)
    {
        const auto element = static_cast<ElementVisual>(index + 1);
        const int contact = ContactEffectID(element);
        const int arrow = ProjectileEffectID(ProjectileKind::Arrow, element);
        const int magic = ProjectileEffectID(ProjectileKind::Magic, element);
        if (!IsContactEffect(contact) || !IsProjectileEffect(arrow) || !IsProjectileEffect(magic) ||
            EffectElement(contact) != element || EffectElement(arrow) != element || EffectElement(magic) != element ||
            NeutralEffectID(contact) != BasicImpactEffectID || NeutralEffectID(arrow) != ArrowProjectileEffectID ||
            NeutralEffectID(magic) != MagicProjectileEffectID || ElementSuffix(element)[0] == L'\0') return false;
        const int ids[] = { contact, arrow, magic };
        for (int id : ids)
        {
            if (id >= 0 || id == ArrowProjectileEffectID || id == MagicProjectileEffectID) return false;
            for (int previous = 0; previous < count; ++previous) if (seen[previous] == id) return false;
            seen[count++] = id;
        }
    }
    return count == 15;
}
static_assert(ElementVisualSelection(),
    "All five skill elements must select matching flight/contact styles without ID collisions or shared fallback ownership");
static_assert(ContactEffectID(ElementVisual::Neutral) == BasicImpactEffectID &&
    ProjectileEffectID(ProjectileKind::Arrow, ElementVisual::Neutral) == ArrowProjectileEffectID &&
    ProjectileEffectID(ProjectileKind::Magic, ElementVisual::Neutral) == MagicProjectileEffectID &&
    ContactEffectID(static_cast<ElementVisual>(99)) == BasicImpactEffectID &&
    ProjectileEffectID(ProjectileKind::Magic, static_cast<ElementVisual>(99)) == MagicProjectileEffectID,
    "Normal attacks and invalid skill elements must preserve neutral visual fallbacks");
static_assert(EffectElement(1101) == ElementVisual::Neutral && NeutralEffectID(1101) == 1101 &&
    !IsContactEffect(1101) && !IsProjectileEffect(1101), "Reserved element visuals cannot reinterpret real skill IDs");
