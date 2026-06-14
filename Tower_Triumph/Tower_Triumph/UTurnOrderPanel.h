#pragma once
#include "UContainerWidget.h"
#include "UTurnCharacterInfo.h"
#include "STurnTimeline.h"
#include <vector>

class UTurnOrderPanel : public UContainerWidget
{
private:
    STurnTimeline* logicSystem;
    std::vector<UTurnCharacterInfo*> icons;
    
    std::vector<Character*> lastKnownOrder;

public:
    UTurnOrderPanel();
    virtual ~UTurnOrderPanel();
    
    void Setup(RECT clientRect);
    void Bind(STurnTimeline* system);
    
    virtual void Update() override;
    virtual void Draw(HDC hMemDC, HDC hMemDC1) override;

    void RefreshIcons();
};
