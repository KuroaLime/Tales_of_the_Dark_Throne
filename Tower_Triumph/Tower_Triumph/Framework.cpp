#include <windows.h>
#include <tchar.h>
#include <random>
#include <deque>
#include <vector>
#include <ctime>
#include <string>

#include "CStage.h"
#include "Title_Stage.h"
#include "Map_Stage.h"
#include "Battle_Stage.h"
#include "Maintenance_Stage.h"

#pragma comment(lib, "msimg32.lib")

namespace
{
    // Only the portable layout and this solution's known build layouts are checked.
    const wchar_t* const kExecutableAssetSuffixes[] = {
        L"", L"\\..\\..\\Tower_Triumph", L"\\..\\Tower_Triumph", L"\\..\\..", L"\\.."
    };
    const wchar_t* const kWorkingAssetSuffixes[] = {
        L"", L"\\Tower_Triumph", L"\\Tower_Triumph\\Tower_Triumph"
    };
    const wchar_t* const kAssetRootMarkers[] = {
        L"\\Resource\\Warrior_IdlePose.bmp", L"\\Resource\\Mage_IdlePose.bmp"
    };

    bool IsRuntimeAssetRoot(const std::wstring& directory)
    {
        for (const wchar_t* marker : kAssetRootMarkers)
        {
            const DWORD attributes = GetFileAttributesW((directory + marker).c_str());
            if (attributes == INVALID_FILE_ATTRIBUTES || (attributes & FILE_ATTRIBUTE_DIRECTORY))
            {
                return false;
            }
        }
        return true;
    }

    std::wstring FindRuntimeAssetRoot(const std::wstring& base,
        const wchar_t* const* suffixes, size_t count)
    {
        if (base.empty())
        {
            return {};
        }
        for (size_t index = 0; index < count; ++index)
        {
            wchar_t candidate[32768];
            const DWORD length = GetFullPathNameW((base + suffixes[index]).c_str(),
                static_cast<DWORD>(_countof(candidate)), candidate, nullptr);
            if (length != 0 && length < _countof(candidate) && IsRuntimeAssetRoot(candidate))
            {
                return candidate;
            }
        }
        return {};
    }

    void InitializeRuntimeAssetRoot()
    {
        wchar_t path[32768];
        const DWORD executableLength = GetModuleFileNameW(nullptr, path,
            static_cast<DWORD>(_countof(path)));
        std::wstring executableDirectory;
        if (executableLength != 0 && executableLength < _countof(path))
        {
            const std::wstring executablePath(path, executableLength);
            const size_t separator = executablePath.find_last_of(L"\\/");
            if (separator != std::wstring::npos)
            {
                executableDirectory = executablePath.substr(0, separator + 1);
            }
        }

        std::wstring root = FindRuntimeAssetRoot(executableDirectory,
            kExecutableAssetSuffixes, _countof(kExecutableAssetSuffixes));
        if (root.empty())
        {
            const DWORD workingLength = GetCurrentDirectoryW(static_cast<DWORD>(_countof(path)), path);
            if (workingLength != 0 && workingLength < _countof(path))
            {
                root = FindRuntimeAssetRoot(path, kWorkingAssetSuffixes, _countof(kWorkingAssetSuffixes));
            }
        }
        if (root.empty())
        {
            OutputDebugStringW(L"Runtime assets: no verified asset root found; keeping working directory.\n");
            return;
        }
        if (!SetCurrentDirectoryW(root.c_str()))
        {
            OutputDebugStringW(L"Runtime assets: could not select asset root; keeping working directory.\n");
            return;
        }
        OutputDebugStringW((L"Runtime assets: " + root + L"\n").c_str());
    }
}

HINSTANCE g_hInst;

int Win_x = 1280;
int Win_y = 720;

LPCTSTR lpszClass = L"My Window Class";
LPCTSTR lpszWindowName = L"Window Programming 2";

LRESULT CALLBACK WndProc(HWND hWnd, UINT iMessage, WPARAM wParam, LPARAM lParam);

CStage* g_Stage = nullptr;
int g_curStageNum = 1000;

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpszCmdParam, int nCmdShow)
{
    InitializeRuntimeAssetRoot();

    HWND hWnd;
    MSG Message;
    WNDCLASSEX WndClass;

    g_hInst = hInstance;

    WndClass.cbSize = sizeof(WndClass);
    WndClass.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
    WndClass.lpfnWndProc = (WNDPROC)WndProc;
    WndClass.cbClsExtra = 0;
    WndClass.cbWndExtra = 0;
    WndClass.hInstance = hInstance;
    WndClass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    WndClass.hCursor = LoadCursor(NULL, IDC_ARROW);
    WndClass.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
    WndClass.lpszMenuName = NULL;
    WndClass.lpszClassName = lpszClass;
    WndClass.hIconSm = LoadIcon(NULL, IDI_APPLICATION);

    RegisterClassEx(&WndClass);

    hWnd = CreateWindow(
        lpszClass,
        lpszWindowName,
        WS_SYSMENU | WS_MAXIMIZEBOX | WS_MINIMIZEBOX | WS_THICKFRAME | WS_HSCROLL | WS_VSCROLL,
        100,
        50,
        Win_x,
        Win_y,
        NULL,
        (HMENU)NULL,
        hInstance,
        NULL
    );

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    while (GetMessage(&Message, 0, 0, 0))
    {
        TranslateMessage(&Message);
        DispatchMessage(&Message);
    }

    return (int)Message.wParam;
}

void ChangeStage(HWND hWnd, int NextStage)
{
    if (g_curStageNum == NextStage)
    {
        return;
    }

    CStage* nextStage = nullptr;

    if (NextStage == TITLE_STAGE)
    {
        nextStage = new Title_Stage;
    }
    else if (NextStage == MAP_STAGE)
    {
        nextStage = new Map_Stage;
    }
    else if (NextStage == BATTLE_STAGE)
    {
        nextStage = new Battle_Stage;
    }
    else if (NextStage == MAINTENANCE_STAGE)
    {
        nextStage = new Maintenance_Stage;
    }
    else if (NextStage == GAME_END)
    {
        SendMessage(hWnd, WM_CLOSE, 0, 0);
        return;
    }
    else
    {
        return;
    }

    delete g_Stage;
    g_Stage = nextStage;
    g_curStageNum = NextStage;

    if (g_Stage != nullptr)
    {
        g_Stage->Init(hWnd, g_hInst);
    }
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    PAINTSTRUCT ps;
    HDC hDC;
    HDC hMemDC;
    HDC hMemDC1;

    static HBITMAP hBit = NULL;
    static RECT rects{};

    switch (uMsg)
    {
    case WM_CREATE:
    {
        hDC = GetDC(hWnd);

        srand((unsigned int)time(NULL));
        GetClientRect(hWnd, &rects);

        hBit = CreateCompatibleBitmap(hDC, rects.right, rects.bottom);

        ChangeStage(hWnd, TITLE_STAGE);

        SetTimer(hWnd, 1, 16, NULL);
        ReleaseDC(hWnd, hDC);
        break;
    }

    case WM_LBUTTONDOWN:
    case WM_LBUTTONUP:
    case WM_RBUTTONDOWN:
    case WM_RBUTTONUP:
    case WM_LBUTTONDBLCLK:
    case WM_MOUSEMOVE:
    case WM_KEYDOWN:
    case WM_CHAR:
    {
        if (g_Stage != nullptr)
        {
            int NextStage = g_Stage->Input(hWnd, uMsg, wParam, lParam);

            if (NextStage != -2)
            {
                ChangeStage(hWnd, NextStage);
            }
        }

        break;
    }

    case WM_COMMAND:
        break;

    case WM_TIMER:
    {
        hDC = GetDC(hWnd);
        hMemDC = CreateCompatibleDC(hDC);
        hMemDC1 = CreateCompatibleDC(hDC);

        HBITMAP hOldBit = (HBITMAP)SelectObject(hMemDC, hBit);

        HBRUSH hBrush = CreateSolidBrush(RGB(255, 255, 255));
        FillRect(hMemDC, &rects, hBrush);
        DeleteObject(hBrush);

        if (g_Stage != nullptr)
        {
            g_Stage->Update(hWnd);
            g_Stage->Draw(hMemDC, hMemDC1);
        }

        SelectObject(hMemDC, hOldBit);

        DeleteDC(hMemDC);
        DeleteDC(hMemDC1);
        ReleaseDC(hWnd, hDC);

        InvalidateRect(hWnd, NULL, FALSE);
        break;
    }

    case WM_PAINT:
    {
        hDC = BeginPaint(hWnd, &ps);
        hMemDC = CreateCompatibleDC(hDC);

        HBITMAP hOld = (HBITMAP)SelectObject(hMemDC, hBit);

        BitBlt(
            hDC,
            0,
            0,
            rects.right,
            rects.bottom,
            hMemDC,
            0,
            0,
            SRCCOPY
        );

        SelectObject(hMemDC, hOld);
        DeleteDC(hMemDC);

        EndPaint(hWnd, &ps);
        return 0;
    }

    case WM_DESTROY:
    {
        KillTimer(hWnd, 1);

        delete g_Stage;
        g_Stage = nullptr;

        if (hBit != NULL)
        {
            DeleteObject(hBit);
            hBit = NULL;
        }

        PostQuitMessage(0);
        break;
    }
    }

    return DefWindowProc(hWnd, uMsg, wParam, lParam);
}
