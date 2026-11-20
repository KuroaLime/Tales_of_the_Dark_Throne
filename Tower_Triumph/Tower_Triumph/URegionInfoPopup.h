#pragma once
#include "UContainerWidget.h"
#include "UButton.h"
#include <chrono>
#include <string>

class URegionInfoPopup : public UContainerWidget
{
private:
	UButton* btnClose;
	UButton* btnEnter;
	std::wstring rName;
	std::wstring rDesc;
	std::wstring rEnv;
	std::wstring parentRegion;
	RECT panelRect;
	HBITMAP mapPreview;
	BITMAP mapPreviewInfo;
	RECT mapPreviewBounds;
	void DrawMapPreview(HDC target, HDC scratch, const RECT& destination);
	void DrawContent(HDC target, HDC scratch);
	void ReleasePresentationSurface();
	bool EnsurePresentationSurface(HDC target, int width, int height);
	bool presentationVisible = false;
	bool entryCommitted = false;
	double presentationOpacity = 0.0;
	double presentationStart = 0.0;
	double presentationTarget = 0.0;
	double presentationElapsed = 0.0;
	double presentationDuration = 0.0;
	std::chrono::steady_clock::time_point lastPresentationUpdate;
	HDC presentationDC = nullptr;
	HBITMAP presentationBitmap = nullptr;
	HGDIOBJ previousPresentationBitmap = nullptr;
	int presentationWidth = 0, presentationHeight = 0;

	std::function<void()> OnEnterClickCallback;
	std::function<void()> OnCloseClickCallback;

public:
	URegionInfoPopup();
	~URegionInfoPopup() override;
	// Logical visibility changes immediately; the close image remains modal until faded out.
	void SetVisible(bool visible) override;
	void AdvancePresentation(double seconds);
	bool IsPresenting() const { return bVisible || presentationOpacity > 0.0; }
	bool IsClosing() const { return !bVisible && presentationOpacity > 0.0; }
	bool IsOpening() const { return bVisible && presentationOpacity < 1.0; }
	bool IsInputBlocked() const { return IsClosing() || (bVisible && entryCommitted); }
	double GetPresentationOpacity() const { return presentationOpacity; }
	void Setup(HINSTANCE hInst, RECT clientRect);
	void SetInfo(const std::wstring& name, const std::wstring& desc, const std::wstring& env);
	// The map owns this bitmap; the popup only borrows it while the map is alive.
	void SetMapPreview(HBITMAP bitmap, const RECT& worldBounds, const std::wstring& parent = L"");
	void SetOnEnterClick(std::function<void()> callback) { OnEnterClickCallback = callback; }
	void SetOnCloseClick(std::function<void()> callback) { OnCloseClickCallback = callback; }
	virtual void Draw(HDC hMemDC, HDC hMemDC1) override;
	virtual void Update() override;
	virtual UWidget* FindWidgetUnderMouse(int x, int y) override;
	virtual void UpdateMouseHover(int x, int y) override;
};
