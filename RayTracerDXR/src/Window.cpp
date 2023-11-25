#include "Window.h"

#include <cassert>
#include <cstdlib>

#define GEN_KEYBOARD_EVENT(Event)		{Event event(static_cast<uint8>(wParam));\
										EventCallback(event);}\
										break
#define GEN_KEYPRESSED_EVENT()		    {KeyPressedEvent event(static_cast<uint8>(wParam), static_cast<bool>(lParam & 0x40000000));\
										EventCallback(event);}\
										break

#define GEN_KEYRELEASED_EVENT() GEN_KEYBOARD_EVENT(KeyReleasedEvent)
#define GEN_KEYTYPED_EVENT() GEN_KEYBOARD_EVENT(KeyTypedEvent)

#define GEN_MOUSEENTER_EVENT() 		{SetCapture(Handle);\
									MouseEnterEvent event;\
									EventCallback(event);}

#define GEN_MOUSELEAVE_EVENT() 		{ReleaseCapture();\
									MouseLeaveEvent event;\
									EventCallback(event);}

#define GEN_MOUSERAW_EVENT(x, y) {MouseRawInputEvent event(x, y);\
								  EventCallback(event);}

Window::Window(int width, int height, HINSTANCE& instance, const char* title)
	:Width(width), Height(height)
{
	const size_t cSize = strlen(title) + 1;
	wchar_t* wTitle = new wchar_t[cSize];
	std::mbstowcs(wTitle, title, cSize);

	WNDCLASSEX wcex;
	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = InitializeWindow;
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
						  (rc.bottom - rc.top), NULL, NULL, instance, this);

	if (!Handle) assert(true && "Failed to create window");
	else
	{
		ShowWindow(Handle, SW_SHOWDEFAULT);
		//UpdateWindow(Handle);
	}

	RAWINPUTDEVICE rawInput{};
	rawInput.usUsagePage = 0x01;
	rawInput.usUsage = 0x02;
	rawInput.dwFlags = 0;
	rawInput.hwndTarget = nullptr;
	if (RegisterRawInputDevices(&rawInput, 1, sizeof(RAWINPUTDEVICE)) == FALSE)
		assert(false && "Failed to register raw input devices");
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

void Window::OnEvent(Event& event)
{
	Input.OnEvent(event);
}

void Window::SetEventCallbackFunction(const EventCallbackFn& fn)
{
	EventCallback = fn;
}

void Window::ShowCursor()
{
	ShowCursorImpl();
	FreeCursor();
}

void Window::HideCursor()
{
	HideCursorImpl();
	TrapCursor();
}

bool Window::IsCursorVisible() const
{
	return CursorVisibility;
}

void Window::Tick(float delta)
{
	Input.FlushRawInputBuffer();
}

LRESULT Window::InitializeWindow(HWND windowHandle, UINT message, WPARAM wParam, LPARAM lParam)
{
	if (message == WM_CREATE)
	{
		const CREATESTRUCTW* createStruct = reinterpret_cast<CREATESTRUCT*>(lParam);
		Window* const windowPtr = static_cast<Window*>(createStruct->lpCreateParams);
		SetWindowLongPtr(windowHandle, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(windowPtr));
		SetWindowLongPtr(windowHandle, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(WindProc));

		return windowPtr->WindProcImpl(windowHandle, message, wParam, lParam);
	}
	return DefWindowProc(windowHandle, message, wParam, lParam);
}

LRESULT CALLBACK Window::WindProc(HWND windowHandle, UINT message, WPARAM wParam, LPARAM lParam)
{
	Window* const windowPtr = reinterpret_cast<Window*>(GetWindowLongPtr(windowHandle, GWLP_USERDATA));
	return windowPtr->WindProcImpl(windowHandle, message, wParam, lParam);
}

LRESULT Window::WindProcImpl(HWND windowHandle, UINT message, WPARAM wParam, LPARAM lParam)
{
	PAINTSTRUCT pStruct;
	switch (message)
	{
		// Window Events
	case WM_PAINT:
		BeginPaint(windowHandle, &pStruct);
		EndPaint(windowHandle, &pStruct);
		break;
	case WM_CLOSE:
		PostQuitMessage(WM_CLOSE);
		break;
#if ENABLE_EVENTS
	case WM_ACTIVATE:
	{
		if (!IsCursorVisible())
		{
			if (wParam & WA_ACTIVE)
			{
				TrapCursor();
				HideCursor();
			}
			else
			{
				FreeCursor();
				ShowCursor();
			}
		}
		break;
	}

	// Keyboard Events
	case WM_KEYDOWN:
	case WM_SYSKEYDOWN:
	{
		GEN_KEYPRESSED_EVENT();
	}
	case WM_CHAR:
	{
		GEN_KEYTYPED_EVENT();
	}
	case WM_KEYUP:
	case WM_SYSKEYUP:
		GEN_KEYRELEASED_EVENT();

		// Mouse Events
	case WM_MOUSEMOVE:
	{
		if (!IsCursorVisible())
		{
			if (Input.IsMouseInWindow())
			{
				SetCapture(Handle);
				GEN_MOUSEENTER_EVENT();
				HideCursor();
			}
		}

		const POINTS points = MAKEPOINTS(lParam);

		if ((points.x >= 0 || points.x < Width || points.y >= 0 || points.y < Height))
		{
			MouseMovedEvent event(points.x, points.y);
			EventCallback(event);
			if (Input.IsMouseInWindow())
				GEN_MOUSEENTER_EVENT();
		}
		else
		{
			if (wParam & (MK_LBUTTON | MK_RBUTTON))
			{
				MouseMovedEvent event(points.x, points.y);
				EventCallback(event);
			}
			else
				GEN_MOUSELEAVE_EVENT();
		}
	}
	break;
	case WM_MOUSEWHEEL:
	{
		POINTS point = MAKEPOINTS(lParam);
		MouseScrolledEvent event(point.x, point.y, GET_WHEEL_DELTA_WPARAM(wParam));
		EventCallback(event);
	}
	break;

	// Raw Mouse
	case WM_INPUT:
	{
		if (!Input.IsRawInputEnabled())
			break;

		UINT size{ 0 };
		if (GetRawInputData(reinterpret_cast<HRAWINPUT>(lParam), RID_INPUT, nullptr, &size,
							sizeof(RAWINPUTHEADER)) == -1)
			break;

		std::vector<BYTE> buffer{};
		buffer.resize(size);

		if (GetRawInputData(reinterpret_cast<HRAWINPUT>(lParam),
							RID_INPUT, buffer.data(), &size, sizeof(RAWINPUTHEADER)) != size)
			break;

		auto& rawInput = reinterpret_cast<const RAWINPUT&>(*buffer.data());
		if (rawInput.header.dwType == RIM_TYPEMOUSE &&
			(rawInput.data.mouse.lLastX != 0 || rawInput.data.mouse.lLastY != 0))
			GEN_MOUSERAW_EVENT(rawInput.data.mouse.lLastX, rawInput.data.mouse.lLastY);
		break;
	}
#endif // ENABLE_KEYBOARD_EVENTS
	default:
		break;
	}

	return DefWindowProc(windowHandle, message, wParam, lParam);
}

inline void Window::ShowCursorImpl()
{
	CursorVisibility = true;
	while (::ShowCursor(TRUE) < 0);
}

inline void Window::HideCursorImpl()
{
	CursorVisibility = false;
	while (::ShowCursor(FALSE) >= 0);
}

void Window::TrapCursor()
{
	RECT rectangle;
	GetClientRect(Handle, &rectangle);
	MapWindowPoints(Handle, nullptr, reinterpret_cast<POINT*>(&rectangle), 2);
	ClipCursor(&rectangle);
}

void Window::FreeCursor()
{
	ClipCursor(nullptr);
}