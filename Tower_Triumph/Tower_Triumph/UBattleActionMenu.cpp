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
    // Contents vector in UContainerWidget will handle deletion.
}

void UBattleActionMenu::Init()
{
    UContainerWidget::Init();

    int btnW = 80;
    int btnH = 30;

    btnMove->SetText(L"Move");
    btnMove->SetSize(btnW, btnH);
    btnMove->SetDrawTextFallbackBackground(true);
    AddChild(btnMove);

    btnAttack->SetText(L"Attack");
    btnAttack->SetSize(btnW, btnH);
    btnAttack->SetDrawTextFallbackBackground(true);
    AddChild(btnAttack);

    btnSkill->SetText(L"Skill");
    btnSkill->SetSize(btnW, btnH);
    btnSkill->SetDrawTextFallbackBackground(true);
    AddChild(btnSkill);

    btnDefend->SetText(L"Defend");
    btnDefend->SetSize(btnW, btnH);
    btnDefend->SetDrawTextFallbackBackground(true);
    AddChild(btnDefend);

    btnItem->SetText(L"Item");
    btnItem->SetSize(btnW, btnH);
    btnItem->SetDrawTextFallbackBackground(true);
    AddChild(btnItem);

    btnEndTurn->SetText(L"End Turn");
    btnEndTurn->SetSize(btnW, btnH);
    btnEndTurn->SetDrawTextFallbackBackground(true);
    AddChild(btnEndTurn);

    SetVisible(false);
}

void UBattleActionMenu::UpdatePosition(int screenX, int screenY)
{
    int spacingX = 85;
    int spacingY = 35;

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

    btnMove->SetEnabled(!character->HasMoved());

    bool canAct = !character->HasActed();
    btnAttack->SetEnabled(canAct);
    btnSkill->SetEnabled(canAct);
    btnItem->SetEnabled(canAct);
    btnDefend->SetEnabled(canAct);

    btnEndTurn->SetEnabled(true);
}
