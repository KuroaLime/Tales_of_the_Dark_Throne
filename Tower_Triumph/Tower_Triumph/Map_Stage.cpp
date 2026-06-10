#include "Map_Stage.h"
#include "resource.h"
#include "UMouseMark.h"
#include "AUsable.h"
#include "AArmor.h"

Map_Stage::Map_Stage() {
	Camera = nullptr;
	infoPopup = nullptr;
	mainMenu = nullptr;
	locationUI = nullptr;
	MouseMark = nullptr;
	CurStageID = MAP_STAGE;
	NextStageID = -2;
}

Map_Stage::~Map_Stage() {
	if (Camera) delete Camera;
	for (auto m : Markers) delete m;
	if (infoPopup) delete infoPopup;
	if (mainMenu) delete mainMenu;
	if (locationUI) delete locationUI;
	for (auto item : stageItems) delete item;

	if (MouseMark) delete MouseMark;
}

void Map_Stage::Init(HWND hWnd, HINSTANCE const g_hInst) {

	GetClientRect(hWnd, &rects);
	
	hBitBackground = LoadBitmap(g_hInst, MAKEINTRESOURCE(IDB_BITMAP3));
	GetObject(hBitBackground, sizeof(BITMAP), &BackgroundInfo);
	

	TILE_SIZE = 64;

	Camera = new ACamera((float)(((BackgroundInfo.bmWidth) / 2) / 64 * TILE_SIZE - 1280 / 2 + TILE_SIZE / 2),
		(float)(((BackgroundInfo.bmHeight) / 2) / 64 * TILE_SIZE - 720 / 2 + TILE_SIZE / 2),
		((BackgroundInfo.bmWidth) / 2) / 64, ((BackgroundInfo.bmHeight) / 2)/64);

	Loc.first.x = rects.right / 2;
	Loc.first.y = rects.bottom / 2;
	Loc.second.sX = BackgroundInfo.bmWidth / 2;
	Loc.second.sY = BackgroundInfo.bmHeight / 2;

	// 타일 정보 설정
	W = BackgroundInfo.bmWidth / TILE_SIZE;
	H = BackgroundInfo.bmHeight / TILE_SIZE;
	Baord.resize(H, std::vector<Tile>(W));

	//클릭 가능한 지역 마커 및 정보 설정
	URegionMarker* m1 = new URegionMarker();
	m1->Setup(g_hInst, 400, 300, IDB_BITMAP3, IDB_BITMAP3, IDB_BITMAP3);
	m1->SetRegionInfo(L"화이트홀름성", L"기사 훈련소", L"신참 기사들이 무술을 연마하는 장소입니다.", L"맑음 / 평지");
	Markers.push_back(m1);

	URegionMarker* m2 = new URegionMarker();
	m2->Setup(g_hInst, 1200, 800, IDB_BITMAP3, IDB_BITMAP3, IDB_BITMAP3);
	m2->SetRegionInfo(L"하이샌드 대교구", L"고대 마법 도서관", L"수천 년 전의 지식이 잠들어 있는 곳입니다.", L"신비로움 / 도서관");
	Markers.push_back(m2);

	//지역 마커 클릭시 띄울 UI 설정
	infoPopup = new URegionInfoPopup();
	infoPopup->Setup(g_hInst, rects);
	infoPopup->SetOnEnterClick([this]() {
		this->Comp_NextStage(BATTLE_STAGE);
	});

	//m키 입력시 띄울 UI 설정(캐릭터, 인벤토리)
	mainMenu = new UMainMenuPopup();
	mainMenu->Setup(g_hInst, rects);

	//마우스 위치의 지역을 정보를 실시간으로 띄워주는 UI
	locationUI = new ULocationIndicator();
	locationUI->Setup(rects);

	HBITMAP hMenuBG = LoadBitmap(g_hInst, MAKEINTRESOURCE(IDB_BITMAP1));
	mainMenu->SetBackgroundImage(hMenuBG);


	// 테스트를 위한 더미 플레이어 (팀이 비어있을 때만 생성)
	if (TeamManager::Get().GetPlayers().empty()) {
		Character* char1 = new Character();
		char1->Init(ECharacterClass::Warrior, 0, 0);
		char1->SetName(L"Warrior");
		char1->LoadSprites(L"Resource");
		TeamManager::Get().AddPlayer(char1);

		Character* char2 = new Character();
		char2->Init(ECharacterClass::Archer, 0, 0);
		char2->SetName(L"Archer");
		char2->LoadSprites(L"Resource");
		TeamManager::Get().AddPlayer(char2);
	}

	mainMenu->SetTeamData(TeamManager::Get().GetPlayers());

	screenUI.AddChildWindow(infoPopup);
	screenUI.AddChildWindow(mainMenu);
	screenUI.AddChildWindow(locationUI);

	MouseMark = new UMouseMark();
	MouseMark->SetFrame(g_hInst, { IDB_UI_MOUSECUSOR }); 

	// 테스트용 아이템 생성 (포션)
	AUsable* testPotion = new AUsable();
	testPotion->SetItemData(101, L"Test Potion", L"Heals 50 HP", 100);
	testPotion->SetUsableData(50, true);
	testPotion->SetGridPos(600, 400); // 월드 좌표
	stageItems.push_back(testPotion);
}

void Map_Stage::Update(HWND hWnd) {
	std::pair<float, float> CameraOffset = Camera->Getg_Cam();
	int camX = (int)CameraOffset.first;
	int camY = (int)CameraOffset.second;

	// 카메라 이동 로직 (화살표 키 및 마우스 가장자리)
	{
		std::pair<float, float> curGrid = Camera->GetPointGrid();
		float scrollSpeed = 0.5f; // 타일 단위 속도
		
		// 1. 키보드 화살표 입력
		if (GetAsyncKeyState(VK_LEFT) & 0x8000) curGrid.first -= scrollSpeed;
		if (GetAsyncKeyState(VK_RIGHT) & 0x8000) curGrid.first += scrollSpeed;
		if (GetAsyncKeyState(VK_UP) & 0x8000) curGrid.second -= scrollSpeed;
		if (GetAsyncKeyState(VK_DOWN) & 0x8000) curGrid.second += scrollSpeed;

		// 2. 마우스 가장자리 스크롤
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

		// 경계 제한 (타일 단위)
		if (curGrid.first < 0) curGrid.first = 0;
		if (curGrid.first > W) curGrid.first = (float)W;
		if (curGrid.second < 0) curGrid.second = 0;
		if (curGrid.second > H) curGrid.second = (float)H;

		Camera->SetPointGrid(curGrid.first, curGrid.second);
	}

	for (auto m : Markers) m->Update();
	screenUI.Update();
	Camera->Update(W, H, TILE_SIZE, 1280, 720);

	if (MouseMark) {
		bool bMenuOpen = (mainMenu && mainMenu->GetVisible()) || (infoPopup && infoPopup->GetVisible());
		if (bMenuOpen) {
			MouseMark->SetVisible(false);
		}
		else {
			MouseMark->SetVisible(true);
		}
		MouseMark->SetCameraOffset(camX, camY);
		MouseMark->Update();
	}
}

void Map_Stage::Draw(HDC hMemDC, HDC hMemDC1) {
	std::pair<float, float> CameraOffset = Camera->Getg_Cam();
	int camX = (int)CameraOffset.first;
	int camY = (int)CameraOffset.second;

	// 배경 그리기 (카메라 오프셋 적용)
	HBITMAP hOld = (HBITMAP)SelectObject(hMemDC1, hBitBackground);
	TransparentBlt(hMemDC, -camX, -camY, BackgroundInfo.bmWidth, BackgroundInfo.bmHeight, hMemDC1, 0, 0, BackgroundInfo.bmWidth, BackgroundInfo.bmHeight, RGB(255, 255, 255));
	SelectObject(hMemDC1, hOld);

	// 마커 그리기 (월드 좌표)
	for (auto m : Markers) {
		m->DrawWithOffset(hMemDC, hMemDC1, camX, camY);
	}

	// 아이템 그리기 (월드 좌표)
	for (auto item : stageItems) {
		if (item->IsActive()) {
			Location pos = item->GetGridPos();
			int drawX = pos.x - camX;
			int drawY = pos.y - camY;

			// 화면 안에 있을 때만 그리기
			if (drawX > -50 && drawX < 1330 && drawY > -50 && drawY < 770) {
				HBRUSH hBrush = CreateSolidBrush(RGB(0, 255, 0));
				HBRUSH oldBrush = (HBRUSH)SelectObject(hMemDC, hBrush);
				Ellipse(hMemDC, drawX - 15, drawY - 15, drawX + 15, drawY + 15);
				SelectObject(hMemDC, oldBrush);
				DeleteObject(hBrush);

				SetBkMode(hMemDC, TRANSPARENT);
				SetTextColor(hMemDC, RGB(255, 255, 255));
				TextOutW(hMemDC, drawX - 30, drawY + 20, item->GetName().c_str(), (int)item->GetName().length());
			}
		}
	}

	// 화면 UI (스크린 좌표)
	screenUI.Draw(hMemDC, hMemDC1);

	if (MouseMark) MouseMark->Draw(hMemDC, hMemDC1);
}

void Map_Stage::BackgroundMove(int x, int y) {
}

int Map_Stage::Input(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	static int CLK_X, CLK_Y;
	std::pair<float, float> CameraOffset = Camera->Getg_Cam();
	int camX = (int)CameraOffset.first;
	int camY = (int)CameraOffset.second;

	switch (uMsg) {
	case WM_MOUSEMOVE:
		CLK_X = LOWORD(lParam);
		CLK_Y = HIWORD(lParam);
		if (MouseMark) MouseMark->SetMousePos({ CLK_X, CLK_Y });
		screenUI.UpdateMouseHover(CLK_X, CLK_Y);
		{
			URegionMarker* hovered = nullptr;
			bool bMenuOpen = (mainMenu && mainMenu->GetVisible()) || (infoPopup && infoPopup->GetVisible());
			
			// 메뉴가 닫혀있을 때만 마커 호버 체크
			for (auto m : Markers) {
				bool pointed = !bMenuOpen && m->ContainsPointWithOffset(CLK_X, CLK_Y, camX, camY);
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
					int dx = (m->GetLocation().x) - (camX + CLK_X);
					int dy = (m->GetLocation().y) - (camY + CLK_Y);
					float dist = sqrt((float)(dx * dx + dy * dy));
					if (dist < 275.0f) {
						if (dist < minDesc) {
							minDesc = dist;
							closestMarker = m;
						}
						foundNear = true;
					}
				}
			}
			if (foundNear && closestMarker) {
				locationUI->SetInfo(closestMarker->GetParentName(), closestMarker->GetName());
				locationUI->SetVisible(true);
			}
			else {
				locationUI->SetVisible(false);
			}
		}
		break;

	case WM_LBUTTONDOWN:
	case WM_RBUTTONDOWN:
		CLK_X = LOWORD(lParam);
		CLK_Y = HIWORD(lParam);

		// 1. UI 클릭 우선 체크
		if (UWidget* w = screenUI.FindWidgetUnderMouse(CLK_X, CLK_Y)) {
			if (uMsg == WM_LBUTTONDOWN) {
				if (UButton* btn = dynamic_cast<UButton*>(w)) btn->SetIsPressed(true);
			}
			return -2;
		}

		// 메뉴가 열려있으면 월드 클릭 무시
		if ((mainMenu && mainMenu->GetVisible()) || (infoPopup && infoPopup->GetVisible())) {
			return -2;
		}

		if (uMsg == WM_RBUTTONDOWN) return -2; // 월드 우클릭은 현재 기능 없음

		// 0. 아이템 클릭 체크 (월드 좌표 기준)
		{
			auto it = stageItems.begin();
			while (it != stageItems.end()) {
				AItemBase* item = *it;
				Location pos = item->GetGridPos();
				int dx = (camX + CLK_X) - pos.x;
				int dy = (camY + CLK_Y) - pos.y;

				if (dx * dx + dy * dy <= 30 * 30) { 
					TeamManager::Get().AddItem(item);
					it = stageItems.erase(it);
					std::wstring msg = item->GetName() + L"을(를) 획득했습니다!";
					MessageBoxW(hWnd, msg.c_str(), L"획득 알림", MB_OK);
					return -2;
				}
				++it;
			}
		}

		// 2. 월드 마커 클릭 체크
		for (auto m : Markers) {
			if (m->ContainsPointWithOffset(CLK_X, CLK_Y, camX, camY)) {
				int gridX = (camX + CLK_X) / TILE_SIZE;
				int gridY = (camY + CLK_Y) / TILE_SIZE;
				Camera->SetPointGrid(gridX, gridY);
				for (auto other : Markers) other->Deactivate();
				m->Activate();
				infoPopup->SetInfo(m->GetName(), m->GetDesc(), m->GetEnv());
				infoPopup->SetVisible(true); // 명시적으로 보이게 설정
				return -2;
			}
		}

		// 3. 바닥 클릭 시 카메라 이동
		{
			int gridX = (camX + CLK_X) / TILE_SIZE;
			int gridY = (camY + CLK_Y) / TILE_SIZE;
			Camera->SetPointGrid(gridX, gridY);
		}
		break;

	case WM_LBUTTONUP:
	case WM_RBUTTONUP:
		CLK_X = LOWORD(lParam);
		CLK_Y = HIWORD(lParam);
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
				mainMenu->SelectTab(EMenuTab::Inventory);
				mainMenu->SetVisible(!mainMenu->GetVisible());
			}
		}
		else if (wParam == 'p' || wParam == 'P') {
			if (mainMenu) {
				mainMenu->SelectTab(EMenuTab::CharacterInfo);
				mainMenu->SetVisible(!mainMenu->GetVisible());
			}
		}
		else if (wParam == VK_ESCAPE) {
			if (infoPopup && infoPopup->GetVisible()) {
				infoPopup->SetVisible(false);
				for (auto m : Markers) m->Deactivate();
			}
			else if (mainMenu && mainMenu->GetVisible()) {
				mainMenu->SetVisible(false);
			}
		}
		break;
	}
	return -2;
}

void Map_Stage::Comp_NextStage(int ID) {
	if (ID != MAP_STAGE) {
		NextStageID = ID;
	}
}
