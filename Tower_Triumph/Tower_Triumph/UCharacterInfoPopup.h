#pragma once
#include "UContainerWidget.h"
#include "UButton.h"
#include "UProfilePanel.h"
#include "UStatePanel.h"
#include "Character.h"
#include <functional>
#include <vector>

class UCharacterInfoPopup : public UContainerWidget
{
private:
    Character* selectedCharacter;
    std::vector<Character*> teamMembers;
    UProfilePanel* profilePanel;
    UStatePanel* statePanel;
    std::vector<UButton*> charSelectButtons;
    UButton* backButton;
    RECT clientBounds;
    RECT panelBounds;
    void RebuildCharacterButtons();
public:
    std::function<void()> OnBackCallback;
    UCharacterInfoPopup();
    virtual ~UCharacterInfoPopup();
    void Init() override;
    void Draw(HDC hMemDC, HDC hMemDC1) override;
    UWidget* FindWidgetUnderMouse(int mouseX, int mouseY) override;
    void Setup(RECT clientRect);
    void SetTeamData(const std::vector<Character*>& team);
    void SelectCharacter(Character* character);
};
