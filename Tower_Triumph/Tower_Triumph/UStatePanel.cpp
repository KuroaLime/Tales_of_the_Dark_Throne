#include "UStatePanel.h"
#include <string>

UStatePanel::UStatePanel()
    : bgImage(nullptr),
      weaponLabel(nullptr), armorLabel(nullptr), accessoryLabel(nullptr),
      strLabel(nullptr), intLabel(nullptr), defLabel(nullptr), spdLabel(nullptr),
      currentCharacter(nullptr)
{
}

void UStatePanel::SetupLabel(UTextLabel*& label, int x, int y, int w, int h, int fontSize, int weight, int align)
{
    label = new UTextLabel();
    label->SetFont(fontSize, weight, L"Arial");
    label->SetColor(RGB(245, 245, 245));
    label->SetAlign(align);
    label->SetLocation(x, y);
    label->SetSize(w, h);
    AddChild(label);
}

void UStatePanel::Setup(int x, int y)
{
    Loc.x = x;
    Loc.y = y;

    bgImage = new UImage();
    bgImage->SetFrameFromFile({ L"Resource/Map/UI/CharacterUI_State.bmp" });
    if (!bgImage->GetBitmap().empty())
    {
        BITMAP info = bgImage->GetMapInfo()[0];
        bgImage->SetSize(info.bmWidth, info.bmHeight);
        Size = bgImage->GetSize();
    }
    else
    {
        Size.sX = 352;
        Size.sY = 392;
    }
    bgImage->SetTransColor(RGB(255, 174, 201));
    bgImage->SetLocation(Loc.x, Loc.y);
    AddChild(bgImage);

    // Coordinates are based on the imported CharacterUI_State.bmp skin.
    SetupLabel(weaponLabel, Loc.x, Loc.y - 156, 260, 28, 20, FW_BOLD, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    SetupLabel(armorLabel, Loc.x, Loc.y - 109, 260, 28, 20, FW_NORMAL, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    SetupLabel(accessoryLabel, Loc.x, Loc.y - 84, 260, 28, 20, FW_NORMAL, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    
    SetupLabel(strLabel, Loc.x - 55, Loc.y + 123, 130, 26, 17, FW_BOLD, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    SetupLabel(intLabel, Loc.x - 55, Loc.y + 148, 130, 26, 17, FW_BOLD, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    SetupLabel(defLabel, Loc.x + 105, Loc.y + 123, 130, 26, 17, FW_BOLD, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    SetupLabel(spdLabel, Loc.x + 105, Loc.y + 148, 130, 26, 17, FW_BOLD, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
}

void UStatePanel::SetCharacter(Character* character)
{
    currentCharacter = character;
    if (!currentCharacter)
    {
        weaponLabel->SetText(L"Weapon: Empty");
        armorLabel->SetText(L"Armor: Empty");
        accessoryLabel->SetText(L"Accessory: Empty");
        strLabel->SetText(L"STR: 0");
        intLabel->SetText(L"INT: 0");
        defLabel->SetText(L"DEF: 0");
        spdLabel->SetText(L"SPD: 0");
        return;
    }

    CharacterEquipment equip = currentCharacter->GetEquipment();

    weaponLabel->SetText(L"Weapon: " + (equip.WeaponID >= 0 ? ItemData::GetName(equip.WeaponID) : std::wstring(L"Empty")));
    armorLabel->SetText(L"Armor: " + (equip.ArmorID >= 0 ? ItemData::GetName(equip.ArmorID) : std::wstring(L"Empty")));
    accessoryLabel->SetText(L"Accessory: " + (equip.AccessoryID >= 0 ? ItemData::GetName(equip.AccessoryID) : std::wstring(L"Empty")));

    CharacterStat stat = currentCharacter->GetStat();
    strLabel->SetText(L"STR: " + std::to_wstring(stat.STR));
    intLabel->SetText(L"INT: " + std::to_wstring(stat.INT));
    defLabel->SetText(L"DEF: " + std::to_wstring(stat.DEF));
    spdLabel->SetText(L"SPD: " + std::to_wstring(stat.SPD));
}
