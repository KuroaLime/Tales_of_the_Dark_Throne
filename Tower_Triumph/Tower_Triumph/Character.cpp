#include "Character.h"
#include "SoundManager.h"
#include "CharacterMotion.h"
#include "BattleActionTiming.h"
#include "SkillData.h"

static bool IsCharacterSixByThreeSheet(const BITMAP* info)
{
	if (info == nullptr || info->bmWidth <= 0 || info->bmHeight <= 0)
	{
		return false;
	}

	int frameW = info->bmWidth / CHARACTER_6X3_SHEET_COL;
	int frameH = info->bmHeight / CHARACTER_6X3_SHEET_ROW;

	if (frameW <= 0 || frameH <= 0)
	{
		return false;
	}

	int diff = abs(frameW - frameH);
	int bigger = (frameW > frameH) ? frameW : frameH;

	return diff * 4 <= bigger;
}

static void GetCharacterSheetGrid(const BITMAP& info, bool monster, int& columns, int& rows,
	bool authoredWalk = false, bool authoredIdle = false, bool authoredAction = false, bool authoredMonster = false)
{
	if (authoredMonster)
	{
		columns = EnemySpriteLayout::Columns;
		rows = EnemySpriteLayout::Rows;
		return;
	}
	if (authoredAction)
	{
		columns = CharacterSpriteLayout::Action::Columns;
		rows = CharacterSpriteLayout::Action::Rows;
		return;
	}
	if (authoredIdle)
	{
		columns = CharacterSpriteLayout::IdlePose::Columns;
		rows = CharacterSpriteLayout::IdlePose::Rows;
		return;
	}
	if (authoredWalk)
	{
		columns = CharacterSpriteLayout::WalkCycle::Columns;
		rows = CharacterSpriteLayout::WalkCycle::Rows;
		return;
	}
	columns = CHARACTER_SHEET_COL;
	rows = 1;
	if (monster || IsCharacterSixByThreeSheet(&info))
	{
		columns = CHARACTER_6X3_SHEET_COL;
		rows = CHARACTER_6X3_SHEET_ROW;
	}
	else if (info.bmHeight % 3 == 0 && info.bmHeight / 3 == info.bmWidth / columns)
	{
		rows = 3;
	}
}


Character::Character()
{
	CharacterID = -1;

	ClassType = ECharacterClass::Warrior;
	State = ECharacterState::Idle;
	Direction = EDirection::Left;
	Element = EElement::None;

	Name = L"";

	Level = 1;
	Exp = 0;
	Money = 0;

	AP = 3;
	DefendRate = 0.5f;

	GridPos.x = 0;
	GridPos.y = 0;

	PixelPos.x = 0;
	PixelPos.y = 0;

	IsMoving = false;
	KeepWalkingOnArrival = false;

	WorldPixelX = 0.0f;
	WorldPixelY = 0.0f;

	TargetPixelX = 0.0f;
	TargetPixelY = 0.0f;

	TargetGridX = 0;
	TargetGridY = 0;

	MoveSpeed = 240.0f;

	CurFrame = 0;
	MaxFrame = CHARACTER_SHEET_COL;
	AnimationElapsedSeconds = 0.0f;

	IsActionEnd = false;

	bUseMonsterSheetLayout = false;
	bSpriteFacesRight = true;

	bHasMoved = false;
	bHasActed = false;

	hSpriteIdle = NULL;
	hSpriteWalk = NULL;
	hSpriteAttack = NULL;
	hSpriteDefend = NULL;
	hSpriteSkill = NULL;
	hSpriteHit = NULL;
	hSpriteDead = NULL;
	hIllustration = NULL;

	ZeroMemory(&SpriteInfoIdle, sizeof(BITMAP));
	ZeroMemory(&SpriteInfoWalk, sizeof(BITMAP));
	ZeroMemory(&SpriteInfoAttack, sizeof(BITMAP));
	ZeroMemory(&SpriteInfoDefend, sizeof(BITMAP));
	ZeroMemory(&SpriteInfoSkill, sizeof(BITMAP));
	ZeroMemory(&SpriteInfoHit, sizeof(BITMAP));
	ZeroMemory(&SpriteInfoDead, sizeof(BITMAP));
	ZeroMemory(&SpriteInfoIllustration, sizeof(BITMAP));

	ClearEquippedSkills();
}

Character::~Character()
{
	Release();
}


Character::Character(Character&& other) noexcept
{
	CharacterID = other.CharacterID;

	ClassType = other.ClassType;
	State = other.State;
	Direction = other.Direction;
	Element = other.Element;

	Name = std::move(other.Name);

	Stat = other.Stat;
	Equip = other.Equip;

	Level = other.Level;
	Exp = other.Exp;
	Money = other.Money;

	LearnedSkillIDs = std::move(other.LearnedSkillIDs);
	for (int i = 0; i < 3; ++i)
	{
		EquippedSkillIDs[i] = other.EquippedSkillIDs[i];
		other.EquippedSkillIDs[i] = -1;
	}

	AP = other.AP;
	DefendRate = other.DefendRate;

	GridPos = other.GridPos;
	PixelPos = other.PixelPos;

	IsMoving = other.IsMoving;
	KeepWalkingOnArrival = other.KeepWalkingOnArrival;

	WorldPixelX = other.WorldPixelX;
	WorldPixelY = other.WorldPixelY;

	TargetPixelX = other.TargetPixelX;
	TargetPixelY = other.TargetPixelY;

	TargetGridX = other.TargetGridX;
	TargetGridY = other.TargetGridY;

	MoveSpeed = other.MoveSpeed;

	CurFrame = other.CurFrame;
	MaxFrame = other.MaxFrame;
	AnimationElapsedSeconds = other.AnimationElapsedSeconds;
	WalkCycleDistance = other.WalkCycleDistance;
	IdleRecovery = other.IdleRecovery;
	ActionSequence = other.ActionSequence;
	MonsterImpactFrame = other.MonsterImpactFrame;

	IsActionEnd = other.IsActionEnd;
	bUseMonsterSheetLayout = other.bUseMonsterSheetLayout;
	bUseAuthoredMonsterLayout = other.bUseAuthoredMonsterLayout;
	bUseAuthoredWalkLayout = other.bUseAuthoredWalkLayout;
	bUseAuthoredIdleLayout = other.bUseAuthoredIdleLayout;
	bSpriteFacesRight = other.bSpriteFacesRight;
	bHasMoved = other.bHasMoved;
	bHasActed = other.bHasActed;

	hSpriteIdle = other.hSpriteIdle;
	hSpriteWalk = other.hSpriteWalk;
	hSpriteAttack = other.hSpriteAttack;
	hSpriteDefend = other.hSpriteDefend;
	hSpriteSkill = other.hSpriteSkill;
	hSpriteHit = other.hSpriteHit;
	hSpriteDead = other.hSpriteDead;
	hIllustration = other.hIllustration;
	hSpriteIdleCycle = other.hSpriteIdleCycle;
	hSpriteWalkStop = other.hSpriteWalkStop;
	hSpriteIdleReturn = other.hSpriteIdleReturn;
	SpriteInfoIdleCycle = other.SpriteInfoIdleCycle;
	SpriteInfoWalkStop = other.SpriteInfoWalkStop;
	SpriteInfoIdleReturn = other.SpriteInfoIdleReturn;
	IllustrationTransparentColor = other.IllustrationTransparentColor;

	SpriteInfoIdle = other.SpriteInfoIdle;
	SpriteInfoWalk = other.SpriteInfoWalk;
	SpriteInfoAttack = other.SpriteInfoAttack;
	SpriteInfoDefend = other.SpriteInfoDefend;
	SpriteInfoSkill = other.SpriteInfoSkill;
	SpriteInfoHit = other.SpriteInfoHit;
	SpriteInfoDead = other.SpriteInfoDead;
	SpriteInfoIllustration = other.SpriteInfoIllustration;
	SpriteLayouts = std::move(other.SpriteLayouts);
	AuthoredActionSprites = std::move(other.AuthoredActionSprites);
	std::swap(hFlipScratch, other.hFlipScratch);
	std::swap(FlipScratchWidth, other.FlipScratchWidth);
	std::swap(FlipScratchHeight, other.FlipScratchHeight);

	other.hSpriteIdle = NULL;
	other.hSpriteWalk = NULL;
	other.hSpriteAttack = NULL;
	other.hSpriteDefend = NULL;
	other.hSpriteSkill = NULL;
	other.hSpriteHit = NULL;
	other.hSpriteDead = NULL;
	other.hIllustration = NULL;
	other.hSpriteIdleCycle = nullptr;
	other.hSpriteWalkStop = nullptr;
	other.hSpriteIdleReturn = nullptr;
	other.IdleRecovery = {};
	++other.ActionSequence;
	other.IllustrationTransparentColor = RGB(255, 255, 255);
	other.bUseMonsterSheetLayout = false;
	other.bUseAuthoredMonsterLayout = false;
	other.bSpriteFacesRight = true;
	other.IsMoving = false;
	other.KeepWalkingOnArrival = false;
	other.AnimationElapsedSeconds = 0.0f;
	other.WalkCycleDistance = 0.0f;
	other.bUseAuthoredWalkLayout = false;
	other.bUseAuthoredIdleLayout = false;
	other.AuthoredActionSprites.clear();
}

Character& Character::operator=(Character&& other) noexcept
{
	if (this == &other)
	{
		return *this;
	}

	Release();

	CharacterID = other.CharacterID;

	ClassType = other.ClassType;
	State = other.State;
	Direction = other.Direction;
	Element = other.Element;

	Name = std::move(other.Name);

	Stat = other.Stat;
	Equip = other.Equip;

	Level = other.Level;
	Exp = other.Exp;
	Money = other.Money;

	LearnedSkillIDs = std::move(other.LearnedSkillIDs);
	for (int i = 0; i < 3; ++i)
	{
		EquippedSkillIDs[i] = other.EquippedSkillIDs[i];
		other.EquippedSkillIDs[i] = -1;
	}

	AP = other.AP;
	DefendRate = other.DefendRate;

	GridPos = other.GridPos;
	PixelPos = other.PixelPos;

	IsMoving = other.IsMoving;
	KeepWalkingOnArrival = other.KeepWalkingOnArrival;

	WorldPixelX = other.WorldPixelX;
	WorldPixelY = other.WorldPixelY;

	TargetPixelX = other.TargetPixelX;
	TargetPixelY = other.TargetPixelY;

	TargetGridX = other.TargetGridX;
	TargetGridY = other.TargetGridY;

	MoveSpeed = other.MoveSpeed;

	CurFrame = other.CurFrame;
	MaxFrame = other.MaxFrame;
	AnimationElapsedSeconds = other.AnimationElapsedSeconds;
	WalkCycleDistance = other.WalkCycleDistance;
	IdleRecovery = other.IdleRecovery;
	ActionSequence = other.ActionSequence;
	MonsterImpactFrame = other.MonsterImpactFrame;

	IsActionEnd = other.IsActionEnd;
	bUseMonsterSheetLayout = other.bUseMonsterSheetLayout;
	bUseAuthoredMonsterLayout = other.bUseAuthoredMonsterLayout;
	bUseAuthoredWalkLayout = other.bUseAuthoredWalkLayout;
	bUseAuthoredIdleLayout = other.bUseAuthoredIdleLayout;
	bSpriteFacesRight = other.bSpriteFacesRight;
	bHasMoved = other.bHasMoved;
	bHasActed = other.bHasActed;

	hSpriteIdle = other.hSpriteIdle;
	hSpriteWalk = other.hSpriteWalk;
	hSpriteAttack = other.hSpriteAttack;
	hSpriteDefend = other.hSpriteDefend;
	hSpriteSkill = other.hSpriteSkill;
	hSpriteHit = other.hSpriteHit;
	hSpriteDead = other.hSpriteDead;
	hIllustration = other.hIllustration;
	hSpriteIdleCycle = other.hSpriteIdleCycle;
	hSpriteWalkStop = other.hSpriteWalkStop;
	hSpriteIdleReturn = other.hSpriteIdleReturn;
	SpriteInfoIdleCycle = other.SpriteInfoIdleCycle;
	SpriteInfoWalkStop = other.SpriteInfoWalkStop;
	SpriteInfoIdleReturn = other.SpriteInfoIdleReturn;
	IllustrationTransparentColor = other.IllustrationTransparentColor;

	SpriteInfoIdle = other.SpriteInfoIdle;
	SpriteInfoWalk = other.SpriteInfoWalk;
	SpriteInfoAttack = other.SpriteInfoAttack;
	SpriteInfoDefend = other.SpriteInfoDefend;
	SpriteInfoSkill = other.SpriteInfoSkill;
	SpriteInfoHit = other.SpriteInfoHit;
	SpriteInfoDead = other.SpriteInfoDead;
	SpriteInfoIllustration = other.SpriteInfoIllustration;
	SpriteLayouts = std::move(other.SpriteLayouts);
	AuthoredActionSprites = std::move(other.AuthoredActionSprites);
	std::swap(hFlipScratch, other.hFlipScratch);
	std::swap(FlipScratchWidth, other.FlipScratchWidth);
	std::swap(FlipScratchHeight, other.FlipScratchHeight);

	other.hSpriteIdle = NULL;
	other.hSpriteWalk = NULL;
	other.hSpriteAttack = NULL;
	other.hSpriteDefend = NULL;
	other.hSpriteSkill = NULL;
	other.hSpriteHit = NULL;
	other.hSpriteDead = NULL;

	other.hIllustration = NULL;
	other.hSpriteIdleCycle = nullptr;
	other.hSpriteWalkStop = nullptr;
	other.hSpriteIdleReturn = nullptr;
	other.IdleRecovery = {};
	++other.ActionSequence;
	other.IllustrationTransparentColor = RGB(255, 255, 255);
	other.bUseMonsterSheetLayout = false;
	other.bUseAuthoredMonsterLayout = false;
	other.bSpriteFacesRight = true;
	other.IsMoving = false;
	other.KeepWalkingOnArrival = false;
	other.AnimationElapsedSeconds = 0.0f;
	other.WalkCycleDistance = 0.0f;
	other.bUseAuthoredWalkLayout = false;
	other.bUseAuthoredIdleLayout = false;
	other.AuthoredActionSprites.clear();
	return *this;
}


void Character::Init(ECharacterClass classType, int gridX, int gridY)
{
	++ActionSequence;
	IdleRecovery = {};
	bUseAuthoredMonsterLayout = false;
	WalkCycleDistance = 0.0f;
	ClassType = classType;

	State = ECharacterState::Idle;
	Direction = EDirection::Left;
	Element = EElement::None;

	GridPos.x = gridX;
	GridPos.y = gridY;

	PixelPos.x = 0;
	PixelPos.y = 0;

	IsMoving = false;

	WorldPixelX = 0.0f;
	WorldPixelY = 0.0f;

	TargetPixelX = 0.0f;
	TargetPixelY = 0.0f;

	TargetGridX = gridX;
	TargetGridY = gridY;

	KeepWalkingOnArrival = false;
	MoveSpeed = 240.0f;

	CurFrame = 0;
	MaxFrame = CHARACTER_SHEET_COL;
	AnimationElapsedSeconds = 0.0f;

	IsActionEnd = false;

	bUseMonsterSheetLayout = false;
	bSpriteFacesRight = true;

	Level = 1;
	Exp = 0;
	Money = 0;

	LearnedSkillIDs.clear();
	ClearEquippedSkills();

	Equip = CharacterEquipment();

	bHasMoved = false;
	bHasActed = false;

	InitClassValue();
}

void Character::Release()
{
	++ActionSequence;
	IdleRecovery = {};
	bUseAuthoredMonsterLayout = false;
	IllustrationTransparentColor = RGB(255, 255, 255);
	WalkCycleDistance = 0.0f;
	bUseAuthoredWalkLayout = false;
	bUseAuthoredIdleLayout = false;
	SpriteLayouts.clear();
	AuthoredActionSprites.clear();
	if (hFlipScratch) DeleteObject(hFlipScratch);
	hFlipScratch = nullptr;
	FlipScratchWidth = 0;
	FlipScratchHeight = 0;
	IsMoving = false;
	KeepWalkingOnArrival = false;
	AnimationElapsedSeconds = 0.0f;
	HBITMAP handles[] =
	{
		hSpriteIdle,
		hSpriteWalk,
		hSpriteAttack,
		hSpriteDefend,
		hSpriteSkill,
		hSpriteHit,
		hSpriteDead,
		hIllustration, hSpriteIdleCycle, hSpriteWalkStop, hSpriteIdleReturn
	};

	for (int i = 0; i < static_cast<int>(sizeof(handles) / sizeof(handles[0])); ++i)
	{
		if (handles[i] == NULL)
		{
			continue;
		}

		bool alreadyDeleted = false;

		for (int j = 0; j < i; ++j)
		{
			if (handles[i] == handles[j])
			{
				alreadyDeleted = true;
				break;
			}
		}

		if (!alreadyDeleted)
		{
			DeleteObject(handles[i]);
		}
	}

	hSpriteIdle = NULL;
	hSpriteWalk = NULL;
	hSpriteAttack = NULL;
	hSpriteDefend = NULL;
	hSpriteSkill = NULL;
	hSpriteHit = NULL;
	hSpriteDead = NULL;
	hIllustration = NULL;
	hSpriteIdleCycle = nullptr;
	hSpriteWalkStop = nullptr;
	hSpriteIdleReturn = nullptr;
	SpriteInfoIdleCycle = {};
	SpriteInfoWalkStop = {};
	SpriteInfoIdleReturn = {};

	ZeroMemory(&SpriteInfoIdle, sizeof(BITMAP));
	ZeroMemory(&SpriteInfoWalk, sizeof(BITMAP));
	ZeroMemory(&SpriteInfoAttack, sizeof(BITMAP));
	ZeroMemory(&SpriteInfoDefend, sizeof(BITMAP));
	ZeroMemory(&SpriteInfoSkill, sizeof(BITMAP));
	ZeroMemory(&SpriteInfoHit, sizeof(BITMAP));
	ZeroMemory(&SpriteInfoDead, sizeof(BITMAP));
	ZeroMemory(&SpriteInfoIllustration, sizeof(BITMAP));

	bUseMonsterSheetLayout = false;
	bSpriteFacesRight = true;
}


void Character::InitClassValue()
{
	CharacterStat newStat;
	LearnedSkillIDs.clear();

	switch (ClassType)
	{
	case ECharacterClass::Warrior:
		newStat.MaxHP = 150;
		newStat.CurHP = 150;
		newStat.MaxMP = 20;
		newStat.CurMP = 20;
		newStat.STR = 22;
		newStat.INT = 4;
		newStat.DEF = 15;
		newStat.SPD = 8;

		AP = 3;
		DefendRate = 0.45f;
		Element = EElement::Fire;
		MoveSpeed = 225.0f;
		break;

	case ECharacterClass::Archer:
		newStat.MaxHP = 100;
		newStat.CurHP = 100;
		newStat.MaxMP = 30;
		newStat.CurMP = 30;
		newStat.STR = 16;
		newStat.INT = 8;
		newStat.DEF = 7;
		newStat.SPD = 16;

		AP = 4;
		DefendRate = 0.65f;
		Element = EElement::Water;
		MoveSpeed = 270.0f;
		break;

	case ECharacterClass::Mage:
		newStat.MaxHP = 80;
		newStat.CurHP = 80;
		newStat.MaxMP = 80;
		newStat.CurMP = 80;
		newStat.STR = 6;
		newStat.INT = 26;
		newStat.DEF = 4;
		newStat.SPD = 10;

		AP = 3;
		DefendRate = 0.75f;
		Element = EElement::Grass;
		MoveSpeed = 240.0f;
		break;

	default:
		newStat.MaxHP = 100;
		newStat.CurHP = 100;
		newStat.MaxMP = 30;
		newStat.CurMP = 30;
		newStat.STR = 10;
		newStat.INT = 10;
		newStat.DEF = 5;
		newStat.SPD = 10;

		AP = 3;
		DefendRate = 0.5f;
		Element = EElement::None;
		MoveSpeed = 240.0f;
		break;
	}

	Stat = newStat;
	RebuildSkillsFromClassElement();
}


HBITMAP Character::LoadBitmapFromFile(const std::wstring& filePath, BITMAP* outInfo)
{
	HBITMAP hBitmap = (HBITMAP)LoadImageW(
		NULL,
		filePath.c_str(),
		IMAGE_BITMAP,
		0,
		0,
		LR_LOADFROMFILE | LR_CREATEDIBSECTION
	);

	if (hBitmap != NULL && outInfo != nullptr)
	{
		GetObject(hBitmap, sizeof(BITMAP), outInfo);
	}

	return hBitmap;
}

std::wstring Character::GetCharacterPrefix() const
{
	switch (ClassType)
	{
	case ECharacterClass::Warrior:
		return L"Warrior";

	case ECharacterClass::Archer:
		return L"Archer";

	case ECharacterClass::Mage:
		return L"Mage";

	default:
		return L"Character";
	}
}

bool Character::LoadSprites(const std::wstring& folderPath)
{
	Release();

	bUseMonsterSheetLayout = false;
	bSpriteFacesRight = true;

	std::wstring basePath = folderPath;

	if (!basePath.empty())
	{
		wchar_t lastChar = basePath[basePath.length() - 1];

		if (lastChar != L'\\' && lastChar != L'/')
		{
			basePath += L"\\";
		}
	}

	std::wstring prefix = GetCharacterPrefix();
	hSpriteIdleCycle = LoadIdleVisualBitmap(basePath + prefix + L"_IdleCycle.bmp", &SpriteInfoIdleCycle, false);
	hSpriteWalkStop = LoadIdleVisualBitmap(basePath + prefix + L"_WalkStop.bmp", &SpriteInfoWalkStop, true);
	hSpriteIdleReturn = LoadIdleVisualBitmap(basePath + prefix + L"_IdleReturn.bmp", &SpriteInfoIdleReturn, true);

	hSpriteIdle = LoadBitmapFromFile(basePath + prefix + L"_IdlePose.bmp", &SpriteInfoIdle);
	if (hSpriteIdle && !CharacterSpriteLayout::IdlePose::HasDimensions(SpriteInfoIdle.bmWidth, SpriteInfoIdle.bmHeight))
	{
		DeleteObject(hSpriteIdle);
		hSpriteIdle = NULL;
		ZeroMemory(&SpriteInfoIdle, sizeof(SpriteInfoIdle));
	}
	bUseAuthoredIdleLayout = hSpriteIdle != NULL;
	if (!hSpriteIdle)
		hSpriteIdle = LoadBitmapFromFile(basePath + prefix + L"_Idle.bmp", &SpriteInfoIdle);
	hSpriteWalk = LoadBitmapFromFile(basePath + prefix + L"_WalkCycle.bmp", &SpriteInfoWalk);
	if (hSpriteWalk && (SpriteInfoWalk.bmWidth != CharacterSpriteLayout::WalkCycle::Columns * CharacterSpriteLayout::WalkCycle::CellSize ||
		SpriteInfoWalk.bmHeight != CharacterSpriteLayout::WalkCycle::Rows * CharacterSpriteLayout::WalkCycle::CellSize))
	{
		DeleteObject(hSpriteWalk);
		hSpriteWalk = NULL;
		ZeroMemory(&SpriteInfoWalk, sizeof(SpriteInfoWalk));
	}
	bUseAuthoredWalkLayout = hSpriteWalk != NULL;
	if (!hSpriteWalk)
		hSpriteWalk = LoadBitmapFromFile(basePath + prefix + L"_Walk.bmp", &SpriteInfoWalk);
	hSpriteAttack = LoadActionBitmap(basePath, prefix, L"Attack", &SpriteInfoAttack);
	hSpriteDefend = LoadActionBitmap(basePath, prefix, L"Defend", &SpriteInfoDefend);
	hSpriteSkill = LoadActionBitmap(basePath, prefix, L"Skill", &SpriteInfoSkill);
	hSpriteHit = LoadActionBitmap(basePath, prefix, L"Hit", &SpriteInfoHit);
	hSpriteDead = LoadActionBitmap(basePath, prefix, L"Dead", &SpriteInfoDead);
	hIllustration = LoadBitmapFromFile(basePath + prefix + L"_Portrait.bmp", &SpriteInfoIllustration);
	IllustrationTransparentColor = hIllustration ? CHARACTER_TRANSPARENT_COLOR : RGB(255, 255, 255);
	if (!hIllustration)
		hIllustration = LoadBitmapFromFile(basePath + prefix + L"_Illustration.bmp", &SpriteInfoIllustration);

	if (hSpriteIdle == NULL)
	{
		return false;
	}

	PrepareSpriteLayouts();
	UpdateMaxFrameByState();
	return true;
}


HBITMAP Character::LoadActionBitmap(const std::wstring& basePath, const std::wstring& prefix,
	const wchar_t* actionName, BITMAP* outInfo)
{
	const std::wstring stem = basePath + prefix + L"_" + actionName;
	HBITMAP sheet = LoadBitmapFromFile(stem + L"Action.bmp", outInfo);
	if (sheet && !CharacterSpriteLayout::Action::HasDimensions(outInfo->bmWidth, outInfo->bmHeight))
	{
		DeleteObject(sheet);
		sheet = NULL;
		ZeroMemory(outInfo, sizeof(*outInfo));
	}
	if (sheet)
		AuthoredActionSprites.push_back(sheet);
	else
		sheet = LoadBitmapFromFile(stem + L".bmp", outInfo);
	return sheet;
}

bool Character::IsAuthoredActionSprite(HBITMAP sprite) const
{
	return !bUseMonsterSheetLayout && sprite != NULL &&
		std::find(AuthoredActionSprites.begin(), AuthoredActionSprites.end(), sprite) != AuthoredActionSprites.end();
}

bool Character::IsAuthoredPaddedSprite(HBITMAP sprite) const
{
	return sprite != NULL && (IsAuthoredActionSprite(sprite) ||
		sprite == hSpriteWalkStop || sprite == hSpriteIdleReturn);
}

HBITMAP Character::LoadIdleVisualBitmap(const std::wstring& path, BITMAP* outInfo, bool padded)
{
	HBITMAP sheet = LoadBitmapFromFile(path, outInfo);
	const bool valid = sheet && (padded ?
		CharacterSpriteLayout::IdleTransition::HasDimensions(outInfo->bmWidth, outInfo->bmHeight) :
		CharacterSpriteLayout::IdleCycle::HasDimensions(outInfo->bmWidth, outInfo->bmHeight));
	if (!valid)
	{
		if (sheet) DeleteObject(sheet);
		ZeroMemory(outInfo, sizeof(*outInfo));
		return nullptr;
	}
	return sheet;
}

bool Character::LoadMonsterSheet(const std::wstring& folderPath, const std::wstring& fileName, bool sourceFacesRight)
{
	Release();
	MonsterImpactFrame = BattleActionTiming::MonsterAttackFrame(fileName.c_str());

	bUseMonsterSheetLayout = true;
	bSpriteFacesRight = sourceFacesRight;

	std::wstring basePath = folderPath;

	if (!basePath.empty())
	{
		wchar_t lastChar = basePath[basePath.length() - 1];

		if (lastChar != L'\\' && lastChar != L'/')
		{
			basePath += L"\\";
		}
	}

	BITMAP sheetInfo{};
	BITMAP portraitInfo{};
	const std::wstring stem = fileName.substr(0, fileName.find_last_of(L'.'));
	const std::wstring approvedBase = basePath + L"Approved\\" + stem;
	HBITMAP hSheet = LoadBitmapFromFile(approvedBase + L"_Animated.bmp", &sheetInfo);
	HBITMAP portrait = nullptr;
	if (hSheet && EnemySpriteLayout::HasDimensions(sheetInfo.bmWidth, sheetInfo.bmHeight))
		portrait = LoadBitmapFromFile(approvedBase + L"_Portrait.bmp", &portraitInfo);
	if (hSheet && EnemySpriteLayout::HasDimensions(sheetInfo.bmWidth, sheetInfo.bmHeight) &&
		portrait && portraitInfo.bmWidth == EnemySpriteLayout::PortraitSize &&
		portraitInfo.bmHeight == EnemySpriteLayout::PortraitSize)
	{
		// Commit a complete matching set, never a new body with an old portrait.
		bUseAuthoredMonsterLayout = true;
		bSpriteFacesRight = true;
		MonsterImpactFrame = EnemySpriteLayout::AttackImpactFrameFor(stem.c_str());
		IllustrationTransparentColor = CHARACTER_TRANSPARENT_COLOR;
	}
	else
	{
		if (hSheet) DeleteObject(hSheet);
		if (portrait) DeleteObject(portrait);
		portrait = nullptr;
		portraitInfo = {};
		sheetInfo = {};
		hSheet = LoadBitmapFromFile(basePath + fileName, &sheetInfo);
	}

	if (hSheet == NULL)
	{
		bUseMonsterSheetLayout = false;
		bSpriteFacesRight = true;
		return false;
	}

	hSpriteIdle = hSheet;
	hSpriteWalk = hSheet;
	hSpriteAttack = hSheet;
	hSpriteDefend = hSheet;
	hSpriteSkill = hSheet;
	hSpriteHit = hSheet;
	hSpriteDead = hSheet;
	hIllustration = portrait;

	SpriteInfoIdle = sheetInfo;
	SpriteInfoWalk = sheetInfo;
	SpriteInfoAttack = sheetInfo;
	SpriteInfoDefend = sheetInfo;
	SpriteInfoSkill = sheetInfo;
	SpriteInfoHit = sheetInfo;
	SpriteInfoDead = sheetInfo;
	SpriteInfoIllustration = portraitInfo;

	CurFrame = 0;
	AnimationElapsedSeconds = 0.0f;
	IsActionEnd = false;
	UpdateMaxFrameByState();

	PrepareSpriteLayouts();
	return true;
}

void Character::PrepareSpriteLayouts()
{
	SpriteLayouts.clear();
	const HBITMAP sheets[] = { hSpriteIdle, hSpriteWalk, hSpriteAttack, hSpriteDefend,
		hSpriteSkill, hSpriteHit, hSpriteDead, hSpriteIdleCycle, hSpriteWalkStop, hSpriteIdleReturn };
	HDC sourceDC = CreateCompatibleDC(nullptr);
	if (!sourceDC) return;
	for (HBITMAP sheet : sheets)
	{
		if (!sheet || SpriteLayouts.find(sheet) != SpriteLayouts.end()) continue;
		BITMAP info{};
		if (!GetObject(sheet, sizeof(info), &info) || info.bmWidth <= 0 || info.bmHeight <= 0) continue;
		BITMAPINFO dib{};
		dib.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
		dib.bmiHeader.biWidth = info.bmWidth;
		dib.bmiHeader.biHeight = -info.bmHeight;
		dib.bmiHeader.biPlanes = 1;
		dib.bmiHeader.biBitCount = 32;
		dib.bmiHeader.biCompression = BI_RGB;
		std::vector<std::uint32_t> pixels(static_cast<size_t>(info.bmWidth) * info.bmHeight);
		if (GetDIBits(sourceDC, sheet, 0, info.bmHeight, pixels.data(), &dib, DIB_RGB_COLORS) != info.bmHeight)
			continue;
		int columns, rows;
		const bool authoredWalk = (bUseAuthoredWalkLayout && sheet == hSpriteWalk) || sheet == hSpriteIdleCycle;
		const bool authoredIdle = bUseAuthoredIdleLayout && sheet == hSpriteIdle;
		const bool authoredAction = IsAuthoredPaddedSprite(sheet);
		GetCharacterSheetGrid(info, bUseMonsterSheetLayout, columns, rows, authoredWalk, authoredIdle, authoredAction, bUseAuthoredMonsterLayout);
		auto layout = CharacterSpriteLayout::Analyze(pixels.data(),
			info.bmWidth, info.bmHeight, columns, rows, !bUseMonsterSheetLayout);
		if (bUseAuthoredMonsterLayout)
		{
			for (int index = 0; index < static_cast<int>(layout.frames.size()); ++index)
				layout.frames[index] = EnemySpriteLayout::WithAnchor(layout.frames[index], index);
		}
		else if (authoredAction)
		{
			for (int index = 0; index < static_cast<int>(layout.frames.size()); ++index)
				layout.frames[index] = CharacterSpriteLayout::WithActionAnchor(layout.frames[index], index);
		}
		else if (authoredIdle && !layout.frames.empty())
		{
			layout.frames[0] = CharacterSpriteLayout::WithIdlePoseAnchor(layout.frames[0]);
		}
		else if (authoredWalk)
		{
			// Cropping may trim empty pixels, but a planted foot must travel behind
			// the fixed root. Recentring each pose on its feet erases that motion.
			for (int index = 0; index < static_cast<int>(layout.frames.size()); ++index)
			{
				layout.frames[index] = CharacterSpriteLayout::WithWalkCycleAnchor(layout.frames[index], index);
			}
		}
		SpriteLayouts.emplace(sheet, std::move(layout));
	}
	DeleteDC(sourceDC);
}


HBITMAP Character::GetSpriteIdle() const
{
	return hSpriteIdle;
}

HBITMAP Character::GetIllustration() const
{
	return hIllustration;
}

const BITMAP* Character::GetIllustrationInfo() const
{
	if (hIllustration == NULL)
	{
		return nullptr;
	}

	return &SpriteInfoIllustration;
}

COLORREF Character::GetIllustrationTransparentColor() const
{
	return IllustrationTransparentColor;
}


HBITMAP Character::GetCurrentSprite() const
{
	if (bUseAuthoredMonsterLayout) return hSpriteIdle;
	switch (State)
	{
	case ECharacterState::Idle:
		switch (IdleRecovery.kind)
		{
		case CharacterMotion::IdleRecoveryKind::WalkContact: return hSpriteWalk;
		case CharacterMotion::IdleRecoveryKind::WalkStop: return hSpriteWalkStop;
		case CharacterMotion::IdleRecoveryKind::ActionTail:
			return IdleRecovery.branch == 0 ? hSpriteAttack : IdleRecovery.branch == 1 ? hSpriteSkill : hSpriteHit;
		case CharacterMotion::IdleRecoveryKind::ActionReturn: return hSpriteIdleReturn;
		default: break;
		}
		if (hSpriteIdleCycle != NULL) return hSpriteIdleCycle;
		if (hSpriteIdle != NULL) return hSpriteIdle;
		break;

	case ECharacterState::Walk:
		if (hSpriteWalk != NULL) return hSpriteWalk;
		break;

	case ECharacterState::Attack:
		if (hSpriteAttack != NULL) return hSpriteAttack;
		break;

	case ECharacterState::Defend:
		if (hSpriteDefend != NULL) return hSpriteDefend;
		break;

	case ECharacterState::Skill:
		if (hSpriteSkill != NULL) return hSpriteSkill;
		break;

	case ECharacterState::Hit:
		if (hSpriteHit != NULL) return hSpriteHit;
		break;

	case ECharacterState::Dead:
		if (hSpriteDead != NULL) return hSpriteDead;
		break;
	}

	return hSpriteIdle;
}

const BITMAP* Character::GetCurrentSpriteInfo() const
{
	const HBITMAP sprite = GetCurrentSprite();
	if (sprite && sprite == hSpriteIdleCycle) return &SpriteInfoIdleCycle;
	if (sprite && sprite == hSpriteWalkStop) return &SpriteInfoWalkStop;
	if (sprite && sprite == hSpriteIdleReturn) return &SpriteInfoIdleReturn;
	if (sprite && sprite == hSpriteWalk) return &SpriteInfoWalk;
	if (sprite && sprite == hSpriteAttack) return &SpriteInfoAttack;
	if (sprite && sprite == hSpriteDefend) return &SpriteInfoDefend;
	if (sprite && sprite == hSpriteSkill) return &SpriteInfoSkill;
	if (sprite && sprite == hSpriteHit) return &SpriteInfoHit;
	if (sprite && sprite == hSpriteDead) return &SpriteInfoDead;
	return &SpriteInfoIdle;
}

int Character::GetDirectionRow() const
{
	return 0;
}

bool Character::IsNeedFlip() const
{
	if (bUseMonsterSheetLayout)
	{
		return bSpriteFacesRight ? (Direction == EDirection::Left) : (Direction == EDirection::Right);
	}

	return Direction == EDirection::Left;
}

void Character::UpdateMaxFrameByState()
{
	const BITMAP* info = GetCurrentSpriteInfo();

	if (bUseAuthoredMonsterLayout)
	{
		MaxFrame = EnemySpriteLayout::StateFrames;
	}
	else if (bUseMonsterSheetLayout)
	{
		switch (State)
		{
		case ECharacterState::Idle:
		case ECharacterState::Walk:
		case ECharacterState::Attack:
		case ECharacterState::Skill:
		case ECharacterState::Defend:
			MaxFrame = CHARACTER_6X3_SHEET_COL; 
			break;

		case ECharacterState::Hit:
		case ECharacterState::Dead:
			MaxFrame = 3;
			break;

		default:
			MaxFrame = 1;
			break;
		}
	}
	else if (State == ECharacterState::Idle && hSpriteIdleCycle)
	{
		MaxFrame = CharacterSpriteLayout::IdleCycle::FrameCount;
	}
	else if (IsAuthoredActionSprite(GetCurrentSprite()))
	{
		MaxFrame = CharacterSpriteLayout::Action::FrameCount;
	}
	else if (bUseAuthoredIdleLayout && GetCurrentSprite() == hSpriteIdle)
	{
		MaxFrame = CharacterSpriteLayout::IdlePose::FrameCount;
	}
	else if (bUseAuthoredWalkLayout && State == ECharacterState::Walk)
	{
		MaxFrame = CharacterSpriteLayout::WalkCycle::FrameCount;
	}
	else if (IsCharacterSixByThreeSheet(info))
	{
		MaxFrame = CHARACTER_6X3_FRAME_COUNT;
	}
	else
	{
		MaxFrame = CHARACTER_SHEET_COL;
	}

	if (MaxFrame <= 0)
	{
		MaxFrame = 1;
	}

	if (CurFrame >= MaxFrame)
	{
		CurFrame = 0;
	}
}


float Character::GetWalkCycleLength() const
{
	if (!bUseAuthoredWalkLayout && !bUseAuthoredMonsterLayout) return CharacterMotion::WalkCycleDistancePixels;
	switch (ClassType)
	{
	case ECharacterClass::Warrior: return CharacterMotion::WarriorWalkCycleDistancePixels;
	case ECharacterClass::Archer: return CharacterMotion::ArcherWalkCycleDistancePixels;
	case ECharacterClass::Mage: return CharacterMotion::MageWalkCycleDistancePixels;
	default: return CharacterMotion::WalkCycleDistancePixels;
	}
}

void Character::BeginIdleRecovery(ECharacterState outgoingState)
{
	IdleRecovery = {};
	if (bUseAuthoredMonsterLayout)
	{
		if (outgoingState == ECharacterState::Walk && MoveSpeed > 0.0f)
		{
			const float frameDistance = GetWalkCycleLength() / EnemySpriteLayout::StateFrames;
			const float elapsedInPose = (WalkCycleDistance - CurFrame * frameDistance) / MoveSpeed;
			IdleRecovery = EnemySpriteLayout::BeginWalkRecovery(CurFrame, frameDistance / MoveSpeed,
				elapsedInPose > 0.0f ? elapsedInPose : 0.0f);
		}
		else if (outgoingState == ECharacterState::Attack || outgoingState == ECharacterState::Skill ||
			outgoingState == ECharacterState::Hit)
		{
			const int branch = outgoingState == ECharacterState::Hit ? 2 : outgoingState == ECharacterState::Skill ? 1 : 0;
			IdleRecovery = EnemySpriteLayout::BeginActionRecovery(CurFrame, branch,
				GetAnimationFrameSeconds(), AnimationElapsedSeconds);
		}
		return;
	}
	if (bUseMonsterSheetLayout) return;
	if (outgoingState == ECharacterState::Walk && bUseAuthoredWalkLayout && hSpriteWalkStop && MoveSpeed > 0.0f)
	{
		const float frameDistance = GetWalkCycleLength() / CharacterSpriteLayout::WalkCycle::FrameCount;
		const float elapsedInPose = (WalkCycleDistance - CurFrame * frameDistance) / MoveSpeed;
		IdleRecovery = CharacterMotion::BeginWalkRecovery(CurFrame, frameDistance / MoveSpeed,
			elapsedInPose > 0.0f ? elapsedInPose : 0.0f);
		return;
	}
	int branch = -1;
	switch (outgoingState)
	{
	case ECharacterState::Attack: branch = 0; break;
	case ECharacterState::Skill: branch = 1; break;
	case ECharacterState::Hit: branch = 2; break;
	default: break;
	}
	if (branch >= 0 && hSpriteIdleReturn && IsAuthoredActionSprite(GetCurrentSprite()))
	{
		IdleRecovery = CharacterMotion::BeginActionRecovery(CurFrame, branch,
			CharacterMotion::ActionTiming::FrameSeconds(CharacterSpriteLayout::Action::FrameCount,
				CharacterMotion::ActionTiming::AttackSeconds), AnimationElapsedSeconds);
	}
}

void Character::UpdateIdleVisual(float elapsedSeconds)
{
	if (IdleRecovery.kind != CharacterMotion::IdleRecoveryKind::None)
	{
		elapsedSeconds = bUseAuthoredMonsterLayout ? EnemySpriteLayout::AdvanceRecovery(IdleRecovery, elapsedSeconds) :
			CharacterMotion::AdvanceIdleRecovery(IdleRecovery, elapsedSeconds);
		if (IdleRecovery.kind != CharacterMotion::IdleRecoveryKind::None) return;
		CurFrame = 0;
		AnimationElapsedSeconds = 0.0f;
		UpdateMaxFrameByState();
	}
	if (!hSpriteIdleCycle && !bUseAuthoredMonsterLayout)
	{
		CurFrame = 0;
		AnimationElapsedSeconds = 0.0f;
		return;
	}
	AnimationElapsedSeconds += elapsedSeconds;
	const auto animation = CharacterMotion::AdvanceAnimation(AnimationElapsedSeconds,
		bUseAuthoredMonsterLayout ? EnemySpriteLayout::IdleFrameSeconds : CharacterMotion::IdleTiming::CycleFrameSeconds);
	AnimationElapsedSeconds = animation.remainingSeconds;
	CurFrame = (CurFrame + animation.frames) % (bUseAuthoredMonsterLayout ?
		EnemySpriteLayout::StateFrames : CharacterSpriteLayout::IdleCycle::FrameCount);
}

float Character::AdvanceMovement(float elapsedSeconds)
{
	if (State == ECharacterState::Dead || Stat.CurHP <= 0)
	{
		IsMoving = false;
		KeepWalkingOnArrival = false;
		return 0.0f;
	}

	if (!IsMoving) return 0.0f;
	const float dx = TargetPixelX - WorldPixelX;
	const float dy = TargetPixelY - WorldPixelY;
	const float distance = sqrtf(dx * dx + dy * dy);
	const auto step = CharacterMotion::Advance(distance, MoveSpeed,
		CharacterMotion::ClampDelta(elapsedSeconds));
	if ((!bUseMonsterSheetLayout || bUseAuthoredMonsterLayout) && State == ECharacterState::Walk)
	{
		const float cycleLength = GetWalkCycleLength();
		WalkCycleDistance = CharacterMotion::AdvanceWalkCycle(WalkCycleDistance, step.distance, cycleLength);
		CurFrame = CharacterMotion::WalkFrame(WalkCycleDistance, MaxFrame, cycleLength);
	}
	if (step.arrived)
	{
		WorldPixelX = TargetPixelX;
		WorldPixelY = TargetPixelY;
		GridPos.x = TargetGridX;
		GridPos.y = TargetGridY;
		IsMoving = false;
		if (!KeepWalkingOnArrival) SetState(ECharacterState::Idle);
		return step.remainingSeconds;
	}
	if (distance > 0.0f)
	{
		WorldPixelX += dx / distance * step.distance;
		WorldPixelY += dy / distance * step.distance;
	}
	return 0.0f;
}

float Character::GetAnimationFrameSeconds() const
{
	if (bUseAuthoredMonsterLayout)
	{
		switch (State)
		{
		case ECharacterState::Attack:
		case ECharacterState::Skill: return EnemySpriteLayout::AttackFrameSeconds;
		case ECharacterState::Hit: return EnemySpriteLayout::HitFrameSeconds;
		case ECharacterState::Dead: return EnemySpriteLayout::DeadFrameSeconds;
		default: return EnemySpriteLayout::IdleFrameSeconds;
		}
	}
	float framesPerSecond = 8.0f;
	if (bUseMonsterSheetLayout)
	{
		switch (State)
		{
		case ECharacterState::Walk:
		case ECharacterState::Attack:
		case ECharacterState::Skill:
		case ECharacterState::Hit:
			framesPerSecond = 12.0f;
			break;
		default:
			break;
		}
	}
	else if (IsCharacterSixByThreeSheet(GetCurrentSpriteInfo()))
	{
		switch (State)
		{
		case ECharacterState::Idle: framesPerSecond = 12.0f; break;
		case ECharacterState::Dead: framesPerSecond = 20.0f; break;
		case ECharacterState::Walk:
		case ECharacterState::Attack:
		case ECharacterState::Skill:
		case ECharacterState::Hit:
			framesPerSecond = 30.0f;
			break;
		default:
			break;
		}
	}

	float frameSeconds = 1.0f / framesPerSecond;
	if (IsAuthoredActionSprite(GetCurrentSprite()))
	{
		float durationSeconds = CharacterMotion::ActionTiming::AttackSeconds;
		switch (State)
		{
		case ECharacterState::Attack: durationSeconds = CharacterMotion::ActionTiming::AttackSeconds; break;
		case ECharacterState::Skill: durationSeconds = CharacterMotion::ActionTiming::SkillSeconds; break;
		case ECharacterState::Hit: durationSeconds = CharacterMotion::ActionTiming::HitSeconds; break;
		case ECharacterState::Dead: durationSeconds = CharacterMotion::ActionTiming::DeadSeconds; break;
		case ECharacterState::Defend: durationSeconds = CharacterMotion::ActionTiming::DefendSeconds; break;
		default: break;
		}
		frameSeconds = CharacterMotion::ActionTiming::FrameSeconds(MaxFrame, durationSeconds);
	}
	return frameSeconds;
}

float Character::GetCurrentActionImpactSeconds() const
{
	if (State != ECharacterState::Attack && State != ECharacterState::Skill) return 0.0f;
	if (bUseAuthoredMonsterLayout) return MonsterImpactFrame * EnemySpriteLayout::AttackFrameSeconds;
	if (bUseMonsterSheetLayout) return MonsterImpactFrame * GetAnimationFrameSeconds();
	const bool skill = State == ECharacterState::Skill;
	int frame = 6;
	switch (ClassType)
	{
	case ECharacterClass::Warrior: frame = skill ? BattleActionTiming::WarriorSkillFrame : BattleActionTiming::WarriorAttackFrame; break;
	case ECharacterClass::Archer: frame = skill ? BattleActionTiming::ArcherSkillFrame : BattleActionTiming::ArcherAttackFrame; break;
	case ECharacterClass::Mage: frame = skill ? BattleActionTiming::MageSkillFrame : BattleActionTiming::MageAttackFrame; break;
	default: break;
	}
	return BattleActionTiming::MapAuthoredFrame(frame, MaxFrame) * GetAnimationFrameSeconds();
}

BattleActionTiming::ProjectileKind Character::GetCurrentProjectileKind() const
{
	if (bUseMonsterSheetLayout || (State != ECharacterState::Attack && State != ECharacterState::Skill))
		return BattleActionTiming::ProjectileKind::None;
	if (ClassType == ECharacterClass::Archer) return BattleActionTiming::ProjectileKind::Arrow;
	if (ClassType == ECharacterClass::Mage) return BattleActionTiming::ProjectileKind::Magic;
	return BattleActionTiming::ProjectileKind::None;
}

float Character::Update(float elapsedSeconds)
{
	elapsedSeconds = CharacterMotion::ClampDelta(elapsedSeconds);
	if (Stat.CurHP <= 0 && State != ECharacterState::Dead) Die();
	const bool wasMoving = IsMoving;
	const float remainingMovementSeconds = AdvanceMovement(elapsedSeconds);
	// Walking is driven by actual displacement, including path continuation
	// handled after this update. Waiting or a blocked step cannot shuffle feet.
	if ((!bUseMonsterSheetLayout || bUseAuthoredMonsterLayout) && State == ECharacterState::Walk) return remainingMovementSeconds;

	if (State == ECharacterState::Idle)
	{
		if (bUseAuthoredMonsterLayout || (!bUseMonsterSheetLayout && (hSpriteIdleCycle ||
			IdleRecovery.kind != CharacterMotion::IdleRecoveryKind::None ||
			bUseAuthoredIdleLayout || !IsCharacterSixByThreeSheet(GetCurrentSpriteInfo()))))
		{
			// Arrival already consumed part of this update's time while moving.
			UpdateIdleVisual(wasMoving ? remainingMovementSeconds : elapsedSeconds);
			return remainingMovementSeconds;
		}
	}

	if (IsActionEnd) return remainingMovementSeconds;
	const float frameSeconds = GetAnimationFrameSeconds();
	AnimationElapsedSeconds += elapsedSeconds;
	const auto animation = CharacterMotion::AdvanceAnimation(AnimationElapsedSeconds, frameSeconds);
	AnimationElapsedSeconds = animation.remainingSeconds;
	for (int frame = 0; frame < animation.frames; ++frame)
	{
		++CurFrame;
		if (CurFrame < MaxFrame) continue;
		if (State == ECharacterState::Walk || State == ECharacterState::Idle)
		{
			CurFrame = 0;
			continue;
		}
		CurFrame = MaxFrame - 1;
		IsActionEnd = true;
		AnimationElapsedSeconds = 0.0f;
		break;
	}
	return remainingMovementSeconds;
}

int Character::GetAuthoredMonsterFrameIndex() const
{
	if (State == ECharacterState::Idle && IdleRecovery.kind != CharacterMotion::IdleRecoveryKind::None)
		return EnemySpriteLayout::RecoveryFrameIndex(IdleRecovery);
	EnemySpriteLayout::Pose pose = EnemySpriteLayout::Pose::Idle;
	switch (State)
	{
	case ECharacterState::Walk: pose = EnemySpriteLayout::Pose::Walk; break;
	case ECharacterState::Attack:
	case ECharacterState::Skill: pose = EnemySpriteLayout::Pose::Attack; break;
	case ECharacterState::Hit: pose = EnemySpriteLayout::Pose::Hit; break;
	case ECharacterState::Dead: pose = EnemySpriteLayout::Pose::Dead; break;
	default: break;
	}
	return EnemySpriteLayout::FrameIndex(pose, CurFrame);
}

void Character::Draw(HDC hMemDC, HDC hMemDC1)
{
	Draw(hMemDC, hMemDC1, 0, 0);
}

void Character::Draw(HDC hMemDC, HDC hMemDC1, int drawW, int drawH)
{
	const BITMAP* info = GetCurrentSpriteInfo();
	if (!info || info->bmWidth <= 0 || info->bmHeight <= 0) return;
	int columns, rows;
	const HBITMAP sprite = GetCurrentSprite();
	const bool authoredAction = IsAuthoredPaddedSprite(sprite);
	GetCharacterSheetGrid(*info, bUseMonsterSheetLayout, columns, rows,
		(bUseAuthoredWalkLayout && sprite == hSpriteWalk) || sprite == hSpriteIdleCycle,
		bUseAuthoredIdleLayout && sprite == hSpriteIdle, authoredAction, bUseAuthoredMonsterLayout);
	if (drawW <= 0) drawW = bUseAuthoredMonsterLayout ? EnemySpriteLayout::ReferenceFrameHeight :
		authoredAction ? CharacterSpriteLayout::Action::ReferenceFrameHeight : info->bmWidth / columns;
	if (drawH <= 0) drawH = bUseAuthoredMonsterLayout ? EnemySpriteLayout::ReferenceFrameHeight :
		authoredAction ? CharacterSpriteLayout::Action::ReferenceFrameHeight : info->bmHeight / rows;
	DrawAtFoot(hMemDC, hMemDC1, PixelPos.x + drawW / 2, PixelPos.y + drawH, drawH);
}

void Character::DrawAtFoot(HDC hMemDC, HDC hMemDC1, int footX, int footY, int referenceFrameHeight)
{
	HBITMAP sprite = GetCurrentSprite();
	const BITMAP* info = GetCurrentSpriteInfo();
	if (!sprite || !info || info->bmWidth <= 0 || info->bmHeight <= 0 || referenceFrameHeight <= 0) return;

	int columns, rows;
	const bool authoredWalk = (bUseAuthoredWalkLayout && sprite == hSpriteWalk) || sprite == hSpriteIdleCycle;
	const bool authoredIdle = bUseAuthoredIdleLayout && sprite == hSpriteIdle;
	const bool authoredAction = IsAuthoredPaddedSprite(sprite);
	GetCharacterSheetGrid(*info, bUseMonsterSheetLayout, columns, rows, authoredWalk, authoredIdle, authoredAction, bUseAuthoredMonsterLayout);
	const int nominalWidth = info->bmWidth / columns;
	const int nominalHeight = info->bmHeight / rows;
	if (nominalWidth <= 0 || nominalHeight <= 0) return;
	const bool recovering = State == ECharacterState::Idle && IdleRecovery.kind != CharacterMotion::IdleRecoveryKind::None;
	int frame = recovering ? IdleRecovery.frame : (CurFrame < 0 ? 0 : CurFrame);
	if (!recovering && MaxFrame > 0 && frame >= MaxFrame) frame = MaxFrame - 1;
	int column = frame % columns;
	int row = rows == CHARACTER_6X3_SHEET_ROW ? frame / columns : 0;
	if (bUseAuthoredMonsterLayout)
	{
		const int authoredIndex = GetAuthoredMonsterFrameIndex();
		column = authoredIndex % columns;
		row = authoredIndex / columns;
	}
	else if (bUseMonsterSheetLayout)
	{
		switch (State)
		{
		case ECharacterState::Attack:
		case ECharacterState::Skill: row = 1; break;
		case ECharacterState::Hit: row = 2; column = frame % 3; break;
		case ECharacterState::Dead: row = 2; column = 3 + frame % 3; break;
		default: row = 0; break;
		}
	}
	else if (!authoredWalk && !authoredIdle && !authoredAction && !IsCharacterSixByThreeSheet(info))
	{
		row = rows == 3 ? 1 : 0;
	}

	const int index = row * columns + column;
	CharacterSpriteLayout::Frame sourceFrame{
		{ column * nominalWidth, row * nominalHeight, (column + 1) * nominalWidth, (row + 1) * nominalHeight },
		column * nominalWidth + nominalWidth * 0.5f, static_cast<float>((row + 1) * nominalHeight)
	};
	if (bUseAuthoredMonsterLayout)
	{
		sourceFrame = EnemySpriteLayout::WithAnchor(sourceFrame, index);
	}
	else if (authoredAction)
	{
		sourceFrame = CharacterSpriteLayout::WithActionAnchor(sourceFrame, index);
	}
	else if (authoredIdle)
	{
		sourceFrame = CharacterSpriteLayout::WithIdlePoseAnchor(sourceFrame);
	}
	else if (authoredWalk)
	{
		sourceFrame = CharacterSpriteLayout::WithWalkCycleAnchor(sourceFrame, index);
	}
	const auto cached = SpriteLayouts.find(sprite);
	if (cached != SpriteLayouts.end() && index >= 0 && index < static_cast<int>(cached->second.frames.size()))
		sourceFrame = cached->second.frames[index];
	// Invalid frame metadata must never sample outside the loaded bitmap.
	if (sourceFrame.source.left < 0 || sourceFrame.source.top < 0 ||
		sourceFrame.source.right > info->bmWidth || sourceFrame.source.bottom > info->bmHeight)
		return;

	const bool flipped = IsNeedFlip();
	const int scaleReferenceHeight = bUseAuthoredMonsterLayout ? EnemySpriteLayout::ReferenceFrameHeight :
		authoredAction ? CharacterSpriteLayout::Action::ReferenceFrameHeight : nominalHeight;
	const auto destination = CharacterSpriteLayout::PlaceAtFoot(sourceFrame, scaleReferenceHeight,
		referenceFrameHeight, footX, footY, flipped);
	const int sourceWidth = sourceFrame.source.right - sourceFrame.source.left;
	const int sourceHeight = sourceFrame.source.bottom - sourceFrame.source.top;
	if (destination.width <= 0 || destination.height <= 0 || sourceWidth <= 0 || sourceHeight <= 0) return;
	HBITMAP oldBitmap = static_cast<HBITMAP>(SelectObject(hMemDC1, sprite));
	if (!oldBitmap || oldBitmap == HGDI_ERROR) return;
	if (!flipped)
	{
		TransparentBlt(hMemDC, destination.x, destination.y, destination.width, destination.height,
			hMemDC1, sourceFrame.source.left, sourceFrame.source.top, sourceWidth, sourceHeight,
			CHARACTER_TRANSPARENT_COLOR);
	}
	else
	{
		// Reuse the mirror surface instead of allocating a bitmap every drawn frame.
		if (!hFlipScratch || FlipScratchWidth < sourceWidth || FlipScratchHeight < sourceHeight)
		{
			const int width = sourceWidth > FlipScratchWidth ? sourceWidth : FlipScratchWidth;
			const int height = sourceHeight > FlipScratchHeight ? sourceHeight : FlipScratchHeight;
			HBITMAP replacement = CreateCompatibleBitmap(hMemDC, width, height);
			if (replacement)
			{
				if (hFlipScratch) DeleteObject(hFlipScratch);
				hFlipScratch = replacement;
				FlipScratchWidth = width;
				FlipScratchHeight = height;
			}
		}
		if (hFlipScratch && FlipScratchWidth >= sourceWidth && FlipScratchHeight >= sourceHeight)
		{
			HDC flipDC = CreateCompatibleDC(hMemDC);
			if (flipDC)
			{
				HBITMAP oldFlip = static_cast<HBITMAP>(SelectObject(flipDC, hFlipScratch));
				if (oldFlip && oldFlip != HGDI_ERROR)
				{
					const RECT scratchBounds{ 0, 0, sourceWidth, sourceHeight };
					SetDCBrushColor(flipDC, CHARACTER_TRANSPARENT_COLOR);
					SetStretchBltMode(flipDC, COLORONCOLOR);
					// Clear reused pixels, then start at the last pixel (width - 1).
					// Starting at width leaves column zero black/stale and clips the
					// opposite edge when GDI copies with a negative destination width.
					if (FillRect(flipDC, &scratchBounds, static_cast<HBRUSH>(GetStockObject(DC_BRUSH))) &&
						StretchBlt(flipDC, sourceWidth - 1, 0, -sourceWidth, sourceHeight, hMemDC1,
							sourceFrame.source.left, sourceFrame.source.top, sourceWidth, sourceHeight, SRCCOPY))
					{
						TransparentBlt(hMemDC, destination.x, destination.y, destination.width, destination.height,
							flipDC, 0, 0, sourceWidth, sourceHeight, CHARACTER_TRANSPARENT_COLOR);
					}
					SelectObject(flipDC, oldFlip);
				}
				DeleteDC(flipDC);
			}
		}
	}
	SelectObject(hMemDC1, oldBitmap);
}


void Character::SetState(ECharacterState newState, bool restartAnimation, bool playSound)
{
	if (State == newState && !restartAnimation)
	{
		return;
	}

	++ActionSequence;
	// Capture the outgoing pose before clearing its clock. Recovery is purely
	// visual and never prevents a new gameplay state from taking effect now.
	if (newState == ECharacterState::Idle) BeginIdleRecovery(State);
	else IdleRecovery = {};
	State = newState;
	WalkCycleDistance = 0.0f;
	if (newState != ECharacterState::Walk) KeepWalkingOnArrival = false;

	CurFrame = 0;
	AnimationElapsedSeconds = 0.0f;
	IsActionEnd = false;

	UpdateMaxFrameByState();

	// A skill's shared impact sound can own this reaction without changing its animation.
	if (!playSound) return;
	switch (newState)
	{
	case ECharacterState::Attack:
		SoundManager::PlaySFX(L"Resource\\Sound\\Character_Attack.wav");
		break;

	case ECharacterState::Skill:
		SoundManager::PlaySFX(L"Resource\\Sound\\Character_Skill.wav");
		break;

	case ECharacterState::Defend:
		SoundManager::PlaySFX(L"Resource\\Sound\\Guard.wav");
		break;

	case ECharacterState::Hit:
		SoundManager::PlaySFX(L"Resource\\Sound\\Character_Hit.wav");
		break;

	case ECharacterState::Dead:
		SoundManager::PlaySFX(L"Resource\\Sound\\Character_Dead.wav");
		break;
	}
}

ECharacterState Character::GetState() const
{
	return State;
}

void Character::ResetBattleAnimation()
{
	++ActionSequence;
	State = IsAlive() ? ECharacterState::Idle : ECharacterState::Dead;
	IdleRecovery = {};
	WalkCycleDistance = 0.0f;
	CurFrame = 0;
	AnimationElapsedSeconds = 0.0f;
	IsActionEnd = false;
	IsMoving = false;
	KeepWalkingOnArrival = false;
	UpdateMaxFrameByState();
}

void Character::SetDirection(EDirection newDirection)
{
	Direction = newDirection;
}

EDirection Character::GetDirection() const
{
	return Direction;
}

bool Character::GetIsActionEnd() const
{
	return IsActionEnd;
}

void Character::SetIsActionEnd(bool value)
{
	IsActionEnd = value;
}


void Character::SetCharacterID(int id)
{
	CharacterID = id;
}

int Character::GetCharacterID() const
{
	return CharacterID;
}

void Character::SetName(const std::wstring& newName)
{
	Name = newName;
}

std::wstring Character::GetName() const
{
	return Name;
}

ECharacterClass Character::GetClassType() const
{
	return ClassType;
}

void Character::SetElement(EElement newElement)
{
	Element = newElement;
	RebuildSkillsFromClassElement();
}

EElement Character::GetElement() const
{
	return Element;
}

std::wstring Character::GetElementName() const
{
	return SkillData::GetElementName(Element);
}


void Character::SetGridPos(int gx, int gy)
{
	GridPos.x = gx;
	GridPos.y = gy;
}

Location Character::GetGridPos() const
{
	return GridPos;
}

void Character::SetPixelPos(int px, int py)
{
	PixelPos.x = px;
	PixelPos.y = py;
}

Location Character::GetPixelPos() const
{
	return PixelPos;
}

void Character::SyncWorldPixelToGrid(int tileSize)
{
	IsMoving = false;
	KeepWalkingOnArrival = false;
	WorldPixelX = static_cast<float>(GridPos.x * tileSize);
	WorldPixelY = static_cast<float>(GridPos.y * tileSize);

	TargetPixelX = WorldPixelX;
	TargetPixelY = WorldPixelY;

	TargetGridX = GridPos.x;
	TargetGridY = GridPos.y;
}

Location Character::GetWorldPixelPos(int tileSize) const
{
	Location result;

	if (IsMoving)
	{
		result.x = static_cast<int>(WorldPixelX);
		result.y = static_cast<int>(WorldPixelY);
	}
	else
	{
		result.x = GridPos.x * tileSize;
		result.y = GridPos.y * tileSize;
	}

	return result;
}

bool Character::StartMoveToGrid(int targetX, int targetY, int tileSize, bool keepWalkingOnArrival)
{
	if (IsMoving || State == ECharacterState::Dead || Stat.CurHP <= 0 || tileSize <= 0)
	{
		return false;
	}

	if (!IsInMoveRange(targetX, targetY))
	{
		return false;
	}

	TargetGridX = targetX;
	TargetGridY = targetY;

	WorldPixelX = static_cast<float>(GridPos.x * tileSize);
	WorldPixelY = static_cast<float>(GridPos.y * tileSize);

	TargetPixelX = static_cast<float>(targetX * tileSize);
	TargetPixelY = static_cast<float>(targetY * tileSize);

	LookAtGrid(targetX, targetY);

	KeepWalkingOnArrival = keepWalkingOnArrival;
	IsMoving = true;
	SetState(ECharacterState::Walk);

	return true;
}

bool Character::GetIsMoving() const
{
	return IsMoving;
}

bool Character::MoveToGrid(int targetX, int targetY)
{
	if (!IsInMoveRange(targetX, targetY))
	{
		return false;
	}

	LookAtGrid(targetX, targetY);

	GridPos.x = targetX;
	GridPos.y = targetY;

	IsMoving = false;
	KeepWalkingOnArrival = false;

	SyncWorldPixelToGrid(1);

	return true;
}


CharacterStat Character::GetStat() const
{
	return Stat;
}

void Character::SetStat(const CharacterStat& newStat)
{
	Stat = newStat;

	if (Stat.MaxHP < 1)
	{
		Stat.MaxHP = 1;
	}

	if (Stat.CurHP > Stat.MaxHP)
	{
		Stat.CurHP = Stat.MaxHP;
	}

	if (Stat.CurHP < 0)
	{
		Stat.CurHP = 0;
	}

	if (Stat.MaxMP < 0)
	{
		Stat.MaxMP = 0;
	}

	if (Stat.CurMP > Stat.MaxMP)
	{
		Stat.CurMP = Stat.MaxMP;
	}

	if (Stat.CurMP < 0)
	{
		Stat.CurMP = 0;
	}
}

int Character::GetHP() const
{
	return Stat.CurHP;
}

int Character::GetMaxHP() const
{
	return Stat.MaxHP;
}

int Character::GetMP() const
{
	return Stat.CurMP;
}

int Character::GetMaxMP() const
{
	return Stat.MaxMP;
}

int Character::GetLevel() const
{
	return Level;
}

void Character::SetLevel(int newLevel)
{
	Level = newLevel;

	if (Level < 1)
	{
		Level = 1;
	}
}

int Character::GetExp() const
{
	return Exp;
}

void Character::AddExp(int addExp)
{
	if (addExp <= 0)
	{
		return;
	}

	Exp += addExp;

	while (Exp >= Level * 100)
	{
		Exp -= Level * 100;
		Level++;

		switch (ClassType)
		{
		case ECharacterClass::Warrior:
			Stat.MaxHP += 15;
			Stat.MaxMP += 2;
			Stat.STR += 3;
			Stat.INT += 1;
			Stat.DEF += 2;
			Stat.SPD += 1;
			break;

		case ECharacterClass::Archer:
			Stat.MaxHP += 10;
			Stat.MaxMP += 4;
			Stat.STR += 2;
			Stat.INT += 1;
			Stat.DEF += 1;
			Stat.SPD += 2;
			break;

		case ECharacterClass::Mage:
			Stat.MaxHP += 7;
			Stat.MaxMP += 8;
			Stat.STR += 1;
			Stat.INT += 3;
			Stat.DEF += 1;
			Stat.SPD += 1;
			break;

		default:
			Stat.MaxHP += 10;
			Stat.MaxMP += 5;
			Stat.STR += 2;
			Stat.INT += 2;
			Stat.DEF += 1;
			Stat.SPD += 1;
			break;
		}

		Stat.CurHP = Stat.MaxHP;
		Stat.CurMP = Stat.MaxMP;
	}
}

int Character::GetMoney() const
{
	return Money;
}

void Character::AddMoney(int addMoney)
{
	if (addMoney <= 0)
	{
		return;
	}

	Money += addMoney;
}

bool Character::UseMoney(int useMoney)
{
	if (useMoney <= 0)
	{
		return true;
	}

	if (Money < useMoney)
	{
		return false;
	}

	Money -= useMoney;

	return true;
}

int Character::GetAP() const
{
	return AP;
}

void Character::SetAP(int newAP)
{
	AP = newAP;

	if (AP < 0)
	{
		AP = 0;
	}
}


CharacterEquipment Character::GetEquipment() const
{
	return Equip;
}

void Character::SetEquipment(const CharacterEquipment& newEquipment)
{
	Equip = newEquipment;
}

void Character::SetWeaponID(int itemID)
{
	Equip.WeaponID = itemID;
}

void Character::SetArmorID(int itemID)
{
	Equip.ArmorID = itemID;
}

void Character::SetAccessoryID(int itemID)
{
	Equip.AccessoryID = itemID;
}


void Character::AddSkillID(int skillID)
{
	for (int id : LearnedSkillIDs)
	{
		if (id == skillID)
		{
			return;
		}
	}

	LearnedSkillIDs.push_back(skillID);
}

void Character::ClearSkillIDs()
{
	LearnedSkillIDs.clear();
	ClearEquippedSkills();
}

void Character::RebuildSkillsFromClassElement()
{
	LearnedSkillIDs.clear();
	ClearEquippedSkills();

	std::vector<int> skillIDs = SkillData::GetSkillIDs(ClassType, Element);

	for (int skillID : skillIDs)
	{
		LearnedSkillIDs.push_back(skillID);
	}

	int equipCount = (int)skillIDs.size();
	if (equipCount > 3)
	{
		equipCount = 3;
	}

	for (int i = 0; i < equipCount; ++i)
	{
		EquippedSkillIDs[i] = skillIDs[i];
	}
}

bool Character::HasSkillID(int skillID) const
{
	for (int id : LearnedSkillIDs)
	{
		if (id == skillID)
		{
			return true;
		}
	}

	return false;
}

std::vector<int> Character::GetLearnedSkillIDs() const
{
	return LearnedSkillIDs;
}

void Character::SetEquippedSkillID(int slot, int skillID)
{
	if (slot < 0 || slot >= 3)
	{
		return;
	}

	if (skillID >= 0)
	{
		if (!HasSkillID(skillID) || !SkillData::CanCharacterUse(this, skillID))
		{
			return;
		}
	}

	EquippedSkillIDs[slot] = skillID;
}

int Character::GetEquippedSkillID(int slot) const
{
	if (slot < 0 || slot >= 3)
	{
		return -1;
	}

	return EquippedSkillIDs[slot];
}

std::vector<int> Character::GetEquippedSkillIDs() const
{
	std::vector<int> result;

	for (int i = 0; i < 3; ++i)
	{
		if (EquippedSkillIDs[i] >= 0 && HasSkillID(EquippedSkillIDs[i]))
		{
			result.push_back(EquippedSkillIDs[i]);
		}
	}

	return result;
}

void Character::ClearEquippedSkills()
{
	for (int i = 0; i < 3; ++i)
	{
		EquippedSkillIDs[i] = -1;
	}
}


bool Character::IsAlive() const
{
	return Stat.CurHP > 0;
}

bool Character::IsInMoveRange(int targetX, int targetY) const
{
	int dx = std::abs(targetX - GridPos.x);
	int dy = std::abs(targetY - GridPos.y);

	return dx + dy <= AP;
}

bool Character::IsInAttackRange(const Character& target) const
{
	Location targetPos = target.GetGridPos();

	return IsInAttackRangeByGrid(targetPos.x, targetPos.y);
}

bool Character::IsInAttackRangeByGrid(int targetX, int targetY) const
{
	int dx = std::abs(targetX - GridPos.x);
	int dy = std::abs(targetY - GridPos.y);

	int attackRange = 1;

	switch (ClassType)
	{
	case ECharacterClass::Warrior:
		attackRange = 1;
		break;

	case ECharacterClass::Archer:
		attackRange = 3;
		break;

	case ECharacterClass::Mage:
		attackRange = 2;
		break;

	default:
		attackRange = 1;
		break;
	}

	return dx + dy <= attackRange;
}

int Character::CalcAttackDamage(const Character& target) const
{
	CharacterStat targetStat = target.GetStat();

	int damage = 1;

	switch (ClassType)
	{
	case ECharacterClass::Warrior:
		damage = Stat.STR * 2 - targetStat.DEF;
		break;

	case ECharacterClass::Archer:
		damage = Stat.STR * 2 + Stat.SPD / 2 - targetStat.DEF;
		break;

	case ECharacterClass::Mage:
		damage = Stat.INT + Stat.STR - targetStat.DEF;
		break;

	default:
		damage = Stat.STR * 2 - targetStat.DEF;
		break;
	}

	float elementRate = SkillData::GetElementRate(Element, target.GetElement());
	damage = (int)(damage * elementRate);

	if (damage < 1)
	{
		damage = 1;
	}

	return damage;
}

int Character::CalcSkillDamage(const Character& target) const
{
	CharacterStat targetStat = target.GetStat();

	int damage = 1;

	switch (ClassType)
	{
	case ECharacterClass::Warrior:
		damage = Stat.STR * 3 - targetStat.DEF;
		break;

	case ECharacterClass::Archer:
		damage = Stat.STR * 2 + Stat.SPD - targetStat.DEF / 2;
		break;

	case ECharacterClass::Mage:
		damage = Stat.INT * 3 - targetStat.DEF / 2;
		break;

	default:
		damage = Stat.INT * 2 - targetStat.DEF;
		break;
	}

	if (damage < 1)
	{
		damage = 1;
	}

	return damage;
}

void Character::GetDamage(int damage, bool playReactionSound)
{
	if (State == ECharacterState::Dead)
	{
		return;
	}

	if (damage < 0)
	{
		damage = 0;
	}

	if (State == ECharacterState::Defend)
	{
		damage = static_cast<int>(damage * DefendRate);
	}

	Stat.CurHP -= damage;

	if (Stat.CurHP <= 0)
	{
		Stat.CurHP = 0;
		Die(playReactionSound);
	}
	else
	{
		SetState(ECharacterState::Hit, true, playReactionSound);
	}
}

void Character::Die(bool playReactionSound)
{
	Stat.CurHP = 0;
	IsMoving = false;
	KeepWalkingOnArrival = false;

	SetState(ECharacterState::Dead, false, playReactionSound);
}

float Character::GetDefendRate() const
{
	return DefendRate;
}

void Character::SetDefendRate(float rate)
{
	DefendRate = rate;

	if (DefendRate < 0.0f)
	{
		DefendRate = 0.0f;
	}

	if (DefendRate > 1.0f)
	{
		DefendRate = 1.0f;
	}
}


void Character::LookAtGrid(int targetX, int targetY)
{
	int dx = targetX - GridPos.x;

	if (dx < 0)
	{
		SetDirection(EDirection::Left);
	}
	else if (dx > 0)
	{
		SetDirection(EDirection::Right);
	}
}
