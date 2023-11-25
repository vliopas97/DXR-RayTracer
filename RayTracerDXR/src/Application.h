#pragma once

#include <Window.h>
#include <chrono>
#include <memory>

struct Timer
{
	Timer();
	virtual ~Timer() = default;

	float Get();
	float GetAndReset();
protected:
	std::chrono::steady_clock::time_point Start;
};

#include "Graphics.h"

class Application
{
public:
	static Application& GetApp();
	int Run();
	static void Init(int width, int height, HINSTANCE instance, const char* title);

	const std::unique_ptr<Window>& GetWindow() { return MainWindow; }
	void OnEvent(Event& e);

private:
	Application(int width, int height, HINSTANCE instance, const char* title);
	Application(const Application&) = delete;
	Application& operator=(const Application&) = delete;

	void Tick();
private:
	std::unique_ptr<Window> MainWindow;
	std::unique_ptr<Graphics> GraphicsInterface;

	static Application* Instance;
	Timer Benchmarker;
};