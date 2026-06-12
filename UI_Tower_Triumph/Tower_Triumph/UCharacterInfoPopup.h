#pragma once
#include "UContainerWidget.h"
#include "UButton.h"
#include "UImage.h"
#include "Character.h"
#include "UProfilePanel.h"
#include "UStatePanel.h"
#include <vector>
#include <functional>

class UCharacterInfoPopup : public UContainerWidget
{
private:
    Character* selectedCharacter;
    std::vector<Character*> teamMembers;

    UImage* imgKey;
    UProfilePanel* profilePanel;
    UStatePanel* statePanel;

    std::vector<UButton*> charSelectButtons;

    UContainerWidget* equipSelectionUI;

public:
    UCharacterInfoPopup();
    virtual ~UCharacterInfoPopup();

    virtual void Init() override;

    void Setup(RECT clientRect);
    void SetTeamData(const std::vector<Character*>& team);
    void SelectCharacter(Character* character);
    void ShowEquipSelection(int slotIndex);

    std::function<void()> OnBackCallback;
private:
    int currentSlotIndex;
};
