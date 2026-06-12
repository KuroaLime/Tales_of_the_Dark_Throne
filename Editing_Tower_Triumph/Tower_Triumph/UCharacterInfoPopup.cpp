#include "UCharacterInfoPopup.h"
#include "GameManager.h"
#include "ItemData.h"
#include <string>

UCharacterInfoPopup::UCharacterInfoPopup() : selectedCharacter(nullptr), currentSlotIndex(-1) {
    bVisible = false;
    imgKey = nullptr;
    profilePanel = nullptr;
    statePanel = nullptr;
    equipSelectionUI = nullptr;
}

UCharacterInfoPopup::~UCharacterInfoPopup() {
}

void UCharacterInfoPopup::Init() {}

void UCharacterInfoPopup::Setup(RECT clientRect) {
    Size.sX = clientRect.right;
    Size.sY = clientRect.bottom;
    Loc.x = clientRect.right / 2;
    Loc.y = clientRect.bottom / 2;

    int screenCenterX = clientRect.right / 2;
    int screenCenterY = clientRect.bottom / 2;

    COLORREF pinkTrans = RGB(255, 174, 201);

    // 1. Title/Key UI (Top Center)
    imgKey = new UImage();
    imgKey->SetFrameFromFile({ L"Resource/Map/UI/CharacterUI_Key.bmp" });
    if (!imgKey->GetBitmap().empty()) {
        BITMAP info = imgKey->GetMapInfo()[0];
        imgKey->SetSize(info.bmWidth, info.bmHeight);
    }
    imgKey->SetTransColor(pinkTrans);
    
    int keyUI_X = screenCenterX;
    int keyUI_Y = imgKey->GetSize().sY / 2; 
    imgKey->SetLocation(keyUI_X, keyUI_Y);
    AddChild(imgKey);

    // 2. Profile Panel (Left Side)
    profilePanel = new UProfilePanel();
    int charUI_X = screenCenterX - 350; 
    int charUI_Y = screenCenterY + 50;
    profilePanel->Setup(charUI_X, charUI_Y);
    AddChild(profilePanel);

    // 3. State & Equipment Panel (Right Side)
    statePanel = new UStatePanel();
    int stateUI_X = screenCenterX; 
    int stateUI_Y = screenCenterY + 125;
    statePanel->Setup(stateUI_X, stateUI_Y);
    statePanel->OnEquipSlotClicked = [this](int index) { this->ShowEquipSelection(index); };
    AddChild(statePanel);

    // 4. Equipment Selection List (Right Side)
    equipSelectionUI = new UContainerWidget();
    equipSelectionUI->SetSize(300, 500);

    int selectionUI_X = clientRect.right - 180;
    int selectionUI_Y = screenCenterY;
    equipSelectionUI->SetLocation(selectionUI_X, selectionUI_Y);
    equipSelectionUI->SetVisible(false);
    AddChild(equipSelectionUI);
}

void UCharacterInfoPopup::SetTeamData(const std::vector<Character*>& team) {
    teamMembers = team;
    if (!teamMembers.empty() && !selectedCharacter) {
        SelectCharacter(teamMembers[0]);
    }
    
    for (auto b : charSelectButtons) RemoveChild(b, true);
    charSelectButtons.clear();

    // 6. Character Selection Buttons (Top)
    int btnCount = teamMembers.size();
    int btnW = 140;
    int spacing = 20;
    int totalW = (btnW * btnCount) + (spacing * (btnCount - 1));
    
    int charSelectBtn_StartX = Loc.x - (totalW / 2) + (btnW / 2);
    int charSelectBtn_Y = 40;

    for (size_t i = 0; i < teamMembers.size(); ++i) {
        UButton* btn = new UButton();
        btn->SetFrameFromFile({ L"Resource/Map/UI/CharacterUI_Select.bmp" }); 
        btn->SetTransColor(RGB(255, 174, 201)); // 투명화할 색상 설정 (색상값이 다르면 이 부분을 수정)
        if (!btn->GetBitmap().empty()) {
            BITMAP info = btn->GetMapInfo()[0];
            btn->SetSize(info.bmWidth, info.bmHeight); 
        } else {
            btn->SetSize(btnW, 50); 
        }
        btn->SetText(teamMembers[i]->GetName());
        btn->SetFont(24, FW_BOLD, L"Gabriola");
        btn->SetLocation(charSelectBtn_StartX + (int)i * (btnW + spacing), charSelectBtn_Y); 
        btn->SetAnimationType(1); // Set to Animation1
        
        Character* c = teamMembers[i];
        btn->SetOnClick([this, c]() { 
            this->SelectCharacter(c); 
            this->equipSelectionUI->SetVisible(false);
        });
        AddChild(btn);
        charSelectButtons.push_back(btn);
    }
}

void UCharacterInfoPopup::SelectCharacter(Character* character) {
    selectedCharacter = character;
    if (profilePanel) profilePanel->SetCharacter(character);
    if (statePanel) statePanel->SetCharacter(character);
}

void UCharacterInfoPopup::ShowEquipSelection(int slotIndex) {
    currentSlotIndex = slotIndex;
    equipSelectionUI->ClearChildren(true);
    equipSelectionUI->SetVisible(true);

    const auto& inventory = GameManager::Get().GetInventory();
    std::vector<int> matchItemIDs;
    
    EEquipPart part = (EEquipPart)slotIndex;
    for (const auto& invItem : inventory) {
        if (ItemData::IsEquipment(invItem.ItemID) && ItemData::GetEquipPart(invItem.ItemID) == part) {
            matchItemIDs.push_back(invItem.ItemID);
        }
    }

    int equipList_X = equipSelectionUI->GetLocation().x;
    int equipList_StartY = equipSelectionUI->GetLocation().y - 200; 
    int equipList_SpacingY = 50;

    if (matchItemIDs.empty()) {
        UButton* btnNone = new UButton();
        btnNone->SetText(L"No suitable items");
        btnNone->SetSize(250, 40);
        btnNone->SetLocation(equipList_X, equipList_StartY);
        equipSelectionUI->AddChild(btnNone);
    } else {
        for (size_t i = 0; i < matchItemIDs.size(); ++i) {
            int itemID = matchItemIDs[i];
            UButton* btn = new UButton();
            btn->SetText(ItemData::GetName(itemID));
            btn->SetSize(250, 40);
            btn->SetLocation(equipList_X, equipList_StartY + (int)i * equipList_SpacingY);
            btn->SetOnClick([this, itemID]() {
                GameManager::Get().EquipItem(selectedCharacter, itemID, (EEquipPart)currentSlotIndex);
                this->SelectCharacter(selectedCharacter); // Refresh panels
                this->equipSelectionUI->SetVisible(false);
            });
            equipSelectionUI->AddChild(btn);
        }
    }
}
