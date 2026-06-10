#include "UTurnOrderPanel.h"
#include <algorithm>

UTurnOrderPanel::UTurnOrderPanel()
    : logicSystem(nullptr)
{
    bVisible = true;
}

UTurnOrderPanel::~UTurnOrderPanel()
{
    // icons 벡터는 자식 포인터를 복사해서 가지고 있었으므로,
    // 부모인 UContainerWidget의 소멸자가 ClearChildren(true)를 호출할 때 
    // 실제 객체들이 삭제됩니다. 여기서는 icons만 비워줍니다.
    icons.clear();
}

void UTurnOrderPanel::Setup(RECT clientRect)
{
    Loc.x = clientRect.right / 2;
    Loc.y = clientRect.bottom - 100;
    Size.sX = clientRect.right;
    Size.sY = 150;
}

void UTurnOrderPanel::Bind(STurnTimeline* system)
{
    logicSystem = system;
    RefreshIcons();
}

void UTurnOrderPanel::Update()
{
    if (!logicSystem) return;

    const auto& currentOrder = logicSystem->GetTurnOrder();
    
    bool bChanged = false;
    if (currentOrder.size() != lastKnownOrder.size()) {
        bChanged = true;
    } else {
        for (size_t i = 0; i < currentOrder.size(); ++i) {
            if (currentOrder[i].character != lastKnownOrder[i]) {
                bChanged = true;
                break;
            }
        }
    }

    if (bChanged) {
        RefreshIcons();
    }

    UContainerWidget::Update();
}

void UTurnOrderPanel::RefreshIcons()
{
    if (!logicSystem) return;

    // Clear old icons using parent's method to avoid double-delete
    ClearChildren(true);
    icons.clear();
    lastKnownOrder.clear();

    const auto& turnOrder = logicSystem->GetTurnOrder();
    
    int startX = 100; 
    for (size_t i = 0; i < turnOrder.size(); ++i) {
        lastKnownOrder.push_back(turnOrder[i].character);
        
        UTurnCharacterInfo* icon = new UTurnCharacterInfo(turnOrder[i].character, turnOrder[i].isEnemy);
        
        if (i == 0) {
            icon->SetIsCurrentTurn(true);
            icon->SetSize(300, 100);
            icon->SetLocation(startX + 150, Loc.y);
            startX += 320;
        } else {
            icon->SetIsCurrentTurn(false);
            icon->SetSize(80, 80);
            icon->SetLocation(startX + 40, Loc.y);
            startX += 90;
        }
        
        icons.push_back(icon);
        AddChild(icon);
    }
}

void UTurnOrderPanel::Draw(HDC hMemDC, HDC hMemDC1)
{
    if (!bVisible) return;

    // Draw Semi-transparent background
    HBRUSH hBrush = CreateSolidBrush(RGB(10, 10, 10));
    RECT barRect = { 0, Loc.y - 60, Size.sX, Loc.y + 60 };
    FillRect(hMemDC, &barRect, hBrush);
    DeleteObject(hBrush);

    UContainerWidget::Draw(hMemDC, hMemDC1);
}
