#pragma once
#include "UCanvasPanelSlot.h"
#include<vector>

class UCanvasPanel
{
protected:
	std::vector<UCanvasPanelSlot*> Slots;
public:
	UCanvasPanelSlot* AddChildSlot(class UDefaultUI content);

	virtual void Draw();
};

