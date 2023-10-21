#include "Window.h"

#include <cassert>
#include <cstdlib>

LRESULT WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	PAINTSTRUCT pStruct;
	switch (message)
	{
	case WM_PAINT:
		BeginPaint(hWnd, &pStruct);
		EndPaint(hWnd, &pStruct);
		break;
	case WM_KEYUP:
		if (wParam == VK_ESCAPE) PostQuitMessage(0);
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}

	return 0;
}

Window::Window(int width, int height, HINSTANCE& instance, const char* title)
	:Width(width), Height(height)
{
	const size_t cSize = strlen(title) + 1;
	wchar_t* wTitle = new wchar_t[cSize];
	std::mbstowcs(wTitle, title, cSize);

	WNDCLASSEX wcex;
	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = instance;
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszMenuName = NULL;
	wcex.lpszClassName = L"WindowClass";
	wcex.hIcon = nullptr;
	wcex.hIconSm = nullptr;

	if (!RegisterClassEx(&wcex))
		assert(true && "Failed to register window");

	RECT desktop;
	const HWND desktopHandle = GetDesktopWindow();
	GetWindowRect(desktopHandle, &desktop);

	int x = (desktop.right - width) / 2;

	RECT rc = { 0, 0, width, height };
	AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);
	Handle = CreateWindow(wcex.lpszClassName, wTitle, WS_OVERLAPPEDWINDOW, x, 200, (rc.right - rc.left),
						  (rc.bottom - rc.top), NULL, NULL, instance, NULL);

	if (!Handle) assert(true && "Failed to create window");
	else
	{
		ShowWindow(Handle, SW_SHOWDEFAULT);
		UpdateWindow(Handle);
	}
}

std::optional<int> Window::ProcessMessages()
{
	MSG msg;
	while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
	{
		if (msg.message == WM_QUIT)
			return msg.wParam;

		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return std::nullopt;
}
