#pragma once
#include "Widget.h"
#include <functional>
#include <string>

class UButton : public UWidget
{
protected:
	bool isPressed;
	bool isSelected; // Added to indicate active tab/selection
	bool bEnabled;   // Added to support disabled state
	std::function<void()> OnClickCallback;
	std::function<void()> OnRightClickCallback;
	std::function<void()> OnHoverCallback;
	std::function<void()> OnUnhoverCallback;
	bool bWasPointed;
	std::wstring ButtonText;

public:
	UButton();
	virtual void Draw(HDC hMemDC, HDC hMemDC1) override;
	virtual void Update() override;
	
	void SetOnClick(std::function<void()> Callback) { OnClickCallback = Callback; }
	void Click();

	void SetOnRightClick(std::function<void()> Callback) { OnRightClickCallback = Callback; }
	void RightClick();

	void SetOnHover(std::function<void()> Callback) { OnHoverCallback = Callback; }
	void SetOnUnhover(std::function<void()> Callback) { OnUnhoverCallback = Callback; }

	void SetIsPressed(bool New) { isPressed = New; }
	bool GetIsPressed() const { return isPressed; }

	void SetSelected(bool New) { isSelected = New; }
	bool GetSelected() const { return isSelected; }

	void SetEnabled(bool New) { bEnabled = New; }
	bool GetEnabled() const { return bEnabled; }

	void SetText(const std::wstring& Text) { ButtonText = Text; }
	void SetSize(int sx, int sy) { Size.sX = sx; Size.sY = sy; }
};
