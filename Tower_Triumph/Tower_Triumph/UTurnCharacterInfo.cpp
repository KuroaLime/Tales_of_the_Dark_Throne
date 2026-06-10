#include "UTurnCharacterInfo.h"
#include "Character.h"
#include <string>

UTurnCharacterInfo::UTurnCharacterInfo(Character* character, bool enemy)
    : owner(character), isEnemy(enemy), isCurrentTurn(false)
{
    Size.sX = 80;
    Size.sY = 80;
}

void UTurnCharacterInfo::Draw(HDC hMemDC, HDC hMemDC1)
{
    if (!owner) return;

    int x = Loc.x - Size.sX / 2;
    int y = Loc.y - Size.sY / 2;

    HBRUSH frameBrush = CreateSolidBrush(isEnemy ? RGB(180, 40, 40) : RGB(40, 40, 180));
    if (isCurrentTurn) {
        DeleteObject(frameBrush);
        frameBrush = CreateSolidBrush(isEnemy ? RGB(255, 80, 80) : RGB(80, 80, 255));
    }
    RECT frameRect = { x, y, x + Size.sX, y + Size.sY };
    FillRect(hMemDC, &frameRect, frameBrush);
    DeleteObject(frameBrush);

    int margin = isCurrentTurn ? 6 : 4;
    int portraitH = Size.sY - margin * 2;
    
    RECT portraitRect = { x + margin, y + margin, x + margin + portraitH, y + Size.sY - margin };
    if (!isCurrentTurn) {
        portraitRect = { x + margin, y + margin, x + Size.sX - margin, y + Size.sY - margin };
    }

    HBRUSH innerBrush = CreateSolidBrush(RGB(20, 20, 20));
    FillRect(hMemDC, &portraitRect, innerBrush);
    DeleteObject(innerBrush);

    HBITMAP hSprite = owner->GetSpriteIdle();
    if (hSprite != NULL) {
        HDC hSpriteDC = CreateCompatibleDC(hMemDC);
        HBITMAP hOld = (HBITMAP)SelectObject(hSpriteDC, hSprite);
        TransparentBlt(hMemDC, portraitRect.left + (portraitH - 64)/2, portraitRect.top + (portraitH - 64)/2, 64, 64, 
                       hSpriteDC, 0, 0, 64, 64, RGB(255, 0, 255));
        SelectObject(hSpriteDC, hOld);
        DeleteDC(hSpriteDC);
    }
    else {
        // Fallback: Draw a gray rectangle if portrait is missing
        HBRUSH hFallbackBrush = CreateSolidBrush(RGB(100, 100, 100));
        RECT fallbackRect = { portraitRect.left + 5, portraitRect.top + 5, portraitRect.right - 5, portraitRect.bottom - 5 };
        FillRect(hMemDC, &fallbackRect, hFallbackBrush);
        DeleteObject(hFallbackBrush);

        SetTextColor(hMemDC, RGB(255, 255, 255));
        SetBkMode(hMemDC, TRANSPARENT);
        DrawTextW(hMemDC, L"No Img", -1, &portraitRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    }

    if (isCurrentTurn) {
        int infoX = portraitRect.right + 10;
        int infoY = y + margin;

        SetTextColor(hMemDC, RGB(255, 255, 255));
        SetBkMode(hMemDC, TRANSPARENT);

        HFONT nameFont = CreateFont(20, 0, 0, 0, FW_BOLD, 0, 0, 0, DEFAULT_CHARSET, 0, 0, 0, 0, L"Arial");
        HFONT oldF = (HFONT)SelectObject(hMemDC, nameFont);
        
        std::wstring name = isEnemy ? L"ENEMY" : L"HERO";
        TextOutW(hMemDC, infoX, infoY, name.c_str(), (int)name.length());
        
        SelectObject(hMemDC, oldF);
        DeleteObject(nameFont);
        
        HFONT detailFont = CreateFont(16, 0, 0, 0, FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET, 0, 0, 0, 0, L"Arial");
        oldF = (HFONT)SelectObject(hMemDC, detailFont);
        
        std::wstring className = L"Unknown";
        switch(owner->GetClassType()) {
            case ECharacterClass::Warrior: className = L"Warrior"; break;
            case ECharacterClass::Archer: className = L"Archer"; break;
            case ECharacterClass::Mage: className = L"Mage"; break;
        }
        TextOutW(hMemDC, infoX, infoY + 25, className.c_str(), (int)className.length());

        CharacterStat stat = owner->GetStat();
        std::wstring hpStr = L"HP: " + std::to_wstring(stat.CurHP) + L" / " + std::to_wstring(stat.MaxHP);
        TextOutW(hMemDC, infoX, infoY + 45, hpStr.c_str(), (int)hpStr.length());

        SelectObject(hMemDC, oldF);
        DeleteObject(detailFont);
    }
}
