#pragma once
#include"CStage.h"
#include"ACamera.h"
#include"URegionMarker.h"
#include"URegionInfoPopup.h"
#include"UMainMenuPopup.h"
#include"ULocationIndicator.h"
#include"UCanvasPanel.h"
#include"UImage.h"
#include "GameManager.h"
#include "ItemData.h"

class Map_Stage : public CStage
{
private:
	ACamera* Camera;

	std::vector<URegionMarker*> Markers;
	std::vector<int> MarkerBattleStageIds;
	struct SMapStageItem
	{
		int ItemID;
		Location WorldPos;
		bool Active;

		SMapStageItem()
			: ItemID(-1), WorldPos{ 0, 0 }, Active(false)
		{
		}

		SMapStageItem(int itemID, int x, int y)
			: ItemID(itemID), WorldPos{ x, y }, Active(true)
		{
		}
	};

	std::vector<SMapStageItem> stageItems;
	URegionInfoPopup* infoPopup;
	UMainMenuPopup* mainMenu;
	ULocationIndicator* locationUI;
	UCanvasPanel screenUI;

	class UMouseMark* MouseMark;
	int NextStageID;
	int SelectedBattleStageId;
	int SelectedMarkerNextStageID;
public:
	Map_Stage();
	virtual ~Map_Stage();
	virtual void Init(HWND hWnd, HINSTANCE const g_hInst);
	virtual void Update(HWND hWnd);
	virtual void Draw(HDC hMemDC, HDC hMemDC1);
	virtual int Input(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	virtual void Comp_NextStage(int ID);
	virtual void BackgroundMove(int x, int y) override;
};

