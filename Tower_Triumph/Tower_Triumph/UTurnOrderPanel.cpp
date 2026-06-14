#include "UTurnOrderPanel.h"
#include <algorithm>

UTurnOrderPanel::UTurnOrderPanel()
    : logicSystem(nullptr)
{
    bVisible = true;
}

UTurnOrderPanel::~UTurnOrderPanel()
{
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

    HBRUSH hBrush = CreateSolidBrush(RGB(12, 14, 20));
    RECT barRect = { 0, Loc.y - 60, Size.sX, Loc.y + 60 };
    FillRect(hMemDC, &barRect, hBrush);
    DeleteObject(hBrush);

    UContainerWidget::Draw(hMemDC, hMemDC1);
}
