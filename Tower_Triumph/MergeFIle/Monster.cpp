#include "Monster.h"
#include "Character.h"
#include "SoundManager.h"


// ============================================================
// 몬스터 스프라이트 시트 판정 유틸
// ============================================================

static bool IsMonsterSixByThreeSheet(const BITMAP* info)
{
	if (info == nullptr || info->bmWidth <= 0 || info->bmHeight <= 0)
	{
		return false;
	}

	int frameW = info->bmWidth / MONSTER_6X3_SHEET_COL;
	int frameH = info->bmHeight / MONSTER_6X3_SHEET_ROW;

	if (frameW <= 0 || frameH <= 0)
	{
		return false;
	}

	// 6x3 몬스터 시트는 보통 한 칸이 거의 정사각형이다.
	// 이미지 생성본처럼 1~2px 정도 안 맞을 수 있으니 25% 오차를 허용한다.
	int diff = abs(frameW - frameH);
	int bigger = (frameW > frameH) ? frameW : frameH;

	return diff * 4 <= bigger;
}

static int GetMonsterSheetCol(const BITMAP* info)
{
	return IsMonsterSixByThreeSheet(info) ? MONSTER_6X3_SHEET_COL : MONSTER_SHEET_COL;
}

static int GetMonsterSheetRow(const BITMAP* info)
{
	return IsMonsterSixByThreeSheet(info) ? MONSTER_6X3_SHEET_ROW : MONSTER_SHEET_ROW;
}

static int GetMonsterFrameCount(const BITMAP* info)
{
	int col = GetMonsterSheetCol(info);
	int row = GetMonsterSheetRow(info);

	int count = col * row;

	if (count <= 0)
	{
		count = 1;
	}

	return count;
}


// ============================================================
// 생성자 / 소멸자
// ============================================================

Monster::Monster()
{
	MonsterID = -1;

	Type = EMonsterType::None;
	State = EMonsterState::Idle;
	AIType = EMonsterAIType::None;
	AIState = EMonsterAIState::Wait;
	Direction = EMonsterDirection::Left;

	IsBossMonster = false;

	Level = 1;
	AP = 3;
	AttackRange = 1;

	GridPos.x = 0;
	GridPos.y = 0;

	PixelPos.x = 0;
	PixelPos.y = 0;

	IsMoving = false;

	WorldPixelX = 0.0f;
	WorldPixelY = 0.0f;

	TargetPixelX = 0.0f;
	TargetPixelY = 0.0f;

	TargetGridX = 0;
	TargetGridY = 0;

	MoveSpeed = 6.0f;

	CurFrame = 0;
	MaxFrame = MONSTER_SHEET_COL;
	AniTick = 0;
	AniDelay = 8;

	IsActionEnd = false;

	hSpriteIdle = NULL;
	hSpriteWalk = NULL;
	hSpriteAttack = NULL;
	hSpriteHit = NULL;
	hSpriteDead = NULL;

	ZeroMemory(&SpriteInfoIdle, sizeof(BITMAP));
	ZeroMemory(&SpriteInfoWalk, sizeof(BITMAP));
	ZeroMemory(&SpriteInfoAttack, sizeof(BITMAP));
	ZeroMemory(&SpriteInfoHit, sizeof(BITMAP));
	ZeroMemory(&SpriteInfoDead, sizeof(BITMAP));
}

Monster::~Monster()
{
	Release();
}

// ============================================================
// 이동 생성자 / 이동 대입 연산자
// HBITMAP 중복 삭제 방지용
// ============================================================

Monster::Monster(Monster&& other) noexcept
{
	MonsterID = other.MonsterID;

	Type = other.Type;
	State = other.State;
	AIType = other.AIType;
	AIState = other.AIState;
	Direction = other.Direction;

	IsBossMonster = other.IsBossMonster;

	Stat = other.Stat;
	Reward = other.Reward;

	Level = other.Level;
	AP = other.AP;
	AttackRange = other.AttackRange;

	GridPos = other.GridPos;
	PixelPos = other.PixelPos;

	IsMoving = other.IsMoving;

	WorldPixelX = other.WorldPixelX;
	WorldPixelY = other.WorldPixelY;

	TargetPixelX = other.TargetPixelX;
	TargetPixelY = other.TargetPixelY;

	TargetGridX = other.TargetGridX;
	TargetGridY = other.TargetGridY;

	MoveSpeed = other.MoveSpeed;

	CurFrame = other.CurFrame;
	MaxFrame = other.MaxFrame;
	AniTick = other.AniTick;
	AniDelay = other.AniDelay;

	IsActionEnd = other.IsActionEnd;

	hSpriteIdle = other.hSpriteIdle;
	hSpriteWalk = other.hSpriteWalk;
	hSpriteAttack = other.hSpriteAttack;
	hSpriteHit = other.hSpriteHit;
	hSpriteDead = other.hSpriteDead;

	SpriteInfoIdle = other.SpriteInfoIdle;
	SpriteInfoWalk = other.SpriteInfoWalk;
	SpriteInfoAttack = other.SpriteInfoAttack;
	SpriteInfoHit = other.SpriteInfoHit;
	SpriteInfoDead = other.SpriteInfoDead;

	other.hSpriteIdle = NULL;
	other.hSpriteWalk = NULL;
	other.hSpriteAttack = NULL;
	other.hSpriteHit = NULL;
	other.hSpriteDead = NULL;
}

Monster& Monster::operator=(Monster&& other) noexcept
{
	if (this == &other)
	{
		return *this;
	}

	Release();

	MonsterID = other.MonsterID;

	Type = other.Type;
	State = other.State;
	AIType = other.AIType;
	AIState = other.AIState;
	Direction = other.Direction;

	IsBossMonster = other.IsBossMonster;

	Stat = other.Stat;
	Reward = other.Reward;

	Level = other.Level;
	AP = other.AP;
	AttackRange = other.AttackRange;

	GridPos = other.GridPos;
	PixelPos = other.PixelPos;

	IsMoving = other.IsMoving;

	WorldPixelX = other.WorldPixelX;
	WorldPixelY = other.WorldPixelY;

	TargetPixelX = other.TargetPixelX;
	TargetPixelY = other.TargetPixelY;

	TargetGridX = other.TargetGridX;
	TargetGridY = other.TargetGridY;

	MoveSpeed = other.MoveSpeed;

	CurFrame = other.CurFrame;
	MaxFrame = other.MaxFrame;
	AniTick = other.AniTick;
	AniDelay = other.AniDelay;

	IsActionEnd = other.IsActionEnd;

	hSpriteIdle = other.hSpriteIdle;
	hSpriteWalk = other.hSpriteWalk;
	hSpriteAttack = other.hSpriteAttack;
	hSpriteHit = other.hSpriteHit;
	hSpriteDead = other.hSpriteDead;

	SpriteInfoIdle = other.SpriteInfoIdle;
	SpriteInfoWalk = other.SpriteInfoWalk;
	SpriteInfoAttack = other.SpriteInfoAttack;
	SpriteInfoHit = other.SpriteInfoHit;
	SpriteInfoDead = other.SpriteInfoDead;

	other.hSpriteIdle = NULL;
	other.hSpriteWalk = NULL;
	other.hSpriteAttack = NULL;
	other.hSpriteHit = NULL;
	other.hSpriteDead = NULL;

	return *this;
}

// ============================================================
// 초기화 / 해제
// ============================================================

void Monster::Init(EMonsterType monsterType, int gridX, int gridY)
{
	Type = monsterType;

	State = EMonsterState::Idle;
	AIState = EMonsterAIState::Wait;
	Direction = EMonsterDirection::Left;

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

	MoveSpeed = 6.0f;

	CurFrame = 0;
	MaxFrame = MONSTER_SHEET_COL;
	AniTick = 0;
	AniDelay = 8;

	IsActionEnd = false;

	InitMonsterValue();
}

void Monster::Release()
{
	HBITMAP handles[5] =
	{
		hSpriteIdle,
		hSpriteWalk,
		hSpriteAttack,
		hSpriteHit,
		hSpriteDead
	};

	for (int i = 0; i < 5; ++i)
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
	hSpriteHit = NULL;
	hSpriteDead = NULL;

	ZeroMemory(&SpriteInfoIdle, sizeof(BITMAP));
	ZeroMemory(&SpriteInfoWalk, sizeof(BITMAP));
	ZeroMemory(&SpriteInfoAttack, sizeof(BITMAP));
	ZeroMemory(&SpriteInfoHit, sizeof(BITMAP));
	ZeroMemory(&SpriteInfoDead, sizeof(BITMAP));
}

// ============================================================
// 몬스터 타입별 기본값
// ============================================================

void Monster::InitMonsterValue()
{
	IsBossMonster = false;

	switch (Type)
	{
	case EMonsterType::Slime:
		Stat.MaxHP = 40;
		Stat.CurHP = 40;
		Stat.MaxMP = 0;
		Stat.CurMP = 0;
		Stat.STR = 7;
		Stat.INT = 0;
		Stat.DEF = 2;
		Stat.SPD = 5;

		AIType = EMonsterAIType::Melee;
		AP = 3;
		AttackRange = 1;

		Reward.Exp = 8;
		Reward.Money = 4;
		Reward.DropItemID = -1;
		Reward.DropRate = 0;
		break;

	case EMonsterType::Goblin:
		Stat.MaxHP = 55;
		Stat.CurHP = 55;
		Stat.MaxMP = 0;
		Stat.CurMP = 0;
		Stat.STR = 10;
		Stat.INT = 0;
		Stat.DEF = 4;
		Stat.SPD = 8;

		AIType = EMonsterAIType::Melee;
		AP = 4;
		AttackRange = 1;

		Reward.Exp = 12;
		Reward.Money = 8;
		Reward.DropItemID = -1;
		Reward.DropRate = 0;
		break;

	case EMonsterType::Skeleton:
		Stat.MaxHP = 70;
		Stat.CurHP = 70;
		Stat.MaxMP = 0;
		Stat.CurMP = 0;
		Stat.STR = 12;
		Stat.INT = 0;
		Stat.DEF = 6;
		Stat.SPD = 6;

		AIType = EMonsterAIType::Melee;
		AP = 3;
		AttackRange = 1;

		Reward.Exp = 15;
		Reward.Money = 10;
		Reward.DropItemID = -1;
		Reward.DropRate = 0;
		break;

	case EMonsterType::Zombie:
		Stat.MaxHP = 95;
		Stat.CurHP = 95;
		Stat.MaxMP = 0;
		Stat.CurMP = 0;
		Stat.STR = 14;
		Stat.INT = 0;
		Stat.DEF = 8;
		Stat.SPD = 3;

		AIType = EMonsterAIType::Melee;
		AP = 2;
		AttackRange = 1;

		Reward.Exp = 18;
		Reward.Money = 12;
		Reward.DropItemID = -1;
		Reward.DropRate = 0;
		break;

	case EMonsterType::Ghost:
		Stat.MaxHP = 55;
		Stat.CurHP = 55;
		Stat.MaxMP = 30;
		Stat.CurMP = 30;
		Stat.STR = 7;
		Stat.INT = 15;
		Stat.DEF = 3;
		Stat.SPD = 9;

		AIType = EMonsterAIType::Caster;
		AP = 4;
		AttackRange = 3;

		Reward.Exp = 20;
		Reward.Money = 15;
		Reward.DropItemID = -1;
		Reward.DropRate = 0;
		break;

	case EMonsterType::DarkKnight:
		Stat.MaxHP = 130;
		Stat.CurHP = 130;
		Stat.MaxMP = 20;
		Stat.CurMP = 20;
		Stat.STR = 20;
		Stat.INT = 8;
		Stat.DEF = 14;
		Stat.SPD = 7;

		AIType = EMonsterAIType::Melee;
		AP = 3;
		AttackRange = 1;

		Reward.Exp = 35;
		Reward.Money = 30;
		Reward.DropItemID = -1;
		Reward.DropRate = 0;
		break;

	case EMonsterType::FireSlime:
		Stat.MaxHP = 60;
		Stat.CurHP = 60;
		Stat.MaxMP = 0;
		Stat.CurMP = 0;
		Stat.STR = 12;
		Stat.INT = 0;
		Stat.DEF = 4;
		Stat.SPD = 6;

		AIType = EMonsterAIType::Melee;
		AP = 3;
		AttackRange = 1;

		Reward.Exp = 16;
		Reward.Money = 10;
		Reward.DropItemID = -1;
		Reward.DropRate = 0;
		break;

	case EMonsterType::LavaGolem:
		Stat.MaxHP = 160;
		Stat.CurHP = 160;
		Stat.MaxMP = 0;
		Stat.CurMP = 0;
		Stat.STR = 22;
		Stat.INT = 0;
		Stat.DEF = 18;
		Stat.SPD = 2;

		AIType = EMonsterAIType::Melee;
		AP = 2;
		AttackRange = 1;

		Reward.Exp = 40;
		Reward.Money = 25;
		Reward.DropItemID = -1;
		Reward.DropRate = 0;
		break;

	case EMonsterType::FlameDemon:
		Stat.MaxHP = 120;
		Stat.CurHP = 120;
		Stat.MaxMP = 60;
		Stat.CurMP = 60;
		Stat.STR = 16;
		Stat.INT = 22;
		Stat.DEF = 10;
		Stat.SPD = 8;

		AIType = EMonsterAIType::Caster;
		AP = 3;
		AttackRange = 3;

		Reward.Exp = 45;
		Reward.Money = 35;
		Reward.DropItemID = -1;
		Reward.DropRate = 0;
		break;

	case EMonsterType::SeaSlime:
		Stat.MaxHP = 50;
		Stat.CurHP = 50;
		Stat.MaxMP = 0;
		Stat.CurMP = 0;
		Stat.STR = 9;
		Stat.INT = 0;
		Stat.DEF = 3;
		Stat.SPD = 6;

		AIType = EMonsterAIType::Melee;
		AP = 3;
		AttackRange = 1;

		Reward.Exp = 12;
		Reward.Money = 8;
		Reward.DropItemID = -1;
		Reward.DropRate = 0;
		break;

	case EMonsterType::Mermaid:
		Stat.MaxHP = 75;
		Stat.CurHP = 75;
		Stat.MaxMP = 45;
		Stat.CurMP = 45;
		Stat.STR = 8;
		Stat.INT = 18;
		Stat.DEF = 5;
		Stat.SPD = 9;

		AIType = EMonsterAIType::Caster;
		AP = 4;
		AttackRange = 3;

		Reward.Exp = 24;
		Reward.Money = 20;
		Reward.DropItemID = -1;
		Reward.DropRate = 0;
		break;

	case EMonsterType::SeaSerpent:
		Stat.MaxHP = 140;
		Stat.CurHP = 140;
		Stat.MaxMP = 30;
		Stat.CurMP = 30;
		Stat.STR = 18;
		Stat.INT = 12;
		Stat.DEF = 12;
		Stat.SPD = 7;

		AIType = EMonsterAIType::Melee;
		AP = 3;
		AttackRange = 2;

		Reward.Exp = 38;
		Reward.Money = 28;
		Reward.DropItemID = -1;
		Reward.DropRate = 0;
		break;

	case EMonsterType::Harpy:
		Stat.MaxHP = 65;
		Stat.CurHP = 65;
		Stat.MaxMP = 0;
		Stat.CurMP = 0;
		Stat.STR = 13;
		Stat.INT = 0;
		Stat.DEF = 4;
		Stat.SPD = 13;

		AIType = EMonsterAIType::Melee;
		AP = 5;
		AttackRange = 1;

		Reward.Exp = 22;
		Reward.Money = 18;
		Reward.DropItemID = -1;
		Reward.DropRate = 0;
		break;

	case EMonsterType::AngelGuardian:
		Stat.MaxHP = 110;
		Stat.CurHP = 110;
		Stat.MaxMP = 40;
		Stat.CurMP = 40;
		Stat.STR = 16;
		Stat.INT = 16;
		Stat.DEF = 12;
		Stat.SPD = 8;

		AIType = EMonsterAIType::Melee;
		AP = 3;
		AttackRange = 1;

		Reward.Exp = 35;
		Reward.Money = 30;
		Reward.DropItemID = -1;
		Reward.DropRate = 0;
		break;

	case EMonsterType::SkySpirit:
		Stat.MaxHP = 70;
		Stat.CurHP = 70;
		Stat.MaxMP = 50;
		Stat.CurMP = 50;
		Stat.STR = 7;
		Stat.INT = 20;
		Stat.DEF = 4;
		Stat.SPD = 12;

		AIType = EMonsterAIType::Caster;
		AP = 4;
		AttackRange = 3;

		Reward.Exp = 28;
		Reward.Money = 22;
		Reward.DropItemID = -1;
		Reward.DropRate = 0;
		break;

	case EMonsterType::Boss:
		Stat.MaxHP = 300;
		Stat.CurHP = 300;
		Stat.MaxMP = 100;
		Stat.CurMP = 100;
		Stat.STR = 30;
		Stat.INT = 25;
		Stat.DEF = 20;
		Stat.SPD = 10;

		AIType = EMonsterAIType::Boss;
		AP = 3;
		AttackRange = 2;

		Reward.Exp = 100;
		Reward.Money = 100;
		Reward.DropItemID = -1;
		Reward.DropRate = 0;

		IsBossMonster = true;
		break;

	default:
		Stat = MonsterStat();
		Reward = MonsterReward();

		AIType = EMonsterAIType::Melee;
		AP = 3;
		AttackRange = 1;
		break;
	}
}

// ============================================================
// 스프라이트 로드
// ============================================================

HBITMAP Monster::LoadBitmapFromFile(const std::wstring& filePath, BITMAP* outInfo)
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

std::wstring Monster::GetMonsterPrefix() const
{
	switch (Type)
	{
	case EMonsterType::Slime:
		return L"Slime";

	case EMonsterType::Goblin:
		return L"Goblin";

	case EMonsterType::Skeleton:
		return L"Skeleton";

	case EMonsterType::Zombie:
		return L"Zombie";

	case EMonsterType::Ghost:
		return L"Ghost";

	case EMonsterType::DarkKnight:
		return L"DarkKnight";

	case EMonsterType::FireSlime:
		return L"FireSlime";

	case EMonsterType::LavaGolem:
		return L"LavaGolem";

	case EMonsterType::FlameDemon:
		return L"FlameDemon";

	case EMonsterType::SeaSlime:
		return L"SeaSlime";

	case EMonsterType::Mermaid:
		return L"Mermaid";

	case EMonsterType::SeaSerpent:
		return L"SeaSerpent";

	case EMonsterType::Harpy:
		return L"Harpy";

	case EMonsterType::AngelGuardian:
		return L"AngelGuardian";

	case EMonsterType::SkySpirit:
		return L"SkySpirit";

	case EMonsterType::Boss:
		return L"Boss";

	default:
		return L"Monster";
	}
}

bool Monster::LoadSprites(const std::wstring& folderPath)
{
	Release();

	std::wstring basePath = folderPath;

	if (!basePath.empty())
	{
		wchar_t lastChar = basePath[basePath.length() - 1];

		if (lastChar != L'\\' && lastChar != L'/')
		{
			basePath += L"\\";
		}
	}

	std::wstring prefix = GetMonsterPrefix();

	hSpriteIdle = LoadBitmapFromFile(basePath + prefix + L"_Idle.bmp", &SpriteInfoIdle);
	hSpriteWalk = LoadBitmapFromFile(basePath + prefix + L"_Walk.bmp", &SpriteInfoWalk);
	hSpriteAttack = LoadBitmapFromFile(basePath + prefix + L"_Attack.bmp", &SpriteInfoAttack);
	hSpriteHit = LoadBitmapFromFile(basePath + prefix + L"_Hit.bmp", &SpriteInfoHit);
	hSpriteDead = LoadBitmapFromFile(basePath + prefix + L"_Dead.bmp", &SpriteInfoDead);

	if (hSpriteIdle == NULL)
	{
		return false;
	}

	return true;
}

HBITMAP Monster::GetSpriteIdle() const
{
	return hSpriteIdle;
}

// ============================================================
// 현재 상태에 맞는 스프라이트 반환
// ============================================================

HBITMAP Monster::GetCurrentSprite() const
{
	switch (State)
	{
	case EMonsterState::Idle:
		if (hSpriteIdle != NULL) return hSpriteIdle;
		break;

	case EMonsterState::Walk:
		if (hSpriteWalk != NULL) return hSpriteWalk;
		break;

	case EMonsterState::Attack:
		if (hSpriteAttack != NULL) return hSpriteAttack;
		break;

	case EMonsterState::Hit:
		if (hSpriteHit != NULL) return hSpriteHit;
		break;

	case EMonsterState::Dead:
		if (hSpriteDead != NULL) return hSpriteDead;
		break;
	}

	return hSpriteIdle;
}

const BITMAP* Monster::GetCurrentSpriteInfo() const
{
	switch (State)
	{
	case EMonsterState::Idle:
		if (hSpriteIdle != NULL) return &SpriteInfoIdle;
		break;

	case EMonsterState::Walk:
		if (hSpriteWalk != NULL) return &SpriteInfoWalk;
		break;

	case EMonsterState::Attack:
		if (hSpriteAttack != NULL) return &SpriteInfoAttack;
		break;

	case EMonsterState::Hit:
		if (hSpriteHit != NULL) return &SpriteInfoHit;
		break;

	case EMonsterState::Dead:
		if (hSpriteDead != NULL) return &SpriteInfoDead;
		break;
	}

	return &SpriteInfoIdle;
}

int Monster::GetDirectionRow() const
{
	return 0;
}

bool Monster::IsNeedFlip() const
{
	// 신규 몬스터 스프라이트 원본은 오른쪽 방향 기준이다.
	// 따라서 왼쪽 방향으로 출력할 때만 좌우 반전한다.
	return Direction == EMonsterDirection::Left;
}

void Monster::UpdateMaxFrameByState()
{
	const BITMAP* curInfo = GetCurrentSpriteInfo();

	MaxFrame = GetMonsterFrameCount(curInfo);

	if (MaxFrame <= 0)
	{
		MaxFrame = 1;
	}

	if (CurFrame >= MaxFrame)
	{
		CurFrame = 0;
	}
}

// ============================================================
// Update / Draw
// ============================================================

void Monster::Update()
{
	if (Stat.CurHP <= 0 && State != EMonsterState::Dead)
	{
		Die();
	}

	if (IsMoving)
	{
		float dx = TargetPixelX - WorldPixelX;
		float dy = TargetPixelY - WorldPixelY;

		float dist = sqrtf(dx * dx + dy * dy);

		if (dist <= MoveSpeed)
		{
			WorldPixelX = TargetPixelX;
			WorldPixelY = TargetPixelY;

			GridPos.x = TargetGridX;
			GridPos.y = TargetGridY;

			IsMoving = false;
			SetState(EMonsterState::Idle);
		}
		else if (dist > 0.0f)
		{
			WorldPixelX += (dx / dist) * MoveSpeed;
			WorldPixelY += (dy / dist) * MoveSpeed;
		}
	}

	const BITMAP* curInfo = GetCurrentSpriteInfo();
	bool useSixByThree = IsMonsterSixByThreeSheet(curInfo);

	// 기존 4x1 Idle 시트는 기존처럼 0번 프레임만 사용한다.
	// 신규 6x3 Idle 시트는 18프레임 Idle 애니메이션을 재생한다.
	if (State == EMonsterState::Idle && !useSixByThree)
	{
		CurFrame = 0;
		return;
	}

	int currentAniDelay = AniDelay;

	if (useSixByThree)
	{
		switch (State)
		{
		case EMonsterState::Idle:
			currentAniDelay = 5;
			break;

		case EMonsterState::Walk:
			currentAniDelay = 3;
			break;

		case EMonsterState::Attack:
		case EMonsterState::Hit:
			currentAniDelay = 2;
			break;

		case EMonsterState::Dead:
			currentAniDelay = 3;
			break;
		}
	}

	AniTick++;

	if (AniTick < currentAniDelay)
	{
		return;
	}

	AniTick = 0;
	CurFrame++;

	if (CurFrame >= MaxFrame)
	{
		switch (State)
		{
		case EMonsterState::Idle:
		case EMonsterState::Walk:
			CurFrame = 0;
			break;

		case EMonsterState::Attack:
		case EMonsterState::Hit:
		case EMonsterState::Dead:
			CurFrame = MaxFrame - 1;
			IsActionEnd = true;
			break;

		default:
			CurFrame = 0;
			break;
		}
	}
}

void Monster::Draw(HDC hMemDC, HDC hMemDC1)
{
	Draw(hMemDC, hMemDC1, 0, 0);
}

void Monster::Draw(HDC hMemDC, HDC hMemDC1, int drawW, int drawH)
{
	HBITMAP curSprite = GetCurrentSprite();
	const BITMAP* curInfo = GetCurrentSpriteInfo();

	if (curSprite == NULL || curInfo == nullptr)
	{
		return;
	}

	if (curInfo->bmWidth <= 0 || curInfo->bmHeight <= 0)
	{
		return;
	}

	int sheetCol = GetMonsterSheetCol(curInfo);
	int sheetRow = GetMonsterSheetRow(curInfo);

	if (sheetCol <= 0 || sheetRow <= 0)
	{
		return;
	}

	int frameW = curInfo->bmWidth / sheetCol;
	int frameH = curInfo->bmHeight / sheetRow;

	if (frameW <= 0 || frameH <= 0)
	{
		return;
	}

	int frameCount = sheetCol * sheetRow;

	if (frameCount <= 0)
	{
		frameCount = 1;
	}

	if (CurFrame < 0)
	{
		CurFrame = 0;
	}

	if (CurFrame >= frameCount)
	{
		CurFrame = frameCount - 1;
	}

	if (drawW <= 0)
	{
		drawW = frameW;
	}

	if (drawH <= 0)
	{
		drawH = frameH;
	}

	int srcCol = CurFrame % sheetCol;
	int srcRow = CurFrame / sheetCol;

	int srcX = srcCol * frameW;
	int srcY = srcRow * frameH;

	int dstX = PixelPos.x;
	int dstY = PixelPos.y;

	HBITMAP hOldBitmap = (HBITMAP)SelectObject(hMemDC1, curSprite);

	if (!IsNeedFlip())
	{
		TransparentBlt(
			hMemDC,
			dstX,
			dstY,
			drawW,
			drawH,
			hMemDC1,
			srcX,
			srcY,
			frameW,
			frameH,
			MONSTER_TRANSPARENT_COLOR
		);
	}
	else
	{
		HDC hFlipDC = CreateCompatibleDC(hMemDC);

		HBITMAP hFlipBitmap = CreateCompatibleBitmap(
			hMemDC,
			frameW,
			frameH
		);

		HBITMAP hOldFlipBitmap = (HBITMAP)SelectObject(hFlipDC, hFlipBitmap);

		HBRUSH hBrush = CreateSolidBrush(MONSTER_TRANSPARENT_COLOR);

		RECT rc;
		rc.left = 0;
		rc.top = 0;
		rc.right = frameW;
		rc.bottom = frameH;

		FillRect(hFlipDC, &rc, hBrush);
		DeleteObject(hBrush);

		StretchBlt(
			hFlipDC,
			frameW,
			0,
			-frameW,
			frameH,
			hMemDC1,
			srcX,
			srcY,
			frameW,
			frameH,
			SRCCOPY
		);

		TransparentBlt(
			hMemDC,
			dstX,
			dstY,
			drawW,
			drawH,
			hFlipDC,
			0,
			0,
			frameW,
			frameH,
			MONSTER_TRANSPARENT_COLOR
		);

		SelectObject(hFlipDC, hOldFlipBitmap);
		DeleteObject(hFlipBitmap);
		DeleteDC(hFlipDC);
	}

	SelectObject(hMemDC1, hOldBitmap);
}

// ============================================================
// 상태
// ============================================================

void Monster::SetState(EMonsterState newState)
{
	if (State == newState)
	{
		return;
	}

	State = newState;

	CurFrame = 0;
	AniTick = 0;
	IsActionEnd = false;

	UpdateMaxFrameByState();

	// ------------------------------------------------------------
	// 몬스터 상태별 효과음
	//
	// 우선 Resource\Sound\<몬스터이름>_<상태>.wav 를 찾고,
	// 없으면 Resource\Sound\Monster_<상태>.wav 공용 효과음을 사용한다.
	//
	// 예)
	// Resource\Sound\FireSlime_Attack.wav
	// Resource\Sound\Monster_Attack.wav
	// ------------------------------------------------------------
	std::wstring soundFolder = L"Resource\\Sound\\";
	std::wstring prefix = GetMonsterPrefix();

	switch (newState)
	{
	case EMonsterState::Attack:
		if (!SoundManager::PlaySFX(soundFolder + prefix + L"_Attack.wav"))
		{
			SoundManager::PlaySFX(soundFolder + L"Monster_Attack.wav");
		}
		break;

	case EMonsterState::Hit:
		if (!SoundManager::PlaySFX(soundFolder + prefix + L"_Hit.wav"))
		{
			SoundManager::PlaySFX(soundFolder + L"Monster_Hit.wav");
		}
		break;

	case EMonsterState::Dead:
		if (!SoundManager::PlaySFX(soundFolder + prefix + L"_Dead.wav"))
		{
			SoundManager::PlaySFX(soundFolder + L"Monster_Dead.wav");
		}
		break;
	}
}

EMonsterState Monster::GetState() const
{
	return State;
}

void Monster::SetAIState(EMonsterAIState newAIState)
{
	AIState = newAIState;
}

EMonsterAIState Monster::GetAIState() const
{
	return AIState;
}

EMonsterAIType Monster::GetAIType() const
{
	return AIType;
}

void Monster::SetDirection(EMonsterDirection newDirection)
{
	Direction = newDirection;
}

EMonsterDirection Monster::GetDirection() const
{
	return Direction;
}

bool Monster::GetIsActionEnd() const
{
	return IsActionEnd;
}

void Monster::SetIsActionEnd(bool value)
{
	IsActionEnd = value;
}

// ============================================================
// ID / 타입
// ============================================================

void Monster::SetMonsterID(int id)
{
	MonsterID = id;
}

int Monster::GetMonsterID() const
{
	return MonsterID;
}

EMonsterType Monster::GetMonsterType() const
{
	return Type;
}

bool Monster::GetIsBossMonster() const
{
	return IsBossMonster;
}

void Monster::SetIsBossMonster(bool value)
{
	IsBossMonster = value;
}

// ============================================================
// 위치
// ============================================================

void Monster::SetGridPos(int gx, int gy)
{
	GridPos.x = gx;
	GridPos.y = gy;
}

Location Monster::GetGridPos() const
{
	return GridPos;
}

void Monster::SetPixelPos(int px, int py)
{
	PixelPos.x = px;
	PixelPos.y = py;
}

Location Monster::GetPixelPos() const
{
	return PixelPos;
}

void Monster::SyncWorldPixelToGrid(int tileSize)
{
	WorldPixelX = static_cast<float>(GridPos.x * tileSize);
	WorldPixelY = static_cast<float>(GridPos.y * tileSize);

	TargetPixelX = WorldPixelX;
	TargetPixelY = WorldPixelY;

	TargetGridX = GridPos.x;
	TargetGridY = GridPos.y;
}

Location Monster::GetWorldPixelPos(int tileSize) const
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

bool Monster::StartMoveToGrid(int targetX, int targetY, int tileSize)
{
	if (IsMoving)
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

	IsMoving = true;
	SetState(EMonsterState::Walk);

	return true;
}

bool Monster::GetIsMoving() const
{
	return IsMoving;
}

bool Monster::MoveToGrid(int targetX, int targetY)
{
	if (!IsInMoveRange(targetX, targetY))
	{
		return false;
	}

	LookAtGrid(targetX, targetY);

	GridPos.x = targetX;
	GridPos.y = targetY;

	IsMoving = false;

	SyncWorldPixelToGrid(1);

	return true;
}

// ============================================================
// 스탯
// ============================================================

MonsterStat Monster::GetStat() const
{
	return Stat;
}

void Monster::SetStat(const MonsterStat& newStat)
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

int Monster::GetHP() const
{
	return Stat.CurHP;
}

int Monster::GetMaxHP() const
{
	return Stat.MaxHP;
}

int Monster::GetMP() const
{
	return Stat.CurMP;
}

int Monster::GetMaxMP() const
{
	return Stat.MaxMP;
}

int Monster::GetLevel() const
{
	return Level;
}

void Monster::SetLevel(int newLevel)
{
	Level = newLevel;

	if (Level < 1)
	{
		Level = 1;
	}
}

int Monster::GetAP() const
{
	return AP;
}

void Monster::SetAP(int newAP)
{
	AP = newAP;

	if (AP < 0)
	{
		AP = 0;
	}
}

int Monster::GetAttackRange() const
{
	return AttackRange;
}

void Monster::SetAttackRange(int newRange)
{
	AttackRange = newRange;

	if (AttackRange < 1)
	{
		AttackRange = 1;
	}
}

// ============================================================
// 보상
// ============================================================

MonsterReward Monster::GetReward() const
{
	return Reward;
}

void Monster::SetReward(const MonsterReward& newReward)
{
	Reward = newReward;

	if (Reward.DropRate < 0)
	{
		Reward.DropRate = 0;
	}

	if (Reward.DropRate > 100)
	{
		Reward.DropRate = 100;
	}
}

// ============================================================
// 전투 판정
// ============================================================

bool Monster::IsAlive() const
{
	return Stat.CurHP > 0;
}

bool Monster::IsInMoveRange(int targetX, int targetY) const
{
	int dx = std::abs(targetX - GridPos.x);
	int dy = std::abs(targetY - GridPos.y);

	return dx + dy <= AP;
}

bool Monster::IsInAttackRange(const Character& target) const
{
	Location targetPos = target.GetGridPos();

	return IsInAttackRangeByGrid(targetPos.x, targetPos.y);
}

bool Monster::IsInAttackRangeByGrid(int targetX, int targetY) const
{
	int dx = std::abs(targetX - GridPos.x);
	int dy = std::abs(targetY - GridPos.y);

	return dx + dy <= AttackRange;
}

int Monster::CalcAttackDamage(const Character& target) const
{
	CharacterStat targetStat = target.GetStat();

	int damage = Stat.STR * 2 - targetStat.DEF;

	if (damage < 1)
	{
		damage = 1;
	}

	return damage;
}

void Monster::GetDamage(int damage)
{
	if (State == EMonsterState::Dead)
	{
		return;
	}

	if (damage < 0)
	{
		damage = 0;
	}

	Stat.CurHP -= damage;

	if (Stat.CurHP <= 0)
	{
		Stat.CurHP = 0;
		Die();
	}
	else
	{
		SetState(EMonsterState::Hit);
	}
}

void Monster::Die()
{
	Stat.CurHP = 0;
	IsMoving = false;

	SetState(EMonsterState::Dead);
	AIState = EMonsterAIState::Dead;
}

// ============================================================
// AI 보조
// ============================================================

void Monster::LookAtGrid(int targetX, int targetY)
{
	int dx = targetX - GridPos.x;

	if (dx < 0)
	{
		SetDirection(EMonsterDirection::Left);
	}
	else if (dx > 0)
	{
		SetDirection(EMonsterDirection::Right);
	}
}

Location Monster::GetNextStepToward(int targetX, int targetY) const
{
	Location next = GridPos;

	int dx = targetX - GridPos.x;
	int dy = targetY - GridPos.y;

	int absDx = std::abs(dx);
	int absDy = std::abs(dy);

	if (absDx > absDy)
	{
		if (dx < 0)
		{
			next.x -= 1;
		}
		else if (dx > 0)
		{
			next.x += 1;
		}
	}
	else
	{
		if (dy < 0)
		{
			next.y -= 1;
		}
		else if (dy > 0)
		{
			next.y += 1;
		}
	}

	return next;
}