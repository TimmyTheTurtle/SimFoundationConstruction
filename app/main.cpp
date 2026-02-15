#include "platform/Win32Window.h"
#include "gfx/Dx11Device.h"
#include <windows.h>

int main()
{
	HINSTANCE hInstance = GetModuleHandleW(nullptr);

	const int w = 800, h = 600;
	Win32Window window(hInstance, w, h, L"DX11 Resize Test");

	Dx11Device dx(window.hwnd(), w, h);

	window.SetOnResize([&](int newW, int newH)
		{
			dx.Resize(newW, newH);
			dx.Clear(0.1f, 0.2f, 0.4f, 1.0f);
			dx.Present(true);
		});

	while (window.PumpMessages())
	{
		dx.Clear(0.1f, 0.2f, 0.4f, 1.0f);
		dx.Present(true);
		//Sleep(1);
	}
	return 0;
}