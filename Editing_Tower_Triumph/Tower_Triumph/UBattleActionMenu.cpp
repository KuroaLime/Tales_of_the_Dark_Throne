#include "UBattleActionMenu.h"
#include "Character.h"

UBattleActionMenu::UBattleActionMenu()
{
    btnMove = new UButton();
    btnAttack = new UButton();
    btnSkill = new UButton();
    btnDefend = new UButton();
    btnItem = new UButton();
    btnEndTurn = new UButton();

    ScreenRect = { 0, 0, 1280, 720 };

    // 버튼 2열 3행이 안정적으로 들어가도록
    // v2의 176 x 118에서 살짝 키운 크기
    PanelWidth = 190;
    PanelHeight = 142;
}

UBattleActionMenu::~UBattleActionMenu()
{
    // Contents vector in UContainerWidget will handle deletion.
}

void UBattleActionMenu::Init()
{
    UContainerWidget::Init();

    SetupCommandButton(btnMove, L"이동");
    AddChild(btnMove);

    SetupCommandButton(btnAttack, L"공격");
    AddChild(btnAttack);

    SetupCommandButton(btnSkill, L"마법");
    AddChild(btnSkill);

    SetupCommandButton(btnDefend, L"방어");
    AddChild(btnDefend);

    SetupCommandButton(btnItem, L"아이템");
    AddChild(btnItem);

    SetupCommandButton(btnEndTurn, L"대기");
    AddChild(btnEndTurn);

    SetSize(PanelWidth, PanelHeight);
    SetVisible(false);
}

void UBattleActionMenu::SetupCommandButton(UButton* button, const std::wstring& text)
{
    if (button == nullptr)
    {
        return;
    }

    button->SetText(text);

    // 패널 190 기준으로 2열 배치가 맞도록 72x24 사용
    button->SetSize(72, 24);
    button->SetFont(13, FW_BOLD, L"Malgun Gothic");

    // 버튼 확대 애니메이션 때문에 패널 밖으로 삐져나가 보일 수 있으므로 끔
    button->SetAnimationType(0);
    button->SetDrawTextFallbackBackground(true);

    button->SetFallbackColors(
        RGB(38, 44, 58),     // normal
        RGB(60, 72, 98),     // hover
        RGB(28, 33, 44),     // pressed
        RGB(120, 86, 28),    // selected
        RGB(34, 36, 40)      // disabled
    );

    button->SetFallbackTextColors(
        RGB(238, 232, 214),  // normal
        RGB(255, 224, 142),  // hover
        RGB(185, 175, 145),  // pressed
        RGB(255, 235, 160),  // selected
        RGB(120, 120, 120)   // disabled
    );

    button->SetFallbackBorder(RGB(160, 135, 76), 1);
}

void UBattleActionMenu::SetScreenRect(const RECT& clientRect)
{
    ScreenRect = clientRect;
}

int UBattleActionMenu::ClampInt(int value, int minValue, int maxValue) const
{
    if (maxValue < minValue)
    {
        return minValue;
    }

    if (value < minValue)
    {
        return minValue;
    }

    if (value > maxValue)
    {
        return maxValue;
    }

    return value;
}

bool UBattleActionMenu::IsRectIntersect(const RECT& a, const RECT& b) const
{
    if (a.right <= b.left) return false;
    if (b.right <= a.left) return false;
    if (a.bottom <= b.top) return false;
    if (b.bottom <= a.top) return false;

    return true;
}

bool UBattleActionMenu::IsCandidateUsable(const RECT& candidate, const RECT& avoidRect, int maxPanelBottom) const
{
    const int safeMargin = 8;
    const int topReserved = 36;

    if (candidate.left < ScreenRect.left + safeMargin) return false;
    if (candidate.right > ScreenRect.right - safeMargin) return false;
    if (candidate.top < ScreenRect.top + topReserved) return false;
    if (candidate.bottom > maxPanelBottom) return false;
    if (IsRectIntersect(candidate, avoidRect)) return false;

    return true;
}

void UBattleActionMenu::UpdatePosition(int screenX, int screenY)
{
    const int safeMargin = 8;
    const int topReserved = 36;

    // 하단 턴 순서 UI 영역은 비워둔다.
    int bottomReserved = 170;
    int maxPanelBottom = ScreenRect.bottom - bottomReserved;

    if (maxPanelBottom < ScreenRect.top + topReserved + PanelHeight)
    {
        maxPanelBottom = ScreenRect.bottom - safeMargin;
    }

    // 현재 캐릭터 주변은 피한다.
    // 행동창이 캐릭터를 직접 가리지 않도록 넉넉하게 잡는다.
    RECT avoidRect = {
        screenX - 40,
        screenY - 58,
        screenX + 40,
        screenY + 42
    };

    RECT candidates[4] = {
        // 우상단
        { screenX + 42, screenY - PanelHeight - 26, screenX + 42 + PanelWidth, screenY - 26 },

        // 좌상단
        { screenX - PanelWidth - 42, screenY - PanelHeight - 26, screenX - 42, screenY - 26 },

        // 우하단
        { screenX + 42, screenY + 34, screenX + 42 + PanelWidth, screenY + 34 + PanelHeight },

        // 좌하단
        { screenX - PanelWidth - 42, screenY + 34, screenX - 42, screenY + 34 + PanelHeight }
    };

    int panelLeft = candidates[0].left;
    int panelTop = candidates[0].top;
    bool found = false;

    for (int i = 0; i < 4; ++i)
    {
        if (IsCandidateUsable(candidates[i], avoidRect, maxPanelBottom))
        {
            panelLeft = candidates[i].left;
            panelTop = candidates[i].top;
            found = true;
            break;
        }
    }

    if (!found)
    {
        // 모든 후보가 막힌 경우에는 화면 안쪽으로만 보정한다.
        panelLeft = ClampInt(
            panelLeft,
            ScreenRect.left + safeMargin,
            ScreenRect.right - PanelWidth - safeMargin
        );

        panelTop = ClampInt(
            panelTop,
            ScreenRect.top + topReserved,
            maxPanelBottom - PanelHeight
        );
    }

    SetLocation(panelLeft + PanelWidth / 2, panelTop + PanelHeight / 2);
    SetSize(PanelWidth, PanelHeight);

    LayoutButtons(panelLeft, panelTop);
}

void UBattleActionMenu::LayoutButtons(int panelLeft, int panelTop)
{
    const int buttonW = 72;
    const int buttonH = 24;

    const int gapX = 10;
    const int gapY = 7;

    // 패널 190 기준:
    // 버튼 2개 + 간격 = 72 + 10 + 72 = 154
    // 남는 공간 36 -> 좌우 여백 18
    const int marginX = 18;

    // 제목 영역 아래에서 버튼 시작
    const int firstButtonTop = 38;

    int col0 = panelLeft + marginX + buttonW / 2;
    int col1 = col0 + buttonW + gapX;

    int row0 = panelTop + firstButtonTop + buttonH / 2;
    int row1 = row0 + buttonH + gapY;
    int row2 = row1 + buttonH + gapY;

    btnMove->SetLocation(col0, row0);
    btnAttack->SetLocation(col1, row0);

    btnSkill->SetLocation(col0, row1);
    btnDefend->SetLocation(col1, row1);

    btnItem->SetLocation(col0, row2);
    btnEndTurn->SetLocation(col1, row2);
}

void UBattleActionMenu::Draw(HDC hMemDC, HDC hMemDC1)
{
    if (!bVisible)
    {
        return;
    }

    RECT panelRect = {
        Loc.x - PanelWidth / 2,
        Loc.y - PanelHeight / 2,
        Loc.x + PanelWidth / 2,
        Loc.y + PanelHeight / 2
    };

    RECT shadowRect = panelRect;
    OffsetRect(&shadowRect, 3, 3);

    HBRUSH shadowBrush = CreateSolidBrush(RGB(0, 0, 0));
    HPEN noPen = CreatePen(PS_NULL, 0, RGB(0, 0, 0));

    HPEN oldPen = (HPEN)SelectObject(hMemDC, noPen);
    HBRUSH oldBrush = (HBRUSH)SelectObject(hMemDC, shadowBrush);

    RoundRect(
        hMemDC,
        shadowRect.left,
        shadowRect.top,
        shadowRect.right,
        shadowRect.bottom,
        10,
        10
    );

    SelectObject(hMemDC, oldBrush);
    SelectObject(hMemDC, oldPen);

    DeleteObject(noPen);
    DeleteObject(shadowBrush);

    HBRUSH panelBrush = CreateSolidBrush(RGB(19, 23, 31));
    HPEN panelPen = CreatePen(PS_SOLID, 2, RGB(176, 146, 78));

    oldPen = (HPEN)SelectObject(hMemDC, panelPen);
    oldBrush = (HBRUSH)SelectObject(hMemDC, panelBrush);

    RoundRect(
        hMemDC,
        panelRect.left,
        panelRect.top,
        panelRect.right,
        panelRect.bottom,
        10,
        10
    );

    SelectObject(hMemDC, oldBrush);
    SelectObject(hMemDC, oldPen);

    DeleteObject(panelPen);
    DeleteObject(panelBrush);

    RECT titleRect = {
        panelRect.left + 8,
        panelRect.top + 6,
        panelRect.right - 8,
        panelRect.top + 30
    };

    HBRUSH titleBrush = CreateSolidBrush(RGB(48, 38, 24));
    FillRect(hMemDC, &titleRect, titleBrush);
    DeleteObject(titleBrush);

    int oldMode = SetBkMode(hMemDC, TRANSPARENT);
    COLORREF oldColor = SetTextColor(hMemDC, RGB(245, 220, 150));

    HFONT titleFont = CreateFontW(
        14,
        0,
        0,
        0,
        FW_BOLD,
        FALSE,
        FALSE,
        FALSE,
        DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS,
        CLIP_DEFAULT_PRECIS,
        DEFAULT_QUALITY,
        DEFAULT_PITCH | FF_DONTCARE,
        L"Malgun Gothic"
    );

    HFONT oldFont = (HFONT)SelectObject(hMemDC, titleFont);

    std::wstring title = L"행동";

    RECT titleTextRect = titleRect;
    OffsetRect(&titleTextRect, 0, 2);

    DrawTextW(
        hMemDC,
        title.c_str(),
        -1,
        &titleTextRect,
        DT_CENTER | DT_VCENTER | DT_SINGLELINE
    );

    SelectObject(hMemDC, oldFont);
    DeleteObject(titleFont);

    SetTextColor(hMemDC, oldColor);
    SetBkMode(hMemDC, oldMode);

    for (int i = 0; i < (int)Contents.size(); ++i)
    {
        Contents[i]->Draw(hMemDC, hMemDC1);
    }
}

void UBattleActionMenu::UpdateButtonStates(Character* character)
{
    if (character == nullptr)
    {
        return;
    }

    btnMove->SetEnabled(!character->HasMoved());

    bool canAct = !character->HasActed();

    btnAttack->SetEnabled(canAct);
    btnSkill->SetEnabled(canAct);
    btnItem->SetEnabled(canAct);
    btnDefend->SetEnabled(canAct);

    btnEndTurn->SetEnabled(true);
}