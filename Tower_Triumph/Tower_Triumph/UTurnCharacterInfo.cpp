#include "UTurnCharacterInfo.h"

#include <string>
#include <map>
#include <vector>


static bool ContainsTurnIconText(const std::wstring& value, const std::wstring& keyword)
{
    return value.find(keyword) != std::wstring::npos;
}

static std::wstring GetEnemyTurnIconFileName(const std::wstring& name)
{
    if (ContainsTurnIconText(name, L"Skeleton")) return L"S1_Skeleton_Turn.bmp";
    if (ContainsTurnIconText(name, L"Lich")) return L"S1_Leach_Turn.bmp";

    if (ContainsTurnIconText(name, L"Baby Angel")) return L"S2_BAngel_Turn.bmp";
    if (ContainsTurnIconText(name, L"Wing Angel")) return L"S2_WAngel_Turn.bmp";
    if (ContainsTurnIconText(name, L"Falcon")) return L"S2_Falcon_Turn.bmp";
    if (ContainsTurnIconText(name, L"Angel Guard")) return L"S2_Angel_Turn.bmp";
    if (ContainsTurnIconText(name, L"Angel")) return L"S2_Angel_Turn.bmp";

    if (ContainsTurnIconText(name, L"Sea Slime")) return L"S4_Slime_Turn.bmp";
    if (ContainsTurnIconText(name, L"Mermaid")) return L"S4_Snale_Turn.bmp";
    if (ContainsTurnIconText(name, L"Spear Crab")) return L"S4_Spear_Turn.bmp";
    if (ContainsTurnIconText(name, L"Jellyfish")) return L"S4_WFly_Turn.bmp";

    if (ContainsTurnIconText(name, L"Fire Demon")) return L"S5_Dragon_Turn.bmp";
    if (ContainsTurnIconText(name, L"Lava Golem")) return L"S5_Golem_Turn.bmp";
    if (ContainsTurnIconText(name, L"Lava Hound")) return L"S5_Hound_Turn.bmp";
    if (ContainsTurnIconText(name, L"Lava Larva")) return L"S5_Larva_Turn.bmp";

    return L"";
}

static HBITMAP LoadTurnIconBitmap(const std::wstring& fileName, BITMAP* outInfo)
{
    static std::map<std::wstring, HBITMAP> cache;

    if (fileName.empty())
    {
        return NULL;
    }

    auto found = cache.find(fileName);
    if (found != cache.end())
    {
        if (found->second != NULL && outInfo != nullptr)
        {
            GetObject(found->second, sizeof(BITMAP), outInfo);
        }

        return found->second;
    }

    std::vector<std::wstring> pathCandidates;
    pathCandidates.push_back(L"Resource\\TurnEnemy\\" + fileName);
    pathCandidates.push_back(L"Tower_Triumph\\Tower_Triumph\\Resource\\TurnEnemy\\" + fileName);
    pathCandidates.push_back(L"Tower_Triumph\\Resource\\TurnEnemy\\" + fileName);
    pathCandidates.push_back(L"..\\Tower_Triumph\\Tower_Triumph\\Resource\\TurnEnemy\\" + fileName);
    pathCandidates.push_back(L"..\\Tower_Triumph\\Resource\\TurnEnemy\\" + fileName);

    HBITMAP loaded = NULL;

    for (const std::wstring& path : pathCandidates)
    {
        loaded = (HBITMAP)LoadImageW(
            NULL,
            path.c_str(),
            IMAGE_BITMAP,
            0,
            0,
            LR_LOADFROMFILE | LR_CREATEDIBSECTION
        );

        if (loaded != NULL)
        {
            break;
        }
    }

    cache[fileName] = loaded;

    if (loaded != NULL && outInfo != nullptr)
    {
        GetObject(loaded, sizeof(BITMAP), outInfo);
    }

    return loaded;
}



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

    // A matching approved portrait belongs to the loaded enemy sprite set.
    // Keep the old name-based icon only as the legacy-sheet fallback.
    if (isEnemy && owner->GetIllustration() == NULL)
    {
        std::wstring iconFileName = GetEnemyTurnIconFileName(owner->GetName());

        if (!iconFileName.empty())
        {
            BITMAP iconInfo;
            ZeroMemory(&iconInfo, sizeof(BITMAP));

            HBITMAP hIcon = LoadTurnIconBitmap(iconFileName, &iconInfo);

            if (hIcon != NULL && iconInfo.bmWidth > 0 && iconInfo.bmHeight > 0)
            {
                HDC hIconDC = CreateCompatibleDC(hMemDC);
                HBITMAP hOldIcon = (HBITMAP)SelectObject(hIconDC, hIcon);

                StretchBlt(
                    hMemDC,
                    portraitRect.left,
                    portraitRect.top,
                    portraitRect.right - portraitRect.left,
                    portraitRect.bottom - portraitRect.top,
                    hIconDC,
                    0,
                    0,
                    iconInfo.bmWidth,
                    iconInfo.bmHeight,
                    SRCCOPY
                );

                SelectObject(hIconDC, hOldIcon);
                DeleteDC(hIconDC);
                return;
            }
        }
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

        srcW = spriteInfo.bmWidth;
        srcH = spriteInfo.bmHeight;
        if (!CharacterSpriteLayout::IdlePose::HasDimensions(spriteInfo.bmWidth, spriteInfo.bmHeight))
        {
            srcW /= CHARACTER_SHEET_COL;
            if (spriteInfo.bmHeight % 3 == 0 && (spriteInfo.bmHeight / 3) == srcW)
            {
                srcH = spriteInfo.bmHeight / 3;
            }
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
        useSpriteFallback ? CHARACTER_TRANSPARENT_COLOR : owner->GetIllustrationTransparentColor()
    );

    SelectObject(hPortraitDC, hOld);
    DeleteDC(hPortraitDC);
}
