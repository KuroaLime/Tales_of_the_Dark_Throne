#pragma once

#include <windows.h>
#include <algorithm>
#include <cmath>

// Owns a raster snapshot in screen coordinates. Handles never escape the owner;
// callers can capture, copy and composite a region without managing GDI lifetime.
class UIFrameSnapshot
{
private:
    HDC DC = nullptr;
    HBITMAP Bitmap = nullptr;
    HGDIOBJ PreviousBitmap = nullptr;
    int Width = 0;
    int Height = 0;
    RECT Bounds{};
    bool Valid = false;

    bool Ensure(HDC reference, int width, int height)
    {
        if (DC && Bitmap && Width == width && Height == height) return true;
        Reset();
        if (!IsRasterTarget(reference) || width <= 0 || height <= 0) return false;
        DC = CreateCompatibleDC(reference);
        Bitmap = CreateCompatibleBitmap(reference, width, height);
        if (!DC || !Bitmap) { Reset(); return false; }
        PreviousBitmap = SelectObject(DC, Bitmap);
        if (!PreviousBitmap || PreviousBitmap == HGDI_ERROR)
        {
            PreviousBitmap = nullptr;
            Reset();
            return false;
        }
        Width = width;
        Height = height;
        return true;
    }

public:
    UIFrameSnapshot() = default;
    ~UIFrameSnapshot() { Reset(); }
    UIFrameSnapshot(const UIFrameSnapshot&) = delete;
    UIFrameSnapshot& operator=(const UIFrameSnapshot&) = delete;

    static bool IsRasterTarget(HDC destination)
    {
        const DWORD type = destination ? GetObjectType(destination) : 0;
        return type && type != OBJ_ENHMETADC && type != OBJ_METADC;
    }

    void Reset()
    {
        if (DC && PreviousBitmap) SelectObject(DC, PreviousBitmap);
        if (Bitmap) DeleteObject(Bitmap);
        if (DC) DeleteDC(DC);
        DC = nullptr;
        Bitmap = nullptr;
        PreviousBitmap = nullptr;
        Width = Height = 0;
        Bounds = {};
        Valid = false;
    }

    bool HasFrame() const { return Valid; }
    bool Matches(const RECT& bounds) const { return Valid && EqualRect(&Bounds, &bounds); }

    bool Capture(HDC source, const RECT& bounds)
    {
        // Recording text into a metafile must not replace a displayed frame.
        if (!IsRasterTarget(source)) return false;
        const int width = bounds.right - bounds.left, height = bounds.bottom - bounds.top;
        if (!Ensure(source, width, height) ||
            !BitBlt(DC, 0, 0, width, height, source, bounds.left, bounds.top, SRCCOPY))
        {
            Reset();
            return false;
        }
        Bounds = bounds;
        Valid = true;
        return true;
    }

    bool CopyFrom(const UIFrameSnapshot& source, const RECT& bounds)
    {
        if (this == &source) return Matches(bounds);
        if (!source.Matches(bounds) || !Ensure(source.DC, source.Width, source.Height) ||
            !BitBlt(DC, 0, 0, source.Width, source.Height, source.DC, 0, 0, SRCCOPY))
        {
            Reset();
            return false;
        }
        Bounds = bounds;
        Valid = true;
        return true;
    }

    bool BlendRegion(HDC destination, const RECT& region, double opacity) const
    {
        if (!Valid || !IsRasterTarget(destination) || !std::isfinite(opacity)) return false;
        RECT clipped{};
        if (!IntersectRect(&clipped, &region, &Bounds)) return true;
        opacity = (std::max)(0.0, (std::min)(1.0, opacity));
        const BLENDFUNCTION blend{ AC_SRC_OVER, 0, static_cast<BYTE>(opacity * 255.0 + 0.5), 0 };
        return AlphaBlend(destination, clipped.left, clipped.top,
            clipped.right - clipped.left, clipped.bottom - clipped.top,
            DC, clipped.left - Bounds.left, clipped.top - Bounds.top,
            clipped.right - clipped.left, clipped.bottom - clipped.top, blend) != FALSE;
    }
};

// A transition owns its outgoing snapshot and clock. The stage only chooses
// which regions to blend; it cannot desynchronize handles, bounds and progress.
class UIRegionTransition
{
private:
    UIFrameSnapshot Outgoing;
    double Elapsed = 0.0;
    double Duration = 0.0;
    bool Active = false;

public:
    bool Begin(const UIFrameSnapshot& presented, const RECT& bounds, double duration)
    {
        if (!std::isfinite(duration) || duration <= 0.0 || !Outgoing.CopyFrom(presented, bounds))
        {
            Reset();
            return false;
        }
        Duration = duration;
        Elapsed = 0.0;
        Active = true;
        return true;
    }

    void Advance(double seconds)
    {
        if (!Active || !std::isfinite(seconds) || seconds <= 0.0) return;
        Elapsed = (std::min)(Duration, Elapsed + seconds);
        if (Elapsed >= Duration) Active = false;
    }

    void Reset()
    {
        Active = false;
        Elapsed = Duration;
        Outgoing.Reset();
    }

    bool IsActive() const { return Active; }
    double Progress() const { return Active ? Elapsed / Duration : 1.0; }

    bool CanComposite(HDC destination, const RECT& bounds) const
    {
        return Active && Outgoing.Matches(bounds) && UIFrameSnapshot::IsRasterTarget(destination);
    }

    bool Composite(HDC destination, const RECT& bounds, const RECT& region)
    {
        if (!Active || !UIFrameSnapshot::IsRasterTarget(destination)) return true;
        if (!Outgoing.Matches(bounds)) { Reset(); return false; }
        const double progress = Progress();
        const double eased = progress * progress * (3.0 - 2.0 * progress);
        if (!Outgoing.BlendRegion(destination, region, 1.0 - eased))
        {
            Reset();
            return false;
        }
        return true;
    }
};
