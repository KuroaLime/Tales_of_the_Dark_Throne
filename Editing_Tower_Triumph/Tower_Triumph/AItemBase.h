#pragma once
#include "AActor.h"
#include <string>

class Character;

enum class EItemType
{
    Usable,
    Armor
};

class AItemBase : public AActor
{
protected:
    int ItemID;
    int ResourceID; // New: Icon resource ID
    std::wstring Name;
    std::wstring Description;
    int Price;
    EItemType ItemType;

public:
    AItemBase() : ItemID(-1), Price(0), ItemType(EItemType::Usable) {}
    virtual ~AItemBase() {}

    virtual void Use(Character* user) = 0;

    int GetItemID() const { return ItemID; }
    std::wstring GetName() const { return Name; }
    std::wstring GetDescription() const { return Description; }
    int GetPrice() const { return Price; }
    EItemType GetItemType() const { return ItemType; }

    int GetResourceID() const { return ResourceID; }
    void SetResourceID(int resID) { ResourceID = resID; }

    void SetItemData(int id, const std::wstring& name, const std::wstring& desc, int price)
    {
        ItemID = id;
        Name = name;
        Description = desc;
        Price = price;
    }
};

