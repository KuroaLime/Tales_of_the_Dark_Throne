#pragma once

#include "BattleMapData.h"

// Compose a geometry-safe fallback from collision tiles at world resolution.
// A painted, grid-aligned scene can be loaded by Battle_Stage in preference.
namespace GraveyardRenderer
{
    HBITMAP CreateBackground(const SBattleMapData& map, HBITMAP atlas);

    // Feet remain inside their tile even when the upper body overlaps scenery.
    constexpr int FootXNumerator = 1;
    constexpr int FootXDenominator = 2;
    constexpr int FootYNumerator = 3;
    constexpr int FootYDenominator = 4;
}
