#pragma once

#include <Windows.h>
#include <string>
#include <vector>

#include "Character.h"

enum class ESkillAreaType
{
    Single,
    Splash1,
    Splash2,
    Line
};

enum class ESkillStatType
{
    STR,
    INT,
    STR_INT,
    SPD_STR
};

struct SSkillData
{
    int SkillID;
    std::wstring Name;
    std::wstring Description;
    ECharacterClass ClassLimit;
    EElement Element;
    ESkillAreaType AreaType;
    ESkillStatType StatType;
    int Range;
    int MPCost;
    int Power;
    int DefenseRatePercent;
    COLORREF Color;
};

class SkillData
{
public:
    static const SSkillData* Find(int skillID);
    static const std::vector<SSkillData>& GetAllSkills();
    static std::vector<int> GetSkillIDs(ECharacterClass classType, EElement element);
    static std::wstring GetName(int skillID);
    static std::wstring GetDescription(int skillID);
    static ECharacterClass GetClassLimit(int skillID);
    static EElement GetElement(int skillID);
    static ESkillAreaType GetAreaType(int skillID);
    static ESkillStatType GetStatType(int skillID);
    static int GetRange(int skillID);
    static int GetMPCost(int skillID);
    static int GetPower(int skillID);
    static int GetDefenseRatePercent(int skillID);
    static COLORREF GetColor(int skillID);
    static bool IsValidSkill(int skillID);
    static bool CanCharacterUse(const Character* character, int skillID);
    static std::wstring GetElementName(EElement element);
    static float GetElementRate(EElement attackElement, EElement defenseElement);
};
