#include "UTurnCharacterInfo.h"

#include <string>

UTurnCharacterInfo::UTurnCharacterInfo(Character* character, bool enemy)
    : owner(character), isEnemy(enemy), isCurrentTurn(false)
{
    Size.sX = 80;
    Size.sY = 80;
}

void UTurnCharacterInfo::Draw(HDC hMemDC, HDC hMemDC1)
{
    if (!owner)
    {
        return;
    }

    int x = Loc.x - Size.sX / 2;
    int y = Loc.y - Size.sY / 2;

    HBRUSH frameBrush = CreateSolidBrush(isEnemy ? RGB(180, 40, 40) : RGB(40, 40, 180));

    if (isCurrentTurn)
    {
        DeleteObject(frameBrush);
        frameBrush = CreateSolidBrush(isEnemy ? RGB(255, 80, 80) : RGB(80, 80, 255));
    }

    RECT frameRect = { x, y, x + Size.sX, y + Size.sY };
    FillRect(hMemDC, &frameRect, frameBrush);
    DeleteObject(frameBrush);

    int margin = isCurrentTurn ? 6 : 4;
    int portraitH = Size.sY - margin * 2;

    RECT portraitRect =
    {
        x + margin,
        y + margin,
        x + margin + portraitH,
        y + Size.sY - margin
    };

    if (!isCurrentTurn)
    {
        portraitRect =
        {
            x + margin,
            y + margin,
            x + Size.sX - margin,
            y + Size.sY - margin
        };
    }

    HBRUSH innerBrush = CreateSolidBrush(RGB(20, 20, 20));
    FillRect(hMemDC, &portraitRect, innerBrush);
    DeleteObject(innerBrush);

    DrawPortrait(hMemDC, portraitRect);

    if (isCurrentTurn)
    {
        int infoX = portraitRect.right + 10;
        int infoY = y + margin;

        SetTextColor(hMemDC, RGB(255, 255, 255));
        SetBkMode(hMemDC, TRANSPARENT);

        HFONT nameFont = CreateFont(
            20, 0, 0, 0,
            FW_BOLD,
            FALSE, FALSE, FALSE,
            DEFAULT_CHARSET,
            OUT_DEFAULT_PRECIS,
            CLIP_DEFAULT_PRECIS,
            DEFAULT_QUALITY,
            DEFAULT_PITCH,
            L"Arial"
        );

        HFONT oldFont = (HFONT)SelectObject(hMemDC, nameFont);

        std::wstring name = isEnemy ? L"적" : L"아군";
        TextOutW(hMemDC, infoX, infoY, name.c_str(), (int)name.length());

        SelectObject(hMemDC, oldFont);
        DeleteObject(nameFont);

        HFONT detailFont = CreateFont(
            16, 0, 0, 0,
            FW_NORMAL,
            FALSE, FALSE, FALSE,
            DEFAULT_CHARSET,
            OUT_DEFAULT_PRECIS,
            CLIP_DEFAULT_PRECIS,
            DEFAULT_QUALITY,
            DEFAULT_PITCH,
            L"Arial"
        );

        oldFont = (HFONT)SelectObject(hMemDC, detailFont);

        std::wstring className = L"Unknown";

        switch (owner->GetClassType())
        {
        case ECharacterClass::Warrior:
            className = L"전사";
            break;

        case ECharacterClass::Archer:
            className = L"궁수";
            break;

        case ECharacterClass::Mage:
            className = L"마법사";
            break;
        }

        TextOutW(hMemDC, infoX, infoY + 25, className.c_str(), (int)className.length());

        CharacterStat stat = owner->GetStat();

        std::wstring hpStr =
            L"HP: " +
            std::to_wstring(stat.CurHP) +
            L" / " +
            std::to_wstring(stat.MaxHP);

        TextOutW(hMemDC, infoX, infoY + 45, hpStr.c_str(), (int)hpStr.length());

        std::wstring mpStr =
            L"MP: " +
            std::to_wstring(stat.CurMP) +
            L" / " +
            std::to_wstring(stat.MaxMP);

        TextOutW(hMemDC, infoX, infoY + 65, mpStr.c_str(), (int)mpStr.length());

        SelectObject(hMemDC, oldFont);
        DeleteObject(detailFont);
    }
}

void UTurnCharacterInfo::DrawPortrait(HDC hMemDC, const RECT& portraitRect)
{
    if (owner == nullptr)
    {
        return;
    }

    HBITMAP hPortrait = owner->GetIllustration();
    const BITMAP* portraitInfo = owner->GetIllustrationInfo();

    bool useSpriteFallback = false;
    if (hPortrait == NULL || portraitInfo == nullptr)
    {
        hPortrait = owner->GetSpriteIdle();
        portraitInfo = nullptr;
        useSpriteFallback = true;
    }

    if (hPortrait == NULL)
    {
        HBRUSH hFallbackBrush = CreateSolidBrush(RGB(100, 100, 100));
        FillRect(hMemDC, &portraitRect, hFallbackBrush);
        DeleteObject(hFallbackBrush);

        int oldMode = SetBkMode(hMemDC, TRANSPARENT);
        COLORREF oldColor = SetTextColor(hMemDC, RGB(255, 255, 255));

        std::wstring noImage = L"NO IMAGE";
        DrawTextW(hMemDC, noImage.c_str(), -1, (RECT*)&portraitRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

        SetTextColor(hMemDC, oldColor);
        SetBkMode(hMemDC, oldMode);
        return;
    }

    int srcW = 0;
    int srcH = 0;

    if (useSpriteFallback)
    {
        BITMAP spriteInfo;
        ZeroMemory(&spriteInfo, sizeof(BITMAP));
        GetObject(hPortrait, sizeof(BITMAP), &spriteInfo);

        srcW = spriteInfo.bmWidth / CHARACTER_SHEET_COL;
        srcH = spriteInfo.bmHeight;

        if (spriteInfo.bmHeight % 3 == 0 && (spriteInfo.bmHeight / 3) == srcW)
        {
            srcH = spriteInfo.bmHeight / 3;
        }
    }
    else
    {
        srcW = portraitInfo->bmWidth;
        srcH = portraitInfo->bmHeight;
    }

    if (srcW <= 0 || srcH <= 0)
    {
        return;
    }

    int portraitW = portraitRect.right - portraitRect.left;
    int portraitH = portraitRect.bottom - portraitRect.top;

    int dstW = portraitW;
    int dstH = portraitH;

    float srcRatio = (float)srcW / (float)srcH;
    float dstRatio = (float)portraitW / (float)portraitH;

    if (srcRatio > dstRatio)
    {
        dstW = portraitW;
        dstH = (int)((float)portraitW / srcRatio);
    }
    else
    {
        dstH = portraitH;
        dstW = (int)((float)portraitH * srcRatio);
    }

    int dstX = portraitRect.left + (portraitW - dstW) / 2;
    int dstY = portraitRect.top + (portraitH - dstH) / 2;

    HDC hPortraitDC = CreateCompatibleDC(hMemDC);
    HBITMAP hOld = (HBITMAP)SelectObject(hPortraitDC, hPortrait);

    TransparentBlt(
        hMemDC,
        dstX,
        dstY,
        dstW,
        dstH,
        hPortraitDC,
        0,
        0,
        srcW,
        srcH,
        CHARACTER_TRANSPARENT_COLOR
    );

    SelectObject(hPortraitDC, hOld);
    DeleteDC(hPortraitDC);
}
