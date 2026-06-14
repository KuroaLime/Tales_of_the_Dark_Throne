#include "Title_Stage.h"
#include "resource.h"
#include "SoundManager.h"

Title_Stage::Title_Stage()
{
    NextStageID = -2;
    menuGroup = nullptr;
    settingPopup = nullptr;
    btnStart = nullptr;
    btnSetting = nullptr;
    btnEnd = nullptr;
    hBitBackground = nullptr;
    ZeroMemory(&BackgroundInfo, sizeof(BackgroundInfo));
    CurStageID = TITLE_STAGE;
}

Title_Stage::~Title_Stage()
{
    if (menuGroup) delete menuGroup;
    if (settingPopup) delete settingPopup;

    if (hBitBackground)
    {
        DeleteObject(hBitBackground);
        hBitBackground = nullptr;
    }
}

void Title_Stage::Init(HWND hWnd, HINSTANCE const g_hInst)
{
    GetClientRect(hWnd, &rects);

    // 타이틀/월드맵 분위기 BGM
    SoundManager::PlayBGM(L"Resource\\Sound\\TowerTriumph_Stage_Map_Loop_42s.wav", true);

    hBitBackground = LoadBitmap(g_hInst, MAKEINTRESOURCE(IDB_SMAP_BACKGROUND));
    if (!hBitBackground)
    {
        hBitBackground = (HBITMAP)LoadImage(
            NULL,
            L"Source\\SMap\\background\\Test00.bmp",
            IMAGE_BITMAP,
            0,
            0,
            LR_LOADFROMFILE | LR_CREATEDIBSECTION
        );
    }

    if (hBitBackground)
    {
        GetObject(hBitBackground, sizeof(BITMAP), &BackgroundInfo);
    }
    else
    {
        BackgroundInfo.bmWidth = rects.right;
        BackgroundInfo.bmHeight = rects.bottom;
    }

    Loc.first.x = rects.right / 2;
    Loc.first.y = rects.bottom / 2;
    Loc.second.sX = BackgroundInfo.bmWidth / 2;
    Loc.second.sY = BackgroundInfo.bmHeight / 2;

    NextStageID = -2;

    menuGroup = new UContainerWidget();
    settingPopup = new USettingPopup();
    settingPopup->Setup(g_hInst, rects);

    int startY = rects.bottom / 2 + 50;

    btnStart = new UButton();
    btnStart->SetLocation(rects.right / 2, startY);
    btnStart->SetUIID(0);
    btnStart->SetText(L"START GAME");
    btnStart->SetOnClick([this]() { this->NextStageID = MAP_STAGE; });
    menuGroup->AddChild(btnStart);

    btnSetting = new UButton();
    btnSetting->SetLocation(rects.right / 2, startY + 80);
    btnSetting->SetUIID(1);
    btnSetting->SetText(L"SETTINGS");
    btnSetting->SetOnClick([this]() { this->settingPopup->SetVisible(true); });
    menuGroup->AddChild(btnSetting);

    btnEnd = new UButton();
    btnEnd->SetLocation(rects.right / 2, startY + 160);
    btnEnd->SetUIID(2);
    btnEnd->SetText(L"EXIT");
    btnEnd->SetOnClick([this]() { this->NextStageID = GAME_END; });
    menuGroup->AddChild(btnEnd);

    mainHUD.AddChildWindow(menuGroup);
    mainHUD.AddChildWindow(settingPopup);
}

void Title_Stage::Update(HWND hWnd)
{
    mainHUD.Update();
}

void Title_Stage::Draw(HDC hMemDC, HDC hMemDC1)
{
    if (hBitBackground)
    {
        HBITMAP hOld = (HBITMAP)SelectObject(hMemDC1, hBitBackground);
        StretchBlt(
            hMemDC,
            0,
            0,
            rects.right,
            rects.bottom,
            hMemDC1,
            0,
            0,
            BackgroundInfo.bmWidth,
            BackgroundInfo.bmHeight,
            SRCCOPY
        );
        SelectObject(hMemDC1, hOld);
    }
    else
    {
        HBRUSH hBrush = CreateSolidBrush(RGB(40, 40, 40));
        FillRect(hMemDC, &rects, hBrush);
        DeleteObject(hBrush);
    }

    mainHUD.Draw(hMemDC, hMemDC1);
}

void Title_Stage::Comp_NextStage(int ID)
{
}

int Title_Stage::Input(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    static int CLK_X, CLK_Y;

    switch (uMsg)
    {
    case WM_MOUSEMOVE:
        CLK_X = LOWORD(lParam);
        CLK_Y = HIWORD(lParam);
        mainHUD.UpdateMouseHover(CLK_X, CLK_Y);
        break;

    case WM_LBUTTONDOWN:
        CLK_X = LOWORD(lParam);
        CLK_Y = HIWORD(lParam);
        if (UWidget* w = mainHUD.FindWidgetUnderMouse(CLK_X, CLK_Y))
        {
            if (UButton* btn = dynamic_cast<UButton*>(w))
            {
                btn->SetIsPressed(true);
            }
        }
        break;

    case WM_LBUTTONUP:
        CLK_X = LOWORD(lParam);
        CLK_Y = HIWORD(lParam);
        if (UWidget* w = mainHUD.FindWidgetUnderMouse(CLK_X, CLK_Y))
        {
            if (UButton* btn = dynamic_cast<UButton*>(w))
            {
                btn->Click();
            }
        }

        if (btnStart) btnStart->SetIsPressed(false);
        if (btnSetting) btnSetting->SetIsPressed(false);
        if (btnEnd) btnEnd->SetIsPressed(false);

        if (NextStageID != -2)
        {
            int ret = NextStageID;
            NextStageID = -2;
            return ret;
        }
        break;

    case WM_KEYDOWN:
        if (wParam == VK_ESCAPE)
        {
            SendMessage(hWnd, WM_CLOSE, 0, 0);
        }
        break;
    }

    return -2;
}
