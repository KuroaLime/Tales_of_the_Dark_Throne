#include "URegionInfoPopup.h"
#include "StarlightUI.h"
#include <algorithm>
#include <cmath>

URegionInfoPopup::URegionInfoPopup()
    : btnClose(nullptr), btnEnter(nullptr), panelRect{}, mapPreview(nullptr),
      mapPreviewInfo{}, mapPreviewBounds{}
{
    bVisible = false;
    lastPresentationUpdate = std::chrono::steady_clock::now();
}

URegionInfoPopup::~URegionInfoPopup()
{
    ReleasePresentationSurface();
}

void URegionInfoPopup::SetVisible(bool visible)
{
    if (bVisible == visible && presentationVisible == visible) return;
    UWidget::SetVisible(visible);
    presentationVisible = visible;
    entryCommitted = false;
    CancelPress();
    presentationStart = presentationOpacity;
    presentationTarget = visible ? 1.0 : 0.0;
    presentationElapsed = 0.0;
    presentationDuration = (visible ? 0.20 : 0.16) * std::abs(presentationTarget - presentationStart);
    if (presentationDuration == 0.0) presentationOpacity = presentationTarget;
    lastPresentationUpdate = std::chrono::steady_clock::now();
}

void URegionInfoPopup::AdvancePresentation(double seconds)
{
    if (!std::isfinite(seconds) || seconds < 0.0) return;
    if (bVisible != presentationVisible) SetVisible(bVisible);
    lastPresentationUpdate = std::chrono::steady_clock::now();
    if (presentationDuration <= 0.0) return;
    presentationElapsed = (std::min)(presentationDuration, presentationElapsed + seconds);
    const double t = presentationElapsed / presentationDuration;
    const double eased = t * t * (3.0 - 2.0 * t);
    presentationOpacity = presentationStart + (presentationTarget - presentationStart) * eased;
    if (presentationElapsed >= presentationDuration)
    {
        presentationOpacity = presentationTarget;
        presentationDuration = 0.0;
    }
}

void URegionInfoPopup::Update()
{
    const auto now = std::chrono::steady_clock::now();
    AdvancePresentation(std::chrono::duration<double>(now - lastPresentationUpdate).count());
    if (bVisible && !entryCommitted) UContainerWidget::Update();
}

UWidget* URegionInfoPopup::FindWidgetUnderMouse(int x, int y)
{
    if (IsInputBlocked() || IsOpening()) return this;
    return UContainerWidget::FindWidgetUnderMouse(x, y);
}

void URegionInfoPopup::UpdateMouseHover(int x, int y)
{
    if (IsInputBlocked() || IsOpening())
    {
        IsPointed = false;
        for (auto child : Contents) child->UpdateMouseHover(-100000, -100000);
        return;
    }
    UContainerWidget::UpdateMouseHover(x, y);
}

void URegionInfoPopup::ReleasePresentationSurface()
{
    if (presentationDC && previousPresentationBitmap) SelectObject(presentationDC, previousPresentationBitmap);
    if (presentationBitmap) DeleteObject(presentationBitmap);
    if (presentationDC) DeleteDC(presentationDC);
    presentationDC = nullptr; presentationBitmap = nullptr; previousPresentationBitmap = nullptr;
    presentationWidth = presentationHeight = 0;
}

bool URegionInfoPopup::EnsurePresentationSurface(HDC target, int width, int height)
{
    if (presentationDC && presentationWidth == width && presentationHeight == height) return true;
    ReleasePresentationSurface();
    presentationDC = CreateCompatibleDC(target);
    presentationBitmap = CreateCompatibleBitmap(target, width, height);
    if (!presentationDC || !presentationBitmap) { ReleasePresentationSurface(); return false; }
    previousPresentationBitmap = SelectObject(presentationDC, presentationBitmap);
    if (!previousPresentationBitmap || previousPresentationBitmap == HGDI_ERROR)
    { previousPresentationBitmap = nullptr; ReleasePresentationSurface(); return false; }
    presentationWidth = width; presentationHeight = height;
    return true;
}

void URegionInfoPopup::Setup(HINSTANCE, RECT clientRect)
{
    panelRect = StarlightUI::Centered(clientRect, 830, 500);
    SetLocation((panelRect.left + panelRect.right) / 2, (panelRect.top + panelRect.bottom) / 2);
    SetSize(panelRect.right - panelRect.left, panelRect.bottom - panelRect.top);

    btnEnter = new UButton();
    btnEnter->SetLocation(panelRect.right - 112, panelRect.bottom - 54);
    btnEnter->SetSize(160, 44);
    btnEnter->SetText(L"ENTER");
    StarlightUI::StyleButton(btnEnter, true);
    btnEnter->SetOnClick([this]() {
        if (!bVisible || entryCommitted || IsOpening()) return;
        entryCommitted = true;
        CancelPress();
        if (OnEnterClickCallback) OnEnterClickCallback();
    });
    AddChild(btnEnter);

    btnClose = new UButton();
    btnClose->SetLocation(panelRect.right - 284, panelRect.bottom - 54);
    btnClose->SetSize(160, 44);
    btnClose->SetText(L"CLOSE");
    StarlightUI::StyleButton(btnClose);
    btnClose->SetOnClick([this]() {
        if (!bVisible || entryCommitted || IsOpening()) return;
        SetVisible(false);
        if (OnCloseClickCallback) OnCloseClickCallback();
    });
    AddChild(btnClose);
}

void URegionInfoPopup::SetInfo(const std::wstring& name, const std::wstring& desc, const std::wstring& env)
{
    rName = name;
    rDesc = desc;
    rEnv = env;
    SetVisible(true);
}

void URegionInfoPopup::SetMapPreview(HBITMAP bitmap, const RECT& worldBounds, const std::wstring& parent)
{
    parentRegion = parent;
    mapPreview = nullptr;
    mapPreviewInfo = {};
    if (bitmap && GetObject(bitmap, sizeof(mapPreviewInfo), &mapPreviewInfo) &&
        mapPreviewInfo.bmWidth > 0 && mapPreviewInfo.bmHeight > 0)
    {
        mapPreview = bitmap;
        mapPreviewBounds = worldBounds;
    }
}

void URegionInfoPopup::DrawMapPreview(HDC target, HDC scratch, const RECT& destination)
{
    StarlightUI::Surface(target, destination);
    RECT imageRect = destination;
    InflateRect(&imageRect, -2, -2);
    const int width = imageRect.right - imageRect.left;
    const int height = imageRect.bottom - imageRect.top;
    if (!mapPreview || !scratch || width <= 0 || height <= 0) return;

    // Fit one uniformly scaled crop around the landmark, including nearby map context.
    int cropWidth = static_cast<int>((std::max)(1L, mapPreviewBounds.right - mapPreviewBounds.left + 100));
    int cropHeight = static_cast<int>((std::max)(1L, mapPreviewBounds.bottom - mapPreviewBounds.top + 80));
    if (cropWidth * height < cropHeight * width)
        cropWidth = (cropHeight * width + height - 1) / height;
    else
        cropHeight = (cropWidth * height + width - 1) / width;
    if (cropWidth > mapPreviewInfo.bmWidth) {
        cropWidth = mapPreviewInfo.bmWidth;
        cropHeight = (std::max)(1, cropWidth * height / width);
    }
    if (cropHeight > mapPreviewInfo.bmHeight) {
        cropHeight = mapPreviewInfo.bmHeight;
        cropWidth = (std::max)(1, cropHeight * width / height);
    }
    const int centerX = (mapPreviewBounds.left + mapPreviewBounds.right) / 2;
    const int centerY = (mapPreviewBounds.top + mapPreviewBounds.bottom) / 2;
    const int sourceX = (std::max)(0, (std::min)(centerX - cropWidth / 2, static_cast<int>(mapPreviewInfo.bmWidth) - cropWidth));
    const int sourceY = (std::max)(0, (std::min)(centerY - cropHeight / 2, static_cast<int>(mapPreviewInfo.bmHeight) - cropHeight));
    const int saved = SaveDC(target);
    SetStretchBltMode(target, COLORONCOLOR);
    HBITMAP oldBitmap = static_cast<HBITMAP>(SelectObject(scratch, mapPreview));
    StretchBlt(target, imageRect.left, imageRect.top, width, height,
        scratch, sourceX, sourceY, cropWidth, cropHeight, SRCCOPY);
    SelectObject(scratch, oldBitmap);
    RestoreDC(target, saved);
}

void URegionInfoPopup::Draw(HDC hMemDC, HDC hMemDC1)
{
    if (!IsPresenting() || presentationOpacity <= 0.0 || !hMemDC || !hMemDC1) return;
    if (presentationOpacity >= 1.0)
    {
        DrawContent(hMemDC, hMemDC1);
        return;
    }
    // Copy the live backdrop, then draw the unchanged popup over it. Constant
    // alpha avoids relying on GDI text's undefined per-pixel alpha channel.
    const RECT bounds{ panelRect.left - 6, panelRect.top - 44, panelRect.right + 6, panelRect.bottom + 6 };
    const int width = bounds.right - bounds.left, height = bounds.bottom - bounds.top;
    if (!EnsurePresentationSurface(hMemDC, width, height))
    { DrawContent(hMemDC, hMemDC1); return; }
    BitBlt(presentationDC, 0, 0, width, height, hMemDC, bounds.left, bounds.top, SRCCOPY);
    const int saved = SaveDC(presentationDC);
    SetViewportOrgEx(presentationDC, -bounds.left, -bounds.top, nullptr);
    DrawContent(presentationDC, hMemDC1);
    RestoreDC(presentationDC, saved);
    const BLENDFUNCTION blend{ AC_SRC_OVER, 0, static_cast<BYTE>(std::lround(presentationOpacity * 255.0)), 0 };
    AlphaBlend(hMemDC, bounds.left, bounds.top, width, height, presentationDC, 0, 0, width, height, blend);
}

void URegionInfoPopup::DrawContent(HDC hMemDC, HDC hMemDC1)
{
    StarlightUI::Panel(hMemDC, panelRect, rName, parentRegion);
    const int contentTop = panelRect.top + 108;
    const int contentBottom = panelRect.bottom - 104;
    int textLeft = panelRect.left + 32;
    if (mapPreview) {
        const int landscapeWidth = (panelRect.right - panelRect.left) * 2 / 5;
        RECT landscape = { panelRect.left + 32, contentTop,
            panelRect.left + landscapeWidth, contentBottom };
        DrawMapPreview(hMemDC, hMemDC1, landscape);
        textLeft = landscape.right + 28;
    }
    RECT envRect = { textLeft, contentTop, panelRect.right - 32, contentTop + 42 };
    StarlightUI::Text(hMemDC, L"Env: " + rEnv, envRect, 18, StarlightUI::Gold);
    RECT descRect = { textLeft, contentTop + 58, panelRect.right - 32, contentBottom };
    StarlightUI::Text(hMemDC, rDesc, descRect, 20);
    // Children retain their normal visibility while the parent closes visually.
    // Do not temporarily change bVisible: input observes the logical closed state.
    for (auto child : Contents) child->Draw(hMemDC, hMemDC1);
}
