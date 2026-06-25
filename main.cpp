#ifndef UNICODE
#define UNICODE
#endif

#ifndef _UNICODE
#define _UNICODE
#endif

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include <windows.h>
#include <dwmapi.h>
#include <shellapi.h>

#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "kernel32.lib")
#pragma comment(lib, "shell32.lib")

#ifndef DWMWA_WINDOW_CORNER_PREFERENCE
#define DWMWA_WINDOW_CORNER_PREFERENCE 33
#endif

#ifndef DWMWCP_ROUND
#define DWMWCP_ROUND 2
#endif

#define WM_TRAYICON (WM_USER + 1)
#define IDI_TRAY 1

#define ID_TRAY_SHOW 3001
#define ID_TRAY_EXIT 3002

#define TIMER_DISPLAY    1001
#define TIMER_AUTOHIDE   1002
#define TIMER_FADE_IN    1003
#define TIMER_FADE_OUT   1004

#define FADE_STEP   17
#define FADE_INTERVAL 10

static const wchar_t* HEALTH_TIPS[] = {
    L"Time to stand up and stretch!",
    L"Drink a glass of water!",
    L"Rest your eyes for 20 seconds",
    L"Take a deep breath!",
    L"Walk around for 2 minutes",
    L"Check your posture",
    L"Do some neck stretches",
    L"Blink your eyes several times"
};

static const int HEALTH_TIP_COUNT = sizeof(HEALTH_TIPS) / sizeof(HEALTH_TIPS[0]);

static wchar_t g_currentTip[128] = L"Time to stand up and stretch!";
static BYTE    g_alpha = 255;
static HWND    g_hwnd = nullptr;
static NOTIFYICONDATAW g_nid = {};
static UINT g_wmTaskbarCreated = 0;

static int GetRandomIndex(int count) {
    static unsigned int seed = (unsigned int)GetTickCount64();
    seed = seed * 1103515245 + 12345;
    return (int)((seed >> 16) % count);
}

static void PickRandomTip() {
    int idx = GetRandomIndex(HEALTH_TIP_COUNT);
    const wchar_t* src = HEALTH_TIPS[idx];
    int i = 0;
    while (src[i] && i < 126) {
        g_currentTip[i] = src[i];
        i++;
    }
    g_currentTip[i] = L'\0';
}

static void FadeIn(HWND hwnd) {
    g_alpha = 0;
    ShowWindow(hwnd, SW_SHOW);
    SetLayeredWindowAttributes(hwnd, 0, 0, LWA_ALPHA);
    SetTimer(hwnd, TIMER_FADE_IN, FADE_INTERVAL, NULL);
}

static void FadeOut(HWND hwnd) {
    g_alpha = 255;
    SetLayeredWindowAttributes(hwnd, 0, 255, LWA_ALPHA);
    SetTimer(hwnd, TIMER_FADE_OUT, FADE_INTERVAL, NULL);
}

static void HideImmediately(HWND hwnd) {
    KillTimer(hwnd, TIMER_FADE_IN);
    KillTimer(hwnd, TIMER_FADE_OUT);
    KillTimer(hwnd, TIMER_AUTOHIDE);
    g_alpha = 0;
    SetLayeredWindowAttributes(hwnd, 0, 0, LWA_ALPHA);
    ShowWindow(hwnd, SW_HIDE);
}

static void ShowNotification(HWND hwnd) {
    PickRandomTip();
    FadeIn(hwnd);
    KillTimer(hwnd, TIMER_AUTOHIDE);
    SetTimer(hwnd, TIMER_AUTOHIDE, 15000, NULL);
}

static void AddTrayIcon(HWND hwnd) {
    g_nid.cbSize = sizeof(NOTIFYICONDATAW);
    g_nid.hWnd = hwnd;
    g_nid.uID = IDI_TRAY;
    g_nid.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
    g_nid.uCallbackMessage = WM_TRAYICON;
    g_nid.hIcon = LoadIconW(nullptr, IDI_APPLICATION);
    wcscpy_s(g_nid.szTip, L"noticeMe - Health Notification");
    Shell_NotifyIconW(NIM_ADD, &g_nid);
}

static void RemoveTrayIcon() {
    Shell_NotifyIconW(NIM_DELETE, &g_nid);
}

static void ShowContextMenu(HWND hwnd) {
    POINT pt;
    GetCursorPos(&pt);

    HMENU hMenu = CreatePopupMenu();
    AppendMenuW(hMenu, MF_STRING, ID_TRAY_SHOW, L"Show Notification");
    AppendMenuW(hMenu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(hMenu, MF_STRING, ID_TRAY_EXIT, L"Exit");

    SetForegroundWindow(hwnd);
    TrackPopupMenu(hMenu, TPM_RIGHTBUTTON, pt.x, pt.y, 0, hwnd, nullptr);
    PostMessageW(hwnd, WM_NULL, 0, 0);
    DestroyMenu(hMenu);
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    if (uMsg == g_wmTaskbarCreated && g_wmTaskbarCreated != 0) {
        AddTrayIcon(hwnd);
        return 0;
    }

    switch (uMsg) {
        case WM_CREATE: {
            int cornerPreference = DWMWCP_ROUND;
            DwmSetWindowAttribute(hwnd, DWMWA_WINDOW_CORNER_PREFERENCE, &cornerPreference, sizeof(cornerPreference));
            AddTrayIcon(hwnd);
            return 0;
        }

        case WM_TRAYICON: {
            if (lParam == WM_LBUTTONUP) {
                if (IsWindowVisible(hwnd)) {
                    FadeOut(hwnd);
                } else {
                    ShowNotification(hwnd);
                }
            } else if (lParam == WM_RBUTTONUP) {
                ShowContextMenu(hwnd);
            }
            return 0;
        }

        case WM_COMMAND: {
            if (LOWORD(wParam) == ID_TRAY_SHOW) {
                ShowNotification(hwnd);
            } else if (LOWORD(wParam) == ID_TRAY_EXIT) {
                DestroyWindow(hwnd);
            }
            return 0;
        }

        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);

            HBRUSH hBrush = CreateSolidBrush(RGB(30, 30, 30));
            FillRect(hdc, &ps.rcPaint, hBrush);
            DeleteObject(hBrush);

            SetBkMode(hdc, TRANSPARENT);
            SetTextColor(hdc, RGB(255, 255, 255));

            RECT rect;
            GetClientRect(hwnd, &rect);

            DrawTextW(hdc, g_currentTip, -1, &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

            EndPaint(hwnd, &ps);
            return 0;
        }

        case WM_LBUTTONDOWN: {
            FadeOut(hwnd);
            return 0;
        }

        case WM_TIMER: {
            if (wParam == TIMER_DISPLAY) {
                ShowNotification(hwnd);
                return 0;
            }

            if (wParam == TIMER_AUTOHIDE) {
                KillTimer(hwnd, TIMER_AUTOHIDE);
                FadeOut(hwnd);
                return 0;
            }

            if (wParam == TIMER_FADE_IN) {
                g_alpha = (BYTE)(g_alpha + FADE_STEP);
                if (g_alpha >= 255) {
                    g_alpha = 255;
                    KillTimer(hwnd, TIMER_FADE_IN);
                }
                SetLayeredWindowAttributes(hwnd, 0, g_alpha, LWA_ALPHA);
                return 0;
            }

            if (wParam == TIMER_FADE_OUT) {
                if (g_alpha > FADE_STEP) {
                    g_alpha = (BYTE)(g_alpha - FADE_STEP);
                } else {
                    g_alpha = 0;
                }
                SetLayeredWindowAttributes(hwnd, 0, g_alpha, LWA_ALPHA);
                if (g_alpha == 0) {
                    KillTimer(hwnd, TIMER_FADE_OUT);
                    KillTimer(hwnd, TIMER_AUTOHIDE);
                    ShowWindow(hwnd, SW_HIDE);
                }
                return 0;
            }

            return DefWindowProcW(hwnd, uMsg, wParam, lParam);
        }

        case WM_DESTROY: {
            HideImmediately(hwnd);
            KillTimer(hwnd, TIMER_DISPLAY);
            RemoveTrayIcon();
            PostQuitMessage(0);
            return 0;
        }

        default:
            return DefWindowProcW(hwnd, uMsg, wParam, lParam);
    }
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    HANDLE hMutex = CreateMutexW(NULL, TRUE, L"noticeMeSingleInstanceMutex");
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        return 0;
    }

    g_wmTaskbarCreated = RegisterWindowMessageW(L"TaskbarCreated");

    const wchar_t CLASS_NAME[] = L"NoticeMeNotificationWindowClass";

    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    wc.lpszClassName = CLASS_NAME;

    if (!RegisterClassExW(&wc)) {
        ReleaseMutex(hMutex);
        CloseHandle(hMutex);
        return -1;
    }

    const int width = 350;
    const int height = 150;

    RECT workArea = {};
    int x = 100;
    int y = 100;
    if (SystemParametersInfoW(SPI_GETWORKAREA, 0, &workArea, 0)) {
        x = workArea.right - width - 20;
        y = workArea.bottom - height - 20;
    } else {
        x = GetSystemMetrics(SM_CXSCREEN) - width - 20;
        y = GetSystemMetrics(SM_CYSCREEN) - height - 20;
    }

    HWND hwnd = CreateWindowExW(
        WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_TOOLWINDOW,
        CLASS_NAME,
        L"noticeMe Overlay",
        WS_POPUP,
        x, y, width, height,
        nullptr, nullptr, hInstance, nullptr
    );

    if (!hwnd) {
        ReleaseMutex(hMutex);
        CloseHandle(hMutex);
        return -1;
    }

    g_hwnd = hwnd;
    SetLayeredWindowAttributes(hwnd, 0, 0, LWA_ALPHA);

    PickRandomTip();

    SetTimer(hwnd, TIMER_DISPLAY, 3600000, NULL);
    // SetTimer(hwnd, TIMER_DISPLAY, 10000, NULL); // 10 seconds for testing

    FadeIn(hwnd);
    SetTimer(hwnd, TIMER_AUTOHIDE, 15000, NULL);

    MSG msg = {};
    while (GetMessageW(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    ReleaseMutex(hMutex);
    CloseHandle(hMutex);

    return static_cast<int>(msg.wParam);
}
