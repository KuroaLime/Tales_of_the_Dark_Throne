#pragma once

#include <Windows.h>
#include <string>
#include <vector>
#include <cmath>
#include <cstdlib>
#include <utility>

#include "Structure.h"
#pragma comment(lib, "Msimg32.lib")



#define CHARACTER_SHEET_COL 4
#define CHARACTER_SHEET_ROW 1

#define CHARACTER_6X3_SHEET_COL 6
#define CHARACTER_6X3_SHEET_ROW 3
#define CHARACTER_6X3_FRAME_COUNT (CHARACTER_6X3_SHEET_COL * CHARACTER_6X3_SHEET_ROW)

#define WALK_SHEET_COL CHARACTER_6X3_SHEET_COL
#define WALK_SHEET_ROW CHARACTER_6X3_SHEET_ROW
#define WALK_SHEET_FRAME_COUNT CHARACTER_6X3_FRAME_COUNT

#define CHARACTER_TRANSPARENT_COLOR RGB(255, 0, 255)



enum class ECharacterClass
{
	Warrior,
	Archer,
	Mage
};



enum class ECharacterState
{
	Idle,
	Walk,
	Attack,
	Defend,
	Skill,
	Hit,
	Dead
};



enum class EDirection
{
	Left,
	Right
};



enum class EElement
{
	None,
	Fire,
	Water,
	Grass,
	Light,
	Dark,
	Heal
};



enum class EEquipPart
{
	Weapon,
	Armor,
	Accessory
};



struct CharacterEquipment
{
	int WeaponID;
	int ArmorID;
	int AccessoryID;

	CharacterEquipment()
		: WeaponID(-1), ArmorID(-1), AccessoryID(-1)
	{
	}
};



class Character
{
private:
	int CharacterID;

	ECharacterClass ClassType;
	ECharacterState State;
	EDirection Direction;
	EElement Element;

	std::wstring Name;

	CharacterStat Stat;
	CharacterEquipment Equip;

	int Level;
	int Exp;
	int Money;

	std::vector<int> LearnedSkillIDs;
	int EquippedSkillIDs[3];

	int AP;

	float DefendRate;

	Location GridPos;
	Location PixelPos;

	bool IsMoving;

	float WorldPixelX;
	float WorldPixelY;

	float TargetPixelX;
	float TargetPixelY;

	int TargetGridX;
	int TargetGridY;

	float MoveSpeed;

	int CurFrame;
	int MaxFrame;
	int AniTick;
	int AniDelay;

	bool IsActionEnd;

	HBITMAP hSpriteIdle;
	HBITMAP hSpriteWalk;
	HBITMAP hSpriteAttack;
	HBITMAP hSpriteDefend;
	HBITMAP hSpriteSkill;
	HBITMAP hSpriteHit;
	HBITMAP hSpriteDead;
	HBITMAP hIllustration;

	BITMAP SpriteInfoIdle;
	BITMAP SpriteInfoWalk;
	BITMAP SpriteInfoAttack;
	BITMAP SpriteInfoDefend;
	BITMAP SpriteInfoSkill;
	BITMAP SpriteInfoHit;
	BITMAP SpriteInfoDead;
	BITMAP SpriteInfoIllustration;

	bool bUseMonsterSheetLayout;
	bool bSpriteFacesRight;

private:

	HBITMAP GetCurrentSprite() const;

	const BITMAP* GetCurrentSpriteInfo() const;

	int GetDirectionRow() const;

	bool IsNeedFlip() const;

	void UpdateMaxFrameByState();

	void InitClassValue();

	HBITMAP LoadBitmapFromFile(const std::wstring& filePath, BITMAP* outInfo);

	std::wstring GetCharacterPrefix() const;

public:
	Character();
	~Character();

	Character(const Character& other) = delete;
	Character& operator=(const Character& other) = delete;

	Character(Character&& other) noexcept;
	Character& operator=(Character&& other) noexcept;

	void Init(ECharacterClass classType, int gridX, int gridY);
	void Release();


	bool LoadSprites(const std::wstring& folderPath);

	bool LoadMonsterSheet(const std::wstring& folderPath, const std::wstring& fileName, bool sourceFacesRight);

	HBITMAP GetSpriteIdle() const;
	HBITMAP GetIllustration() const;
	const BITMAP* GetIllustrationInfo() const;

	void Update();

	void Draw(HDC hMemDC, HDC hMemDC1);

	void Draw(HDC hMemDC, HDC hMemDC1, int drawW, int drawH);

	void SetState(ECharacterState newState);
	ECharacterState GetState() const;

	void SetDirection(EDirection newDirection);
	EDirection GetDirection() const;

	bool GetIsActionEnd() const;
	void SetIsActionEnd(bool value);

	void SetCharacterID(int id);
	int GetCharacterID() const;

	void SetName(const std::wstring& newName);
	std::wstring GetName() const;

	ECharacterClass GetClassType() const;

	void SetElement(EElement newElement);
	EElement GetElement() const;
	std::wstring GetElementName() const;

	void SetGridPos(int gx, int gy);
	Location GetGridPos() const;

	void SetPixelPos(int px, int py);
	Location GetPixelPos() const;

	void SyncWorldPixelToGrid(int tileSize);

	Location GetWorldPixelPos(int tileSize) const;

	bool StartMoveToGrid(int targetX, int targetY, int tileSize);

	bool GetIsMoving() const;

	bool MoveToGrid(int targetX, int targetY);

	CharacterStat GetStat() const;
	void SetStat(const CharacterStat& newStat);

	int GetHP() const;
	int GetMaxHP() const;

	int GetMP() const;
	int GetMaxMP() const;

	int GetLevel() const;
	void SetLevel(int newLevel);

	int GetExp() const;
	void AddExp(int addExp);

	int GetMoney() const;
	void AddMoney(int addMoney);
	bool UseMoney(int useMoney);

	int GetAP() const;
	void SetAP(int newAP);

	bool HasMoved() const { return bHasMoved; }
	void SetHasMoved(bool value) { bHasMoved = value; }

	bool HasActed() const { return bHasActed; }
	void SetHasActed(bool value) { bHasActed = value; }

	void ResetTurnState() { bHasMoved = false; bHasActed = false; }

private:
	bool bHasMoved;
	bool bHasActed;

public:
	CharacterEquipment GetEquipment() const;
	void SetEquipment(const CharacterEquipment& newEquipment);

	void SetWeaponID(int itemID);
	void SetArmorID(int itemID);
	void SetAccessoryID(int itemID);

	void AddSkillID(int skillID);
	void ClearSkillIDs();
	void RebuildSkillsFromClassElement();
	bool HasSkillID(int skillID) const;
	std::vector<int> GetLearnedSkillIDs() const;
	void SetEquippedSkillID(int slot, int skillID);
	int GetEquippedSkillID(int slot) const;
	std::vector<int> GetEquippedSkillIDs() const;
	void ClearEquippedSkills();


	bool IsAlive() const;

	bool IsInMoveRange(int targetX, int targetY) const;

	bool IsInAttackRange(const Character& target) const;

	bool IsInAttackRangeByGrid(int targetX, int targetY) const;

	int CalcAttackDamage(const Character& target) const;

	int CalcSkillDamage(const Character& target) const;

	void GetDamage(int damage);

	void Die();

	float GetDefendRate() const;
	void SetDefendRate(float rate);


	void LookAtGrid(int targetX, int targetY);
};