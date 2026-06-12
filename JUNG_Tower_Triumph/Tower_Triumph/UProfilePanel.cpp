#include "UProfilePanel.h"
#include <string>

UProfilePanel::UProfilePanel() : currentCharacter(nullptr)
{
    bgImage = nullptr;
    nameLabel = nullptr;
    levelLabel = nullptr;
}

void UProfilePanel::Setup(int x, int y)
{
    Loc.x = x;
    Loc.y = y;

    bgImage = new UImage();
    bgImage->SetFrameFromFile({ L"Resource/Map/UI/CharacterUI_Character.bmp" });
    if (!bgImage->GetBitmap().empty()) {
        BITMAP info = bgImage->GetMapInfo()[0];
        bgImage->SetSize(info.bmWidth, info.bmHeight);
        Size = bgImage->GetSize();
    }
    bgImage->SetTransColor(RGB(255, 174, 201));
    // Original's bgImage position was offset relative to Setup(x,y)
    bgImage->SetLocation(Loc.x, Loc.y); 
    AddChild(bgImage);

    // Labels positioned relative to the panel center
    nameLabel = new UTextLabel();
    nameLabel->SetFont(24, FW_BOLD, L"Arial");
    nameLabel->SetLocation(Loc.x-83, Loc.y + 135);
    nameLabel->SetSize(250, 40);
    nameLabel->SetAlign(DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    AddChild(nameLabel);

    levelLabel = new UTextLabel();
    levelLabel->SetFont(26, FW_NORMAL, L"Arial");
    levelLabel->SetLocation(Loc.x-78, Loc.y + 90);
    levelLabel->SetSize(250, 40);
    levelLabel->SetAlign(DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    AddChild(levelLabel);
}

void UProfilePanel::SetCharacter(Character* character)
{
    currentCharacter = character;
    if (currentCharacter) {
        nameLabel->SetText(currentCharacter->GetName());
        levelLabel->SetText(L"Lv. " + std::to_wstring(currentCharacter->GetLevel()) + L"  EXP: " + std::to_wstring(currentCharacter->GetExp()));
    } else {
        nameLabel->SetText(L"");
        levelLabel->SetText(L"");
    }
}

void UProfilePanel::Draw(HDC hMemDC, HDC hMemDC1)
{
    if (!bVisible) return;
    
    UContainerWidget::Draw(hMemDC, hMemDC1);

    if (currentCharacter) {
        HBITMAP hIllustration = currentCharacter->GetIllustration();
        const BITMAP* info = currentCharacter->GetIllustrationInfo();
        
        bool isFallback = false;
        if (!hIllustration || !info) {
            hIllustration = currentCharacter->GetSpriteIdle();
            isFallback = true;
        }

        if (hIllustration) {
            HDC hSpriteDC = CreateCompatibleDC(hMemDC);
            HBITMAP hOld = (HBITMAP)SelectObject(hSpriteDC, hIllustration);
            
            BITMAP bm;
            GetObject(hIllustration, sizeof(BITMAP), &bm);

            int srcW = bm.bmWidth;
            int srcH = bm.bmHeight;
            if (isFallback) {
                srcW = bm.bmWidth / CHARACTER_SHEET_COL;
                srcH = bm.bmHeight / CHARACTER_SHEET_ROW;
            }

            // Draw illustration centered in the upper part of the panel
            int drawW = srcW;
            int drawH = srcH;

            // Scale down if too large for panel
            float scale = 1.0f;
            if (drawW > 240) scale = 240.0f / drawW;
            if (drawH * scale > 300) scale = 300.0f / drawH;
            
            drawW = (int)(drawW * scale);
            drawH = (int)(drawH * scale);

            int drawX = Loc.x - drawW / 2;
            int drawY = Loc.y - 240; // Adjusted upward

            COLORREF transColor = isFallback ? CHARACTER_TRANSPARENT_COLOR : RGB(255, 255, 255);
            TransparentBlt(hMemDC, drawX, drawY, drawW, drawH, hSpriteDC, 0, 0, srcW, srcH, transColor);

            SelectObject(hSpriteDC, hOld);
            DeleteDC(hSpriteDC);
        }
    }
}
