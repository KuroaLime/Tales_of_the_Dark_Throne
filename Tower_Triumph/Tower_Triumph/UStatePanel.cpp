#include "UStatePanel.h"
#include "StarlightUI.h"
#include <algorithm>

UStatePanel::UStatePanel() : currentCharacter(nullptr), panelBounds{} {}
void UStatePanel::Setup(int x, int y)
{
    SetPanelBounds({ x - 236, y - 199, x + 236, y + 199 });
}
void UStatePanel::SetPanelBounds(const RECT& bounds)
{
    panelBounds = bounds;
    SetLocation((bounds.left + bounds.right) / 2, (bounds.top + bounds.bottom) / 2);
    SetSize(bounds.right - bounds.left, bounds.bottom - bounds.top);
}
void UStatePanel::SetCharacter(Character* character)
{
    currentCharacter = character;
}
void UStatePanel::Draw(HDC hMemDC, HDC)
{
    if (!bVisible) return;
    StarlightUI::Surface(hMemDC, panelBounds);
    StarlightUI::Text(hMemDC, L"장비", { panelBounds.left + 18, panelBounds.top + 16,
        panelBounds.right - 18, panelBounds.top + 43 }, 19, StarlightUI::Gold);
    const CharacterEquipment equipment = currentCharacter ? currentCharacter->GetEquipment() : CharacterEquipment();
    const int ids[] = { currentCharacter ? equipment.WeaponID : -1,
        currentCharacter ? equipment.ArmorID : -1, currentCharacter ? equipment.AccessoryID : -1 };
    const wchar_t* slots[] = { L"Weapon", L"Armor", L"Accessory" };
    for (int i = 0; i < 3; ++i)
    {
        const int y = panelBounds.top + 50 + i * 44;
        StarlightUI::Text(hMemDC, slots[i], { panelBounds.left + 18, y,
            panelBounds.left + 120, y + 32 }, 16, StarlightUI::Muted, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
        StarlightUI::Text(hMemDC, ids[i] >= 0 ? ItemData::GetName(ids[i]) : L"Empty",
            { panelBounds.left + 126, y, panelBounds.right - 18, y + 32 }, 18,
            StarlightUI::Ink, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    }
    const int statsTop = (std::max)(int(panelBounds.top) + 232, int(panelBounds.bottom) - 162);
    StarlightUI::Text(hMemDC, L"능력치", { panelBounds.left + 18, statsTop - 32,
        panelBounds.right - 18, statsTop - 5 }, 19, StarlightUI::Gold);
    const CharacterStat stat = currentCharacter ? currentCharacter->GetStat() : CharacterStat();
    const int values[] = { currentCharacter ? stat.STR : 0, currentCharacter ? stat.INT : 0,
        currentCharacter ? stat.DEF : 0, currentCharacter ? stat.SPD : 0 };
    const wchar_t* names[] = { L"STR", L"INT", L"DEF", L"SPD" };
    const int width = (panelBounds.right - panelBounds.left - 48) / 2;
    for (int i = 0; i < 4; ++i)
    {
        const int x = panelBounds.left + 18 + (i % 2) * (width + 12);
        const int y = statsTop + (i / 2) * 74;
        RECT cell = { x, y, x + width, y + 62 };
        StarlightUI::Surface(hMemDC, cell);
        StarlightUI::Text(hMemDC, names[i], { x + 12, y + 6, x + width - 12, y + 26 }, 15, StarlightUI::Muted);
        StarlightUI::Text(hMemDC, std::to_wstring(values[i]),
            { x + 12, y + 26, x + width - 12, y + 56 }, 23, StarlightUI::Ink);
    }
}
