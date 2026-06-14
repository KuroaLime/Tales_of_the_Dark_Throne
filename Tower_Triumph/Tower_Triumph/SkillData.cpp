#include "SkillData.h"

#include <algorithm>

static std::vector<SSkillData> BuildSkillTable()
{
    std::vector<SSkillData> v;

    v.push_back({ 1101, L"Flame Slash", L"Fire sword strike. Strong single target damage.", ECharacterClass::Warrior, EElement::Fire, ESkillAreaType::Single, ESkillStatType::STR, 1, 4, 135, 100, RGB(255, 100, 60) });
    v.push_back({ 1102, L"Burning Cleave", L"Fire cleave that hits nearby enemies.", ECharacterClass::Warrior, EElement::Fire, ESkillAreaType::Splash1, ESkillStatType::STR, 1, 7, 105, 90, RGB(255, 130, 60) });
    v.push_back({ 1103, L"Blaze Break", L"Hot armor-breaking slash.", ECharacterClass::Warrior, EElement::Fire, ESkillAreaType::Single, ESkillStatType::STR, 1, 6, 115, 40, RGB(255, 170, 70) });

    v.push_back({ 1201, L"Aqua Slash", L"Water blade with stable damage.", ECharacterClass::Warrior, EElement::Water, ESkillAreaType::Single, ESkillStatType::STR, 1, 4, 125, 85, RGB(80, 170, 255) });
    v.push_back({ 1202, L"Wave Cutter", L"Straight water wave attack.", ECharacterClass::Warrior, EElement::Water, ESkillAreaType::Line, ESkillStatType::STR, 3, 7, 95, 80, RGB(90, 200, 255) });
    v.push_back({ 1203, L"Tide Breaker", L"Heavy water strike with defense pierce.", ECharacterClass::Warrior, EElement::Water, ESkillAreaType::Single, ESkillStatType::STR, 1, 6, 115, 35, RGB(110, 220, 255) });

    v.push_back({ 1301, L"Vine Slash", L"Grass sword strike.", ECharacterClass::Warrior, EElement::Grass, ESkillAreaType::Single, ESkillStatType::STR, 1, 4, 125, 90, RGB(90, 220, 100) });
    v.push_back({ 1302, L"Thorn Cleave", L"Thorn slash that damages nearby enemies.", ECharacterClass::Warrior, EElement::Grass, ESkillAreaType::Splash1, ESkillStatType::STR, 1, 7, 100, 80, RGB(120, 230, 100) });
    v.push_back({ 1303, L"Root Crusher", L"Powerful rooted smash.", ECharacterClass::Warrior, EElement::Grass, ESkillAreaType::Single, ESkillStatType::STR, 1, 6, 140, 100, RGB(150, 220, 90) });

    v.push_back({ 1401, L"Holy Slash", L"Light sword strike.", ECharacterClass::Warrior, EElement::Light, ESkillAreaType::Single, ESkillStatType::STR, 1, 4, 125, 90, RGB(255, 240, 120) });
    v.push_back({ 1402, L"Radiant Break", L"Light attack that pierces armor.", ECharacterClass::Warrior, EElement::Light, ESkillAreaType::Single, ESkillStatType::STR, 1, 7, 120, 30, RGB(255, 250, 160) });
    v.push_back({ 1403, L"Judgment Cleave", L"Small area light damage.", ECharacterClass::Warrior, EElement::Light, ESkillAreaType::Splash1, ESkillStatType::STR, 1, 8, 105, 70, RGB(255, 245, 190) });

    v.push_back({ 1501, L"Dark Slash", L"Dark sword strike.", ECharacterClass::Warrior, EElement::Dark, ESkillAreaType::Single, ESkillStatType::STR, 1, 4, 130, 100, RGB(170, 90, 255) });
    v.push_back({ 1502, L"Shadow Break", L"Dark defense-piercing attack.", ECharacterClass::Warrior, EElement::Dark, ESkillAreaType::Single, ESkillStatType::STR, 1, 7, 115, 25, RGB(190, 100, 255) });
    v.push_back({ 1503, L"Abyss Cleave", L"Dark cleave around target.", ECharacterClass::Warrior, EElement::Dark, ESkillAreaType::Splash1, ESkillStatType::STR, 1, 8, 105, 70, RGB(150, 80, 220) });

    v.push_back({ 2101, L"Flame Arrow", L"Fire arrow single target shot.", ECharacterClass::Archer, EElement::Fire, ESkillAreaType::Single, ESkillStatType::SPD_STR, 4, 5, 120, 90, RGB(255, 120, 70) });
    v.push_back({ 2102, L"Explosive Arrow", L"Small fire area explosion.", ECharacterClass::Archer, EElement::Fire, ESkillAreaType::Splash1, ESkillStatType::SPD_STR, 3, 8, 95, 80, RGB(255, 150, 80) });
    v.push_back({ 2103, L"Blazing Snipe", L"Long-range high fire damage.", ECharacterClass::Archer, EElement::Fire, ESkillAreaType::Single, ESkillStatType::SPD_STR, 5, 9, 150, 100, RGB(255, 180, 90) });

    v.push_back({ 2201, L"Aqua Arrow", L"Water arrow shot.", ECharacterClass::Archer, EElement::Water, ESkillAreaType::Single, ESkillStatType::SPD_STR, 4, 5, 115, 85, RGB(90, 190, 255) });
    v.push_back({ 2202, L"Piercing Wave", L"Straight water arrow wave.", ECharacterClass::Archer, EElement::Water, ESkillAreaType::Line, ESkillStatType::SPD_STR, 5, 8, 90, 75, RGB(80, 220, 255) });
    v.push_back({ 2203, L"Tidal Snipe", L"Long water snipe with armor pierce.", ECharacterClass::Archer, EElement::Water, ESkillAreaType::Single, ESkillStatType::SPD_STR, 5, 9, 130, 35, RGB(120, 230, 255) });

    v.push_back({ 2301, L"Thorn Arrow", L"Grass arrow shot.", ECharacterClass::Archer, EElement::Grass, ESkillAreaType::Single, ESkillStatType::SPD_STR, 4, 5, 115, 90, RGB(120, 230, 100) });
    v.push_back({ 2302, L"Vine Volley", L"Grass area arrow attack.", ECharacterClass::Archer, EElement::Grass, ESkillAreaType::Splash1, ESkillStatType::SPD_STR, 3, 8, 95, 80, RGB(140, 240, 100) });
    v.push_back({ 2303, L"Root Snipe", L"High power grass shot.", ECharacterClass::Archer, EElement::Grass, ESkillAreaType::Single, ESkillStatType::SPD_STR, 5, 9, 145, 100, RGB(160, 220, 90) });

    v.push_back({ 2401, L"Holy Arrow", L"Light arrow shot.", ECharacterClass::Archer, EElement::Light, ESkillAreaType::Single, ESkillStatType::SPD_STR, 4, 5, 115, 90, RGB(255, 240, 140) });
    v.push_back({ 2402, L"Radiant Shot", L"Light shot with defense pierce.", ECharacterClass::Archer, EElement::Light, ESkillAreaType::Single, ESkillStatType::SPD_STR, 5, 8, 125, 35, RGB(255, 245, 180) });
    v.push_back({ 2403, L"Judgment Rain", L"Wide light area attack.", ECharacterClass::Archer, EElement::Light, ESkillAreaType::Splash2, ESkillStatType::SPD_STR, 4, 12, 85, 70, RGB(255, 250, 200) });

    v.push_back({ 2501, L"Dark Arrow", L"Dark arrow shot.", ECharacterClass::Archer, EElement::Dark, ESkillAreaType::Single, ESkillStatType::SPD_STR, 4, 5, 120, 100, RGB(170, 90, 255) });
    v.push_back({ 2502, L"Shadow Pierce", L"Dark straight-line piercing shot.", ECharacterClass::Archer, EElement::Dark, ESkillAreaType::Line, ESkillStatType::SPD_STR, 5, 8, 95, 50, RGB(190, 100, 255) });
    v.push_back({ 2503, L"Abyss Snipe", L"Long range dark snipe.", ECharacterClass::Archer, EElement::Dark, ESkillAreaType::Single, ESkillStatType::SPD_STR, 5, 10, 150, 90, RGB(150, 80, 220) });

    v.push_back({ 3101, L"Fire Bolt", L"Fire magic single target.", ECharacterClass::Mage, EElement::Fire, ESkillAreaType::Single, ESkillStatType::INT, 3, 6, 120, 80, RGB(255, 100, 70) });
    v.push_back({ 3102, L"Flame Burst", L"Fire magic small area.", ECharacterClass::Mage, EElement::Fire, ESkillAreaType::Splash1, ESkillStatType::INT, 3, 10, 100, 70, RGB(255, 130, 70) });
    v.push_back({ 3103, L"Meteor Burst", L"Wide fire magic attack.", ECharacterClass::Mage, EElement::Fire, ESkillAreaType::Splash2, ESkillStatType::INT, 4, 16, 90, 70, RGB(255, 170, 90) });

    v.push_back({ 3201, L"Water Shot", L"Water magic single target.", ECharacterClass::Mage, EElement::Water, ESkillAreaType::Single, ESkillStatType::INT, 3, 6, 115, 75, RGB(90, 190, 255) });
    v.push_back({ 3202, L"Aqua Wave", L"Straight water magic wave.", ECharacterClass::Mage, EElement::Water, ESkillAreaType::Line, ESkillStatType::INT, 4, 10, 90, 70, RGB(90, 220, 255) });
    v.push_back({ 3203, L"Tidal Blast", L"Water magic defense pierce.", ECharacterClass::Mage, EElement::Water, ESkillAreaType::Splash1, ESkillStatType::INT, 3, 13, 105, 35, RGB(130, 230, 255) });

    v.push_back({ 3301, L"Vine Whip", L"Grass magic single target.", ECharacterClass::Mage, EElement::Grass, ESkillAreaType::Single, ESkillStatType::INT, 3, 6, 115, 80, RGB(110, 230, 100) });
    v.push_back({ 3302, L"Thorn Field", L"Grass magic small area.", ECharacterClass::Mage, EElement::Grass, ESkillAreaType::Splash1, ESkillStatType::INT, 3, 10, 100, 70, RGB(130, 240, 100) });
    v.push_back({ 3303, L"Root Storm", L"Wide grass magic attack.", ECharacterClass::Mage, EElement::Grass, ESkillAreaType::Splash2, ESkillStatType::INT, 4, 16, 90, 70, RGB(160, 230, 100) });

    v.push_back({ 3401, L"Holy Ray", L"Light magic single target.", ECharacterClass::Mage, EElement::Light, ESkillAreaType::Single, ESkillStatType::INT, 4, 7, 120, 70, RGB(255, 245, 150) });
    v.push_back({ 3402, L"Smite Undead", L"Light magic with strong penetration.", ECharacterClass::Mage, EElement::Light, ESkillAreaType::Single, ESkillStatType::INT, 4, 11, 145, 25, RGB(255, 250, 190) });
    v.push_back({ 3403, L"Sanctuary Burst", L"Wide light magic damage.", ECharacterClass::Mage, EElement::Light, ESkillAreaType::Splash2, ESkillStatType::INT, 4, 16, 90, 70, RGB(255, 250, 210) });

    v.push_back({ 3501, L"Dark Ball", L"Dark magic single target.", ECharacterClass::Mage, EElement::Dark, ESkillAreaType::Single, ESkillStatType::INT, 4, 7, 120, 80, RGB(170, 90, 255) });
    v.push_back({ 3502, L"Shadow Spike", L"Dark magic that ignores most defense.", ECharacterClass::Mage, EElement::Dark, ESkillAreaType::Single, ESkillStatType::INT, 4, 11, 135, 20, RGB(190, 100, 255) });
    v.push_back({ 3503, L"Abyss Blast", L"Wide dark magic attack.", ECharacterClass::Mage, EElement::Dark, ESkillAreaType::Splash2, ESkillStatType::INT, 4, 16, 90, 70, RGB(150, 80, 220) });

    return v;
}

static const std::vector<SSkillData>& SkillTable()
{
    static std::vector<SSkillData> table = BuildSkillTable();
    return table;
}

const SSkillData* SkillData::Find(int skillID)
{
    const std::vector<SSkillData>& table = SkillTable();

    for (const SSkillData& skill : table)
    {
        if (skill.SkillID == skillID)
        {
            return &skill;
        }
    }

    return nullptr;
}

const std::vector<SSkillData>& SkillData::GetAllSkills()
{
    return SkillTable();
}

std::vector<int> SkillData::GetSkillIDs(ECharacterClass classType, EElement element)
{
    std::vector<int> result;
    const std::vector<SSkillData>& table = SkillTable();

    for (const SSkillData& skill : table)
    {
        if (skill.ClassLimit == classType && skill.Element == element)
        {
            result.push_back(skill.SkillID);
        }
    }

    return result;
}

std::wstring SkillData::GetName(int skillID)
{
    const SSkillData* skill = Find(skillID);
    return skill ? skill->Name : L"Unknown Skill";
}

std::wstring SkillData::GetDescription(int skillID)
{
    const SSkillData* skill = Find(skillID);
    return skill ? skill->Description : L"";
}

ECharacterClass SkillData::GetClassLimit(int skillID)
{
    const SSkillData* skill = Find(skillID);
    return skill ? skill->ClassLimit : ECharacterClass::Warrior;
}

EElement SkillData::GetElement(int skillID)
{
    const SSkillData* skill = Find(skillID);
    return skill ? skill->Element : EElement::None;
}

ESkillAreaType SkillData::GetAreaType(int skillID)
{
    const SSkillData* skill = Find(skillID);
    return skill ? skill->AreaType : ESkillAreaType::Single;
}

ESkillStatType SkillData::GetStatType(int skillID)
{
    const SSkillData* skill = Find(skillID);
    return skill ? skill->StatType : ESkillStatType::STR;
}

int SkillData::GetRange(int skillID)
{
    const SSkillData* skill = Find(skillID);
    return skill ? skill->Range : 1;
}

int SkillData::GetMPCost(int skillID)
{
    const SSkillData* skill = Find(skillID);
    return skill ? skill->MPCost : 0;
}

int SkillData::GetPower(int skillID)
{
    const SSkillData* skill = Find(skillID);
    return skill ? skill->Power : 100;
}

int SkillData::GetDefenseRatePercent(int skillID)
{
    const SSkillData* skill = Find(skillID);
    return skill ? skill->DefenseRatePercent : 100;
}

COLORREF SkillData::GetColor(int skillID)
{
    const SSkillData* skill = Find(skillID);
    return skill ? skill->Color : RGB(220, 120, 255);
}

bool SkillData::IsValidSkill(int skillID)
{
    return Find(skillID) != nullptr;
}

bool SkillData::CanCharacterUse(const Character* character, int skillID)
{
    const SSkillData* skill = Find(skillID);

    if (character == nullptr || skill == nullptr)
    {
        return false;
    }

    return character->GetClassType() == skill->ClassLimit && character->GetElement() == skill->Element;
}

std::wstring SkillData::GetElementName(EElement element)
{
    switch (element)
    {
    case EElement::Fire:
        return L"Fire";
    case EElement::Water:
        return L"Water";
    case EElement::Grass:
        return L"Grass";
    case EElement::Light:
        return L"Light";
    case EElement::Dark:
        return L"Dark";
    case EElement::Heal:
        return L"Heal";
    default:
        return L"None";
    }
}

float SkillData::GetElementRate(EElement attackElement, EElement defenseElement)
{
    if (attackElement == EElement::None || defenseElement == EElement::None)
    {
        return 1.0f;
    }

    if (attackElement == defenseElement)
    {
        return 1.0f;
    }

    if (attackElement == EElement::Water && defenseElement == EElement::Fire)
    {
        return 1.25f;
    }

    if (attackElement == EElement::Fire && defenseElement == EElement::Grass)
    {
        return 1.25f;
    }

    if (attackElement == EElement::Grass && defenseElement == EElement::Water)
    {
        return 1.25f;
    }

    if (attackElement == EElement::Fire && defenseElement == EElement::Water)
    {
        return 0.75f;
    }

    if (attackElement == EElement::Water && defenseElement == EElement::Grass)
    {
        return 0.75f;
    }

    if (attackElement == EElement::Grass && defenseElement == EElement::Fire)
    {
        return 0.75f;
    }

    if (attackElement == EElement::Light && defenseElement == EElement::Dark)
    {
        return 1.25f;
    }

    if (attackElement == EElement::Dark && defenseElement == EElement::Light)
    {
        return 1.25f;
    }

    return 1.0f;
}
