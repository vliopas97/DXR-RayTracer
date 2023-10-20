#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <Windows.h>
#include <optional>

struct Window
{
	Window(int width, int height, HINSTANCE& instance, const char* title);

	static std::optional<int> ProcessMessages();

	HWND window;
};