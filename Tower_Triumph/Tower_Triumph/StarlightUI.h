#pragma once
#include "UButton.h"
#include "resource.h"
#include <algorithm>
#include <cstring>

// Shared visual language of the approved popup preview. Values/content remain
// owned by the existing widgets and GameManager, never by this drawing layer.
namespace StarlightUI
{
    constexpr COLORREF Ink = RGB(241, 231, 211);
    constexpr COLORREF Gold = RGB(211, 180, 120);
    constexpr COLORREF Muted = RGB(187, 179, 201);
    constexpr COLORREF Navy = RGB(17, 20, 41);

    inline RECT Centered(RECT client, int width, int height)
    {
        width = (std::min)(width, static_cast<int>((std::max)(1L, client.right - client.left - 48)));
        height = (std::min)(height, static_cast<int>((std::max)(1L, client.bottom - client.top - 88)));
        const int x = (client.left + client.right - width) / 2;
        const int y = (client.top + client.bottom - height) / 2 + 12;
        return { x, y, x + width, y + height };
    }

    inline void Text(HDC dc, const std::wstring& text, RECT rect, int height = 18,
        COLORREF color = Ink, UINT format = DT_LEFT | DT_WORDBREAK)
    {
        const int saved = SaveDC(dc);
        HFONT font = CreateFontW(-height, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Malgun Gothic");
        SelectObject(dc, font);
        SetBkMode(dc, TRANSPARENT);
        SetTextColor(dc, color);
        DrawTextW(dc, text.c_str(), static_cast<int>(text.size()), &rect, format | DT_NOPREFIX);
        RestoreDC(dc, saved);
        DeleteObject(font);
    }

    inline void Surface(HDC dc, const RECT& rect)
    {
        const int saved = SaveDC(dc);
        HBRUSH brush = CreateSolidBrush(RGB(26, 28, 52));
        HPEN pen = CreatePen(PS_SOLID, 1, RGB(100, 84, 70));
        SelectObject(dc, brush); SelectObject(dc, pen);
        RoundRect(dc, rect.left, rect.top, rect.right, rect.bottom, 6, 6);
        RestoreDC(dc, saved); DeleteObject(pen); DeleteObject(brush);
    }

    inline void Dim(HDC dc, const RECT& rect, BYTE opacity = 120)
    {
        HDC source = CreateCompatibleDC(dc);
        HBITMAP bitmap = CreateCompatibleBitmap(dc, 1, 1);
        if (source && bitmap)
        {
            HGDIOBJ old = SelectObject(source, bitmap);
            SetPixelV(source, 0, 0, RGB(8, 13, 36));
            const BLENDFUNCTION blend{ AC_SRC_OVER, 0, opacity, 0 };
            AlphaBlend(dc, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top,
                source, 0, 0, 1, 1, blend);
            SelectObject(source, old);
        }
        if (bitmap) DeleteObject(bitmap);
        if (source) DeleteDC(source);
    }

    inline void Crest(HDC dc, int centerX, int top)
    {
        struct ResourceBitmap
        {
            HBITMAP bitmap = nullptr;
            ResourceBitmap()
            {
                HMODULE module = GetModuleHandleW(nullptr);
                HRSRC resource = FindResourceW(module, MAKEINTRESOURCEW(IDB_UI_STARLIGHT_CREST), RT_BITMAP);
                if (!resource) return;
                const DWORD bytes = SizeofResource(module, resource);
                HGLOBAL loaded = LoadResource(module, resource);
                const BYTE* data = loaded ? static_cast<const BYTE*>(LockResource(loaded)) : nullptr;
                BITMAPINFO info{};
                if (!data || bytes < sizeof(BITMAPINFOHEADER)) return;
                std::memcpy(&info.bmiHeader, data, sizeof(BITMAPINFOHEADER));
                if (info.bmiHeader.biSize != 40 || info.bmiHeader.biWidth != 134 ||
                    info.bmiHeader.biHeight != -51 || info.bmiHeader.biBitCount != 32 ||
                    info.bmiHeader.biCompression != BI_RGB || bytes < 40 + 134 * 51 * 4) return;
                void* pixels = nullptr;
                bitmap = CreateDIBSection(nullptr, &info, DIB_RGB_COLORS, &pixels, nullptr, 0);
                if (bitmap && pixels) std::memcpy(pixels, data + 40, 134 * 51 * 4);
            }
            ~ResourceBitmap() { if (bitmap) DeleteObject(bitmap); }
        };
        static ResourceBitmap crest;
        if (!crest.bitmap) return;
        HDC source = CreateCompatibleDC(dc);
        HGDIOBJ old = SelectObject(source, crest.bitmap);
        const BLENDFUNCTION blend{ AC_SRC_OVER, 0, 255, AC_SRC_ALPHA };
        AlphaBlend(dc, centerX - 80, top - 42, 160, 61, source, 0, 0, 134, 51, blend);
        SelectObject(source, old); DeleteDC(source);
    }

    inline void Panel(HDC dc, const RECT& rect, const std::wstring& title,
        const std::wstring& eyebrow = L"")
    {
        const int saved = SaveDC(dc);
        HBRUSH brush = CreateSolidBrush(Navy);
        HPEN pen = CreatePen(PS_SOLID, 1, Gold);
        SelectObject(dc, brush); SelectObject(dc, pen);
        RoundRect(dc, rect.left, rect.top, rect.right, rect.bottom, 10, 10);
        SelectObject(dc, GetStockObject(NULL_BRUSH));
        HPEN outer = CreatePen(PS_SOLID, 1, RGB(120, 98, 66));
        SelectObject(dc, outer);
        RoundRect(dc, rect.left - 4, rect.top - 4, rect.right + 4, rect.bottom + 4, 12, 12);
        MoveToEx(dc, rect.left + 28, rect.top + 88, nullptr);
        LineTo(dc, rect.right - 28, rect.top + 88);
        RestoreDC(dc, saved); DeleteObject(outer); DeleteObject(pen); DeleteObject(brush);
        Crest(dc, (rect.left + rect.right) / 2, rect.top);
        Text(dc, eyebrow, { rect.left + 30, rect.top + 18, rect.right - 70, rect.top + 39 }, 13, Gold, DT_LEFT | DT_SINGLELINE);
        Text(dc, title, { rect.left + 30, rect.top + 42, rect.right - 70, rect.top + 79 }, 28, Ink, DT_LEFT | DT_SINGLELINE);
    }

    inline void StyleButton(UButton* button, bool primary = false, int fontSize = 17)
    {
        if (!button) return;
        button->SetFont(fontSize, FW_NORMAL, L"Malgun Gothic");
        button->SetAnimationType(0);
        button->SetTextWrapping(true);
        button->SetFallbackColors(primary ? RGB(83, 67, 45) : RGB(27, 32, 54),
            RGB(58, 52, 80), RGB(31, 27, 44), RGB(52, 48, 71), RGB(27, 28, 39));
        button->SetFallbackTextColors(Ink, RGB(255, 240, 206), Gold, RGB(255, 240, 206), RGB(125, 121, 138));
        button->SetFallbackBorder(primary ? Gold : RGB(112, 94, 76), 1);
    }
}
