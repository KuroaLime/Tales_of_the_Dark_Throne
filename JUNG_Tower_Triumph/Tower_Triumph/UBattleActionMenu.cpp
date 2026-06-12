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
}

UBattleActionMenu::~UBattleActionMenu()
{
    // Contents vector in UContainerWidget will handle deletion
}

void UBattleActionMenu::Init()
{
    UContainerWidget::Init();

    int btnW = 80;
    int btnH = 30;

    btnMove->SetText(L"Move");
    btnMove->SetSize(btnW, btnH);
    AddChild(btnMove);

    btnAttack->SetText(L"Attack");
    btnAttack->SetSize(btnW, btnH);
    AddChild(btnAttack);

    btnSkill->SetText(L"Skill");
    btnSkill->SetSize(btnW, btnH);
    AddChild(btnSkill);

    btnDefend->SetText(L"Defend");
    btnDefend->SetSize(btnW, btnH);
    AddChild(btnDefend);

    btnItem->SetText(L"Item");
    btnItem->SetSize(btnW, btnH);
    AddChild(btnItem);

    btnEndTurn->SetText(L"End Turn");
    btnEndTurn->SetSize(btnW, btnH);
    AddChild(btnEndTurn);

    SetVisible(false);
}

void UBattleActionMenu::Draw(HDC hMemDC, HDC hMemDC1)
{
    if (!bVisible)
    {
        return;
    }

    RECT panel = { 0, 0, 0, 0 };
    bool hasRect = false;

    std::vector<UButton*> buttons =
    {
        btnMove,
        btnAttack,
        btnSkill,
        btnDefend,
        btnItem,
        btnEndTurn
    };

    for (UButton* button : buttons)
    {
        if (button == nullptr)
        {
            continue;
        }

        Location loc = button->GetLocation();
        BuSIZE size = button->GetSize();

        RECT rc =
        {
            loc.x - size.sX / 2,
            loc.y - size.sY / 2,
            loc.x + size.sX / 2,
            loc.y + size.sY / 2
        };

        if (!hasRect)
        {
            panel = rc;
            hasRect = true;
        }
        else
        {
            if (rc.left < panel.left) panel.left = rc.left;
            if (rc.top < panel.top) panel.top = rc.top;
            if (rc.right > panel.right) panel.right = rc.right;
            if (rc.bottom > panel.bottom) panel.bottom = rc.bottom;
        }
    }

    if (hasRect)
    {
        InflateRect(&panel, 12, 10);

        HBRUSH bgBrush = CreateSolidBrush(RGB(12, 16, 28));
        FillRect(hMemDC, &panel, bgBrush);
        DeleteObject(bgBrush);

        HPEN borderPen = CreatePen(PS_SOLID, 2, RGB(230, 230, 230));
        HPEN oldPen = (HPEN)SelectObject(hMemDC, borderPen);
        HBRUSH oldBrush = (HBRUSH)SelectObject(hMemDC, GetStockObject(NULL_BRUSH));
        Rectangle(hMemDC, panel.left, panel.top, panel.right, panel.bottom);

        int centerX = (panel.left + panel.right) / 2;
        int row1 = panel.top + (panel.bottom - panel.top) / 3;
        int row2 = panel.top + ((panel.bottom - panel.top) * 2) / 3;
        MoveToEx(hMemDC, centerX, panel.top, nullptr);
        LineTo(hMemDC, centerX, panel.bottom);
        MoveToEx(hMemDC, panel.left, row1, nullptr);
        LineTo(hMemDC, panel.right, row1);
        MoveToEx(hMemDC, panel.left, row2, nullptr);
        LineTo(hMemDC, panel.right, row2);

        SelectObject(hMemDC, oldBrush);
        SelectObject(hMemDC, oldPen);
        DeleteObject(borderPen);
    }

    UContainerWidget::Draw(hMemDC, hMemDC1);
}

void UBattleActionMenu::UpdatePosition(int screenX, int screenY)
{
    // 캐릭터 주변에 2열 3행으로 배치
    int spacingX = 85;
    int spacingY = 35;

    // 캐릭터의 오른쪽 위에 배치 시도
    int startX = screenX + 40;
    int startY = screenY - 60;

    btnMove->SetLocation(startX, startY);
    btnAttack->SetLocation(startX + spacingX, startY);

    btnSkill->SetLocation(startX, startY + spacingY);
    btnDefend->SetLocation(startX + spacingX, startY + spacingY);

    btnItem->SetLocation(startX, startY + spacingY * 2);
    btnEndTurn->SetLocation(startX + spacingX, startY + spacingY * 2);
}

void UBattleActionMenu::UpdateButtonStates(Character* character)
{
    if (character == nullptr) return;

    // 이미 이동했으면 이동 버튼 비활성화
    btnMove->SetEnabled(!character->HasMoved());

    // 이미 행동했으면 행동 관련 버튼들 비활성화
    bool canAct = !character->HasActed();
    btnAttack->SetEnabled(canAct);
    btnSkill->SetEnabled(canAct);
    btnItem->SetEnabled(canAct);
    btnDefend->SetEnabled(canAct);

    // 턴 종료는 항상 활성화
    btnEndTurn->SetEnabled(true);
}
