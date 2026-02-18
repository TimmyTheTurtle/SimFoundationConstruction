#include "platform/Win32Window.h"

static const wchar_t* kClassName = L"CMake1WindowClass";

Win32Window::Win32Window(HINSTANCE hInstance, int width, int height, const wchar_t* title)
    : m_hInstance(hInstance)
{
    WNDCLASSW wc{};
    wc.lpfnWndProc = Win32Window::WndProc;
    wc.hInstance = m_hInstance;
    wc.lpszClassName = kClassName;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);

    RegisterClassW(&wc);

    DWORD style = WS_OVERLAPPEDWINDOW;

    RECT r{ 0, 0, width, height };
    AdjustWindowRect(&r, style, FALSE);

    m_hwnd = CreateWindowExW(
        0,
        kClassName,
        title,
        style,
        CW_USEDEFAULT, CW_USEDEFAULT,
        r.right - r.left,
        r.bottom - r.top,
        nullptr, nullptr,
        m_hInstance,
        this);

    ShowWindow(m_hwnd, SW_SHOW);
}

Win32Window::~Win32Window()
{
    if (m_hwnd)
        DestroyWindow(m_hwnd);
}

bool Win32Window::PumpMessages()
{
    MSG msg{};
    while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE))
    {
        if (msg.message == WM_QUIT)
            return false;
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    return true;
}

LRESULT CALLBACK Win32Window::WndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    Win32Window* self = nullptr;

    if (msg == WM_NCCREATE)
    {
        auto cs = reinterpret_cast<CREATESTRUCTW*>(lparam);
        self = reinterpret_cast<Win32Window*>(cs->lpCreateParams);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
    }
    else
    {
        self = reinterpret_cast<Win32Window*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    }

    switch (msg)
    {
    case WM_SIZE:
        if (self && wparam != SIZE_MINIMIZED)
        {
            int width = LOWORD(lparam);
            int height = HIWORD(lparam);

            if (self->m_onResize) {
                self->m_onResize(width, height);
            }
        }
        return 0;

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProcW(hwnd, msg, wparam, lparam);
}

