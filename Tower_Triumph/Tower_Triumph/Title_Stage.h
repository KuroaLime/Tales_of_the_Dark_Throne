#pragma once
#include"CStage.h"
#include"UCanvasPanel.h"
#include"UContainerWidget.h"
#include"UButton.h"
#include"UImage.h"
#include"USettingPopup.h"

class Title_Stage : public CStage
{
private:
    UCanvasPanel mainHUD;
    UContainerWidget* menuGroup;
    USettingPopup* settingPopup;
    UImage* titleName;

    UButton* btnStart;
    UButton* btnSetting;
    UButton* btnEnd;

public:
    Title_Stage();
    virtual ~Title_Stage();
    virtual void Init(HWND hWnd, HINSTANCE const g_hInst);
    virtual void Update(HWND hWnd);
    virtual void Draw(HDC hMemDC, HDC hMemDC1);
    virtual int Input(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

    virtual void Comp_NextStage(int ID);
    virtual void BackgroundMove(int x, int y) {};
};
