#pragma once

#include <Window.h>
#include <memory>

#include "Graphics.h"

class Application
{
public:
	static Application& GetApp();
	int Run();
	static void Init(int width, int height, HINSTANCE instance, const char* title);

private:
	Application(int width, int height, HINSTANCE instance, const char* title);
	Application(const Application&) = delete;
	Application& operator=(const Application&) = delete;

	void Tick();
private:
	std::unique_ptr<Window> MainWindow;
	std::unique_ptr<Graphics> GraphicsInterface;

	static Application* Instance;
};