#include "UProfilePanel.h"
#include "StarlightUI.h"
#include <algorithm>

UProfilePanel::UProfilePanel() : currentCharacter(nullptr), panelBounds{} {}
void UProfilePanel::Setup(int x, int y)
{
    SetPanelBounds({ x - 125, y - 199, x + 125, y + 199 });
}
void UProfilePanel::SetPanelBounds(const RECT& bounds)
{
    panelBounds = bounds;
    SetLocation((bounds.left + bounds.right) / 2, (bounds.top + bounds.bottom) / 2);
    SetSize(bounds.right - bounds.left, bounds.bottom - bounds.top);
}
void UProfilePanel::SetCharacter(Character* character)
{
    currentCharacter = character;
}
void UProfilePanel::Draw(HDC hMemDC, HDC)
{
    if (!bVisible) return;
    StarlightUI::Surface(hMemDC, panelBounds);
    const RECT portraitBounds = { panelBounds.left + 18, panelBounds.top + 18,
        panelBounds.right - 18, panelBounds.bottom - 132 };
    if (!currentCharacter)
    {
        StarlightUI::Text(hMemDC, L"캐릭터를 선택하세요.", portraitBounds, 17,
            StarlightUI::Muted, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        return;
    }
    HBITMAP portrait = currentCharacter->GetIllustration();
    const bool fallback = !portrait || !currentCharacter->GetIllustrationInfo();
    if (fallback) portrait = currentCharacter->GetSpriteIdle();
    if (portrait)
    {
        BITMAP bitmap{};
        GetObject(portrait, sizeof(bitmap), &bitmap);
        int width = bitmap.bmWidth, height = bitmap.bmHeight;
        if (fallback && !CharacterSpriteLayout::IdlePose::HasDimensions(width, height))
        {
            width /= CHARACTER_SHEET_COL;
            height /= CHARACTER_SHEET_ROW;
        }
        if (width > 0 && height > 0)
        {
            const double scale = (std::min)(double(portraitBounds.right - portraitBounds.left) / width,
                double(portraitBounds.bottom - portraitBounds.top) / height);
            const int drawW = (std::max)(1, int(width * scale));
            const int drawH = (std::max)(1, int(height * scale));
            HDC spriteDC = CreateCompatibleDC(hMemDC);
            HGDIOBJ oldBitmap = SelectObject(spriteDC, portrait);
            const int oldMode = SetStretchBltMode(hMemDC, COLORONCOLOR);
            TransparentBlt(hMemDC, (portraitBounds.left + portraitBounds.right - drawW) / 2,
                (portraitBounds.top + portraitBounds.bottom - drawH) / 2, drawW, drawH,
                spriteDC, 0, 0, width, height,
                fallback ? CHARACTER_TRANSPARENT_COLOR : currentCharacter->GetIllustrationTransparentColor());
            SetStretchBltMode(hMemDC, oldMode);
            SelectObject(spriteDC, oldBitmap);
            DeleteDC(spriteDC);
        }
    }
    const UINT centered = DT_CENTER | DT_VCENTER | DT_SINGLELINE;
    StarlightUI::Text(hMemDC, currentCharacter->GetName(),
        { panelBounds.left + 10, panelBounds.bottom - 118, panelBounds.right - 10, panelBounds.bottom - 78 },
        22, StarlightUI::Ink, centered);
    StarlightUI::Text(hMemDC, L"Lv. " + std::to_wstring(currentCharacter->GetLevel()),
        { panelBounds.left + 12, panelBounds.bottom - 75, panelBounds.right - 12, panelBounds.bottom - 47 },
        19, StarlightUI::Gold, centered);
    StarlightUI::Text(hMemDC, L"EXP: " + std::to_wstring(currentCharacter->GetExp()),
        { panelBounds.left + 12, panelBounds.bottom - 44, panelBounds.right - 12, panelBounds.bottom - 16 },
        17, StarlightUI::Muted, centered);
}
