#include "UCharacterInfoPopup.h"

UCharacterInfoPopup::UCharacterInfoPopup()
    : SelectedCharacter(nullptr),
      ProfilePanel(nullptr),
      StatePanel(nullptr),
      BackButton(nullptr)
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

    ProfilePanel = new UProfilePanel();
    ProfilePanel->Setup(Loc.x - 420, Loc.y + 40);
    AddChild(ProfilePanel);

    StatePanel = new UStatePanel();
    StatePanel->Setup(Loc.x + 80, Loc.y + 85);
    AddChild(StatePanel);

    BackButton = new UButton();
    BackButton->SetText(L"뒤로");
    BackButton->SetSize(120, 46);
    BackButton->SetFont(18, FW_BOLD, L"Malgun Gothic");
    BackButton->SetLocation(clientRect.right - 90, 58);
    BackButton->SetOnClick([this]() {
        if (OnBackCallback)
        {
            OnBackCallback();
        }
        else
        {
            SetVisible(false);
        }
    });
    AddChild(BackButton);
}

void UCharacterInfoPopup::SetTeamData(const std::vector<Character*>& team)
{
    TeamMembers = team;

    if (!TeamMembers.empty() && SelectedCharacter == nullptr)
    {
        SelectedCharacter = TeamMembers[0];
    }

    RebuildCharacterButtons();
    SelectCharacter(SelectedCharacter);
}

void UCharacterInfoPopup::RebuildCharacterButtons()
{
    for (UButton* button : CharacterButtons)
    {
        RemoveChild(button, true);
    }
    CharacterButtons.clear();

    int btnW = 130;
    int btnH = 38;
    int gap = 12;
    int count = (int)TeamMembers.size();
    int totalW = count * btnW + (count > 0 ? (count - 1) * gap : 0);
    int startX = Loc.x - totalW / 2 + btnW / 2;
    int y = 96;

    for (int i = 0; i < count; ++i)
    {
        Character* character = TeamMembers[i];
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
        CharacterButtons.push_back(button);
    }
}

void UCharacterInfoPopup::SelectCharacter(Character* character)
{
    SelectedCharacter = character;

    if (ProfilePanel)
    {
        ProfilePanel->SetCharacter(SelectedCharacter);
    }

    if (StatePanel)
    {
        StatePanel->SetCharacter(SelectedCharacter);
    }

    for (UButton* button : CharacterButtons)
    {
        button->SetSelected(false);
    }

    for (size_t i = 0; i < TeamMembers.size() && i < CharacterButtons.size(); ++i)
    {
        if (TeamMembers[i] == SelectedCharacter)
        {
            CharacterButtons[i]->SetSelected(true);
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

    SetBkMode(hMemDC, TRANSPARENT);
    SetTextColor(hMemDC, RGB(255, 230, 120));
    HFONT titleFont = CreateFontW(34, 0, 0, 0, FW_HEAVY, FALSE, FALSE, FALSE, DEFAULT_CHARSET, 0, 0, 0, 0, L"Arial");
    HFONT oldFont = (HFONT)SelectObject(hMemDC, titleFont);
    RECT title = { 0, 20, Size.sX, 70 };
    DrawTextW(hMemDC, L"캐릭터", -1, &title, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    SelectObject(hMemDC, oldFont);
    DeleteObject(titleFont);

    UContainerWidget::Draw(hMemDC, hMemDC1);
}
