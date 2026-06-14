#include "UTextLabel.h"

UTextLabel::UTextLabel()
{
    hFont = NULL;
    TextColor = RGB(255, 255, 255);
    TextAlignFlags = DT_LEFT | DT_VCENTER | DT_SINGLELINE;
}

UTextLabel::~UTextLabel()
{
    if (hFont) DeleteObject(hFont);
}

void UTextLabel::SetFont(int nHeight, int nWeight, const std::wstring& fontName)
{
    if (hFont) DeleteObject(hFont);
    hFont = CreateFont(nHeight, 0, 0, 0, nWeight, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, fontName.c_str());
}

void UTextLabel::Draw(HDC hMemDC, HDC hMemDC1)
{
    if (!bVisible || Text.empty()) return;

    int oldMode = SetBkMode(hMemDC, TRANSPARENT);
    COLORREF oldColor = SetTextColor(hMemDC, TextColor);
    HFONT oldFont = NULL;

    if (hFont) {
        oldFont = (HFONT)SelectObject(hMemDC, hFont);
    }

    RECT drawRect = { Loc.x - Size.sX / 2, Loc.y - Size.sY / 2, Loc.x + Size.sX / 2, Loc.y + Size.sY / 2 };

    SetTextColor(hMemDC, RGB(0, 0, 0));
    RECT shadowRect = drawRect;
    OffsetRect(&shadowRect, 2, 2);
    DrawTextW(hMemDC, Text.c_str(), -1, &shadowRect, TextAlignFlags);

    SetTextColor(hMemDC, TextColor);
    DrawTextW(hMemDC, Text.c_str(), -1, &drawRect, TextAlignFlags);

    if (oldFont) SelectObject(hMemDC, oldFont);
    SetTextColor(hMemDC, oldColor);
    SetBkMode(hMemDC, oldMode);
}
