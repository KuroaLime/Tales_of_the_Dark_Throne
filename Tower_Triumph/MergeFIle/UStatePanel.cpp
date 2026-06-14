#include "UStatePanel.h"

#include <string>

UStatePanel::UStatePanel()
    : CurrentCharacter(nullptr)
{
    Size.sX = 470;
    Size.sY = 410;

    for (int i = 0; i < 8; ++i)
    {
        StatLabels[i] = nullptr;
    }

    for (int i = 0; i < 3; ++i)
    {
        EquipLabels[i] = nullptr;
    }
}

UStatePanel::~UStatePanel()
{
}

void UStatePanel::Setup(int x, int y)
{
    Loc.x = x;
    Loc.y = y;
    Size.sX = 470;
    Size.sY = 410;

    int leftX = Loc.x - Size.sX / 2 + 140;
    int rightX = Loc.x + 95;
    int statY = Loc.y - 120;

    for (int i = 0; i < 8; ++i)
    {
        StatLabels[i] = new UTextLabel();
        StatLabels[i]->SetFont(19, FW_BOLD, L"Malgun Gothic");
        StatLabels[i]->SetColor(RGB(235, 235, 245));
        StatLabels[i]->SetSize(190, 28);
        StatLabels[i]->SetLocation((i < 4) ? leftX : rightX, statY + (i % 4) * 34);
        AddChild(StatLabels[i]);
    }

    int equipY = Loc.y + 65;
    for (int i = 0; i < 3; ++i)
    {
        EquipLabels[i] = new UTextLabel();
        EquipLabels[i]->SetFont(18, FW_BOLD, L"Malgun Gothic");
        EquipLabels[i]->SetColor(RGB(220, 220, 235));
        EquipLabels[i]->SetSize(360, 32);
        EquipLabels[i]->SetLocation(Loc.x + 20, equipY + i * 42);
        EquipLabels[i]->SetAlign(DT_LEFT | DT_VCENTER | DT_SINGLELINE);
        AddChild(EquipLabels[i]);
    }
}

std::wstring UStatePanel::GetEquipLine(EEquipPart part, int itemID) const
{
    std::wstring partName = L"UNKNOWN";

    switch (part)
    {
    case EEquipPart::Weapon:
        partName = L"무기";
        break;
    case EEquipPart::Armor:
        partName = L"방어구";
        break;
    case EEquipPart::Accessory:
        partName = L"장신구";
        break;
    }

    if (itemID < 0)
    {
        return partName + L": 없음";
    }

    return partName + L": " + ItemData::GetName(itemID);
}

void UStatePanel::SetCharacter(Character* character)
{
    CurrentCharacter = character;

    if (CurrentCharacter == nullptr)
    {
        for (int i = 0; i < 8; ++i)
        {
            if (StatLabels[i]) StatLabels[i]->SetText(L"");
        }

        for (int i = 0; i < 3; ++i)
        {
            if (EquipLabels[i]) EquipLabels[i]->SetText(L"");
        }

        return;
    }

    CharacterStat stat = CurrentCharacter->GetStat();

    std::wstring lines[8] =
    {
        L"HP  " + std::to_wstring(stat.CurHP) + L" / " + std::to_wstring(stat.MaxHP),
        L"MP  " + std::to_wstring(stat.CurMP) + L" / " + std::to_wstring(stat.MaxMP),
        L"STR " + std::to_wstring(stat.STR),
        L"INT " + std::to_wstring(stat.INT),
        L"DEF " + std::to_wstring(stat.DEF),
        L"SPD " + std::to_wstring(stat.SPD),
        L"AP  " + std::to_wstring(CurrentCharacter->GetAP()),
        L"EXP " + std::to_wstring(CurrentCharacter->GetExp())
    };

    for (int i = 0; i < 8; ++i)
    {
        if (StatLabels[i]) StatLabels[i]->SetText(lines[i]);
    }

    CharacterEquipment equip = CurrentCharacter->GetEquipment();

    if (EquipLabels[0]) EquipLabels[0]->SetText(GetEquipLine(EEquipPart::Weapon, equip.WeaponID));
    if (EquipLabels[1]) EquipLabels[1]->SetText(GetEquipLine(EEquipPart::Armor, equip.ArmorID));
    if (EquipLabels[2]) EquipLabels[2]->SetText(GetEquipLine(EEquipPart::Accessory, equip.AccessoryID));
}

void UStatePanel::Draw(HDC hMemDC, HDC hMemDC1)
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

    SetBkMode(hMemDC, TRANSPARENT);
    SetTextColor(hMemDC, RGB(255, 230, 120));

    HFONT titleFont = CreateFontW(22, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, 0, 0, 0, 0, L"Arial");
    HFONT oldFont = (HFONT)SelectObject(hMemDC, titleFont);

    RECT statTitle = { panel.left + 28, panel.top + 22, panel.right - 28, panel.top + 55 };
    DrawTextW(hMemDC, L"상태", -1, &statTitle, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

    RECT equipTitle = { panel.left + 28, Loc.y + 28, panel.right - 28, Loc.y + 62 };
    DrawTextW(hMemDC, L"장비", -1, &equipTitle, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

    SelectObject(hMemDC, oldFont);
    DeleteObject(titleFont);

    UContainerWidget::Draw(hMemDC, hMemDC1);
}
