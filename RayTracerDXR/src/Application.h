#pragma once

#include <Window.h>
#include <memory>

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

	static Application* Instance;
};