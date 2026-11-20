#include "UButton.h"

#include <algorithm>

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
    bDrawOnlyText = false;
    bWrapText = false;

    NormalColor = RGB(38, 44, 58);
    HoverColor = RGB(60, 72, 98);
    PressedColor = RGB(28, 33, 44);
    SelectedColor = RGB(120, 86, 28);
    DisabledColor = RGB(34, 36, 40);

    NormalTextColor = RGB(238, 232, 214);
    HoverTextColor = RGB(255, 224, 142);
    PressedTextColor = RGB(185, 175, 145);
    SelectedTextColor = RGB(255, 235, 160);
    DisabledTextColor = RGB(120, 120, 120);

    BorderColor = RGB(160, 135, 76);
    BorderWidth = 1;
}

UButton::~UButton()
{
    if (hButtonFont != NULL)
    {
        DeleteObject(hButtonFont);
        hButtonFont = NULL;
    }
}

void UButton::SetFallbackColors(COLORREF normal, COLORREF hover, COLORREF pressed, COLORREF selected, COLORREF disabled)
{
    NormalColor = normal;
    HoverColor = hover;
    PressedColor = pressed;
    SelectedColor = selected;
    DisabledColor = disabled;
}

void UButton::SetFallbackTextColors(COLORREF normal, COLORREF hover, COLORREF pressed, COLORREF selected, COLORREF disabled)
{
    NormalTextColor = normal;
    HoverTextColor = hover;
    PressedTextColor = pressed;
    SelectedTextColor = selected;
    DisabledTextColor = disabled;
}

void UButton::SetFallbackBorder(COLORREF color, int width)
{
    BorderColor = color;
    BorderWidth = width;
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
    if (!bEnabled || !bVisible)
    {
        IsPointed = false;
        isPressed = false;
        bWasPointed = false;
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
        if (!bDrawOnlyText)
        {
            COLORREF fillColor = NormalColor;
            COLORREF borderColor = BorderColor;
            int borderWidth = BorderWidth;

            if (!bEnabled)
            {
                fillColor = DisabledColor;
                borderColor = RGB(72, 72, 78);
            }
            else if (isPressed)
            {
                fillColor = PressedColor;
            }
            else if (isSelected)
            {
                fillColor = SelectedColor;
                borderColor = RGB(255, 220, 120);
                borderWidth = 2;
            }
            else if (IsPointed)
            {
                fillColor = HoverColor;
                borderColor = RGB(210, 178, 100);
            }

            HBRUSH hBrush = CreateSolidBrush(fillColor);
            HPEN hPen = CreatePen(PS_SOLID, borderWidth, borderColor);
            HBRUSH oldBrush = (HBRUSH)SelectObject(hMemDC, hBrush);
            HPEN oldPen = (HPEN)SelectObject(hMemDC, hPen);

            RoundRect(
                hMemDC,
                visualRect.left,
                visualRect.top,
                visualRect.right,
                visualRect.bottom,
                8,
                8
            );

            SelectObject(hMemDC, oldBrush);
            SelectObject(hMemDC, oldPen);
            DeleteObject(hBrush);
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

        COLORREF textColor;
        if (!bEnabled) textColor = DisabledTextColor;
        else if (isPressed) textColor = PressedTextColor;
        else if (IsPointed) textColor = HoverTextColor;
        else if (isSelected) textColor = SelectedTextColor;
        else textColor = NormalTextColor;

        int textYOffset = (frame == -1 || bDrawTextFallbackBackground) ? 2 : 0;

        RECT textRect = visualRect;
        OffsetRect(&textRect, 0, textYOffset);
        UINT textFlags = DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX;
        if (bWrapText)
        {
            InflateRect(&textRect, -6, -3);
            RECT measured = textRect;
            DrawTextW(hMemDC, ButtonText.c_str(), -1, &measured, DT_CENTER | DT_WORDBREAK | DT_CALCRECT | DT_NOPREFIX);
            const int height = measured.bottom - measured.top;
            textRect.top += (std::max)(0, static_cast<int>(textRect.bottom - textRect.top - height) / 2);
            textFlags = DT_CENTER | DT_WORDBREAK | DT_NOPREFIX;
        }

        RECT shadowRect = textRect;
        OffsetRect(&shadowRect, 1, 1);

        SetTextColor(hMemDC, RGB(0, 0, 0));
        DrawTextW(hMemDC, ButtonText.c_str(), -1, &shadowRect, textFlags);

        SetTextColor(hMemDC, textColor);
        DrawTextW(hMemDC, ButtonText.c_str(), -1, &textRect, textFlags);

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
    if (!bVisible || !bEnabled) return;
    // Inventory callbacks can rebuild/delete this button. Keep the callable
    // alive until it returns and do not touch this widget afterwards.
    const auto callback = OnClickCallback;
    if (callback) callback();
}

void UButton::RightClick()
{
    if (!bVisible || !bEnabled) return;
    const auto callback = OnRightClickCallback;
    if (callback) callback();
}

UWidget* UButton::FindWidgetUnderMouse(int MouseX, int MouseY)
{
    return bEnabled ? UWidget::FindWidgetUnderMouse(MouseX, MouseY) : nullptr;
}

void UButton::UpdateMouseHover(int MouseX, int MouseY)
{
    UWidget::UpdateMouseHover(MouseX, MouseY);
    if (!bEnabled) IsPointed = false;
    if (!IsPointed && bWasPointed)
    {
        bWasPointed = false;
        const auto callback = OnUnhoverCallback;
        if (callback) callback();
    }
}
