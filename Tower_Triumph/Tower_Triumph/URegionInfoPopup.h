#pragma once
#include "UContainerWidget.h"
#include "UButton.h"
#include <string>

class URegionInfoPopup : public UContainerWidget
{
private:
	UButton* btnClose;
	UButton* btnEnter;
	std::wstring rName;
	std::wstring rDesc;
	std::wstring rEnv;

	std::function<void()> OnEnterClickCallback;
	std::function<void()> OnCloseClickCallback;

public:
	URegionInfoPopup();
	void Setup(HINSTANCE hInst, RECT clientRect);
	void SetInfo(const std::wstring& name, const std::wstring& desc, const std::wstring& env);
	void SetOnEnterClick(std::function<void()> callback) { OnEnterClickCallback = callback; }
	void SetOnCloseClick(std::function<void()> callback) { OnCloseClickCallback = callback; }
	virtual void Draw(HDC hMemDC, HDC hMemDC1) override;
};
