#include "GameManager.h"
#include "ItemData.h"

#include <algorithm>
#include <string>

static void ApplyItemStatBonusToCharacter(Character* target, int itemID, int sign)
{
    if (target == nullptr || itemID < 0 || sign == 0)
    {
        return;
    }

    SItemStatBonus bonus = ItemData::GetBonus(itemID);
    CharacterStat stat = target->GetStat();

    stat.MaxHP += bonus.MaxHP * sign;
    stat.CurHP += bonus.MaxHP * sign;
    stat.MaxMP += bonus.MaxMP * sign;
    stat.CurMP += bonus.MaxMP * sign;
    stat.STR += bonus.STR * sign;
    stat.INT += bonus.INT * sign;
    stat.DEF += bonus.DEF * sign;
    stat.SPD += bonus.SPD * sign;

    target->SetStat(stat);
}

GameManager::GameManager()
    : PartyGold(0),
      CurrentBattleStageId(1),
      MaxUnlockedBattleStageId(1)
{
}

GameManager::~GameManager()
{
    Clear();
}

GameManager& GameManager::Get()
{
    static GameManager instance;
    return instance;
}

void GameManager::Clear()
{
    for (Character* c : Roster)
    {
        delete c;
    }

    Roster.clear();
    PlayerParty.clear();
    Inventory.clear();
    CollectedMapItemIDs.clear();
    PartyGold = 0;
    CurrentBattleStageId = 1;
    MaxUnlockedBattleStageId = 1;
}

static Character* CreateRosterCharacter(int id, ECharacterClass classType, EElement element, const std::wstring& name)
{
    Character* c = new Character();
    c->Init(classType, 0, 0);
    c->SetCharacterID(id);
    c->SetElement(element);
    c->SetName(name);
    c->LoadSprites(L"Resource");
    return c;
}

void GameManager::InitDefaultParty()
{
    if (!Roster.empty())
    {
        return;
    }

    int id = 1;

    Roster.push_back(CreateRosterCharacter(id++, ECharacterClass::Warrior, EElement::Fire,  L"Fire Warrior"));
    Roster.push_back(CreateRosterCharacter(id++, ECharacterClass::Warrior, EElement::Water, L"Water Warrior"));
    Roster.push_back(CreateRosterCharacter(id++, ECharacterClass::Warrior, EElement::Grass, L"Grass Warrior"));
    Roster.push_back(CreateRosterCharacter(id++, ECharacterClass::Warrior, EElement::Light, L"Light Warrior"));
    Roster.push_back(CreateRosterCharacter(id++, ECharacterClass::Warrior, EElement::Dark,  L"Dark Warrior"));

    Roster.push_back(CreateRosterCharacter(id++, ECharacterClass::Archer, EElement::Fire,  L"Fire Archer"));
    Roster.push_back(CreateRosterCharacter(id++, ECharacterClass::Archer, EElement::Water, L"Water Archer"));
    Roster.push_back(CreateRosterCharacter(id++, ECharacterClass::Archer, EElement::Grass, L"Grass Archer"));
    Roster.push_back(CreateRosterCharacter(id++, ECharacterClass::Archer, EElement::Light, L"Light Archer"));
    Roster.push_back(CreateRosterCharacter(id++, ECharacterClass::Archer, EElement::Dark,  L"Dark Archer"));

    Roster.push_back(CreateRosterCharacter(id++, ECharacterClass::Mage, EElement::Fire,  L"Fire Mage"));
    Roster.push_back(CreateRosterCharacter(id++, ECharacterClass::Mage, EElement::Water, L"Water Mage"));
    Roster.push_back(CreateRosterCharacter(id++, ECharacterClass::Mage, EElement::Grass, L"Grass Mage"));
    Roster.push_back(CreateRosterCharacter(id++, ECharacterClass::Mage, EElement::Light, L"Light Mage"));
    Roster.push_back(CreateRosterCharacter(id++, ECharacterClass::Mage, EElement::Dark,  L"Dark Mage"));

    PlayerParty.clear();

    if (Roster.size() >= 5)
    {
        PlayerParty.push_back(Roster[0]);
        PlayerParty.push_back(Roster[1]);
        PlayerParty.push_back(Roster[7]);
        PlayerParty.push_back(Roster[10]);
        PlayerParty.push_back(Roster[13]);
    }

    PartyGold = 0;
}

bool GameManager::HasParty() const
{
    return !PlayerParty.empty();
}

std::vector<Character*>& GameManager::GetParty()
{
    return PlayerParty;
}

const std::vector<Character*>& GameManager::GetParty() const
{
    return PlayerParty;
}

std::vector<Character*>& GameManager::GetRoster()
{
    return Roster;
}

const std::vector<Character*>& GameManager::GetRoster() const
{
    return Roster;
}

int GameManager::GetMaxActivePartySize() const
{
    return MAX_ACTIVE_PARTY;
}

bool GameManager::IsInActiveParty(Character* character) const
{
    if (character == nullptr)
    {
        return false;
    }

    return std::find(PlayerParty.begin(), PlayerParty.end(), character) != PlayerParty.end();
}

bool GameManager::AddActivePartyMember(Character* character)
{
    if (character == nullptr)
    {
        return false;
    }

    if (IsInActiveParty(character))
    {
        return true;
    }

    if ((int)PlayerParty.size() >= MAX_ACTIVE_PARTY)
    {
        return false;
    }

    PlayerParty.push_back(character);
    return true;
}

bool GameManager::RemoveActivePartyMember(Character* character)
{
    if (character == nullptr)
    {
        return false;
    }

    if (PlayerParty.size() <= 1)
    {
        return false;
    }

    auto it = std::find(PlayerParty.begin(), PlayerParty.end(), character);

    if (it == PlayerParty.end())
    {
        return false;
    }

    PlayerParty.erase(it);
    return true;
}

bool GameManager::ToggleActivePartyMember(Character* character)
{
    if (character == nullptr)
    {
        return false;
    }

    if (IsInActiveParty(character))
    {
        return RemoveActivePartyMember(character);
    }

    return AddActivePartyMember(character);
}

int GameManager::GetGold() const
{
    return PartyGold;
}

void GameManager::SetGold(int gold)
{
    if (gold < 0)
    {
        gold = 0;
    }

    PartyGold = gold;
}

void GameManager::AddGold(int amount)
{
    if (amount <= 0)
    {
        return;
    }

    PartyGold += amount;
}

bool GameManager::SpendGold(int amount)
{
    if (amount <= 0)
    {
        return true;
    }

    if (PartyGold < amount)
    {
        return false;
    }

    PartyGold -= amount;
    return true;
}

bool GameManager::BuyItem(int itemID, int count)
{
    if (itemID < 0 || count <= 0)
    {
        return false;
    }

    if (!ItemData::IsValidItem(itemID))
    {
        return false;
    }

    int price = ItemData::GetPrice(itemID);

    if (price < 0)
    {
        return false;
    }

    int totalPrice = price * count;

    if (!SpendGold(totalPrice))
    {
        return false;
    }

    AddItem(itemID, count);
    return true;
}

int GameManager::GetCurrentBattleStageId() const
{
    return CurrentBattleStageId;
}

void GameManager::SetCurrentBattleStageId(int stageId)
{
    if (stageId < 1)
    {
        stageId = 1;
    }

    if (stageId > 3)
    {
        stageId = 3;
    }

    if (!IsBattleStageUnlocked(stageId))
    {
        return;
    }

    CurrentBattleStageId = stageId;
}

int GameManager::GetMaxUnlockedBattleStageId() const
{
    return MaxUnlockedBattleStageId;
}

bool GameManager::IsBattleStageUnlocked(int stageId) const
{
    return stageId >= 1 && stageId <= MaxUnlockedBattleStageId;
}

void GameManager::UnlockBattleStage(int stageId)
{
    if (stageId < 1)
    {
        return;
    }

    if (stageId > 3)
    {
        stageId = 3;
    }

    if (stageId > MaxUnlockedBattleStageId)
    {
        MaxUnlockedBattleStageId = stageId;
    }
}


bool GameManager::IsMapItemCollected(int mapItemId) const
{
    return std::find(CollectedMapItemIDs.begin(), CollectedMapItemIDs.end(), mapItemId) != CollectedMapItemIDs.end();
}

void GameManager::MarkMapItemCollected(int mapItemId)
{
    if (mapItemId < 0)
    {
        return;
    }

    if (!IsMapItemCollected(mapItemId))
    {
        CollectedMapItemIDs.push_back(mapItemId);
    }
}

void GameManager::ClearCollectedMapItems()
{
    CollectedMapItemIDs.clear();
}

void GameManager::AddItem(int itemID, int count)
{
    if (itemID < 0 || count <= 0)
    {
        return;
    }

    if (!ItemData::IsValidItem(itemID))
    {
        return;
    }

    for (InventoryItem& item : Inventory)
    {
        if (item.ItemID == itemID)
        {
            item.Count += count;
            return;
        }
    }

    Inventory.push_back(InventoryItem(itemID, count));
}

bool GameManager::RemoveItem(int itemID, int count)
{
    if (itemID < 0 || count <= 0)
    {
        return false;
    }

    for (auto it = Inventory.begin(); it != Inventory.end(); ++it)
    {
        if (it->ItemID != itemID)
        {
            continue;
        }

        if (it->Count < count)
        {
            return false;
        }

        it->Count -= count;

        if (it->Count <= 0)
        {
            Inventory.erase(it);
        }

        return true;
    }

    return false;
}

int GameManager::GetItemCount(int itemID) const
{
    if (itemID < 0)
    {
        return 0;
    }

    for (const InventoryItem& item : Inventory)
    {
        if (item.ItemID == itemID)
        {
            return item.Count;
        }
    }

    return 0;
}

bool GameManager::HasItem(int itemID, int count) const
{
    return GetItemCount(itemID) >= count;
}

const std::vector<InventoryItem>& GameManager::GetInventory() const
{
    return Inventory;
}

void GameManager::ClearInventory()
{
    Inventory.clear();
}

bool GameManager::EquipItem(Character* target, int itemID)
{
    if (target == nullptr || itemID < 0)
    {
        return false;
    }

    if (!ItemData::IsValidItem(itemID))
    {
        return false;
    }

    if (!ItemData::IsEquipment(itemID))
    {
        return false;
    }

    EEquipPart part = ItemData::GetEquipPart(itemID);

    return EquipItem(target, itemID, part);
}

bool GameManager::EquipItem(Character* target, int itemID, EEquipPart part)
{
    if (target == nullptr || itemID < 0)
    {
        return false;
    }

    if (!ItemData::IsValidItem(itemID))
    {
        return false;
    }

    if (!ItemData::IsEquipment(itemID))
    {
        return false;
    }

    if (ItemData::GetEquipPart(itemID) != part)
    {
        return false;
    }

    if (!HasItem(itemID, 1))
    {
        return false;
    }

    CharacterEquipment equip = target->GetEquipment();

    int oldItemID = -1;

    switch (part)
    {
    case EEquipPart::Weapon:
        oldItemID = equip.WeaponID;
        equip.WeaponID = itemID;
        break;

    case EEquipPart::Armor:
        oldItemID = equip.ArmorID;
        equip.ArmorID = itemID;
        break;

    case EEquipPart::Accessory:
        oldItemID = equip.AccessoryID;
        equip.AccessoryID = itemID;
        break;

    default:
        return false;
    }

    if (!RemoveItem(itemID, 1))
    {
        return false;
    }

    if (oldItemID >= 0)
    {
        ApplyItemStatBonusToCharacter(target, oldItemID, -1);
        AddItem(oldItemID, 1);
    }

    target->SetEquipment(equip);
    ApplyItemStatBonusToCharacter(target, itemID, 1);

    return true;
}

bool GameManager::UnequipItem(Character* target, EEquipPart part)
{
    if (target == nullptr)
    {
        return false;
    }

    CharacterEquipment equip = target->GetEquipment();

    int oldItemID = -1;

    switch (part)
    {
    case EEquipPart::Weapon:
        oldItemID = equip.WeaponID;
        equip.WeaponID = -1;
        break;

    case EEquipPart::Armor:
        oldItemID = equip.ArmorID;
        equip.ArmorID = -1;
        break;

    case EEquipPart::Accessory:
        oldItemID = equip.AccessoryID;
        equip.AccessoryID = -1;
        break;

    default:
        return false;
    }

    if (oldItemID < 0)
    {
        return false;
    }

    ApplyItemStatBonusToCharacter(target, oldItemID, -1);
    target->SetEquipment(equip);
    AddItem(oldItemID, 1);

    return true;
}

bool GameManager::UseConsumable(Character* target, int itemID)
{
    if (target == nullptr || itemID < 0)
    {
        return false;
    }

    if (!ItemData::IsValidItem(itemID))
    {
        return false;
    }

    if (!ItemData::IsConsumable(itemID))
    {
        return false;
    }

    if (!HasItem(itemID, 1))
    {
        return false;
    }

    int healHP = ItemData::GetHealHP(itemID);
    int healMP = ItemData::GetHealMP(itemID);

    if (healHP <= 0 && healMP <= 0)
    {
        return false;
    }

    CharacterStat stat = target->GetStat();

    if (healHP > 0)
    {
        stat.CurHP += healHP;

        if (stat.CurHP > stat.MaxHP)
        {
            stat.CurHP = stat.MaxHP;
        }
    }

    if (healMP > 0)
    {
        stat.CurMP += healMP;

        if (stat.CurMP > stat.MaxMP)
        {
            stat.CurMP = stat.MaxMP;
        }
    }

    target->SetStat(stat);

    RemoveItem(itemID, 1);

    return true;
}

int GameManager::GetEquippedItemID(Character* target, EEquipPart part) const
{
    if (target == nullptr)
    {
        return -1;
    }

    CharacterEquipment equip = target->GetEquipment();

    switch (part)
    {
    case EEquipPart::Weapon:
        return equip.WeaponID;

    case EEquipPart::Armor:
        return equip.ArmorID;

    case EEquipPart::Accessory:
        return equip.AccessoryID;

    default:
        return -1;
    }
}