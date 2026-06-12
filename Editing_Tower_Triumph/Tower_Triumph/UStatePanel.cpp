#include "UStatePanel.h"
#include "ItemData.h"
#include <string>

UStatePanel::UStatePanel() : currentCharacter(nullptr)
{
    bgImage = nullptr;
}

void UStatePanel::Setup(int x, int y)
{
    Loc.x = x;
    Loc.y = y;

    bgImage = new UImage();
    bgImage->SetFrameFromFile({ L"Resource/Map/UI/CharacterUI_State.bmp" });
    if (!bgImage->GetBitmap().empty()) {
        BITMAP info = bgImage->GetMapInfo()[0];
        bgImage->SetSize(info.bmWidth, info.bmHeight);
        Size = bgImage->GetSize();
    }
    bgImage->SetTransColor(RGB(255, 174, 201));
    bgImage->SetLocation(Loc.x, Loc.y);
    AddChild(bgImage);

    int statX = Loc.x - Size.sX / 2 + 120;
    int statY = Loc.y + 123;

    strLabel = new UTextLabel();
    strLabel->SetFont(18, FW_BOLD, L"Arial");
    strLabel->SetLocation(statX, statY);
    strLabel->SetSize(150, 30); AddChild(strLabel);

    intLabel = new UTextLabel();
    intLabel->SetFont(18, FW_BOLD, L"Arial");
    intLabel->SetLocation(statX, statY + 25);
    intLabel->SetSize(150, 30);
    AddChild(intLabel);

    defLabel = new UTextLabel();
    defLabel->SetFont(18, FW_BOLD, L"Arial");
    defLabel->SetLocation(statX + 160, statY);
    defLabel->SetSize(150, 30);
    AddChild(defLabel);

    spdLabel = new UTextLabel();
    spdLabel->SetFont(18, FW_BOLD, L"Arial");
    spdLabel->SetLocation(statX + 160, statY + 25);
    spdLabel->SetSize(150, 30);
    AddChild(spdLabel);

    int equipSlotY = Loc.y - Size.sY / 2+87;

    UButton* btn = new UButton();
    btn->SetSize(250, 40);
    btn->SetLocation(Loc.x, equipSlotY-47);
    btn->SetFont(18, FW_NORMAL, L"Arial");
    btn->SetOnClick([this]() {
        if (OnEquipSlotClicked) OnEquipSlotClicked(0);
        });
    AddChild(btn);
    equipSlotButtons.push_back(btn);

    for (int i = 1; i < 3; ++i) {
        UButton* btn = new UButton();
        btn->SetSize(250, 40); 
        btn->SetLocation(Loc.x, equipSlotY +(i-1)*25); 
        btn->SetFont(18, FW_NORMAL, L"Arial");
        btn->SetOnClick([this, i]() {
            if (OnEquipSlotClicked) OnEquipSlotClicked(i);
        });
        AddChild(btn);
        equipSlotButtons.push_back(btn);
    }
}

void UStatePanel::SetCharacter(Character* character)
{
    currentCharacter = character;
    if (!currentCharacter) return;

    CharacterStat stat = currentCharacter->GetStat();
    strLabel->SetText(L"STR: " + std::to_wstring(stat.STR));
    intLabel->SetText(L"INT: " + std::to_wstring(stat.INT));
    defLabel->SetText(L"DEF: " + std::to_wstring(stat.DEF));
    spdLabel->SetText(L"SPD: " + std::to_wstring(stat.SPD));

    CharacterEquipment equip = currentCharacter->GetEquipment();
    int itemIDs[3] = { equip.WeaponID, equip.ArmorID, equip.AccessoryID };
    std::wstring slotNames[3] = { L"Weapon: ", L"Armor: ", L"Accessory: " };

    for (int i = 0; i < 3; ++i) {
        if (itemIDs[i] >= 0) equipSlotButtons[i]->SetText(slotNames[i] + ItemData::GetName(itemIDs[i]));
        else equipSlotButtons[i]->SetText(slotNames[i] + L"Empty");
    }
}
