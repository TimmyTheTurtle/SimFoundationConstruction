#pragma once
#include <windows.h>
#include <string>

class Win32Window
{
public:
    Win32Window(HINSTANCE hInstance, int width, int height, const wchar_t* title);
    ~Win32Window();

    Win32Window(const Win32Window&) = delete;
    Win32Window& operator=(const Win32Window&) = delete;

    HWND hwnd() const { return m_hwnd; }
    bool PumpMessages(); // returns false when quit requested

private:
    static LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

    HINSTANCE m_hInstance{};
    HWND      m_hwnd{};
};
