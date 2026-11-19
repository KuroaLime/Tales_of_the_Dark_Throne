#pragma once

// One logical cell per character, aligned to painted basalt and foot paths.
// ~ lava, . floor, + road, = basalt causeway, # rock/wall, B building.
// Ruins reserve their complete visible silhouette; all lava is impassable.
namespace LavaMapLayout
{
    constexpr int Width = 33;
    constexpr int Height = 18;
    constexpr int TileSize = 48;

    constexpr const char* Rows[Height] = {
        "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~",
        "~~~~~~~~~~~~~~~~~~~~~~~~~BBBBB~~~",
        "~~~BBBB........~~~..##..~BBBBB~~~",
        "~~~BBBB........===.......BBBBB~~~",
        "~~~BBBB........===.......BBBBB~~~",
        "~~~BBBB.....#########....BBBBB~~~",
        "~~~......###~~~~~~~~~.........~~~",
        "~~~......###~~~~~~~~~.........~~~",
        "~~~..##...##~~~~~~~~~...##....~~~",
        "~~~..##...##~~~~~~~~~...##....~~~",
        "~~~..##...##~~~~~~~~~...##....~~~",
        "~~~B.......#~~~~~~~~~.........~~~",
        "~~~B............===...........~~~",
        "~~~B............===......#####~~~",
        "~~~B.....####...===......#####~~~",
        "~~~~~~~~~####~~~~~~~~~~~~~~~~~~~~",
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

    static_assert(HasValidRows(), "Lava layout must contain exactly 33 x 18 cells");
}
