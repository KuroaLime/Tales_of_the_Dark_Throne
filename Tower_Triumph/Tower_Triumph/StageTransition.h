#pragma once

#include <windows.h>
#include <cmath>

// The caller draws the new scene, composites this cursor-free snapshot, then
// draws the live cursor once. Time is injected so runtime and replay agree.
class StageTransition
{
private:
    HDC snapshotDC = nullptr;
    HBITMAP snapshot = nullptr;
    HGDIOBJ previousBitmap = nullptr;
    int width = 0;
    int height = 0;
    double elapsed = 0.30;
    double duration = 0.30;
    bool active = false;

    void ReleaseSnapshot()
    {
        if (snapshotDC && previousBitmap) SelectObject(snapshotDC, previousBitmap);
        if (snapshot) DeleteObject(snapshot);
        if (snapshotDC) DeleteDC(snapshotDC);
        snapshotDC = nullptr;
        snapshot = nullptr;
        previousBitmap = nullptr;
        width = height = 0;
    }

public:
    StageTransition() = default;
    ~StageTransition() { ReleaseSnapshot(); }
    StageTransition(const StageTransition&) = delete;
    StageTransition& operator=(const StageTransition&) = delete;

    bool Capture(HDC scene, int sceneWidth, int sceneHeight)
    {
        // A rapid second request must not replace the first transition.
        if (active) return false;
        Cancel();
        if (!scene || sceneWidth <= 0 || sceneHeight <= 0) return false;
        snapshotDC = CreateCompatibleDC(scene);
        snapshot = CreateCompatibleBitmap(scene, sceneWidth, sceneHeight);
        if (!snapshotDC || !snapshot) { Cancel(); return false; }
        previousBitmap = SelectObject(snapshotDC, snapshot);
        if (!previousBitmap || previousBitmap == HGDI_ERROR)
        {
            previousBitmap = nullptr;
            Cancel();
            return false;
        }
        width = sceneWidth;
        height = sceneHeight;
        if (!BitBlt(snapshotDC, 0, 0, width, height, scene, 0, 0, SRCCOPY))
        {
            Cancel();
            return false;
        }
        return true;
    }

    bool Start(double durationSeconds = 0.30)
    {
        if (active) return false;
        if (!snapshot || !std::isfinite(durationSeconds) || durationSeconds <= 0.0)
        {
            Cancel();
            return false;
        }
        duration = durationSeconds;
        elapsed = 0.0;
        active = true;
        return true;
    }

    void Advance(double elapsedSeconds)
    {
        if (!active || !std::isfinite(elapsedSeconds) || elapsedSeconds <= 0.0) return;
        elapsed += elapsedSeconds;
        if (elapsed >= duration) Cancel();
    }

    bool Composite(HDC destination, int sceneWidth, int sceneHeight)
    {
        if (!active) return false;
        if (!destination || sceneWidth != width || sceneHeight != height)
        {
            Cancel();
            return false;
        }
        const double t = Progress();
        const double eased = t * t * (3.0 - 2.0 * t);
        const BYTE opacity = static_cast<BYTE>((1.0 - eased) * 255.0 + 0.5);
        const BLENDFUNCTION blend{ AC_SRC_OVER, 0, opacity, 0 };
        if (!AlphaBlend(destination, 0, 0, width, height, snapshotDC,
            0, 0, width, height, blend))
        {
            Cancel();
            return false;
        }
        return true;
    }

    void Cancel()
    {
        active = false;
        elapsed = duration;
        ReleaseSnapshot();
    }

    bool IsActive() const { return active; }
    double Progress() const { return active ? elapsed / duration : 1.0; }

    bool BlocksInput(UINT message) const
    {
        if (!active) return false;
        switch (message)
        {
        case WM_LBUTTONDOWN: case WM_LBUTTONUP: case WM_LBUTTONDBLCLK:
        case WM_RBUTTONDOWN: case WM_RBUTTONUP: case WM_RBUTTONDBLCLK:
        case WM_MBUTTONDOWN: case WM_MBUTTONUP: case WM_MBUTTONDBLCLK:
        case WM_XBUTTONDOWN: case WM_XBUTTONUP: case WM_XBUTTONDBLCLK:
        case WM_MOUSEWHEEL: case WM_MOUSEHWHEEL:
        case WM_KEYDOWN: case WM_KEYUP: case WM_CHAR:
        case WM_SYSKEYDOWN: case WM_SYSKEYUP: case WM_SYSCHAR:
            return true;
        default:
            return false; // Pointer movement, focus/capture cleanup and painting continue.
        }
    }
};
