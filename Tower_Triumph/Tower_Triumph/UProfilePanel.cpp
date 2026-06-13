#include "UProfilePanel.h"

#include <string>

UProfilePanel::UProfilePanel()
    : CurrentCharacter(nullptr),
      NameLabel(nullptr),
      LevelLabel(nullptr),
      ElementLabel(nullptr)
{
    Size.sX = 320;
    Size.sY = 410;
}

UProfilePanel::~UProfilePanel()
{
}

void UProfilePanel::Setup(int x, int y)
{
    Loc.x = x;
    Loc.y = y;
    Size.sX = 320;
    Size.sY = 410;

    NameLabel = new UTextLabel();
    NameLabel->SetLocation(Loc.x, Loc.y - 165);
    NameLabel->SetSize(280, 36);
    NameLabel->SetFont(24, FW_BOLD, L"Malgun Gothic");
    NameLabel->SetColor(RGB(255, 230, 120));
    NameLabel->SetAlign(DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    AddChild(NameLabel);

    LevelLabel = new UTextLabel();
    LevelLabel->SetLocation(Loc.x, Loc.y - 130);
    LevelLabel->SetSize(280, 28);
    LevelLabel->SetFont(17, FW_BOLD, L"Malgun Gothic");
    LevelLabel->SetColor(RGB(220, 220, 240));
    LevelLabel->SetAlign(DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    AddChild(LevelLabel);

    ElementLabel = new UTextLabel();
    ElementLabel->SetLocation(Loc.x, Loc.y + 170);
    ElementLabel->SetSize(280, 30);
    ElementLabel->SetFont(18, FW_BOLD, L"Malgun Gothic");
    ElementLabel->SetColor(RGB(160, 220, 255));
    ElementLabel->SetAlign(DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    AddChild(ElementLabel);
}

void UProfilePanel::SetCharacter(Character* character)
{
    CurrentCharacter = character;

    if (CurrentCharacter == nullptr)
    {
        if (NameLabel) NameLabel->SetText(L"");
        if (LevelLabel) LevelLabel->SetText(L"");
        if (ElementLabel) ElementLabel->SetText(L"");
        return;
    }

    if (NameLabel)
    {
        NameLabel->SetText(CurrentCharacter->GetName());
    }

    if (LevelLabel)
    {
        LevelLabel->SetText(L"Lv. " + std::to_wstring(CurrentCharacter->GetLevel()) + L"    EXP " + std::to_wstring(CurrentCharacter->GetExp()));
    }

    if (ElementLabel)
    {
        ElementLabel->SetText(L"속성: " + CurrentCharacter->GetElementName());
    }
}

void UProfilePanel::Draw(HDC hMemDC, HDC hMemDC1)
{
    if (!bVisible)
    {
        return;
    }

    RECT panel = { Loc.x - Size.sX / 2, Loc.y - Size.sY / 2, Loc.x + Size.sX / 2, Loc.y + Size.sY / 2 };

    HBRUSH bgBrush = CreateSolidBrush(RGB(19, 23, 31));
    FillRect(hMemDC, &panel, bgBrush);
    DeleteObject(bgBrush);

    HBRUSH frameBrush = CreateSolidBrush(RGB(176, 146, 78));
    FrameRect(hMemDC, &panel, frameBrush);
    DeleteObject(frameBrush);

    RECT portraitBox = { panel.left + 42, panel.top + 105, panel.right - 42, panel.bottom - 62 };
    HBRUSH innerBrush = CreateSolidBrush(RGB(12, 12, 18));
    FillRect(hMemDC, &portraitBox, innerBrush);
    DeleteObject(innerBrush);

    if (CurrentCharacter != nullptr)
    {
        HBITMAP hPortrait = CurrentCharacter->GetIllustration();
        const BITMAP* portraitInfo = CurrentCharacter->GetIllustrationInfo();
        bool useSpriteFallback = false;

        if (hPortrait == NULL || portraitInfo == nullptr)
        {
            hPortrait = CurrentCharacter->GetSpriteIdle();
            useSpriteFallback = true;
        }

        if (hPortrait != NULL)
        {
            BITMAP bm;
            GetObject(hPortrait, sizeof(BITMAP), &bm);

            int srcW = bm.bmWidth;
            int srcH = bm.bmHeight;

            if (useSpriteFallback)
            {
                srcW = bm.bmWidth / CHARACTER_SHEET_COL;
                srcH = bm.bmHeight / CHARACTER_SHEET_ROW;
            }

            HDC hTempDC = CreateCompatibleDC(hMemDC);
            HBITMAP hOld = (HBITMAP)SelectObject(hTempDC, hPortrait);

            int dstW = portraitBox.right - portraitBox.left;
            int dstH = portraitBox.bottom - portraitBox.top;
            COLORREF transColor = useSpriteFallback ? CHARACTER_TRANSPARENT_COLOR : RGB(255, 255, 255);

            TransparentBlt(
                hMemDC,
                portraitBox.left,
                portraitBox.top,
                dstW,
                dstH,
                hTempDC,
                0,
                0,
                srcW,
                srcH,
                transColor
            );

            SelectObject(hTempDC, hOld);
            DeleteDC(hTempDC);
        }
    }

    UContainerWidget::Draw(hMemDC, hMemDC1);
}
