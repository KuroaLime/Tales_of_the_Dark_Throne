#include "UButton.h"

UButton::UButton()
{
    isPressed = false;
    isSelected = false;
    bEnabled = true;
    bWasPointed = false;
    hButtonFont = NULL;
    AnimationType = 0;
    currentVisualOffsetY = 0.0f;
    currentScale = 1.0f;
    bDrawTextFallbackBackground = false;

    // 기존 디버그 기본색은 유지한다. 필요한 버튼만 SetFallbackColors로 별도 지정한다.
    FallbackNormalColor = RGB(255, 0, 0);
    FallbackHoverColor = RGB(0, 255, 0);
    FallbackPressedColor = RGB(100, 100, 100);
    FallbackSelectedColor = RGB(180, 150, 0);
    FallbackDisabledColor = RGB(50, 50, 50);

    TextNormalColor = RGB(255, 255, 255);
    TextHoverColor = RGB(255, 215, 0);
    TextPressedColor = RGB(150, 150, 150);
    TextSelectedColor = RGB(255, 215, 0);
    TextDisabledColor = RGB(100, 100, 100);

    FallbackBorderColor = RGB(255, 255, 255);
    FallbackBorderWidth = 0;
}

UButton::~UButton()
{
    if (hButtonFont != NULL)
    {
        DeleteObject(hButtonFont);
        hButtonFont = NULL;
    }
}

void UButton::SetFont(int nHeight, int nWeight, const std::wstring& fontName)
{
    if (hButtonFont != NULL)
    {
        DeleteObject(hButtonFont);
        hButtonFont = NULL;
    }

    hButtonFont = CreateFontW(
        nHeight,
        0,
        0,
        0,
        nWeight,
        FALSE,
        FALSE,
        FALSE,
        DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS,
        CLIP_DEFAULT_PRECIS,
        DEFAULT_QUALITY,
        DEFAULT_PITCH | FF_DONTCARE,
        fontName.c_str()
    );
}

void UButton::SetFallbackColors(COLORREF normal, COLORREF hover, COLORREF pressed, COLORREF selected, COLORREF disabled)
{
    FallbackNormalColor = normal;
    FallbackHoverColor = hover;
    FallbackPressedColor = pressed;
    FallbackSelectedColor = selected;
    FallbackDisabledColor = disabled;
}

void UButton::SetFallbackTextColors(COLORREF normal, COLORREF hover, COLORREF pressed, COLORREF selected, COLORREF disabled)
{
    TextNormalColor = normal;
    TextHoverColor = hover;
    TextPressedColor = pressed;
    TextSelectedColor = selected;
    TextDisabledColor = disabled;
}

void UButton::SetFallbackBorder(COLORREF color, int width)
{
    FallbackBorderColor = color;
    FallbackBorderWidth = width;
}

void UButton::Animation1()
{
    float targetOffsetY = -15.0f;
    float targetScale = 1.0f;

    if (isPressed)
    {
        targetOffsetY = 0.0f;
        targetScale = 0.9f;
    }
    else if (IsPointed || isSelected)
    {
        targetOffsetY = 0.0f;
        targetScale = 1.0f;
    }

    currentVisualOffsetY += (targetOffsetY - currentVisualOffsetY) * 0.2f;
    currentScale += (targetScale - currentScale) * 0.3f;
}

void UButton::Animation2()
{
    float targetOffsetY = 0.0f;
    float targetScale = 1.0f;

    if (isPressed)
    {
        targetScale = 0.9f;
    }
    else if (IsPointed || isSelected)
    {
        targetScale = 1.05f;
    }

    currentVisualOffsetY += (targetOffsetY - currentVisualOffsetY) * 0.2f;
    currentScale += (targetScale - currentScale) * 0.3f;
}

void UButton::Update()
{
    if (!bEnabled)
    {
        IsPointed = false;
        isPressed = false;
        currentVisualOffsetY = 0.0f;
        currentScale = 1.0f;
        return;
    }

    UWidget::Update();

    switch (AnimationType)
    {
    case 1:
        Animation1();
        break;
    case 2:
        Animation2();
        break;
    default:
        currentVisualOffsetY = 0.0f;
        currentScale = 1.0f;
        break;
    }

    if (IsPointed && !bWasPointed)
    {
        if (OnHoverCallback) OnHoverCallback();
    }
    else if (!IsPointed && bWasPointed)
    {
        if (OnUnhoverCallback) OnUnhoverCallback();
    }

    bWasPointed = IsPointed;
}

void UButton::Draw(HDC hMemDC, HDC hMemDC1)
{
    if (!bVisible) return;

    int frame = 0;
    if (!bEnabled) frame = 0;
    else if (isPressed) frame = (AniCount > 2) ? 2 : (AniCount > 0 ? 0 : -1);
    else if (isSelected) frame = (AniCount > 1) ? 1 : (AniCount > 0 ? 0 : -1);
    else if (IsPointed) frame = (AniCount > 1) ? 1 : (AniCount > 0 ? 0 : -1);
    else frame = (AniCount > 0 ? 0 : -1);

    int visualW = (int)(Size.sX * currentScale);
    int visualH = (int)(Size.sY * currentScale);
    int visualX = Loc.x;
    int visualY = Loc.y + (int)currentVisualOffsetY;

    RECT visualRect = {
        visualX - visualW / 2,
        visualY - visualH / 2,
        visualX + visualW / 2,
        visualY + visualH / 2
    };

    if (frame != -1 && !bBitmap.empty())
    {
        HBITMAP temp = (HBITMAP)SelectObject(hMemDC1, bBitmap[frame]);
        TransparentBlt(
            hMemDC,
            visualRect.left,
            visualRect.top,
            visualW,
            visualH,
            hMemDC1,
            0,
            0,
            bMapinfo[frame].bmWidth,
            bMapinfo[frame].bmHeight,
            RGB(255, 255, 255)
        );
        SelectObject(hMemDC1, temp);

        if (isSelected)
        {
            HPEN hPen = CreatePen(PS_SOLID, 3, RGB(255, 215, 0));
            HPEN oldP = (HPEN)SelectObject(hMemDC, hPen);
            HBRUSH oldB = (HBRUSH)SelectObject(hMemDC, GetStockObject(NULL_BRUSH));
            Rectangle(hMemDC, visualRect.left, visualRect.top, visualRect.right, visualRect.bottom);
            SelectObject(hMemDC, oldB);
            SelectObject(hMemDC, oldP);
            DeleteObject(hPen);
        }
    }
    else
    {
        if (ButtonText.empty() || bDrawTextFallbackBackground)
        {
            COLORREF bgColor = FallbackNormalColor;
            if (!bEnabled) bgColor = FallbackDisabledColor;
            else if (isPressed) bgColor = FallbackPressedColor;
            else if (isSelected) bgColor = FallbackSelectedColor;
            else if (IsPointed) bgColor = FallbackHoverColor;

            HBRUSH bBrush = CreateSolidBrush(bgColor);
            HPEN hPen = CreatePen(PS_SOLID, FallbackBorderWidth > 0 ? FallbackBorderWidth : 1, FallbackBorderColor);
            HGDIOBJ penObj = (FallbackBorderWidth > 0) ? (HGDIOBJ)hPen : GetStockObject(NULL_PEN);
            HPEN oldP = (HPEN)SelectObject(hMemDC, penObj);
            HBRUSH oldB = (HBRUSH)SelectObject(hMemDC, bBrush);

            RoundRect(hMemDC, visualRect.left, visualRect.top, visualRect.right, visualRect.bottom, 8, 8);

            SelectObject(hMemDC, oldB);
            SelectObject(hMemDC, oldP);
            DeleteObject(hPen);
            DeleteObject(bBrush);
        }

        if (isSelected)
        {
            HPEN hPen = CreatePen(PS_SOLID, 3, RGB(255, 255, 255));
            HPEN oldP = (HPEN)SelectObject(hMemDC, hPen);
            HBRUSH oldB = (HBRUSH)SelectObject(hMemDC, GetStockObject(NULL_BRUSH));
            RoundRect(hMemDC, visualRect.left, visualRect.top, visualRect.right, visualRect.bottom, 8, 8);
            SelectObject(hMemDC, oldB);
            SelectObject(hMemDC, oldP);
            DeleteObject(hPen);
        }
    }

    if (!ButtonText.empty())
    {
        int oldMode = SetBkMode(hMemDC, TRANSPARENT);
        COLORREF oldColor = SetTextColor(hMemDC, RGB(255, 255, 255));
        HFONT oldFont = NULL;

        if (hButtonFont != NULL)
        {
            oldFont = (HFONT)SelectObject(hMemDC, hButtonFont);
        }

        COLORREF textColor = TextNormalColor;
        if (!bEnabled) textColor = TextDisabledColor;
        else if (isPressed) textColor = TextPressedColor;
        else if (isSelected) textColor = TextSelectedColor;
        else if (IsPointed) textColor = TextHoverColor;

        RECT shadowRect = visualRect;
        OffsetRect(&shadowRect, 2, 2);
        SetTextColor(hMemDC, RGB(0, 0, 0));
        DrawTextW(hMemDC, ButtonText.c_str(), -1, &shadowRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

        SetTextColor(hMemDC, textColor);
        DrawTextW(hMemDC, ButtonText.c_str(), -1, &visualRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

        if (oldFont != NULL)
        {
            SelectObject(hMemDC, oldFont);
        }

        SetTextColor(hMemDC, oldColor);
        SetBkMode(hMemDC, oldMode);
    }
}

void UButton::Click()
{
    isPressed = false;
    if (OnClickCallback)
    {
        OnClickCallback();
    }
}

void UButton::RightClick()
{
    if (OnRightClickCallback)
    {
        OnRightClickCallback();
    }
}
