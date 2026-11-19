#pragma once

// Collision cells aligned to painted ground and continuous foot placement.
// Each character describes a logical 48px cell; painted rims and vertical
// facades are excluded from the usable foot area rather than stretched to it.
// ~ sky void, . floor, + road, = bridge, # wall/statue, B building,
// T planted bed. All upright ornaments occupy blocked terrain.
namespace CelestialMapLayout
{
    constexpr int Width = 33;
    constexpr int Height = 18;
    constexpr int TileSize = 48;

    constexpr const char* Rows[Height] = {
        "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~",
        "~~~~~~~~~~~~~~~~~~~~~~~~~BBBBBBB~",
        "~~~~~~~~~~~~~...###TTT~~~BBBBBBB~",
        "~~~###..#~~~~...###TTT~~~BBBBBBB~",
        "~~~###..#~~~~...######~~~BBBBBBB~",
        "~~####...#~~~......###~~~BBBBBBB~",
        "~~#.......===......###~~~BBBBBBB~",
        "~~#.......===...##....===......#~",
        "~~........===...##....===......#~",
        "~~.......#~~~...#.....~~~......#~",
        "~~TTT....#~~~##~~==~~~~~~......#~",
        "~~TTT.....==~~~~~==~~~~~~.....TT~",
        "~~TTT.....==....~==~~~~~~.....TT~",
        "~~TTT#....==.......###===.....TT~",
        "~~~~~~~~~~~~~TTT......===....#TT~",
        "~~~~~~~~~~~~~TTT......===....#~~~",
        "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~",
        "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~"
    };

    constexpr bool HasValidRows()
    {
        for (int y = 0; y < Height; ++y)
        {
            if (Rows[y] == nullptr) return false;
            for (int x = 0; x < Width; ++x)
                if (Rows[y][x] == '\0') return false;
            if (Rows[y][Width] != '\0') return false;
        }
        return true;
    }

    static_assert(HasValidRows(), "Celestial layout must contain exactly 33 x 18 cells");
}
