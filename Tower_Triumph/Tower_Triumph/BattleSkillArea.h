#pragma once

#include <vector>
#include <cmath>
#include "BattleActionTiming.h"

// One geometry snapshot drives selection, damage and floor effects. Coordinates
// are grid cells; arrivalSeconds uses the action's command-start clock.
namespace BattleSkillArea
{
    enum class Kind { Single, Splash1, Splash2, Line };
    struct Point { int x = 0; int y = 0; };
    struct Cell { int x = 0; int y = 0; float arrivalSeconds = 0.0f; };
    constexpr float SpreadTileSeconds = 0.09f;
    constexpr int AreaDamageFontSize = 26;
    constexpr int SingleDamageFontSize = 34;
    constexpr float EventEpsilon = 0.000001f;
    constexpr int Abs(int value) { return value < 0 ? -value : value; }
    constexpr int Distance(Point a, Point b) { return Abs(a.x - b.x) + Abs(a.y - b.y); }
    constexpr bool Inside(Point p, int width, int height)
    { return p.x >= 0 && p.y >= 0 && p.x < width && p.y < height; }

    template<class Emit>
    constexpr void ForEachCell(Kind kind, Point caster, Point center, int range, int width, int height, Emit emit)
    {
        if (!Inside(center, width, height)) return;
        if (kind == Kind::Single) { emit(Cell{ center.x, center.y, 0.0f }); return; }
        if (kind == Kind::Line)
        {
            if ((caster.x != center.x && caster.y != center.y) || Distance(caster, center) == 0) return;
            const int dx = center.x == caster.x ? 0 : (center.x > caster.x ? 1 : -1);
            const int dy = center.y == caster.y ? 0 : (center.y > caster.y ? 1 : -1);
            for (int step = 1; step <= range; ++step)
            {
                const Point p{ caster.x + dx * step, caster.y + dy * step };
                if (!Inside(p, width, height)) break;
                emit(Cell{ p.x, p.y, 0.0f });
            }
            return;
        }
        const int radius = kind == Kind::Splash2 ? 2 : 1;
        for (int ring = 0; ring <= radius; ++ring)
            for (int y = center.y - ring; y <= center.y + ring; ++y)
                for (int x = center.x - ring; x <= center.x + ring; ++x)
                    if (Inside({ x, y }, width, height) && Distance({ x, y }, center) == ring)
                        emit(Cell{ x, y, 0.0f });
    }

    inline std::vector<Cell> BuildCells(Kind kind, Point caster, Point center, int range, int width, int height)
    {
        std::vector<Cell> cells;
        ForEachCell(kind, caster, center, range, width, height, [&](Cell cell) { cells.push_back(cell); });
        return cells;
    }
    constexpr float SplashArrival(float release, float flight, int ring)
    { return release + flight + ring * SpreadTileSeconds; }
    constexpr float MeleeLineArrival(float contact, int zeroBasedCell)
    { return contact + zeroBasedCell * SpreadTileSeconds; }
    struct LinePath
    {
        float startX = 0.0f, startY = 0.0f;
        float firstX = 0.0f, firstY = 0.0f;
        float endX = 0.0f, endY = 0.0f;
        float entryLength = 0.0f, spineLength = 0.0f;
    };
    struct PathSample { float x, y, dx, dy; };
    constexpr float TotalLength(const LinePath& path) { return path.entryLength + path.spineLength; }
    inline LinePath MakeLinePath(float startX, float startY, float firstX, float firstY, float endX, float endY)
    {
        const float dx = firstX - startX, dy = firstY - startY;
        const float spineX = endX - firstX, spineY = endY - firstY;
        return { startX, startY, firstX, firstY, endX, endY,
            std::sqrt(dx * dx + dy * dy), std::sqrt(spineX * spineX + spineY * spineY) };
    }
    constexpr float LineCellProgress(const LinePath& path, int zeroBasedCell, float tileSize)
    {
        const float total = TotalLength(path);
        if (!(total > 0.0f)) return 1.0f;
        const float distance = path.entryLength + zeroBasedCell * tileSize;
        return distance < total ? distance / total : 1.0f;
    }
    constexpr float LineArrival(float release, float flight, const LinePath& path, int zeroBasedCell, float tileSize)
    { return release + flight * LineCellProgress(path, zeroBasedCell, tileSize); }
    // Enter the first cell at its body center, then follow the centers of the
    // entire row/column. A weapon's lateral offset must not make early hits miss.
    constexpr PathSample SampleLinePath(const LinePath& path, float progress)
    {
        const float clamped = progress < 0.0f ? 0.0f : (progress > 1.0f ? 1.0f : progress);
        const float distance = TotalLength(path) * clamped;
        const bool entering = distance < path.entryLength || !(path.spineLength > 0.0f);
        const float fromX = entering ? path.startX : path.firstX;
        const float fromY = entering ? path.startY : path.firstY;
        const float dx = entering ? path.firstX - path.startX : path.endX - path.firstX;
        const float dy = entering ? path.firstY - path.startY : path.endY - path.firstY;
        const float length = entering ? path.entryLength : path.spineLength;
        const float local = distance - (entering ? 0.0f : path.entryLength);
        const float fraction = length > 0.0f ? local / length : 1.0f;
        return { fromX + dx * fraction, fromY + dy * fraction, dx, dy };
    }
    constexpr bool IsDue(float elapsed, float arrival) { return elapsed + EventEpsilon >= arrival; }
    // Preserve the complete caller remainder even when a cell boundary is
    // earlier than the next release/end event. This is shared by runtime/tests.
    constexpr BattleActionTiming::ActionStep AdvanceTimeline(BattleActionTiming::ActionTimeline& timeline,
        float& actionElapsed, float seconds, float nextCellArrival = -1.0f)
    {
        float slice = seconds;
        const float untilCell = nextCellArrival - actionElapsed;
        if (nextCellArrival >= 0.0f && untilCell > EventEpsilon && untilCell < slice) slice = untilCell;
        auto step = BattleActionTiming::AdvanceTimeline(timeline, slice);
        actionElapsed += step.beforeEvent;
        step.afterEvent = seconds - step.beforeEvent;
        return step;
    }
    constexpr int SecondaryDamagePercent(Kind kind)
    { return kind == Kind::Splash1 ? 65 : kind == Kind::Splash2 ? 75 : kind == Kind::Line ? 85 : 100; }
}
