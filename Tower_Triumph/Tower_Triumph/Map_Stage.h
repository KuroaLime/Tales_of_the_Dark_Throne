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
#include "UNoticePopup.h"

class Map_Stage : public CStage
{
private:
	ACamera* Camera;

	std::vector<URegionMarker*> Markers;
	std::vector<int> MarkerBattleStageIds;
	struct SMapStageItem
	{
		int MapItemID;
		int ItemID;
		Location WorldPos;
		bool Active;

		SMapStageItem()
			: MapItemID(-1), ItemID(-1), WorldPos{ 0, 0 }, Active(false)
		{
		}

		SMapStageItem(int mapItemID, int itemID, int x, int y)
			: MapItemID(mapItemID), ItemID(itemID), WorldPos{ x, y }, Active(true)
		{
		}
	};

	std::vector<SMapStageItem> stageItems;
	URegionInfoPopup* infoPopup;
	UMainMenuPopup* mainMenu;
	ULocationIndicator* locationUI;
	UNoticePopup* noticePopup;
	UCanvasPanel screenUI;

	class UMouseMark* MouseMark;
	int NextStageID;
	int SelectedBattleStageId;
	int SelectedMarkerNextStageID;

	HBITMAP hBitWave;
	HBITMAP hBitMapOverview;
	bool hasLayeredBackground;
	BITMAP WaveInfo;
	float waveOffset;
	bool IsMapModalOpen() const;
	void RefreshPointer(HWND hWnd);
public:
	// Shared map camera policy for startup, region focus and popup dismissal.
	static constexpr float OverviewZoom = 1.3f;
	static constexpr float RegionFocusZoom = 1.6f;

	Map_Stage();
	virtual ~Map_Stage();
	virtual void Init(HWND hWnd, HINSTANCE const g_hInst);
	void SetupLandmarks(HINSTANCE instance);
	virtual void Update(HWND hWnd);
	virtual void Draw(HDC hMemDC, HDC hMemDC1);
	virtual void DrawCursor(HDC hMemDC, HDC hMemDC1) override;
	virtual int Input(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	virtual bool HandleSetCursor(HWND hWnd, WPARAM wParam, LPARAM lParam) override;

	virtual void Comp_NextStage(int ID);
	virtual void BackgroundMove(int x, int y) override;
};

