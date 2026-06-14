#include "Map_Stage.h"
#include "resource.h"
#include "SoundManager.h"
#include "UMouseMark.h"
#include "GameManager.h"
#include "ItemData.h"
#include "BattleRewardManager.h"

Map_Stage::Map_Stage() {
	Camera = nullptr;
	infoPopup = nullptr;
	mainMenu = nullptr;
	locationUI = nullptr;
	noticePopup = nullptr;
	MouseMark = nullptr;
	CurStageID = MAP_STAGE;
	NextStageID = -2;
	SelectedBattleStageId = 1;
	SelectedMarkerNextStageID = BATTLE_STAGE;

	hBitWave = nullptr;
	ZeroMemory(&WaveInfo, sizeof(WaveInfo));
	waveOffset = 0.0f;
}

Map_Stage::~Map_Stage() {
	if (Camera) delete Camera;
	for (auto m : Markers) delete m;
	if (infoPopup) delete infoPopup;
	if (mainMenu) delete mainMenu;
	if (locationUI) delete locationUI;
	if (noticePopup) delete noticePopup;
	if (MouseMark) delete MouseMark;

	if (hBitWave) {
		DeleteObject(hBitWave);
		hBitWave = nullptr;
	}
}

void Map_Stage::Init(HWND hWnd, HINSTANCE const g_hInst) {

	GetClientRect(hWnd, &rects);

	SoundManager::PlayBGM(L"Resource\\Sound\\TowerTriumph_Stage_Map_Loop_42s.wav", true);
	
	hBitBackground = (HBITMAP)LoadImageW(
		NULL,
		L"Source\\SMap\\background\\Map_Background.bmp",
		IMAGE_BITMAP,
		0,
		0,
		LR_LOADFROMFILE | LR_CREATEDIBSECTION
	);

	if (!hBitBackground)
	{
		hBitBackground = LoadBitmap(g_hInst, MAKEINTRESOURCE(IDB_BITMAP3));
	}

	if (hBitBackground)
	{
		GetObject(hBitBackground, sizeof(BITMAP), &BackgroundInfo);
	}
	else
	{
		BackgroundInfo.bmWidth = 1280;
		BackgroundInfo.bmHeight = 720;
	}
	
	hBitWave = (HBITMAP)LoadImageW(
		NULL,
		L"Source\\SMap\\background\\Wave.bmp",
		IMAGE_BITMAP,
		0,
		0,
		LR_LOADFROMFILE | LR_CREATEDIBSECTION
	);

	if (hBitWave)
	{
		GetObject(hBitWave, sizeof(BITMAP), &WaveInfo);
	}

	TILE_SIZE = 64;

	Camera = new ACamera((float)(BackgroundInfo.bmWidth / 2 - 1280 / 2),
		(float)(BackgroundInfo.bmHeight / 2 - 720 / 2),
		(BackgroundInfo.bmWidth / 2) / TILE_SIZE, (BackgroundInfo.bmHeight / 2) / TILE_SIZE);

	Loc.first.x = rects.right / 2;
	Loc.first.y = rects.bottom / 2;
	Loc.second.sX = BackgroundInfo.bmWidth / 2;
	Loc.second.sY = BackgroundInfo.bmHeight / 2;

	W = BackgroundInfo.bmWidth / TILE_SIZE;
	H = BackgroundInfo.bmHeight / TILE_SIZE;
	Baord.resize(H, std::vector<Tile>(W));

	Markers.clear();
	MarkerBattleStageIds.clear();

	URegionMarker* town = new URegionMarker();
	town->Setup(g_hInst, BackgroundInfo.bmWidth / 2+10, BackgroundInfo.bmHeight / 2-40, L"Source\\SMap\\background\\TOWN_Marker.bmp");
	town->SetRegionInfo(L"Town", L"Preparation Camp", L"Manage party, equipment, inventory, skills, and shop before entering a stage.", L"Always available");
	town->SetIsUnlocked(true); 
	Markers.push_back(town);
	MarkerBattleStageIds.push_back(0);

	URegionMarker* m1 = new URegionMarker();
	m1->Setup(g_hInst, 1250, 540, L"Source\\SMap\\background\\MyoG_Marker.bmp");
	m1->SetRegionInfo(L"Stage 01", L"묘지", L"현재 기준이 되는 묘지 전투 스테이지입니다.", L"Graveyard");
	m1->SetIsUnlocked(GameManager::Get().IsBattleStageUnlocked(1));
	Markers.push_back(m1);
	MarkerBattleStageIds.push_back(1);

	URegionMarker* m2 = new URegionMarker();
	m2->Setup(g_hInst, 440, 280, L"Source\\SMap\\background\\SkySum_Marker.bmp");
	m2->SetRegionInfo(L"Stage 02", L"천상 성전", L"공중에 떠 있는 발판과 빈 공간이 있는 스테이지입니다.", L"Heaven");
	m2->SetIsUnlocked(GameManager::Get().IsBattleStageUnlocked(2));
	Markers.push_back(m2);
	MarkerBattleStageIds.push_back(2);

	URegionMarker* m3 = new URegionMarker();
	m3->Setup(g_hInst, 440, 540, L"Source\\SMap\\background\\GisaSung_Marker.bmp");
	m3->SetRegionInfo(L"Stage 03", L"기본 방", L"기믹 없는 기본 전투 확인용 스테이지입니다.", L"Default");
	m3->SetIsUnlocked(GameManager::Get().IsBattleStageUnlocked(3));
	Markers.push_back(m3);
	MarkerBattleStageIds.push_back(3);

	URegionMarker* m4 = new URegionMarker();
	m4->Setup(g_hInst, 670, 740, L"Source\\SMap\\background\\Ghost_Marker.bmp");
	m4->SetRegionInfo(L"Stage 04", L"바다 폐허", L"물 타일이 이동을 제한하는 해안 전투 스테이지입니다.", L"Sea");
	m4->SetIsUnlocked(GameManager::Get().IsBattleStageUnlocked(4));
	Markers.push_back(m4);
	MarkerBattleStageIds.push_back(4);

	URegionMarker* m5 = new URegionMarker();
	m5->Setup(g_hInst, 1250, 280, L"Source\\SMap\\background\\LavaMaul_Marker.bmp");
	m5->SetRegionInfo(L"Stage 05", L"용암지대", L"용암 타일이 피해를 주는 화산 전투 스테이지입니다.", L"Lava");
	m5->SetIsUnlocked(GameManager::Get().IsBattleStageUnlocked(5));
	Markers.push_back(m5);
	MarkerBattleStageIds.push_back(5);

	infoPopup = new URegionInfoPopup();
	infoPopup->Setup(g_hInst, rects);
	infoPopup->SetOnEnterClick([this]() {
		this->Comp_NextStage(SelectedMarkerNextStageID);
	});
	infoPopup->SetOnCloseClick([this]() {
		for (auto m : Markers) m->Deactivate();
		Camera->SetZoom(1.5f);
	});

	mainMenu = new UMainMenuPopup();
	mainMenu->Setup(g_hInst, rects);

	locationUI = new ULocationIndicator();
	locationUI->Setup(rects);

	noticePopup = new UNoticePopup();
	noticePopup->Setup(rects);

	GameManager::Get().InitDefaultParty();
	mainMenu->SetTeamData(GameManager::Get().GetParty());

	screenUI.AddChildWindow(infoPopup);
	screenUI.AddChildWindow(mainMenu);
	screenUI.AddChildWindow(locationUI);

	MouseMark = new UMouseMark();
	MouseMark->SetFrame(g_hInst, { IDB_UI_MOUSECUSOR }); 

	stageItems.clear();
	if (!GameManager::Get().IsMapItemCollected(1))
	{
		stageItems.push_back(SMapStageItem(1, 101, 600, 400));
	}
}

void Map_Stage::Update(HWND hWnd) {
	std::pair<float, float> CameraOffset = Camera->Getg_Cam();
	float zoom = Camera->GetZoom();
	int camX = (int)CameraOffset.first;
	int camY = (int)CameraOffset.second;

	{
		std::pair<float, float> curGrid = Camera->GetPointGrid();
		float scrollSpeed = 0.5f; 
		
		if (GetAsyncKeyState(VK_LEFT) & 0x8000) curGrid.first -= scrollSpeed;
		if (GetAsyncKeyState(VK_RIGHT) & 0x8000) curGrid.first += scrollSpeed;
		if (GetAsyncKeyState(VK_UP) & 0x8000) curGrid.second -= scrollSpeed;
		if (GetAsyncKeyState(VK_DOWN) & 0x8000) curGrid.second += scrollSpeed;

		POINT mousePos;
		GetCursorPos(&mousePos);
		ScreenToClient(hWnd, &mousePos);

		RECT clientRect;
		GetClientRect(hWnd, &clientRect);
		int edgeMargin = 30;

		if (mousePos.x >= 0 && mousePos.x <= clientRect.right && mousePos.y >= 0 && mousePos.y <= clientRect.bottom)
		{
			if (mousePos.x < edgeMargin) curGrid.first -= scrollSpeed;
			if (mousePos.x > clientRect.right - edgeMargin) curGrid.first += scrollSpeed;
			if (mousePos.y < edgeMargin) curGrid.second -= scrollSpeed;
			if (mousePos.y > clientRect.bottom - edgeMargin) curGrid.second += scrollSpeed;
		}

		float maxGridX = (float)BackgroundInfo.bmWidth / TILE_SIZE;
		float maxGridY = (float)BackgroundInfo.bmHeight / TILE_SIZE;

		if (curGrid.first < 0) curGrid.first = 0;
		if (curGrid.first > maxGridX) curGrid.first = maxGridX;
		if (curGrid.second < 0) curGrid.second = 0;
		if (curGrid.second > maxGridY) curGrid.second = maxGridY;

		Camera->SetPointGrid(curGrid.first, curGrid.second);
	}

	if (hBitWave)
	{
		waveOffset -= 1.0f;
		if (waveOffset <= -WaveInfo.bmWidth) waveOffset += WaveInfo.bmWidth;
	}

	for (auto m : Markers) m->Update();
	screenUI.Update();
	if (noticePopup) noticePopup->Update();
	Camera->Update(W, H, TILE_SIZE, 1280, 720);

	if (MouseMark) {
		bool bMenuOpen = (mainMenu && mainMenu->GetVisible()) || (infoPopup && infoPopup->GetVisible());
		if (bMenuOpen) MouseMark->SetVisible(false);
		else MouseMark->SetVisible(true);
		
		MouseMark->SetCameraInfo(camX, camY, zoom);
		MouseMark->Update();
	}
}

void Map_Stage::Draw(HDC hMemDC, HDC hMemDC1) {
	std::pair<float, float> CameraOffset = Camera->Getg_Cam();
	float zoom = Camera->GetZoom();
	int camX = (int)CameraOffset.first;
	int camY = (int)CameraOffset.second;

	if (hBitWave)
	{
		HBITMAP hOldWave = (HBITMAP)SelectObject(hMemDC1, hBitWave);
		int waveW = WaveInfo.bmWidth;
		int waveH = WaveInfo.bmHeight;
		for (int y = 0; y < 720; y += waveH)
		{
			for (int x = (int)waveOffset; x < 1280; x += waveW)
				BitBlt(hMemDC, x, y, waveW, waveH, hMemDC1, 0, 0, SRCCOPY);
			for (int x = (int)waveOffset - waveW; x < 1280; x += waveW)
				if (x + waveW > 0) BitBlt(hMemDC, x, y, waveW, waveH, hMemDC1, 0, 0, SRCCOPY);
		}
		SelectObject(hMemDC1, hOldWave);
	}
	else
	{
		HBRUSH hBrush = CreateSolidBrush(RGB(0, 100, 200));
		FillRect(hMemDC, &rects, hBrush);
		DeleteObject(hBrush);
	}

	HBITMAP hOld = (HBITMAP)SelectObject(hMemDC1, hBitBackground);
	int viewW = (int)(1280 / zoom);
	int viewH = (int)(720 / zoom);
	StretchBlt(hMemDC, 0, 0, 1280, 720, hMemDC1, camX, camY, viewW, viewH, SRCCOPY);
	SelectObject(hMemDC1, hOld);

	for (auto m : Markers) m->DrawWithOffset(hMemDC, hMemDC1, camX, camY, zoom);

	for (const auto& item : stageItems) {
		if (!item.Active) continue;
		int drawX = (int)((item.WorldPos.x - camX) * zoom);
		int drawY = (int)((item.WorldPos.y - camY) * zoom);
		if (drawX > -50 && drawX < 1330 && drawY > -50 && drawY < 770) {
			int circleSize = (int)(15 * zoom);
			HBRUSH hBrush = CreateSolidBrush(RGB(0, 255, 0));
			HBRUSH oldBrush = (HBRUSH)SelectObject(hMemDC, hBrush);
			Ellipse(hMemDC, drawX - circleSize, drawY - circleSize, drawX + circleSize, drawY + circleSize);
			SelectObject(hMemDC, oldBrush);
			DeleteObject(hBrush);
			std::wstring itemName = ItemData::GetName(item.ItemID);
			SetBkMode(hMemDC, TRANSPARENT);
			SetTextColor(hMemDC, RGB(255, 255, 255));
			TextOutW(hMemDC, drawX - (int)(30 * zoom), drawY + (int)(20 * zoom), itemName.c_str(), (int)itemName.length());
		}
	}

	screenUI.Draw(hMemDC, hMemDC1);
	if (MouseMark) MouseMark->Draw(hMemDC, hMemDC1);
	if (noticePopup) noticePopup->Draw(hMemDC, hMemDC1);
}

void Map_Stage::BackgroundMove(int x, int y) {}

int Map_Stage::Input(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	static int CLK_X, CLK_Y;
	std::pair<float, float> CameraOffset = Camera->Getg_Cam();
	float zoom = Camera->GetZoom();
	int camX = (int)CameraOffset.first;
	int camY = (int)CameraOffset.second;

	if (noticePopup && noticePopup->IsOpened())
	{
		switch (uMsg)
		{
		case WM_MOUSEMOVE:
			CLK_X = LOWORD(lParam); CLK_Y = HIWORD(lParam);
			noticePopup->UpdateMouseHover(CLK_X, CLK_Y);
			break;
		case WM_LBUTTONDOWN:
			CLK_X = LOWORD(lParam); CLK_Y = HIWORD(lParam);
			if (UWidget* w = noticePopup->FindWidgetUnderMouse(CLK_X, CLK_Y))
				if (UButton* btn = dynamic_cast<UButton*>(w)) btn->SetIsPressed(true);
			break;
		case WM_LBUTTONUP:
			CLK_X = LOWORD(lParam); CLK_Y = HIWORD(lParam);
			if (UWidget* w = noticePopup->FindWidgetUnderMouse(CLK_X, CLK_Y))
				if (UButton* btn = dynamic_cast<UButton*>(w)) btn->Click();
			break;
		case WM_KEYDOWN:
			if (wParam == VK_ESCAPE || wParam == VK_RETURN || wParam == VK_SPACE) noticePopup->CloseNotice();
			break;
		}
		return -2;
	}

	switch (uMsg) {
	case WM_MOUSEMOVE:
		CLK_X = LOWORD(lParam); CLK_Y = HIWORD(lParam);
		if (MouseMark) MouseMark->SetMousePos({ CLK_X, CLK_Y });
		screenUI.UpdateMouseHover(CLK_X, CLK_Y);
		{
			URegionMarker* hovered = nullptr;
			bool bMenuOpen = (mainMenu && mainMenu->GetVisible()) || (infoPopup && infoPopup->GetVisible());
			for (auto m : Markers) {
				bool pointed = !bMenuOpen && m->ContainsPointWithOffset(CLK_X, CLK_Y, camX, camY, zoom);
				m->SetIsPointed(pointed);
				if (pointed) hovered = m;
			}
			if (MouseMark) MouseMark->SetTarget(hovered);
		}
		{
			bool foundNear = false;
			float minDesc = 100000.0f;
			URegionMarker* closestMarker = nullptr;
			bool bMenuOpen = (mainMenu && mainMenu->GetVisible()) || (infoPopup && infoPopup->GetVisible());
			if (!bMenuOpen) {
				for (auto m : Markers) {
					int dx = (int)(m->GetLocation().x - (camX + CLK_X / zoom));
					int dy = (int)(m->GetLocation().y - (camY + CLK_Y / zoom));
					float dist = sqrtf(static_cast<float>(dx * dx + dy * dy));
					if (dist < 275.0f / zoom) {
						if (dist < minDesc) { minDesc = dist; closestMarker = m; }
						foundNear = true;
					}
				}
			}
			if (foundNear && closestMarker) {
				locationUI->SetInfo(closestMarker->GetParentName(), closestMarker->GetName());
				locationUI->SetVisible(true);
			}
			else locationUI->SetVisible(false);
		}
		break;

	case WM_LBUTTONDOWN:
	case WM_RBUTTONDOWN:
		CLK_X = LOWORD(lParam); CLK_Y = HIWORD(lParam);
		if (UWidget* w = screenUI.FindWidgetUnderMouse(CLK_X, CLK_Y)) {
			if (uMsg == WM_LBUTTONDOWN)
				if (UButton* btn = dynamic_cast<UButton*>(w)) btn->SetIsPressed(true);
			return -2;
		}
		if ((mainMenu && mainMenu->GetVisible()) || (infoPopup && infoPopup->GetVisible())) return -2;
		if (uMsg == WM_RBUTTONDOWN) return -2;
		{
			auto it = stageItems.begin();
			while (it != stageItems.end()) {
				Location pos = it->WorldPos;
				int dx = (int)(camX + CLK_X / zoom - pos.x);
				int dy = (int)(camY + CLK_Y / zoom - pos.y);
				if (it->Active && dx * dx + dy * dy <= 30 * 30) {
					int mapItemID = it->MapItemID;
					int itemID = it->ItemID;
					GameManager::Get().AddItem(itemID, 1);
					GameManager::Get().MarkMapItemCollected(mapItemID);
					it = stageItems.erase(it);
					if (noticePopup) noticePopup->ShowNotice(L"획득", ItemData::GetName(itemID) + L"을(를) 획득했습니다.");
					if (mainMenu) mainMenu->SetTeamData(GameManager::Get().GetParty());
					return -2;
				}
				++it;
			}
		}
		for (size_t i = 0; i < Markers.size(); ++i) {
			URegionMarker* m = Markers[i];
			if (m->ContainsPointWithOffset(CLK_X, CLK_Y, camX, camY, zoom)) {
				int stageId = (i < MarkerBattleStageIds.size()) ? MarkerBattleStageIds[i] : 0;
				if (stageId > 0 && !GameManager::Get().IsBattleStageUnlocked(stageId)) {
					if (noticePopup) noticePopup->ShowNotice(L"잠김", L"이전 스테이지를 먼저 클리어하세요.");
					return -2;
				}
				if (stageId <= 0) {
					BattleRewardManager::Get().Clear();
					SelectedMarkerNextStageID = MAINTENANCE_STAGE;
				}
				else {
					SelectedBattleStageId = stageId;
					SelectedMarkerNextStageID = BATTLE_STAGE;
					GameManager::Get().SetCurrentBattleStageId(stageId);
				}
				int gridX = (int)(camX + CLK_X / zoom) / TILE_SIZE;
				int gridY = (int)(camY + CLK_Y / zoom) / TILE_SIZE;
				Camera->SetPointGrid((float)gridX, (float)gridY);
				Camera->SetZoom(1.8f);
				for (auto other : Markers) other->Deactivate();
				m->Activate();
				infoPopup->SetInfo(m->GetName(), m->GetDesc(), m->GetEnv());
				infoPopup->SetVisible(true);
				return -2;
			}
		}
		{
			int gridX = (int)(camX + CLK_X / zoom) / TILE_SIZE;
			int gridY = (int)(camY + CLK_Y / zoom) / TILE_SIZE;
			Camera->SetPointGrid((float)gridX, (float)gridY);
		}
		break;

	case WM_LBUTTONUP:
	case WM_RBUTTONUP:
		CLK_X = LOWORD(lParam); CLK_Y = HIWORD(lParam);
		if (UWidget* w = screenUI.FindWidgetUnderMouse(CLK_X, CLK_Y)) {
			if (UButton* btn = dynamic_cast<UButton*>(w)) {
				if (uMsg == WM_LBUTTONUP) btn->Click();
				else if (uMsg == WM_RBUTTONUP) btn->RightClick();
			}
		}
		if (NextStageID != -2) {
			int ret = NextStageID;
			NextStageID = -2;
			return ret;
		}
		break;

	case WM_KEYDOWN:
		if (wParam == 'i' || wParam == 'I') {
			if (mainMenu) {
				mainMenu->SetTeamData(GameManager::Get().GetParty());
				mainMenu->SetVisible(true);
				mainMenu->SelectTab(EMenuTab::Inventory);
				mainMenu->OpenInventoryPopup();
			}
		}
		else if (wParam == 'p' || wParam == 'P') {
			if (mainMenu) {
				mainMenu->SetTeamData(GameManager::Get().GetParty());
				mainMenu->SetVisible(true);
				mainMenu->SelectTab(EMenuTab::CharacterInfo);
				mainMenu->OpenCharacterPopup();
			}
		}
		else if (wParam == VK_ESCAPE) {
			if (infoPopup && infoPopup->GetVisible()) {
				infoPopup->SetVisible(false);
				for (auto m : Markers) m->Deactivate();
				Camera->SetZoom(1.5f);
			}
			else if (mainMenu) {
				if (mainMenu->GetVisible()) {
					if (!mainMenu->HandleBack()) mainMenu->SetVisible(false);
				}
				else {
					mainMenu->SetTeamData(GameManager::Get().GetParty());
					mainMenu->ShowNavigation();
				}
			}
		}
		break;
	}
	return -2;
}

void Map_Stage::Comp_NextStage(int ID) {
	if (ID != MAP_STAGE) NextStageID = ID;
}
