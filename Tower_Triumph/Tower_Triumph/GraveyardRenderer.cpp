#include "GraveyardRenderer.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

namespace
{
    using Pixel = std::uint32_t; // DIB memory: B, G, R, unused.

    float Clamp(float value, float low = 0.0f, float high = 1.0f)
    {
        return (std::max)(low, (std::min)(high, value));
    }

    float Smooth(float low, float high, float value)
    {
        const float t = Clamp((value - low) / (high - low));
        return t * t * (3.0f - 2.0f * t);
    }

    Pixel Mix(Pixel a, Pixel b, float amount)
    {
        amount = Clamp(amount);
        Pixel result = 0;
        for (int shift = 0; shift <= 16; shift += 8)
        {
            const float first = static_cast<float>((a >> shift) & 255);
            const float second = static_cast<float>((b >> shift) & 255);
            result |= static_cast<Pixel>(first + (second - first) * amount + 0.5f) << shift;
        }
        return result;
    }

    float Weathering(int x, int y)
    {
        // Continuous world-space variation, never seeded afresh at a tile edge.
        return std::sin(x * 0.105f + std::sin(y * 0.071f) * 1.7f) * 0.55f +
            std::sin(y * 0.19f + x * 0.047f) * 0.30f +
            std::sin(x * 0.39f - y * 0.31f) * 0.15f;
    }

    Pixel FallbackColor(ETileType type)
    {
        switch (type)
        {
        case ETileType::Road: return 0x4d525a;
        case ETileType::Bridge: return 0x656e78;
        case ETileType::Water: return 0x0a1e3b;
        case ETileType::Lava: return 0x9e3415;
        case ETileType::SkyVoid: return 0x132638;
        case ETileType::Wall: return 0x2b3341;
        case ETileType::Building: return 0x1c212e;
        case ETileType::Tree: return 0x13201e;
        case ETileType::Grave: return 0x2e3139;
        case ETileType::CursedGround: return 0x462754;
        case ETileType::MagicHazard: return 0x155c72;
        default: return 0x2a3234;
        }
    }

    struct Atlas
    {
        int width = 0;
        int height = 0;
        std::vector<Pixel> pixels;

        explicit Atlas(HBITMAP bitmap)
        {
            BITMAP object{};
            if (!bitmap || !GetObject(bitmap, sizeof(object), &object) ||
                object.bmWidth < 32 || object.bmHeight < 32) return;
            width = object.bmWidth;
            height = object.bmHeight;
            pixels.resize(static_cast<size_t>(width) * height);
            BITMAPINFO info{};
            info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
            info.bmiHeader.biWidth = width;
            info.bmiHeader.biHeight = -height;
            info.bmiHeader.biPlanes = 1;
            info.bmiHeader.biBitCount = 32;
            info.bmiHeader.biCompression = BI_RGB;
            HDC dc = CreateCompatibleDC(nullptr);
            const int rows = dc ? GetDIBits(dc, bitmap, 0, height, pixels.data(), &info, DIB_RGB_COLORS) : 0;
            if (dc) DeleteDC(dc);
            if (rows != height) pixels.clear();
        }

        bool Ready() const { return !pixels.empty(); }

        Pixel Sample(int slot, float u, float v, bool deck = false) const
        {
            const int col = slot % 4;
            const int row = slot / 4;
            int left = width * col / 4 + 2;
            int top = height * row / 4 + 2;
            int right = width * (col + 1) / 4 - 2;
            int bottom = height * (row + 1) / 4 - 2;
            if (deck)
            {
                const int h = bottom - top;
                top += h * 40 / 100;
                bottom = top + h * 24 / 100;
                const int center = (left + right) / 2;
                left = center - (bottom - top) / 2;
                right = left + (bottom - top);
            }
            const float sx = left + Clamp(u) * (right - left - 1);
            const float sy = top + Clamp(v) * (bottom - top - 1);
            const int x = static_cast<int>(sx);
            const int y = static_cast<int>(sy);
            const int nx = (std::min)(x + 1, right - 1);
            const int ny = (std::min)(y + 1, bottom - 1);
            const Pixel upper = Mix(pixels[y * width + x], pixels[y * width + nx], sx - x);
            const Pixel lower = Mix(pixels[ny * width + x], pixels[ny * width + nx], sx - x);
            return Mix(upper, lower, sy - y);
        }

        Pixel World(int slot, int x, int y, int span = 192, bool deck = false) const
        {
            // Mirroring joins the same source edge to itself. A 192px material
            // field crosses four game cells; 48px cells no longer restart it.
            int mx = x % (span * 2);
            int my = y % (span * 2);
            if (mx >= span) mx = span * 2 - 1 - mx;
            if (my >= span) my = span * 2 - 1 - my;
            return Sample(slot, mx / static_cast<float>(span - 1),
                my / static_cast<float>(span - 1), deck);
        }

        Pixel Reduced(int slot, float u, float v, int drawWidth, int drawHeight) const
        {
            // Small runes and flame posts need an area sample: a single sample
            // of the large atlas turns thin cyan lines into isolated speckles.
            const int sx = (std::min)(4, (std::max)(1, (width / 4 + drawWidth - 1) / drawWidth));
            const int sy = (std::min)(4, (std::max)(1, (height / 4 + drawHeight - 1) / drawHeight));
            unsigned int red = 0, green = 0, blue = 0;
            for (int y = 0; y < sy; ++y)
                for (int x = 0; x < sx; ++x)
                {
                    const Pixel sample = Sample(slot,
                        u + ((x + 0.5f) / sx - 0.5f) / drawWidth,
                        v + ((y + 0.5f) / sy - 0.5f) / drawHeight);
                    red += (sample >> 16) & 255;
                    green += (sample >> 8) & 255;
                    blue += sample & 255;
                }
            const unsigned int count = sx * sy;
            return ((red / count) << 16) | ((green / count) << 8) | (blue / count);
        }
    };

    Pixel LowWallPixel(const Atlas& atlas, Pixel ground, int x, int y,
        int dx, int dy, int width, int height)
    {
        // A short, raised wall, not a rectangular swatch of masonry. All
        // chipped stones, moss and shadow stay inside the reserved prop box.
        const bool horizontal = width >= height;
        const float along = static_cast<float>(horizontal ? dx : dy);
        const float across = static_cast<float>(horizontal ? dy : dx);
        const float length = static_cast<float>(horizontal ? width : height);
        const float depth = static_cast<float>(horizontal ? height : width);
        const int course = static_cast<int>(along) / 22;
        const int wear = (course * 37 + (x - dx) * 3 + (y - dy) * 5) % 13;
        const float endDistance = (std::min)(along, length - 1.0f - along);
        const float collapsed = 1.0f - Smooth(5.0f, 27.0f, endDistance);
        const float crest = depth * 0.22f + (wear % 3) * 1.3f +
            (wear == 0 ? depth * 0.17f : 0.0f) + collapsed * depth * 0.28f;
        const float foot = depth * 0.78f + Weathering(x, y) * 1.3f;
        const float ends = Smooth(3.0f, 8.0f, endDistance);
        const float shadow = ends * Smooth(crest, crest + 6.0f, across) *
            (1.0f - Smooth(foot, depth - 2.0f, across));
        Pixel color = Mix(ground, 0x0d141c, shadow * 0.48f);
        const float body = ends * Smooth(crest - 0.7f, crest + 0.7f, across) *
            (1.0f - Smooth(foot - 1.0f, foot + 1.0f, across));
        const Pixel stone = atlas.Ready() ? atlas.World(3, x, y, 144) : 0x515b68;
        const float cap = 1.0f - Smooth(crest + 6.0f, crest + 8.5f, across);
        Pixel masonry = Mix(Mix(stone, 0x131d29, 0.24f),
            Mix(stone, 0xa2a9b0, 0.27f), cap);
        // Offset capstone joints and broken ends interrupt the straight line.
        const int joint = (static_cast<int>(along) + wear % 3) % 22;
        if (joint < 2) masonry = Mix(masonry, 0x1b2530, cap * 0.55f);
        masonry = Mix(masonry, 0x2a392e,
            Smooth(foot - 7.0f, foot, across) * (0.30f + Weathering(x, y) * 0.15f));
        color = Mix(color, masonry, body);

        // Loose stones at the foot are part of the same blocked footprint.
        const int rubblePhase = (x - dx + y - dy) % 11;
        const float rx = static_cast<float>((static_cast<int>(along) + rubblePhase) % 29) - 14.0f;
        const float ry = across - depth * 0.88f;
        const float rubble = ends * (1.0f - Smooth(0.6f, 1.0f,
            rx * rx / 22.0f + ry * ry / 7.0f));
        return Mix(color, Mix(stone, 0x75808b, 0.12f), rubble * 0.80f);
    }

    float EdgeDistance(const SBattleMapData& map, ETileType type, int x, int y)
    {
        const int tile = map.tileSize;
        const int gx = x / tile;
        const int gy = y / tile;
        const float left = static_cast<float>(x % tile) + 0.5f;
        const float top = static_cast<float>(y % tile) + 0.5f;
        const float right = tile - left;
        const float bottom = tile - top;
        auto Same = [&](int cx, int cy)
        {
            return cx >= 0 && cy >= 0 && cx < map.width && cy < map.height &&
                map.tiles[cy][cx] == type;
        };
        float distance = static_cast<float>(tile);
        if (!Same(gx - 1, gy)) distance = (std::min)(distance, left);
        if (!Same(gx + 1, gy)) distance = (std::min)(distance, right);
        if (!Same(gx, gy - 1)) distance = (std::min)(distance, top);
        if (!Same(gx, gy + 1)) distance = (std::min)(distance, bottom);
        // Round the visual material transition at an inside corner, while the
        // logical blocked tile boundaries remain unchanged.
        if (!Same(gx - 1, gy - 1)) distance = (std::min)(distance, std::sqrt(left * left + top * top));
        if (!Same(gx + 1, gy - 1)) distance = (std::min)(distance, std::sqrt(right * right + top * top));
        if (!Same(gx - 1, gy + 1)) distance = (std::min)(distance, std::sqrt(left * left + bottom * bottom));
        if (!Same(gx + 1, gy + 1)) distance = (std::min)(distance, std::sqrt(right * right + bottom * bottom));
        return distance;
    }
}

HBITMAP GraveyardRenderer::CreateBackground(const SBattleMapData& map, HBITMAP bitmap)
{
    if (map.width <= 0 || map.height <= 0 || map.tileSize <= 0 ||
        map.tiles.size() != static_cast<size_t>(map.height)) return nullptr;
    for (const auto& row : map.tiles)
        if (row.size() != static_cast<size_t>(map.width)) return nullptr;

    const int width = map.width * map.tileSize;
    const int height = map.height * map.tileSize;
    BITMAPINFO canvas{};
    canvas.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    canvas.bmiHeader.biWidth = width;
    canvas.bmiHeader.biHeight = -height;
    canvas.bmiHeader.biPlanes = 1;
    canvas.bmiHeader.biBitCount = 32;
    canvas.bmiHeader.biCompression = BI_RGB;
    void* memory = nullptr;
    HBITMAP result = CreateDIBSection(nullptr, &canvas, DIB_RGB_COLORS, &memory, nullptr, 0);
    if (!result) return nullptr;
    Pixel* output = static_cast<Pixel*>(memory);
    const Atlas atlas(bitmap);

    std::vector<bool> propCells(static_cast<size_t>(map.width) * map.height, false);
    auto ValidProp = [&](const SBattleMapProp& p)
    {
        return p.x >= 0 && p.y >= 0 && p.width > 0 && p.height > 0 &&
            p.x + p.width <= map.width && p.y + p.height <= map.height &&
            p.atlasIndex >= 0 && p.atlasIndex < 16;
    };
    for (const auto& p : map.props)
        if (ValidProp(p))
            for (int y = p.y; y < p.y + p.height; ++y)
                for (int x = p.x; x < p.x + p.width; ++x)
                    propCells[y * map.width + x] = true;

    for (int y = 0; y < height; ++y)
    {
        for (int x = 0; x < width; ++x)
        {
            const int gx = x / map.tileSize;
            const int gy = y / map.tileSize;
            const ETileType type = map.tiles[gy][gx];
            Pixel color = FallbackColor(type);
            if (atlas.Ready())
            {
                const Pixel ground = atlas.World(0, x, y);
                color = ground;
                const float noise = (type == ETileType::Road || type == ETileType::Water ||
                    type == ETileType::CursedGround) ? Weathering(x, y) : 0.0f;
                if (!propCells[gy * map.width + gx])
                {
                    switch (type)
                    {
                    case ETileType::Road:
                        color = Mix(ground, atlas.World(1, x, y),
                            0.68f * Smooth(1.5f, 18.0f + noise * 4.0f, EdgeDistance(map, type, x, y)));
                        break;
                    case ETileType::Bridge:
                        color = Mix(Mix(ground, atlas.World(1, x, y), 0.68f),
                            Mix(atlas.World(12, x, y, 96, true), ground, 0.10f),
                            Smooth(0.0f, 7.0f, EdgeDistance(map, type, x, y)));
                        break;
                    case ETileType::Water:
                    {
                        const Pixel dampEarth = Mix(ground, 0x151e20, 0.45f);
                        const float bank = 6.0f + noise * 3.5f;
                        color = Mix(dampEarth, atlas.World(2, x, y),
                            Smooth(bank - 2.0f, bank + 3.0f, EdgeDistance(map, type, x, y)));
                        break;
                    }
                    case ETileType::Wall: case ETileType::Fence:
                        color = atlas.World(3, x, y, 144); break;
                    case ETileType::Building:
                        // Terrain-only painted maps still need visibly blocked
                        // building foundations if the authored scene is missing.
                        color = Mix(atlas.World(3, x, y, 144), 0x182435, 0.45f); break;
                    case ETileType::Grave:
                        color = atlas.Sample(4, (x % map.tileSize) / static_cast<float>(map.tileSize - 1),
                            (y % map.tileSize) / static_cast<float>(map.tileSize - 1)); break;
                    case ETileType::Tree:
                        color = atlas.Sample(5, (x % map.tileSize) / static_cast<float>(map.tileSize - 1),
                            (y % map.tileSize) / static_cast<float>(map.tileSize - 1)); break;
                    case ETileType::CursedGround:
                        color = Mix(ground, atlas.World(6, x, y, 144),
                            Smooth(1.0f, 15.0f + noise * 4.0f, EdgeDistance(map, type, x, y))); break;
                    case ETileType::MagicHazard:
                    {
                        const float edge = static_cast<float>((std::min)({x % map.tileSize,
                            y % map.tileSize, map.tileSize - 1 - x % map.tileSize, map.tileSize - 1 - y % map.tileSize}));
                        color = Mix(ground, atlas.Reduced(7,
                            (x % map.tileSize) / static_cast<float>(map.tileSize - 1),
                            (y % map.tileSize) / static_cast<float>(map.tileSize - 1),
                            map.tileSize, map.tileSize), Smooth(0.0f, 7.0f, edge));
                        break;
                    }
                    default: break;
                    }
                }
            }
            output[y * width + x] = color;
        }
    }

    for (const auto& p : map.props)
    {
        if (!ValidProp(p)) continue;
        const int left = p.x * map.tileSize;
        const int top = p.y * map.tileSize;
        const int w = p.width * map.tileSize;
        const int h = p.height * map.tileSize;
        const float feather = (std::min)(34.0f, (std::min)(w, h) * 0.22f);
        for (int dy = 0; dy < h; ++dy)
        {
            for (int dx = 0; dx < w; ++dx)
            {
                const int x = left + dx;
                const int y = top + dy;
                const size_t offset = static_cast<size_t>(y) * width + x;
                if (p.atlasIndex == 3)
                {
                    output[offset] = LowWallPixel(atlas, output[offset], x, y, dx, dy, w, h);
                    continue;
                }
                const float soften = feather + Weathering(x, y) * feather * 0.20f;
                const float alpha = Smooth(0.0f, soften, static_cast<float>(dx)) *
                    Smooth(0.0f, soften, static_cast<float>(dy)) *
                    Smooth(0.0f, soften, static_cast<float>(w - 1 - dx)) *
                    Smooth(0.0f, soften, static_cast<float>(h - 1 - dy));
                const Pixel artwork = atlas.Ready() ? atlas.Reduced(p.atlasIndex,
                    dx / static_cast<float>(w - 1), dy / static_cast<float>(h - 1), w, h) : 0x1a212a;
                // Feather the baked ground and shadow into the common floor,
                // strictly inside this prop's blocked footprint. No card frame.
                output[offset] = Mix(output[offset], artwork, alpha);
            }
        }
    }
    return result;
}
