#include "ULocationIndicator.h"

ULocationIndicator::ULocationIndicator()
    : hThumbnail(NULL)
{
    bVisible = false;
}

void ULocationIndicator::Setup(RECT clientRect)
{
    Size.sX = 400;
    Size.sY = 100;
    Loc.x = 50 + Size.sX / 2;
    Loc.y = clientRect.bottom - 50 - Size.sY / 2;
}

void ULocationIndicator::SetInfo(const std::wstring& parent, const std::wstring& point, HBITMAP hThumb)
{
    parentRegion = parent;
    pointName = point;
    hThumbnail = hThumb;
    if (hThumbnail) {
        GetObject(hThumbnail, sizeof(BITMAP), &thumbInfo);
    }
    bVisible = true;
}

void ULocationIndicator::Draw(HDC hMemDC, HDC hMemDC1)
{
    if (!bVisible) return;

    // 1. Semi-transparent background
    HDC hTempDC = CreateCompatibleDC(hMemDC);
    HBITMAP hTempBit = CreateCompatibleBitmap(hMemDC, Size.sX, Size.sY);
    HBITMAP hOldBit = (HBITMAP)SelectObject(hTempDC, hTempBit);

    HBRUSH bgBrush = CreateSolidBrush(RGB(15, 15, 30));
    RECT r = { 0, 0, Size.sX, Size.sY };
    FillRect(hTempDC, &r, bgBrush);
    DeleteObject(bgBrush);

    BLENDFUNCTION bf;
    bf.BlendOp = AC_SRC_OVER;
    bf.BlendFlags = 0;
    bf.SourceConstantAlpha = 160; // ~60%
    bf.AlphaFormat = 0;

    AlphaBlend(hMemDC, Loc.x - Size.sX / 2, Loc.y - Size.sY / 2, Size.sX, Size.sY,
               hTempDC, 0, 0, Size.sX, Size.sY, bf);

    SelectObject(hTempDC, hOldBit);
    DeleteObject(hTempBit);
    DeleteDC(hTempDC);

    // 2. Thumbnail
    int drawX = Loc.x - Size.sX / 2 + 10;
    int drawY = Loc.y - Size.sY / 2 + 10;
    int thumbSize = 80;

    if (hThumbnail) {
        HBITMAP old = (HBITMAP)SelectObject(hMemDC1, hThumbnail);
        TransparentBlt(hMemDC, drawX, drawY, thumbSize, thumbSize,
                       hMemDC1, 0, 0, thumbInfo.bmWidth, thumbInfo.bmHeight, RGB(255, 255, 255));
        SelectObject(hMemDC1, old);
    } else {
        HBRUSH hB = CreateSolidBrush(RGB(50, 50, 70));
        RECT tr = { drawX, drawY, drawX + thumbSize, drawY + thumbSize };
        FillRect(hMemDC, &tr, hB);
        DeleteObject(hB);
    }

    // 3. Text
    int textX = drawX + thumbSize + 20;
    SetBkMode(hMemDC, TRANSPARENT);
    
    HFONT hTitleF = CreateFont(28, 0, 0, 0, FW_BOLD, 0, 0, 0, DEFAULT_CHARSET, 0, 0, 0, 0, L"Arial");
    HFONT hOldF = (HFONT)SelectObject(hMemDC, hTitleF);
    SetTextColor(hMemDC, RGB(255, 255, 255));
    TextOutW(hMemDC, textX, drawY + 10, parentRegion.c_str(), (int)parentRegion.length());
    DeleteObject(hTitleF);

    HFONT hPointF = CreateFont(20, 0, 0, 0, FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET, 0, 0, 0, 0, L"Arial");
    SelectObject(hMemDC, hPointF);
    SetTextColor(hMemDC, RGB(200, 200, 200));
    TextOutW(hMemDC, textX, drawY + 45, pointName.c_str(), (int)pointName.length());
    DeleteObject(hPointF);

    SelectObject(hMemDC, hOldF);

    UContainerWidget::Draw(hMemDC, hMemDC1);
}
