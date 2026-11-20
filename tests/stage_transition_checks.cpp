#include <windows.h>
#include <cmath>
#include <fstream>
#include <iostream>
#include <limits>
#include <stdexcept>
#include "StageTransition.h"

namespace
{
    int checks = 0;
    void Check(bool condition, const char* message)
    {
        ++checks;
        if (!condition) throw std::runtime_error(message);
    }

    struct Surface
    {
        HDC dc = CreateCompatibleDC(nullptr);
        HBITMAP bitmap = nullptr;
        HGDIOBJ previous = nullptr;
        Surface()
        {
            BITMAPINFO info{};
            info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
            info.bmiHeader.biWidth = 11;
            info.bmiHeader.biHeight = -7;
            info.bmiHeader.biPlanes = 1;
            info.bmiHeader.biBitCount = 32;
            void* bits = nullptr;
            bitmap = CreateDIBSection(dc, &info, DIB_RGB_COLORS, &bits, nullptr, 0);
            previous = SelectObject(dc, bitmap);
            Check(dc && bitmap && bits && previous, "offscreen DIB allocation");
        }
        ~Surface()
        {
            SelectObject(dc, previous);
            DeleteObject(bitmap);
            DeleteDC(dc);
        }
        void Fill(COLORREF color)
        {
            const RECT rectangle{ 0, 0, 11, 7 };
            HBRUSH brush = CreateSolidBrush(color);
            FillRect(dc, &rectangle, brush);
            DeleteObject(brush);
        }
    };

    void ExpectColor(HDC dc, int x, int y, int red, int blue)
    {
        const COLORREF pixel = GetPixel(dc, x, y);
        Check(std::abs(int(GetRValue(pixel)) - red) <= 1 && GetGValue(pixel) == 0 &&
            std::abs(int(GetBValue(pixel)) - blue) <= 1, "actual GDI crossfade pixel");
    }
}

int wmain(int argc, wchar_t** argv)
{
    try
    {
        Surface oldScene, newScene;
        oldScene.Fill(RGB(255, 0, 0));
        newScene.Fill(RGB(0, 0, 255));
        StageTransition transition;
        Check(!transition.IsActive() && transition.Progress() == 1.0, "idle state");
        Check(!transition.Start(), "no start without snapshot");
        Check(!transition.Capture(nullptr, 11, 7), "null capture rejected");
        Check(!transition.Capture(oldScene.dc, 0, 7), "empty capture rejected");
        Check(transition.Capture(oldScene.dc, 11, 7) && transition.Start(), "capture then start");
        oldScene.Fill(RGB(0, 255, 0)); // Snapshot must own its pixels.
        Check(transition.Composite(newScene.dc, 11, 7), "initial composite");
        ExpectColor(newScene.dc, 3, 3, 255, 0);
        Check(!transition.Start() && !transition.Capture(oldScene.dc, 11, 7), "duplicate does not restart");
        Check(transition.Progress() == 0.0, "duplicate preserves progress");
        const UINT blocked[] = { WM_LBUTTONDOWN, WM_LBUTTONUP, WM_LBUTTONDBLCLK,
            WM_RBUTTONDOWN, WM_RBUTTONUP, WM_KEYDOWN, WM_CHAR, WM_MOUSEWHEEL };
        for (UINT message : blocked) Check(transition.BlocksInput(message), "action input blocked");
        const UINT allowed[] = { WM_MOUSEMOVE, WM_SETCURSOR, WM_KILLFOCUS,
            WM_CANCELMODE, WM_CAPTURECHANGED, WM_PAINT, WM_TIMER };
        for (UINT message : allowed) Check(!transition.BlocksInput(message), "live pointer and cleanup allowed");

        transition.Advance(-1.0);
        transition.Advance(std::numeric_limits<double>::quiet_NaN());
        transition.Advance(std::numeric_limits<double>::infinity());
        Check(transition.Progress() == 0.0, "invalid elapsed time ignored");
        for (int quarter = 1; quarter <= 3; ++quarter)
        {
            transition.Advance(0.075);
            Check(std::abs(transition.Progress() - quarter * 0.25) < 0.000001, "deterministic elapsed progress");
            newScene.Fill(RGB(0, 0, 255));
            Check(transition.Composite(newScene.dc, 11, 7), "in-progress composite");
            const int red[] = { 0, 215, 128, 40 };
            for (int y = 0; y < 7; ++y)
                for (int x = 0; x < 11; ++x)
                    ExpectColor(newScene.dc, x, y, red[quarter], 255 - red[quarter]);
            // Framework's live cursor pass follows composition and remains opaque.
            SetPixelV(newScene.dc, quarter, 2, RGB(255, 255, 255));
            Check(GetPixel(newScene.dc, quarter, 2) == RGB(255, 255, 255), "live cursor drawn once above blend");
        }
        transition.Advance(0.075);
        Check(!transition.IsActive() && transition.Progress() == 1.0, "300ms completion");
        newScene.Fill(RGB(0, 0, 255));
        Check(!transition.Composite(newScene.dc, 11, 7), "no overlay after completion");
        ExpectColor(newScene.dc, 3, 3, 0, 255);
        for (UINT message : blocked) Check(!transition.BlocksInput(message), "actions resume after completion");

        Check(transition.Capture(oldScene.dc, 11, 7) && transition.Start(), "resize fixture");
        Check(!transition.Composite(newScene.dc, 12, 7) && !transition.IsActive(), "resize cancels without stretched image");
        ExpectColor(newScene.dc, 3, 3, 0, 255);
        Check(transition.Capture(oldScene.dc, 11, 7) && transition.Start(), "focus fixture");
        transition.Cancel();
        Check(!transition.IsActive() && !transition.BlocksInput(WM_LBUTTONDOWN), "focus cancellation releases input");
        Check(transition.Capture(oldScene.dc, 11, 7) && transition.Start(), "long frame fixture");
        transition.Advance(10.0);
        Check(!transition.IsActive(), "long elapsed frame completes safely");

        GdiFlush();
        const DWORD resourcesBefore = GetGuiResources(GetCurrentProcess(), GR_GDIOBJECTS);
        for (int cycle = 0; cycle < 100; ++cycle)
        {
            StageTransition repeated;
            Check(repeated.Capture(oldScene.dc, 11, 7) && repeated.Start(), "repeated allocation");
            repeated.Advance(0.15);
            Check(repeated.Composite(newScene.dc, 11, 7), "repeated composite");
            if (cycle % 2 == 0) repeated.Cancel();
        }
        GdiFlush();
        const DWORD resourcesAfter = GetGuiResources(GetCurrentProcess(), GR_GDIOBJECTS);
        Check(resourcesAfter <= resourcesBefore, "snapshot GDI objects released on cancel and destruction");
        if (argc > 1)
        {
            std::ofstream report(argv[1]);
            report << "{\"passed\":true,\"checks\":" << checks
                << ",\"durationSeconds\":0.3,\"ease\":\"smoothstep\",\"gdiBefore\":"
                << resourcesBefore << ",\"gdiAfter\":" << resourcesAfter
                << ",\"scope\":\"Actual offscreen GDI compositor; no game launch or save access\"}\n";
        }
        std::cout << "PASS: " << checks << " stage transition checks; GDI "
            << resourcesBefore << " -> " << resourcesAfter << "\n";
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << "FAIL: " << error.what() << "\n";
        return 1;
    }
}
