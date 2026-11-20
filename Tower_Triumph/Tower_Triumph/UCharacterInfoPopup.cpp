#include "UCharacterInfoPopup.h"
#include "StarlightUI.h"
#include <algorithm>

UCharacterInfoPopup::UCharacterInfoPopup()
    : selectedCharacter(nullptr), profilePanel(nullptr), statePanel(nullptr),
      backButton(nullptr), clientBounds{}, panelBounds{}
{
    bVisible = false;
}
UCharacterInfoPopup::~UCharacterInfoPopup() {}
void UCharacterInfoPopup::Init() {}

void UCharacterInfoPopup::Setup(RECT clientRect)
{
    ClearChildren(true);
    charSelectButtons.clear();
    clientBounds = clientRect;
    panelBounds = StarlightUI::Centered(clientRect, 1000, 620);
    SetSize(clientRect.right - clientRect.left, clientRect.bottom - clientRect.top);
    SetLocation((clientRect.left + clientRect.right) / 2, (clientRect.top + clientRect.bottom) / 2);
    const int left = panelBounds.left, top = panelBounds.top;

    profilePanel = new UProfilePanel();
    profilePanel->SetPanelBounds({ left + 228, top + 108, left + 478, panelBounds.bottom - 28 });
    profilePanel->SetCharacter(selectedCharacter);
    AddChild(profilePanel);
    statePanel = new UStatePanel();
    statePanel->SetPanelBounds({ left + 500, top + 108, panelBounds.right - 28, panelBounds.bottom - 28 });
    statePanel->SetCharacter(selectedCharacter);
    AddChild(statePanel);
    backButton = new UButton();
    backButton->SetText(L"뒤로");
    backButton->SetSize(100, 38);
    backButton->SetLocation(panelBounds.right - 78, top + 52);
    StarlightUI::StyleButton(backButton);
    backButton->SetOnClick([this]() {
        if (OnBackCallback) OnBackCallback();
        else SetVisible(false);
    });
    AddChild(backButton);
    RebuildCharacterButtons();
}

void UCharacterInfoPopup::SetTeamData(const std::vector<Character*>& team)
{
    teamMembers = team;
    if (std::find(teamMembers.begin(), teamMembers.end(), selectedCharacter) == teamMembers.end())
        selectedCharacter = teamMembers.empty() ? nullptr : teamMembers.front();
    RebuildCharacterButtons();
    SelectCharacter(selectedCharacter);
}

void UCharacterInfoPopup::RebuildCharacterButtons()
{
    for (UButton* button : charSelectButtons) RemoveChild(button, true);
    charSelectButtons.clear();
    for (size_t i = 0; i < teamMembers.size(); ++i)
    {
        Character* character = teamMembers[i];
        UButton* button = new UButton();
        button->SetText(character ? character->GetName() : L"None");
        button->SetSize(180, 54);
        button->SetLocation(panelBounds.left + 118, panelBounds.top + 165 + int(i) * 66);
        StarlightUI::StyleButton(button, false, 16);
        button->SetSelected(character == selectedCharacter);
        button->SetOnClick([this, character]() { SelectCharacter(character); });
        AddChild(button);
        charSelectButtons.push_back(button);
    }
}

void UCharacterInfoPopup::SelectCharacter(Character* character)
{
    selectedCharacter = character;
    if (profilePanel) profilePanel->SetCharacter(character);
    if (statePanel) statePanel->SetCharacter(character);
    for (size_t i = 0; i < charSelectButtons.size(); ++i)
        charSelectButtons[i]->SetSelected(teamMembers[i] == character);
}

UWidget* UCharacterInfoPopup::FindWidgetUnderMouse(int mouseX, int mouseY)
{
    if (!bVisible) return nullptr;
    UWidget* widget = UContainerWidget::FindWidgetUnderMouse(mouseX, mouseY);
    return widget ? widget : this;
}

void UCharacterInfoPopup::Draw(HDC hMemDC, HDC hMemDC1)
{
    if (!bVisible) return;
    StarlightUI::Dim(hMemDC, clientBounds);
    StarlightUI::Panel(hMemDC, panelBounds, L"캐릭터", L"파티 기록");
    StarlightUI::Text(hMemDC, L"파티", { panelBounds.left + 28, panelBounds.top + 108,
        panelBounds.left + 208, panelBounds.top + 132 }, 17, StarlightUI::Muted);
    UContainerWidget::Draw(hMemDC, hMemDC1);
}
