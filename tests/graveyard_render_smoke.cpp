#include <windows.h>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#include "BattleMapData.h"
#include "GraveyardRenderer.h"

namespace
{
    struct Bitmap
    {
        HBITMAP handle = nullptr;
        explicit Bitmap(HBITMAP value) : handle(value)
        {
            if (!handle) throw std::runtime_error("Bitmap allocation/load failed");
        }
        ~Bitmap() { DeleteObject(handle); }
        Bitmap(const Bitmap&) = delete;
        Bitmap& operator=(const Bitmap&) = delete;
    };

    struct Pixels
    {
        int width;
        int height;
        std::vector<std::uint32_t> values;
    };

    void Require(bool condition, const char* message)
    {
        if (!condition) throw std::runtime_error(message);
    }

    Pixels ReadPixels(HBITMAP bitmap)
    {
        BITMAP object{};
        Require(GetObject(bitmap, sizeof(object), &object) != 0, "GetObject failed");
        Pixels pixels{ object.bmWidth, object.bmHeight, {} };
        pixels.values.resize(static_cast<size_t>(pixels.width) * pixels.height);
        BITMAPINFO info{};
        info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        info.bmiHeader.biWidth = pixels.width;
        info.bmiHeader.biHeight = -pixels.height;
        info.bmiHeader.biPlanes = 1;
        info.bmiHeader.biBitCount = 32;
        info.bmiHeader.biCompression = BI_RGB;
        HDC dc = CreateCompatibleDC(nullptr);
        Require(dc != nullptr, "CreateCompatibleDC failed");
        const int lines = GetDIBits(dc, bitmap, 0, pixels.height, pixels.values.data(), &info, DIB_RGB_COLORS);
        DeleteDC(dc);
        Require(lines == pixels.height, "GetDIBits returned incomplete pixels");
        return pixels;
    }

    void SaveBitmap(const std::filesystem::path& path, const Pixels& pixels)
    {
        BITMAPFILEHEADER file{};
        BITMAPINFOHEADER info{};
        info.biSize = sizeof(info);
        info.biWidth = pixels.width;
        info.biHeight = -pixels.height;
        info.biPlanes = 1;
        info.biBitCount = 32;
        info.biCompression = BI_RGB;
        info.biSizeImage = static_cast<DWORD>(pixels.values.size() * sizeof(std::uint32_t));
        file.bfType = 0x4d42;
        file.bfOffBits = sizeof(file) + sizeof(info);
        file.bfSize = file.bfOffBits + info.biSizeImage;
        std::ofstream stream(path, std::ios::binary);
        Require(static_cast<bool>(stream), "Cannot open output BMP");
        stream.write(reinterpret_cast<const char*>(&file), sizeof(file));
        stream.write(reinterpret_cast<const char*>(&info), sizeof(info));
        stream.write(reinterpret_cast<const char*>(pixels.values.data()), info.biSizeImage);
        Require(static_cast<bool>(stream), "Cannot write output BMP");
    }

    bool InsideProp(int x, int y, const SBattleMapProp& prop, int tile)
    {
        return x >= prop.x * tile && y >= prop.y * tile &&
            x < (prop.x + prop.width) * tile && y < (prop.y + prop.height) * tile;
    }

    bool Blocked(ETileType type)
    {
        return type == ETileType::Water || type == ETileType::Wall ||
            type == ETileType::Building || type == ETileType::Tree ||
            type == ETileType::Grave || type == ETileType::Fence || type == ETileType::SkyVoid;
    }

    void DrawDiagnostics(HBITMAP bitmap, const SBattleMapData& map)
    {
        HDC dc = CreateCompatibleDC(nullptr);
        HGDIOBJ oldBitmap = SelectObject(dc, bitmap);
        HPEN grid = CreatePen(PS_SOLID, 1, RGB(130, 145, 165));
        HGDIOBJ oldPen = SelectObject(dc, grid);
        HGDIOBJ oldBrush = SelectObject(dc, GetStockObject(NULL_BRUSH));
        for (int x = 0; x <= map.width; ++x)
        {
            MoveToEx(dc, x * map.tileSize, 0, nullptr);
            LineTo(dc, x * map.tileSize, map.height * map.tileSize);
        }
        for (int y = 0; y <= map.height; ++y)
        {
            MoveToEx(dc, 0, y * map.tileSize, nullptr);
            LineTo(dc, map.width * map.tileSize, y * map.tileSize);
        }
        HPEN blocked = CreatePen(PS_SOLID, 1, RGB(240, 80, 100));
        SelectObject(dc, blocked);
        for (int y = 0; y < map.height; ++y)
            for (int x = 0; x < map.width; ++x)
                if (Blocked(map.tiles[y][x]))
                {
                    int cx = x * map.tileSize + map.tileSize / 2;
                    int cy = y * map.tileSize + map.tileSize / 2;
                    MoveToEx(dc, cx - 5, cy - 5, nullptr); LineTo(dc, cx + 5, cy + 5);
                    MoveToEx(dc, cx + 5, cy - 5, nullptr); LineTo(dc, cx - 5, cy + 5);
                }
        for (int enemy = 0; enemy < 2; ++enemy)
        {
            const auto& starts = enemy ? map.enemyStarts : map.playerStarts;
            HPEN marker = CreatePen(PS_SOLID, 3, enemy ? RGB(255, 120, 100) : RGB(70, 230, 255));
            SelectObject(dc, marker);
            for (const auto& start : starts)
            {
                const int x = start.x * map.tileSize + map.tileSize / 2;
                const int y = start.y * map.tileSize + map.tileSize * 3 / 4;
                Rectangle(dc, x - 8, y - 6, x + 8, y + 6);
            }
            SelectObject(dc, oldPen);
            DeleteObject(marker);
        }
        SelectObject(dc, oldBrush);
        SelectObject(dc, oldPen);
        SelectObject(dc, oldBitmap);
        DeleteObject(grid);
        DeleteObject(blocked);
        DeleteDC(dc);
    }
}

int wmain(int argc, wchar_t** argv)
{
    try
    {
        Require(argc == 3, "Usage: graveyard_render_smoke.exe <game-source-directory> <output-directory>");
        const std::filesystem::path source(argv[1]);
        const std::filesystem::path output(argv[2]);
        std::filesystem::create_directories(output);
        const SBattleMapData map = BattleMapData::Load(EBattleMapId::Stage01);
        Require(map.width == 33 && map.height == 18 && map.tileSize == 48, "Unexpected production map dimensions");
        Require(!map.allowElementalWaterTraversal, "Production graveyard allows river traversal");
        Require(map.gridAlignedBackground, "Production graveyard does not select its painted scene");
        Require(!map.backgroundFilePath.empty(), "Painted scene path is missing");
        const auto scenePath = source / map.backgroundFilePath;
        Bitmap scene(static_cast<HBITMAP>(LoadImageW(nullptr, scenePath.c_str(), IMAGE_BITMAP,
            0, 0, LR_LOADFROMFILE | LR_CREATEDIBSECTION)));
        const Pixels painted = ReadPixels(scene.handle);
        Require(painted.width == map.width * map.tileSize &&
            painted.height == map.height * map.tileSize,
            "Painted scene dimensions differ from the collision grid");
        SaveBitmap(output / "graveyard-painted-background.bmp", painted);
        DrawDiagnostics(scene.handle, map);
        SaveBitmap(output / "graveyard-painted-debug.bmp", ReadPixels(scene.handle));

        // Exercise the procedural fallback separately. These assertions prove
        // fallback containment, not the painted artwork's obstacle alignment.
        const auto atlasPath = source / map.tileAtlasFilePath;
        Bitmap atlas(static_cast<HBITMAP>(LoadImageW(nullptr, atlasPath.c_str(), IMAGE_BITMAP,
            0, 0, LR_LOADFROMFILE | LR_CREATEDIBSECTION)));
        Bitmap rendered(GraveyardRenderer::CreateBackground(map, atlas.handle));
        const Pixels image = ReadPixels(rendered.handle);
        Require(image.width == 1584 && image.height == 864, "Rendered background does not match the collision grid");
        SaveBitmap(output / "graveyard-atlas-background.bmp", image);

        SBattleMapData bareMap = map;
        bareMap.props.clear();
        Bitmap bare(GraveyardRenderer::CreateBackground(bareMap, atlas.handle));
        const Pixels noProps = ReadPixels(bare.handle);
        std::vector<size_t> changedPerProp(map.props.size(), 0);
        size_t changedPixels = 0;
        for (int y = 0; y < image.height; ++y)
            for (int x = 0; x < image.width; ++x)
            {
                const size_t offset = static_cast<size_t>(y) * image.width + x;
                if ((image.values[offset] & 0xffffff) == (noProps.values[offset] & 0xffffff)) continue;
                bool inside = false;
                for (size_t i = 0; i < map.props.size(); ++i)
                    if (InsideProp(x, y, map.props[i], map.tileSize))
                    {
                        inside = true;
                        ++changedPerProp[i];
                    }
                Require(inside, "Prop drawing changes a pixel outside its collision bounds");
                Require(Blocked(map.tiles[y / map.tileSize][x / map.tileSize]), "Prop drawing changes a passable cell");
                ++changedPixels;
            }
        for (size_t changes : changedPerProp) Require(changes > 0, "A declared prop did not render");

        SBattleMapData dryMap = map;
        for (auto& row : dryMap.tiles)
            for (auto& tile : row)
                if (tile == ETileType::Water) tile = ETileType::Normal;
        Bitmap dry(GraveyardRenderer::CreateBackground(dryMap, atlas.handle));
        const Pixels noWater = ReadPixels(dry.handle);
        size_t changedWater = 0;
        for (int y = 0; y < image.height; ++y)
            for (int x = 0; x < image.width; ++x)
            {
                const size_t offset = static_cast<size_t>(y) * image.width + x;
                if ((image.values[offset] & 0xffffff) == (noWater.values[offset] & 0xffffff)) continue;
                Require(map.tiles[y / map.tileSize][x / map.tileSize] == ETileType::Water,
                    "River/bank drawing spills outside impassable water cells");
                ++changedWater;
            }
        Require(changedWater > 0, "River did not render");

        Bitmap fallback(GraveyardRenderer::CreateBackground(map, nullptr));
        const Pixels fallbackImage = ReadPixels(fallback.handle);
        Require(fallbackImage.width == image.width && fallbackImage.height == image.height,
            "Missing-atlas fallback dimensions differ");
        std::vector<std::uint32_t> fallbackTerrainColors;
        for (const ETileType type : { ETileType::Road, ETileType::Bridge, ETileType::Water })
        {
            bool found = false;
            for (int y = 0; y < map.height && !found; ++y)
                for (int x = 0; x < map.width && !found; ++x)
                    if (map.tiles[y][x] == type)
                    {
                        const size_t center = static_cast<size_t>(y * map.tileSize + map.tileSize / 2)
                            * image.width + x * map.tileSize + map.tileSize / 2;
                        const std::uint32_t color = fallbackImage.values[center] & 0xffffff;
                        Require(color != 0, "Fallback terrain is blank");
                        for (auto previous : fallbackTerrainColors)
                            Require(previous != color, "Fallback does not distinguish road, bridge and river");
                        fallbackTerrainColors.push_back(color);
                        found = true;
                    }
            Require(found, "Production map is missing road, bridge or river terrain");
        }
        SaveBitmap(output / "graveyard-fallback.bmp", fallbackImage);
        DrawDiagnostics(rendered.handle, map);
        SaveBitmap(output / "graveyard-atlas-debug.bmp", ReadPixels(rendered.handle));
        SBattleMapData malformed = map;
        malformed.tiles[0].pop_back();
        Require(GraveyardRenderer::CreateBackground(malformed, atlas.handle) == nullptr,
            "Renderer accepts a malformed tile row");

        std::cout << "PASS: painted scene loads at exact collision-grid dimensions\n"
            << "PASS: atlas renderer, 1584x864 background and missing-atlas fallback\n"
            << "PASS: " << changedPixels << " changed prop pixels stay inside blocked prop bounds\n"
            << "PASS: " << changedWater << " river/bank pixels stay inside water cells\n"
            << "PASS: malformed tile dimensions rejected\n";
        std::wcout << L"Images: " << output.wstring() << L"\n";
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
