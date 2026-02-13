#include "platform/Win32Window.h"
#include <windows.h>

int main()
{
    HINSTANCE hInstance = GetModuleHandleW(nullptr);

    Win32Window window(hInstance, 800, 600, L"Hello, Windows");

    while (window.PumpMessages())
    {
        // idle / later: render loop
        Sleep(16);
    }
    return 0;
}
//force change