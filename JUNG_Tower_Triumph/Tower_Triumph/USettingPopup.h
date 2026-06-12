#pragma once
#include "UContainerWidget.h"
#include "UButton.h"
#include "UImage.h"

class USettingPopup : public UContainerWidget
{
private:
	UButton* btnClose;
	UButton* btnSoundToggle;
	bool bSoundOn;

public:
	USettingPopup();
	virtual void Init() override;
	virtual void Draw(HDC hMemDC, HDC hMemDC1) override;
	
	void Setup(HINSTANCE hInst, RECT clientRect);
};
