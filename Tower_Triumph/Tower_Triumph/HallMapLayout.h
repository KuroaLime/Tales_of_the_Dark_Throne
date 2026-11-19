#pragma once

// One logical cell per character, aligned to the royal hall painting.
// . stone floor, + flush floor medallion, # wall/column, B architecture.
// Interior pockets reserve complete prop silhouettes, with broad routes
// above and below them and no isolated or one-cell walking corridors.
namespace HallMapLayout
{
    constexpr int Width = 33;
    constexpr int Height = 18;
    constexpr int TileSize = 48;

    constexpr const char* Rows[Height] = {
        "#################################",
        "###BBBBB#####BBBBBBB#####BBBBB###",
        "###BBBBB#####BBBBBBB#####BBBBB###",
        "###BBBBB#####BBBBBBB#####BBBBB###",
        "###...........................###",
        "###...........................###",
        "###.................BBB.......###",
        "###......####.+++++.BBB....BBB###",
        "###BBB...####.+++++.BBB....BBB###",
        "###BBB...####.+++++........BBB###",
        "###BBB.....................BBB###",
        "###BBB.........####........BBB###",
        "###BBB.........####........BBB###",
        "###BBB.....................BBB###",
        "###BBB.....................BBB###",
        "#################################",
        "#################################",
        "#################################"
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

    static_assert(HasValidRows(), "Hall layout must contain exactly 33 x 18 cells");
}
