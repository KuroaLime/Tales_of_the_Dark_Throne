#include "../Tower_Triumph/Tower_Triumph/BattleSkillArea.h"

namespace Area = BattleSkillArea;
namespace Timing = BattleActionTiming;
constexpr bool Near(float a, float b) { return a > b - 0.00001f && a < b + 0.00001f; }
constexpr bool NearPoint(float a, float b) { return a > b - 0.0001f && a < b + 0.0001f; }
constexpr float FixtureSqrt(float value)
{
    if (!(value > 0.0f)) return 0.0f;
    float result = value;
    for (int i = 0; i < 24; ++i) result = 0.5f * (result + value / result);
    return result;
}
struct Cells
{
    Area::Cell values[64]{};
    int size = 0;
};
constexpr Cells Make(Area::Kind kind, Area::Point caster, Area::Point center, int range, int w = 20, int h = 20)
{
    Cells cells;
    Area::ForEachCell(kind, caster, center, range, w, h, [&](Area::Cell cell) { cells.values[cells.size++] = cell; });
    return cells;
}
constexpr bool Geometry()
{
    auto small = Make(Area::Kind::Splash1, { 1, 4 }, { 5, 4 }, 4);
    auto wide = Make(Area::Kind::Splash2, { 1, 4 }, { 5, 4 }, 4);
    if (small.size != 5 || wide.size != 13 || small.values[0].x != 5 || small.values[0].y != 4) return false;
    int previousRing = -1;
    for (int i = 0; i < wide.size; ++i)
    {
        const auto& cell = wide.values[i];
        const int ring = Area::Distance({ cell.x, cell.y }, { 5, 4 });
        if (ring < previousRing || ring > 2) return false;
        previousRing = ring;
        for (int j = 0; j < i; ++j)
            if (wide.values[j].x == cell.x && wide.values[j].y == cell.y) return false;
    }
    if (Make(Area::Kind::Splash2, { 3, 0 }, { 0, 0 }, 4).size != 6) return false;
    const auto line = Make(Area::Kind::Line, { 11, 6 }, { 13, 6 }, 5);
    if (line.size != 5 || line.values[0].x != 12 || line.values[4].x != 16) return false;
    for (int i = 0; i < line.size; ++i) if (line.values[i].y != 6 || line.values[i].x <= 11 || line.values[i].x > 16) return false;
    if (Make(Area::Kind::Line, { 1, 6 }, { 0, 6 }, 5).size != 1) return false;
    if (Make(Area::Kind::Line, { 11, 6 }, { 12, 7 }, 5).size != 0) return false;
    if (Make(Area::Kind::Line, { 11, 6 }, { 11, 6 }, 5).size != 0) return false;
    const auto up = Make(Area::Kind::Line, { 4, 3 }, { 4, 1 }, 5);
    return up.size == 3 && up.values[0].y == 2 && up.values[2].y == 0;
}
static_assert(Geometry(), "Preview/target/FX geometry must share 5/13 Manhattan cells and forward clipped lines only");

// Exercise the production boundary splitter with multiple event boundaries in
// one update, and every ring cell sharing exactly one activation time.
constexpr bool ScheduledCells(Area::Kind kind, bool projectile, float updateSeconds)
{
    auto cells = Make(kind, { 11, 6 }, { 13, 6 }, 5);
    constexpr float release = 0.30f;
    const float flight = projectile ? 0.70f : 0.0f;
    const Area::LinePath linePath{568.0f, 15.0f, 600.0f, 12.0f, 792.0f, 12.0f, FixtureSqrt(1033.0f), 192.0f};
    for (int i = 0; i < cells.size; ++i)
    {
        auto& cell = cells.values[i];
        cell.arrivalSeconds = kind == Area::Kind::Line ?
            (projectile ? Area::LineArrival(release, flight, linePath, i, 48.0f) :
                Area::MeleeLineArrival(release, i)) :
            Area::SplashArrival(release, flight, Area::Distance({ cell.x, cell.y }, { 13, 6 }));
    }
    Timing::ActionTimeline timeline{ {0.0f, release, false}, flight, projectile, false, false };
    float elapsed = 0.0f;
    int next = 0, launches = 0, ends = 0, activations[64]{};
    for (int frame = 0; frame < 300 && elapsed < 2.0f; ++frame)
    {
        float remaining = updateSeconds;
        int iterations = 0;
        while (remaining > 0.0f)
        {
            if (++iterations > 32) return false;
            const auto step = Area::AdvanceTimeline(timeline, elapsed, remaining,
                next < cells.size ? cells.values[next].arrivalSeconds : -1.0f);
            if (step.event == Timing::ActionEvent::Launch) ++launches;
            if (step.event == Timing::ActionEvent::Impact) ++ends;
            while (next < cells.size && Area::IsDue(elapsed, cells.values[next].arrivalSeconds))
            {
                if ((!timeline.launched && !timeline.impactCommitted) || !Near(elapsed, cells.values[next].arrivalSeconds)) return false;
                ++activations[next++];
            }
            remaining = step.afterEvent;
            if (step.beforeEvent == 0.0f && step.event == Timing::ActionEvent::None) return false;
        }
    }
    for (int i = 0; i < cells.size; ++i) if (activations[i] != 1) return false;
    return next == cells.size && ends == 1 && launches == (projectile ? 1 : 0);
}
static_assert(ScheduledCells(Area::Kind::Splash2, true, 1.5f) &&
    ScheduledCells(Area::Kind::Line, true, 1.5f) && ScheduledCells(Area::Kind::Line, false, 1.5f),
    "One slow update must process release, every reached ring/cell and endpoint exactly once in order");
static_assert(ScheduledCells(Area::Kind::Splash1, true, 1.0f / 30) &&
    ScheduledCells(Area::Kind::Splash2, true, 1.0f / 60) &&
    ScheduledCells(Area::Kind::Line, true, 1.0f / 120) && ScheduledCells(Area::Kind::Line, false, 1.0f / 60),
    "Cell impact times must be independent of update rate");
constexpr bool LineBodyCenters()
{
    constexpr int directions[4][2] = { {1, 0}, {0, 1}, {-1, 0}, {0, -1} };
    for (int character = 0; character < 2; ++character)
        for (const auto& direction : directions)
            for (int facing : {-1, 1})
                for (int count : {1, 2, 5})
                {
                    const float startX = facing * (character == 0 ? Timing::ArrowLaunchX : Timing::MagicLaunchX);
                    const float startY = (character == 0 ? Timing::ArrowLaunchY : Timing::MagicLaunchY) + Timing::ProjectileTargetHeight;
                    const float firstX = direction[0] * 48.0f, firstY = direction[1] * 48.0f;
                    const float dx = firstX - startX, dy = firstY - startY;
                    const Area::LinePath path{startX, startY, firstX, firstY,
                        firstX * count, firstY * count, FixtureSqrt(dx * dx + dy * dy), (count - 1) * 48.0f};
                    const float flight = Timing::FlightSeconds(character == 0 ? Timing::ProjectileKind::Arrow : Timing::ProjectileKind::Magic,
                        Area::TotalLength(path));
                    const auto start = Area::SampleLinePath(path, 0.0f);
                    if (!NearPoint(start.x, startX) || !NearPoint(start.y, startY)) return false;
                    const auto entryMid = Area::SampleLinePath(path, path.entryLength * 0.5f / Area::TotalLength(path));
                    if (!NearPoint(entryMid.x, (startX + firstX) * 0.5f) || !NearPoint(entryMid.y, (startY + firstY) * 0.5f)) return false;
                    float previousArrival = 0.0f;
                    for (int cell = 0; cell < count; ++cell)
                    {
                        const float arrival = Area::LineArrival(0.3f, flight, path, cell, 48.0f);
                        const auto sample = Area::SampleLinePath(path, (arrival - 0.3f) / flight);
                        if (!NearPoint(sample.x, firstX * (cell + 1)) || !NearPoint(sample.y, firstY * (cell + 1)) ||
                            arrival <= previousArrival) return false;
                        previousArrival = arrival;
                    }
                    if (!Near(previousArrival, 0.3f + flight)) return false;
                    const auto endpoint = Area::SampleLinePath(path, 2.0f);
                    if (!NearPoint(endpoint.x, path.endX) || !NearPoint(endpoint.y, path.endY)) return false;
                }
    return true;
}
static_assert(LineBodyCenters(),
    "Arrows and magic must enter the first body then cross every body center in four directions, either facing, close and clipped ranges");
constexpr bool ClippedLineBodyCenters()
{
    constexpr int directions[4][2] = { {1, 0}, {0, 1}, {-1, 0}, {0, -1} };
    for (const auto& direction : directions)
    {
        const auto cells = Make(Area::Kind::Line, { 2, 2 }, { 2 + direction[0], 2 + direction[1] }, 5, 4, 4);
        if (cells.size < 1 || cells.size > 2) return false;
        const float startX = 147.0f, startY = 101.0f;
        const float firstX = cells.values[0].x * 48.0f + 24.0f, firstY = cells.values[0].y * 48.0f + 12.0f;
        const auto& last = cells.values[cells.size - 1];
        const float dx = firstX - startX, dy = firstY - startY;
        const Area::LinePath path{ startX, startY, firstX, firstY, last.x * 48.0f + 24.0f, last.y * 48.0f + 12.0f,
            FixtureSqrt(dx * dx + dy * dy), (cells.size - 1) * 48.0f };
        for (int i = 0; i < cells.size; ++i)
        {
            const auto sample = Area::SampleLinePath(path, Area::LineCellProgress(path, i, 48.0f));
            if (!NearPoint(sample.x, cells.values[i].x * 48.0f + 24.0f) ||
                !NearPoint(sample.y, cells.values[i].y * 48.0f + 12.0f)) return false;
        }
        if (Area::LineCellProgress(path, cells.size - 1, 48.0f) != 1.0f) return false;
    }
    return true;
}
static_assert(ClippedLineBodyCenters(), "Map clipping must shorten the path to the last actual cell in all four directions");
static_assert(Timing::FlightSeconds(Timing::ProjectileKind::Arrow, 32.0f) == Timing::ArrowMinimumSeconds &&
    Timing::FlightSeconds(Timing::ProjectileKind::Magic, 22.0f) == Timing::MagicMinimumSeconds,
    "A one-cell line retains the existing minimum visible flight duration");
static_assert(Area::SecondaryDamagePercent(Area::Kind::Splash1) == 65 &&
    Area::SecondaryDamagePercent(Area::Kind::Splash2) == 75 &&
    Area::SecondaryDamagePercent(Area::Kind::Line) == 85 &&
    Area::SecondaryDamagePercent(Area::Kind::Single) == 100,
    "Area propagation must not rebalance primary or secondary damage");
static_assert(Area::AreaDamageFontSize == 26 && Area::SingleDamageFontSize == 34,
    "Crowded AoE numbers use smaller type while single hits preserve their old size");
