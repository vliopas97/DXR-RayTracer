#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include "Events/Input.h"
#include <Windows.h>
#include <functional>
#include <optional>

struct Window
{
	using EventCallbackFn = std::function<void(Event&)>;
	Window(int width, int height, HINSTANCE& instance, const char* title);

	static std::optional<int> ProcessMessages();
	
	void OnEvent(Event& event);
	void SetEventCallbackFunction(const EventCallbackFn& fn);

	void ShowCursor();
	void HideCursor();
	bool IsCursorVisible() const;

	void Tick(float delta);

private:
	static LRESULT CALLBACK InitializeWindow(HWND windowHandle, UINT message, WPARAM wParam, LPARAM lParam);
	static LRESULT CALLBACK WindProc(HWND windowHandle, UINT message, WPARAM wParam, LPARAM lParam);
	LRESULT CALLBACK WindProcImpl(HWND windowHandle, UINT message, WPARAM wParam, LPARAM lParam);

	void ShowCursorImpl();
	void HideCursorImpl();
	void TrapCursor();
	void FreeCursor();

public:

	HWND Handle;
	int Width, Height;
	InputManager Input;

private:
	EventCallbackFn EventCallback;
	bool CursorVisibility = false;
};