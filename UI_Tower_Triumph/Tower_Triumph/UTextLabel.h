#pragma once
#include "Widget.h"
#include <string>

class UTextLabel : public UWidget
{
protected:
    std::wstring Text;
    HFONT hFont;
    COLORREF TextColor;
    int TextAlignFlags;

public:
    UTextLabel();
    virtual ~UTextLabel();

    virtual void Draw(HDC hMemDC, HDC hMemDC1) override;
    
    void SetText(const std::wstring& newText) { Text = newText; }
    void SetFont(int nHeight, int nWeight, const std::wstring& fontName);
    void SetColor(COLORREF color) { TextColor = color; }
    void SetAlign(int flags) { TextAlignFlags = flags; } // e.g., DT_LEFT, DT_CENTER
};
