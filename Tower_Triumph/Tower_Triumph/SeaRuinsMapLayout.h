#pragma once

// One logical cell per character, aligned to painted dry ground and foot paths.
// ~ water, . floor, + road, = bridge, # wall/ruin, B building, T vegetation.
// Ruins, coastal boulders and cliff lips reserve the painted foot clearance.
// All water is blocked; bridge banks keep at least two adjacent safe cells.
namespace SeaRuinsMapLayout
{
    constexpr int Width = 33;
    constexpr int Height = 18;
    constexpr int TileSize = 48;

    constexpr const char* Rows[Height] = {
        "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~",
        "~~~~~~~~~~~~~~~~~~~~~~~~~BBBBBB~~",
        "~~~BBBB..#~~~..BBBB..~~~~BBBBBB~~",
        "~~~BBBB..#~~~..BBBB..~~~~BBBBBB~~",
        "~~~BBBB...~~~..BBBB..~~..BBBBBB~~",
        "~~.........==........~~..BBBBBB~~",
        "~~.........==........==.......#~~",
        "~~#.......~~~....#...==.......#~~",
        "~~#...##..~~~...###..~~...#...#~~",
        "~~#..###..~~~...#####~~...##..#~~",
        "~~#.....##~~~...#####~~..###..#~~",
        "~~#.....##.==........~~.......#~~",
        "~~TTTT.....==........==.....TTT~~",
        "~~TTTT.....==#.......==.....TTT~~",
        "~~TTTT..~~~~~##....TT~~.....TTT~~",
        "~~~TTT~~~~~~~##~~~~TT~~~~~~~TTT~~",
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

    static_assert(HasValidRows(), "Sea ruins layout must contain exactly 33 x 18 cells");
}
