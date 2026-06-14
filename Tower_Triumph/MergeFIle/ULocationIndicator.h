#pragma once
#include "UContainerWidget.h"
#include <string>

class ULocationIndicator : public UContainerWidget
{
private:
    std::wstring parentRegion;
    std::wstring pointName;
    HBITMAP hThumbnail;
    BITMAP thumbInfo;

public:
    ULocationIndicator();
    virtual void Draw(HDC hMemDC, HDC hMemDC1) override;

    void SetInfo(const std::wstring& parent, const std::wstring& point, HBITMAP hThumb = NULL);
    void Setup(RECT clientRect);
};
