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

        std::wstring name = isEnemy ? L"ENEMY" : L"HERO";
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
            className = L"Warrior";
            break;

        case ECharacterClass::Archer:
            className = L"Archer";
            break;

        case ECharacterClass::Mage:
            className = L"Mage";
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
    HBITMAP hSprite = owner->GetSpriteIdle();

    if (hSprite == NULL)
    {
        HBRUSH hFallbackBrush = CreateSolidBrush(RGB(100, 100, 100));

        RECT fallbackRect =
        {
            portraitRect.left + 5,
            portraitRect.top + 5,
            portraitRect.right - 5,
            portraitRect.bottom - 5
        };

        FillRect(hMemDC, &fallbackRect, hFallbackBrush);
        DeleteObject(hFallbackBrush);

        SetTextColor(hMemDC, RGB(255, 255, 255));
        SetBkMode(hMemDC, TRANSPARENT);
        DrawTextW(hMemDC, L"No Img", -1, (RECT*)&portraitRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

        return;
    }

    BITMAP spriteInfo;
    ZeroMemory(&spriteInfo, sizeof(BITMAP));
    GetObject(hSprite, sizeof(BITMAP), &spriteInfo);

    if (spriteInfo.bmWidth <= 0 || spriteInfo.bmHeight <= 0)
    {
        return;
    }

    // Temporary sprite sheet: 4 columns, auto-detect 1 or 3 rows.
    //
    int sheetCol = 4;
    int frameW = spriteInfo.bmWidth / sheetCol;

    if (frameW <= 0)
    {
        return;
    }

    int rowCount = 1;

    if (spriteInfo.bmHeight % 3 == 0)
    {
        int candidateFrameH = spriteInfo.bmHeight / 3;

    // Temporary sprite sheet: 4 columns, auto-detect 1 or 3 rows.
        if (candidateFrameH == frameW)
        {
            rowCount = 3;
        }
    }

    int frameH = spriteInfo.bmHeight / rowCount;

    if (frameH <= 0)
    {
        return;
    }

    // Use the first frame as the portrait.
    int srcX = 0;
    int srcY = 0;

    int portraitW = portraitRect.right - portraitRect.left;
    int portraitH = portraitRect.bottom - portraitRect.top;

    int dstW = portraitW;
    int dstH = portraitH;

    // Keep source aspect ratio.
    float srcRatio = (float)frameW / (float)frameH;
    float dstRatio = (float)portraitW / (float)portraitH;

    if (srcRatio > dstRatio)
    {
        dstW = portraitW;
        dstH = (int)(portraitW / srcRatio);
    }
    else
    {
        dstH = portraitH;
        dstW = (int)(portraitH * srcRatio);
    }

    int dstX = portraitRect.left + (portraitW - dstW) / 2;
    int dstY = portraitRect.top + (portraitH - dstH) / 2;

    HDC hSpriteDC = CreateCompatibleDC(hMemDC);
    HBITMAP hOld = (HBITMAP)SelectObject(hSpriteDC, hSprite);

    TransparentBlt(
        hMemDC,
        dstX,
        dstY,
        dstW,
        dstH,
        hSpriteDC,
        srcX,
        srcY,
        frameW,
        frameH,
        RGB(255, 0, 255)
    );

    SelectObject(hSpriteDC, hOld);
    DeleteDC(hSpriteDC);
}
