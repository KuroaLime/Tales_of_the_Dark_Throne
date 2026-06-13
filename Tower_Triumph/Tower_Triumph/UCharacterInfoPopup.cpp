#include "UCharacterInfoPopup.h"

UCharacterInfoPopup::UCharacterInfoPopup()
    : selectedCharacter(nullptr),
      profilePanel(nullptr),
      statePanel(nullptr),
      titleLabel(nullptr),
      backButton(nullptr)
{
    bVisible = false;
}

UCharacterInfoPopup::~UCharacterInfoPopup()
{
}

void UCharacterInfoPopup::Init()
{
}

void UCharacterInfoPopup::Setup(RECT clientRect)
{
    Size.sX = clientRect.right;
    Size.sY = clientRect.bottom;
    Loc.x = clientRect.right / 2;
    Loc.y = clientRect.bottom / 2;

    titleLabel = new UTextLabel();
    titleLabel->SetText(L"캐릭터");
    titleLabel->SetLocation(Loc.x, 45);
    titleLabel->SetSize(Size.sX, 50);
    titleLabel->SetFont(34, FW_HEAVY, L"Arial");
    titleLabel->SetColor(RGB(255, 230, 120));
    titleLabel->SetAlign(DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    AddChild(titleLabel);

    profilePanel = new UProfilePanel();
    profilePanel->Setup(Loc.x - 420, Loc.y + 40);
    AddChild(profilePanel);

    statePanel = new UStatePanel();
    statePanel->Setup(Loc.x + 80, Loc.y + 85);
    AddChild(statePanel);

    backButton = new UButton();
    backButton->SetText(L"뒤로");
    backButton->SetSize(120, 46);
    backButton->SetFont(18, FW_BOLD, L"Malgun Gothic");
    backButton->SetLocation(clientRect.right - 90, 58);
    backButton->SetOnClick([this]() {
        if (OnBackCallback)
        {
            OnBackCallback();
        }
        else
        {
            SetVisible(false);
        }
    });
    AddChild(backButton);
}

void UCharacterInfoPopup::SetTeamData(const std::vector<Character*>& team)
{
    teamMembers = team;

    if (!teamMembers.empty() && selectedCharacter == nullptr)
    {
        selectedCharacter = teamMembers[0];
    }

    RebuildCharacterButtons();
    SelectCharacter(selectedCharacter);
}

void UCharacterInfoPopup::RebuildCharacterButtons()
{
    for (UButton* button : charSelectButtons)
    {
        RemoveChild(button, true);
    }
    charSelectButtons.clear();

    int btnW = 130;
    int btnH = 38;
    int gap = 12;
    int count = (int)teamMembers.size();
    int totalW = count * btnW + (count > 0 ? (count - 1) * gap : 0);
    int startX = Loc.x - totalW / 2 + btnW / 2;
    int y = 96;

    for (int i = 0; i < count; ++i)
    {
        Character* character = teamMembers[i];
        UButton* button = new UButton();
        button->SetText(character ? character->GetName() : L"None");
        button->SetSize(btnW, btnH);
        button->SetLocation(startX + i * (btnW + gap), y);
        button->SetFont(17, FW_BOLD, L"Arial");
        button->SetAnimationType(1);
        button->SetOnClick([this, character]() {
            SelectCharacter(character);
        });

        AddChild(button);
        charSelectButtons.push_back(button);
    }
}

void UCharacterInfoPopup::SelectCharacter(Character* character)
{
    selectedCharacter = character;

    if (profilePanel)
    {
        profilePanel->SetCharacter(selectedCharacter);
    }

    if (statePanel)
    {
        statePanel->SetCharacter(selectedCharacter);
    }

    for (UButton* button : charSelectButtons)
    {
        button->SetSelected(false);
    }

    for (size_t i = 0; i < teamMembers.size() && i < charSelectButtons.size(); ++i)
    {
        if (teamMembers[i] == selectedCharacter)
        {
            charSelectButtons[i]->SetSelected(true);
            break;
        }
    }
}

void UCharacterInfoPopup::Draw(HDC hMemDC, HDC hMemDC1)
{
    if (!bVisible)
    {
        return;
    }

    RECT bg = { Loc.x - Size.sX / 2, Loc.y - Size.sY / 2, Loc.x + Size.sX / 2, Loc.y + Size.sY / 2 };
    HBRUSH bgBrush = CreateSolidBrush(RGB(6, 8, 12));
    FillRect(hMemDC, &bg, bgBrush);
    DeleteObject(bgBrush);

    UContainerWidget::Draw(hMemDC, hMemDC1);
}
