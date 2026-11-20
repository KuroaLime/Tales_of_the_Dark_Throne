#include "ULocationIndicator.h"
#include "StarlightUI.h"
#include <algorithm>

ULocationIndicator::ULocationIndicator()
    : hThumbnail(nullptr), thumbInfo{}
{
    bVisible = false;
}

void ULocationIndicator::Setup(RECT clientRect)
{
    SetSize(420, 96);
    SetLocation(clientRect.left + 32 + Size.sX / 2, clientRect.bottom - 32 - Size.sY / 2);
}

void ULocationIndicator::SetInfo(const std::wstring& parent, const std::wstring& point, HBITMAP hThumb)
{
    parentRegion = parent;
    pointName = point;
    hThumbnail = hThumb;
    thumbInfo = {};
    if (hThumbnail && !GetObject(hThumbnail, sizeof(thumbInfo), &thumbInfo)) hThumbnail = nullptr;
    bVisible = true;
}

void ULocationIndicator::Draw(HDC hMemDC, HDC hMemDC1)
{
    if (!bVisible) return;
    RECT panel = { Loc.x - Size.sX / 2, Loc.y - Size.sY / 2,
        Loc.x + Size.sX / 2, Loc.y + Size.sY / 2 };
    StarlightUI::Surface(hMemDC, panel);
    int textLeft = panel.left + 22;

    if (hThumbnail && thumbInfo.bmWidth > 0 && thumbInfo.bmHeight > 0) {
        const double scale = (std::min)(64.0 / thumbInfo.bmWidth, 64.0 / thumbInfo.bmHeight);
        const int width = (std::max)(1, static_cast<int>(thumbInfo.bmWidth * scale));
        const int height = (std::max)(1, static_cast<int>(thumbInfo.bmHeight * scale));
        HBITMAP previous = static_cast<HBITMAP>(SelectObject(hMemDC1, hThumbnail));
        TransparentBlt(hMemDC, panel.left + 16 + (64 - width) / 2, panel.top + (Size.sY - height) / 2,
            width, height, hMemDC1, 0, 0, thumbInfo.bmWidth, thumbInfo.bmHeight, RGB(255, 255, 255));
        SelectObject(hMemDC1, previous);
        textLeft = panel.left + 98;
    }

    RECT parentRect = { textLeft, panel.top + 16, panel.right - 20, panel.top + 40 };
    StarlightUI::Text(hMemDC, parentRegion, parentRect, 15, StarlightUI::Gold);
    RECT nameRect = { textLeft, panel.top + 44, panel.right - 20, panel.bottom - 14 };
    StarlightUI::Text(hMemDC, pointName, nameRect, 22);
    UContainerWidget::Draw(hMemDC, hMemDC1);
}
