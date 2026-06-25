#ifndef UNICODE
#define UNICODE
#endif

#ifndef _UNICODE
#define _UNICODE
#endif

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include <windows.h>

// Clean Standard Window Procedure
LRESULT CALLBACK WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_DESTROY: {
            PostQuitMessage(0);
            return 0;
        }
        default:
            return DefWindowProcW(hwnd, uMsg, wParam, lParam);
    }
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    // Avoid compiler warnings for unused parameters in native applications
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    const wchar_t CLASS_NAME[] = L"NoticeMeNotificationWindowClass";
    const int WINDOW_WIDTH = 300;
    const int WINDOW_HEIGHT = 150;
    const int MARGIN = 15;

    // Create a solid dark background brush (RGB 30, 30, 30 - standard dark theme)
    HBRUSH hBackgroundBrush = CreateSolidBrush(RGB(30, 30, 30));
    if (!hBackgroundBrush) {
        return -1;
    }

    // Register custom Window Class using modern Unicode RegisterClassExW
    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    wc.hbrBackground = hBackgroundBrush;
    wc.lpszClassName = CLASS_NAME;

    if (!RegisterClassExW(&wc)) {
        DeleteObject(hBackgroundBrush);
        return -1;
    }

    // Query work area size to position the window at the bottom-right corner (accounts for taskbar location/size)
    RECT workArea = {};
    int x = 100;
    int y = 100;
    if (SystemParametersInfoW(SPI_GETWORKAREA, 0, &workArea, 0)) {
        x = workArea.right - WINDOW_WIDTH - MARGIN;
        y = workArea.bottom - WINDOW_HEIGHT - MARGIN;
    } else {
        // Fallback to absolute screen size if system call fails
        x = GetSystemMetrics(SM_CXSCREEN) - WINDOW_WIDTH - MARGIN;
        y = GetSystemMetrics(SM_CYSCREEN) - WINDOW_HEIGHT - MARGIN;
    }

    // Create the lightweight floating overlay window
    HWND hwnd = CreateWindowExW(
        WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_TOOLWINDOW,
        CLASS_NAME,
        L"noticeMe Overlay",
        WS_POPUP, // Borderless, captionless, ideal for floating overlays
        x, y, WINDOW_WIDTH, WINDOW_HEIGHT,
        nullptr, nullptr, hInstance, nullptr
    );

    if (!hwnd) {
        DeleteObject(hBackgroundBrush);
        return -1;
    }

    // Set window opacity (255 is fully opaque)
    if (!SetLayeredWindowAttributes(hwnd, 0, 255, LWA_ALPHA)) {
        DestroyWindow(hwnd);
        DeleteObject(hBackgroundBrush);
        return -1;
    }

    // Display the overlay
    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    // Standard Win32 Message Loop
    MSG msg = {};
    while (GetMessageW(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    // Clean up GDI brush resources to prevent GDI leaks
    DeleteObject(hBackgroundBrush);

    return static_cast<int>(msg.wParam);
}
