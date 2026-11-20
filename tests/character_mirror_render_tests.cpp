// Compile/link against the real Character implementation. Runtime assertions
// require a host that permits native execution; compilation is not a test pass.
// This fixture uses only synthetic bitmaps and offscreen DCs, with no game,
// window, audio, or save access. argv[1] is a disposable fixture output folder.
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "../Tower_Triumph/Tower_Triumph/Character.h"
#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <vector>

namespace {
constexpr std::uint32_t Key = 0x00ff00ffu;
constexpr std::uint32_t Background = 0x00275361u;
constexpr int CanvasWidth = 512, CanvasHeight = 320, FootX = 256, FootY = 270;

void Require(bool condition, const char* message)
{
    if (!condition) throw std::runtime_error(message);
}

struct Pose {
    const wchar_t* filename;
    ECharacterState state;
    int cell, columns, rows, root, ground, left, top, width, height;

    std::uint32_t Pixel(int x, int y) const
    {
        if (x < left || x >= left + width || y < top || y >= top + height) return Key;
        // Transparent interior holes must survive the mirror, while legitimate
        // black art must remain opaque instead of being treated as background.
        if (x == left + 2 && y == top + 2) return 0;
        if (x > left + 4 && x < left + 9 && y > top + 4 && y < top + 9) return Key;
        return static_cast<std::uint32_t>((40 + x) << 16 | (30 + y) << 8 | (90 + (x + y) % 100));
    }
};

const Pose Idle{ L"Warrior_IdlePose.bmp", ECharacterState::Idle, 96, 1, 1, 48, 88, 28, 20, 41, 68 };
const Pose Walk{ L"Warrior_WalkCycle.bmp", ECharacterState::Walk, 96, 4, 3, 48, 88, 15, 24, 61, 64 };
const Pose Attack{ L"Warrior_AttackAction.bmp", ECharacterState::Attack, 128, 4, 3, 64, 112, 10, 25, 101, 87 };
const Pose Skill{ L"Warrior_SkillAction.bmp", ECharacterState::Skill, 128, 4, 3, 64, 112, 39, 37, 75, 75 };
const Pose Hit{ L"Warrior_HitAction.bmp", ECharacterState::Hit, 128, 4, 3, 64, 112, 50, 49, 31, 63 };
const Pose Defend{ L"Warrior_DefendAction.bmp", ECharacterState::Defend, 128, 4, 3, 64, 112, 21, 60, 81, 52 };
const Pose Dead{ L"Warrior_DeadAction.bmp", ECharacterState::Dead, 128, 4, 3, 64, 112, 9, 95, 103, 17 };

void WriteSheet(const std::filesystem::path& folder, const Pose& pose)
{
    const int width = pose.cell * pose.columns, height = pose.cell * pose.rows;
    std::vector<std::uint32_t> pixels(static_cast<std::size_t>(width) * height, Key);
    for (int y = 0; y < height; ++y)
        for (int x = 0; x < width; ++x)
            pixels[static_cast<std::size_t>(y) * width + x] = pose.Pixel(x % pose.cell, y % pose.cell);
    BITMAPFILEHEADER file{};
    BITMAPINFOHEADER info{};
    info.biSize = sizeof(info); info.biWidth = width; info.biHeight = -height;
    info.biPlanes = 1; info.biBitCount = 32; info.biCompression = BI_RGB;
    info.biSizeImage = static_cast<DWORD>(pixels.size() * sizeof(pixels[0]));
    file.bfType = 0x4d42; file.bfOffBits = sizeof(file) + sizeof(info);
    file.bfSize = file.bfOffBits + info.biSizeImage;
    std::ofstream stream(folder / pose.filename, std::ios::binary);
    stream.write(reinterpret_cast<const char*>(&file), sizeof(file));
    stream.write(reinterpret_cast<const char*>(&info), sizeof(info));
    stream.write(reinterpret_cast<const char*>(pixels.data()), info.biSizeImage);
    Require(static_cast<bool>(stream), "Cannot write synthetic sprite fixture");
}

class Canvas {
    HBITMAP bitmap = nullptr;
    HGDIOBJ previous = nullptr;
public:
    HDC dc = nullptr, source = nullptr;
    std::uint32_t* pixels = nullptr;
    Canvas()
    {
        BITMAPINFO info{};
        info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        info.bmiHeader.biWidth = CanvasWidth; info.bmiHeader.biHeight = -CanvasHeight;
        info.bmiHeader.biPlanes = 1; info.bmiHeader.biBitCount = 32;
        info.bmiHeader.biCompression = BI_RGB;
        dc = CreateCompatibleDC(nullptr); source = CreateCompatibleDC(nullptr);
        void* bits = nullptr;
        bitmap = CreateDIBSection(nullptr, &info, DIB_RGB_COLORS, &bits, nullptr, 0);
        Require(dc && source && bitmap && bits, "Cannot allocate offscreen fixture");
        pixels = static_cast<std::uint32_t*>(bits);
        previous = SelectObject(dc, bitmap);
        Require(previous && previous != HGDI_ERROR, "Cannot select fixture bitmap");
    }
    ~Canvas()
    {
        SelectObject(dc, previous); DeleteObject(bitmap); DeleteDC(source); DeleteDC(dc);
    }
    Canvas(const Canvas&) = delete;
    Canvas& operator=(const Canvas&) = delete;
};

void Check(Character& character, Canvas& canvas, const Pose& pose, bool mirror, int scale)
{
    GdiFlush();
    std::fill_n(canvas.pixels, CanvasWidth * CanvasHeight, Background);
    std::vector<std::uint32_t> expected(CanvasWidth * CanvasHeight, Background);
    for (int sy = 0; sy < pose.cell; ++sy)
        for (int sx = 0; sx < pose.cell; ++sx)
        {
            const auto pixel = pose.Pixel(sx, sy);
            if (pixel == Key) continue;
            // Independent expected mapping: each source pixel occupies scale
            // pixels; reflection is about the half-pixel at FootX - 0.5.
            const int dx = FootX + (mirror ? pose.root - sx - 1 : sx - pose.root) * scale;
            const int dy = FootY + (sy - pose.ground) * scale;
            for (int y = 0; y < scale; ++y)
                for (int x = 0; x < scale; ++x)
                    expected[static_cast<std::size_t>(dy + y) * CanvasWidth + dx + x] = pixel;
        }
    character.SetDirection(mirror ? EDirection::Left : EDirection::Right);
    character.DrawAtFoot(canvas.dc, canvas.source, FootX, FootY, 96 * scale);
    GdiFlush();
    for (std::size_t i = 0; i < expected.size(); ++i)
        Require((canvas.pixels[i] & 0x00ffffffu) == expected[i],
            "DrawAtFoot lost/shifted a sprite pixel or leaked background/scratch pixels");
}
}

int wmain(int argc, wchar_t** argv)
{
    try
    {
        Require(argc == 2, "Expected a fixture output directory argument");
        const std::filesystem::path folder = std::filesystem::absolute(argv[1]);
        std::filesystem::create_directories(folder);
        for (const auto* pose : { &Idle, &Walk, &Attack, &Skill, &Hit, &Defend, &Dead }) WriteSheet(folder, *pose);
        Character character;
        character.Init(ECharacterClass::Warrior, 0, 0);
        Require(character.LoadSprites(folder.wstring()), "Real Character loader rejected synthetic sheets");
        Canvas canvas;
        for (int scale : { 1, 2 })
        {
            // A large action followed by narrower and shorter poses exercises
            // reuse of the same scratch allocation across animation changes.
            for (const auto* pose : { &Idle, &Attack, &Hit, &Skill, &Defend, &Dead, &Walk, &Idle })
            {
                character.ResetBattleAnimation();
                character.SetState(pose->state, true, false);
                Check(character, canvas, *pose, false, scale);
                Check(character, canvas, *pose, true, scale);
            }
        }
        std::cout << "PASS: 32 actual Character draws preserve mirrored edges, transparency, black art and scratch reuse.\n";
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
