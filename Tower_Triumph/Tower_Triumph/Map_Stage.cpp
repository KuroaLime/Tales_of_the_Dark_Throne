#include "Map_Stage.h"
#include "resource.h"
#include "SoundManager.h"
#include "UMouseMark.h"
#include "GameManager.h"
#include "ItemData.h"
#include "BattleRewardManager.h"
#include <algorithm>
#include <cmath>

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
	hBitMapOverview = nullptr;
	hasLayeredBackground = false;
	ZeroMemory(&WaveInfo, sizeof(WaveInfo));
	waveOffset = 0.0f;
}

Map_Stage::~Map_Stage() {
	if (MouseMark && MouseMark->GetVisible()) SetCursor(LoadCursor(nullptr, IDC_ARROW));
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
	if (hBitBackground) DeleteObject(hBitBackground);
	if (hBitMapOverview) DeleteObject(hBitMapOverview);
}

void Map_Stage::Init(HWND hWnd, HINSTANCE const g_hInst) {

	GetClientRect(hWnd, &rects);

	SoundManager::PlayBGM(L"Resource\\Sound\\TowerTriumph_Stage_Map_Loop_42s.wav", true);
	
	// Keep the approved full illustration for existing popup/maintenance previews.
	// Only the interactive map uses the clean plate plus independent buildings.
	hBitMapOverview = LoadBitmap(g_hInst, MAKEINTRESOURCE(IDB_BITMAP3));
	hBitBackground = LoadBitmap(g_hInst, MAKEINTRESOURCE(IDB_MAP_CLEAN));
	if (!hBitBackground) hBitBackground = (HBITMAP)LoadImageW(
		NULL,
		L"Source\\SMap\\landmarks\\CleanMap.bmp",
		IMAGE_BITMAP,
		0,
		0,
		LR_LOADFROMFILE | LR_CREATEDIBSECTION
	);

	hasLayeredBackground = hBitBackground != nullptr;
	if (!hBitBackground && hBitMapOverview)
	{
		hBitBackground = static_cast<HBITMAP>(CopyImage(hBitMapOverview, IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION));
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

	Camera = new ACamera(BackgroundInfo.bmWidth / 2 - (rects.right - rects.left) / 2,
		BackgroundInfo.bmHeight / 2 - (rects.bottom - rects.top) / 2,
		(BackgroundInfo.bmWidth / 2) / TILE_SIZE, (BackgroundInfo.bmHeight / 2) / TILE_SIZE, OverviewZoom);

	Loc.first.x = rects.right / 2;
	Loc.first.y = rects.bottom / 2;
	Loc.second.sX = BackgroundInfo.bmWidth / 2;
	Loc.second.sY = BackgroundInfo.bmHeight / 2;

	W = BackgroundInfo.bmWidth / TILE_SIZE;
	H = BackgroundInfo.bmHeight / TILE_SIZE;
	Baord.resize(H, std::vector<Tile>(W));

	SetupLandmarks(g_hInst);

	infoPopup = new URegionInfoPopup();
	infoPopup->Setup(g_hInst, rects);
	infoPopup->SetOnEnterClick([this]() {
		this->Comp_NextStage(SelectedMarkerNextStageID);
	});
	infoPopup->SetOnCloseClick([this]() {
		for (auto m : Markers) m->Deactivate();
		Camera->SetZoom(OverviewZoom);
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
	MouseMark->SetVisible(false);
	RefreshPointer(hWnd);

	stageItems.clear();
	if (!GameManager::Get().IsMapItemCollected(1))
	{
		stageItems.push_back(SMapStageItem(1, 101, 600, 400));
	}
}

void Map_Stage::SetupLandmarks(HINSTANCE instance)
{
	for (auto marker : Markers) delete marker;
	Markers.clear();
	MarkerBattleStageIds.clear();

	URegionMarker* town = new URegionMarker();
	// Exact registered crop bounds from the approved transparent building layer.
	// These fixed rectangles are the sole input targets; floating markers are decoration.
	town->SetupLandmark({ 687, 282, 1013, 494 }, L"준비");
	town->SetRegionInfo(L"Town", L"Preparation Camp", L"Manage party, equipment, inventory, skills, and shop before entering a stage.", L"Always available");
	town->SetIsUnlocked(true); 
	town->LoadArtwork(instance, IDB_MAP_CAMP, IDB_MAP_MARKER_OPEN, IDB_MAP_MARKER_CLOSED, 0.00);
	Markers.push_back(town);
	MarkerBattleStageIds.push_back(0);

	URegionMarker* m1 = new URegionMarker();
	m1->SetupLandmark({ 1141, 412, 1474, 621 }, L"01");
	m1->SetRegionInfo(L"Stage 01", L"묘지", L"현재 기준이 되는 묘지 전투 스테이지입니다.", L"Graveyard");
	m1->SetIsUnlocked(GameManager::Get().IsBattleStageUnlocked(1));
	m1->LoadArtwork(instance, IDB_MAP_GRAVEYARD, IDB_MAP_MARKER_OPEN, IDB_MAP_MARKER_CLOSED, 0.37);
	Markers.push_back(m1);
	MarkerBattleStageIds.push_back(1);

	URegionMarker* m2 = new URegionMarker();
	m2->SetupLandmark({ 352, 148, 513, 317 }, L"02");
	m2->SetRegionInfo(L"Stage 02", L"천상 성전", L"공중에 떠 있는 발판과 빈 공간이 있는 스테이지입니다.", L"Heaven");
	m2->SetIsUnlocked(GameManager::Get().IsBattleStageUnlocked(2));
	m2->LoadArtwork(instance, IDB_MAP_HEAVEN, IDB_MAP_MARKER_OPEN, IDB_MAP_MARKER_CLOSED, 0.74);
	Markers.push_back(m2);
	MarkerBattleStageIds.push_back(2);

	URegionMarker* m3 = new URegionMarker();
	m3->SetupLandmark({ 241, 385, 494, 600 }, L"03");
	m3->SetRegionInfo(L"Stage 03", L"기본 방", L"기믹 없는 기본 전투 확인용 스테이지입니다.", L"Default");
	m3->SetIsUnlocked(GameManager::Get().IsBattleStageUnlocked(3));
	m3->LoadArtwork(instance, IDB_MAP_HALL, IDB_MAP_MARKER_OPEN, IDB_MAP_MARKER_CLOSED, 1.11);
	Markers.push_back(m3);
	MarkerBattleStageIds.push_back(3);

	URegionMarker* m4 = new URegionMarker();
	m4->SetupLandmark({ 432, 623, 820, 815 }, L"04");
	m4->SetRegionInfo(L"Stage 04", L"바다 폐허", L"물 타일이 이동을 제한하는 해안 전투 스테이지입니다.", L"Sea");
	m4->SetIsUnlocked(GameManager::Get().IsBattleStageUnlocked(4));
	m4->LoadArtwork(instance, IDB_MAP_SEA, IDB_MAP_MARKER_OPEN, IDB_MAP_MARKER_CLOSED, 1.48);
	Markers.push_back(m4);
	MarkerBattleStageIds.push_back(4);

	URegionMarker* m5 = new URegionMarker();
	m5->SetupLandmark({ 1226, 201, 1400, 302 }, L"05");
	m5->SetRegionInfo(L"Stage 05", L"용암지대", L"용암 타일이 피해를 주는 화산 전투 스테이지입니다.", L"Lava");
	m5->SetIsUnlocked(GameManager::Get().IsBattleStageUnlocked(5));
	m5->LoadArtwork(instance, IDB_MAP_LAVA, IDB_MAP_MARKER_OPEN, IDB_MAP_MARKER_CLOSED, 1.85);
	Markers.push_back(m5);
	MarkerBattleStageIds.push_back(5);

}

void Map_Stage::Update(HWND hWnd) {
	if (!IsMapModalOpen() && GetForegroundWindow() == hWnd)
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

		POINT screenPos = mousePos;
		ClientToScreen(hWnd, &screenPos);
		if (PtInRect(&clientRect, mousePos) && WindowFromPoint(screenPos) == hWnd)
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

	screenUI.Update();
	if (noticePopup) noticePopup->Update();
	Camera->Update(W, H, TILE_SIZE, rects.right - rects.left, rects.bottom - rects.top);
	RefreshPointer(hWnd);
	for (size_t i = 0; i < Markers.size(); ++i) {
		const int stage = i < MarkerBattleStageIds.size() ? MarkerBattleStageIds[i] : 0;
		Markers[i]->SetIsUnlocked(stage <= 0 || GameManager::Get().IsBattleStageUnlocked(stage));
		Markers[i]->Update();
	}
	if (MouseMark) MouseMark->Update();
}

void Map_Stage::Draw(HDC hMemDC, HDC hMemDC1) {
	const int viewportWidth = rects.right - rects.left;
	const int viewportHeight = rects.bottom - rects.top;
	if (!Camera || !hMemDC || !hMemDC1 || viewportWidth <= 0 || viewportHeight <= 0) return;
	std::pair<float, float> CameraOffset = Camera->Getg_Cam();
	float zoom = Camera->GetZoom();
	int camX = (int)CameraOffset.first;
	int camY = (int)CameraOffset.second;
	const int mapSaved = SaveDC(hMemDC);
	IntersectClipRect(hMemDC, rects.left, rects.top, rects.right, rects.bottom);

	if (hBitWave)
	{
		HBITMAP hOldWave = (HBITMAP)SelectObject(hMemDC1, hBitWave);
		int waveW = WaveInfo.bmWidth;
		int waveH = WaveInfo.bmHeight;
		for (int y = 0; y < viewportHeight; y += waveH)
		{
			for (int x = (int)waveOffset; x < viewportWidth; x += waveW)
				BitBlt(hMemDC, x, y, waveW, waveH, hMemDC1, 0, 0, SRCCOPY);
			for (int x = (int)waveOffset - waveW; x < viewportWidth; x += waveW)
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

	if (hBitBackground && zoom > 0.0f) {
		HBITMAP hOld = (HBITMAP)SelectObject(hMemDC1, hBitBackground);
		SetStretchBltMode(hMemDC, COLORONCOLOR);
		// Match the building transform exactly; don't quantize the source view size.
		const int left = static_cast<int>(std::lround(-camX * zoom));
		const int top = static_cast<int>(std::lround(-camY * zoom));
		const int right = static_cast<int>(std::lround((BackgroundInfo.bmWidth - camX) * zoom));
		const int bottom = static_cast<int>(std::lround((BackgroundInfo.bmHeight - camY) * zoom));
		StretchBlt(hMemDC, left, top, right - left, bottom - top, hMemDC1,
			0, 0, BackgroundInfo.bmWidth, BackgroundInfo.bmHeight, SRCCOPY);
		SelectObject(hMemDC1, hOld);
	}

	if (hasLayeredBackground)
		for (auto m : Markers) m->DrawWithOffset(hMemDC, hMemDC1, camX, camY, zoom);

	for (const auto& item : stageItems) {
		if (!item.Active) continue;
		int drawX = (int)((item.WorldPos.x - camX) * zoom);
		int drawY = (int)((item.WorldPos.y - camY) * zoom);
		if (drawX > -50 && drawX < viewportWidth + 50 && drawY > -50 && drawY < viewportHeight + 50) {
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

	RestoreDC(hMemDC, mapSaved);
	screenUI.Draw(hMemDC, hMemDC1);
	if (noticePopup) noticePopup->Draw(hMemDC, hMemDC1);

}

void Map_Stage::DrawCursor(HDC hMemDC, HDC hMemDC1)
{
	if (MouseMark) MouseMark->Draw(hMemDC, hMemDC1);
}

void Map_Stage::BackgroundMove(int x, int y) {}

bool Map_Stage::IsMapModalOpen() const
{
	return (mainMenu && mainMenu->GetVisible()) || (infoPopup && infoPopup->IsPresenting()) ||
		(noticePopup && noticePopup->IsOpened());
}

void Map_Stage::RefreshPointer(HWND hWnd)
{
	if (!MouseMark || !Camera) return;
	POINT screenPos{};
	const bool hasPointer = GetCursorPos(&screenPos) != FALSE;
	POINT clientPos = screenPos;
	ScreenToClient(hWnd, &clientPos);
	RECT clientRect{};
	GetClientRect(hWnd, &clientRect);
	const bool inside = hasPointer && GetForegroundWindow() == hWnd &&
		WindowFromPoint(screenPos) == hWnd && PtInRect(&clientRect, clientPos);
	const bool mapActive = inside && !IsMapModalOpen();
	MouseMark->SetMousePos(clientPos);
	// The approved popup preview uses the same fixed-size quill. Modal state
	// disables map hover/clicks, while the cursor stays above the popup itself.
	MouseMark->SetVisible(inside && MouseMark->IsDrawable());
	if (inside) SetCursor(MouseMark->GetVisible() ? nullptr : LoadCursor(nullptr, IDC_ARROW));

	// Re-evaluate against this frame's camera even while the mouse is stationary.
	const auto offset = Camera->Getg_Cam();
	const int camX = static_cast<int>(offset.first);
	const int camY = static_cast<int>(offset.second);
	const float zoom = Camera->GetZoom();
	URegionMarker* closest = nullptr;
	float nearest = 275.0f / zoom;
	for (auto marker : Markers)
	{
		marker->SetPresentationActive(mapActive);
		marker->SetIsPointed(mapActive && marker->ContainsPointWithOffset(
			clientPos.x, clientPos.y, camX, camY, zoom));
		if (!mapActive) continue;
		const float dx = marker->GetLocation().x - (camX + clientPos.x / zoom);
		const float dy = marker->GetLocation().y - (camY + clientPos.y / zoom);
		const float distance = sqrtf(dx * dx + dy * dy);
		if (marker->GetIsPointed() || distance < nearest)
		{
			closest = marker;
			nearest = marker->GetIsPointed() ? 0.0f : distance;
		}
	}
	if (locationUI)
	{
		if (closest) locationUI->SetInfo(closest->GetParentName(), closest->GetName());
		locationUI->SetVisible(closest != nullptr);
	}
}

bool Map_Stage::HandleSetCursor(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
	if (LOWORD(lParam) != HTCLIENT || reinterpret_cast<HWND>(wParam) != hWnd) return false;
	RefreshPointer(hWnd);
	return MouseMark && MouseMark->GetVisible();
}

int Map_Stage::Input(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	static int CLK_X, CLK_Y;
	if (uMsg == WM_KEYDOWN && (lParam & (1LL << 30))) return -2;
	std::pair<float, float> CameraOffset = Camera->Getg_Cam();
	float zoom = Camera->GetZoom();
	int camX = (int)CameraOffset.first;
	int camY = (int)CameraOffset.second;
	if (uMsg == WM_KEYDOWN || uMsg == WM_CANCELMODE || uMsg == WM_CAPTURECHANGED || uMsg == WM_KILLFOCUS ||
		uMsg == WM_LBUTTONDOWN || uMsg == WM_RBUTTONDOWN)
	{
		screenUI.CancelPress();
		if (noticePopup) noticePopup->CancelPress();
	}
	if (hWnd && uMsg == WM_LBUTTONDOWN) SetCapture(hWnd);
	if (hWnd && uMsg == WM_KILLFOCUS && GetCapture() == hWnd) ReleaseCapture();
	if (uMsg == WM_KILLFOCUS || uMsg == WM_CANCELMODE) {
		for (auto marker : Markers) marker->SetPresentationActive(false);
		if (MouseMark) MouseMark->SetVisible(false);
	}
	// A visually closing dialog stays modal, even though GetVisible() is already
	// false. Also reject a second ENTER while Framework consumes the first route.
	if (infoPopup && infoPopup->IsInputBlocked())
	{
		screenUI.CancelPress();
		if (hWnd && GetCapture() == hWnd) ReleaseCapture();
		if (uMsg == WM_MOUSEMOVE) RefreshPointer(hWnd);
		return -2;
	}

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
			{
				UButton* btn = dynamic_cast<UButton*>(noticePopup->FindWidgetUnderMouse(CLK_X, CLK_Y));
				const bool activate = btn && btn->GetIsPressed();
				noticePopup->CancelPress();
				if (hWnd && GetCapture() == hWnd) ReleaseCapture();
				if (activate) btn->Click();
			}
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
		screenUI.UpdateMouseHover(CLK_X, CLK_Y);
		RefreshPointer(hWnd);
		break;

	case WM_LBUTTONDOWN:
	case WM_RBUTTONDOWN:
		CLK_X = LOWORD(lParam); CLK_Y = HIWORD(lParam);
		if (UWidget* w = screenUI.FindWidgetUnderMouse(CLK_X, CLK_Y)) {
			if (UButton* btn = dynamic_cast<UButton*>(w)) btn->SetIsPressed(true);
			return -2;
		}
		if ((mainMenu && mainMenu->GetVisible()) || (infoPopup && infoPopup->IsPresenting())) return -2;
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
					for (auto marker : Markers) marker->SetPresentationActive(false);
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
					for (auto marker : Markers) marker->SetPresentationActive(false);
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
				Camera->SetZoom(RegionFocusZoom);
				for (auto other : Markers) other->Deactivate();
				m->Activate();
				infoPopup->SetMapPreview(hBitMapOverview, m->GetWorldBounds(), m->GetParentName());
				infoPopup->SetInfo(m->GetName(), m->GetDesc(), m->GetEnv());
				infoPopup->SetVisible(true);
				for (auto marker : Markers) marker->SetPresentationActive(false);
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
		{
			UButton* btn = dynamic_cast<UButton*>(screenUI.FindWidgetUnderMouse(CLK_X, CLK_Y));
			const bool activate = btn && btn->GetIsPressed();
			screenUI.CancelPress();
			if (hWnd && GetCapture() == hWnd) ReleaseCapture();
			if (activate) {
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
		// Do not stack a second popup over the region or confirmation currently
		// being reviewed. Escape always returns through the current modal first.
		if (wParam != VK_ESCAPE && IsMapModalOpen()) return -2;
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
				Camera->SetZoom(OverviewZoom);
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
	if (IsMapModalOpen())
		for (auto marker : Markers) marker->SetPresentationActive(false);
	return -2;
}

void Map_Stage::Comp_NextStage(int ID) {
	if (ID != MAP_STAGE) NextStageID = ID;
}
