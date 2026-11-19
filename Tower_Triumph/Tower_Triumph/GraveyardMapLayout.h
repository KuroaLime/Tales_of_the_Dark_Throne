#pragma once

// The battle grid is the source of truth for the graveyard background.
// Every character is one complete 48px cell, with no image-space coordinates.
// . floor, + road, = bridge, ~ water, # wall, B building, T tree, G grave,
// C cursed ground and M magic hazard. The scene guide uses these exact bounds.
namespace GraveyardMapLayout
{
    constexpr int Width = 33;
    constexpr int Height = 18;
    constexpr int TileSize = 48;

    constexpr const char* Rows[Height] = {
        "#################################",
        "#TTTGGG..~~~~TTTGGGG#..TTTBBBBTT#",
        "#TTTGGG..~~~~TTTGGGG...TTTBBBBTT#",
        "#TTTGGG..~~~~GGGGGGG...BBBBBBBTT#",
        "#TTGGGG..~~~~#####GG...BBBBBBBBB#",
        "#TTGGGG..~~~.........++BBBBBBBBB#",
        "#GGG.....===++++++++++BBBBBBBBBB#",
        "###G+++++===+++#GGGG+++BBBBBBBBB#",
        "#++++++++===+++##GGG++++++++++..#",
        "#+++++++~~~~.....+++++++++......#",
        "##G+++++~~~~.....++++##GGG#GTTTT#",
        "#GGBBB++~~~~..#GG++++#GGGG#GTTTT#",
        "#GGBBB++~~~~..#GG....#GGG##GTTTT#",
        "#TTBBB++====++##G++#####GGGGGGGG#",
        "#TTTGG++====++++++++GGGGGTTTTTTT#",
        "#TTTGG++====++++++++GGGGGTTTTTTT#",
        "#TTTGG++~~~~........GGGGGTTTTTTT#",
        "#################################"
    };

    constexpr bool HasValidRows()
    {
        for (int y = 0; y < Height; ++y)
        {
            if (Rows[y] == nullptr)
            {
                return false;
            }

            for (int x = 0; x < Width; ++x)
            {
                if (Rows[y][x] == '\0')
                {
                    return false;
                }
            }

            if (Rows[y][Width] != '\0')
            {
                return false;
            }
        }
        return true;
    }

    static_assert(HasValidRows(), "Graveyard layout must contain exactly 33 x 18 cells");
}
