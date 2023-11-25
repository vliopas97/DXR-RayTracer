#include "Application.h"

#include <cassert>

Application* Application::Instance = nullptr;

Timer::Timer()
{
	Start = std::chrono::steady_clock::now();
}

float Timer::Get()
{
	return std::chrono::duration<float>(std::chrono::steady_clock::now() - Start).count();
}

float Timer::GetAndReset()
{
	auto temp = Start;
	Start = std::chrono::steady_clock::now();
	return  std::chrono::duration<float>(std::chrono::steady_clock::now() - temp).count();
}

Application& Application::GetApp()
{
	assert(Instance && "App instance not initialized");
	return *Instance;
}

int Application::Run()
{
	while (true)
	{
		if (const auto ecode = Window::ProcessMessages())
			return *ecode;
		Tick();
	}
}

void Application::Init(int width, int height, HINSTANCE instance, const char* title)
{
	Instance = new Application(width, height, instance, title);
}

void Application::OnEvent(Event& e)
{
	MainWindow->OnEvent(e);
}

void Application::Tick()
{
	float delta = Benchmarker.GetAndReset();
	GraphicsInterface->Tick(delta);

	if (MainWindow->Input.IsKeyPressed(VK_F1))
	{
		if (MainWindow->IsCursorVisible())
		{
			MainWindow->HideCursor();
			MainWindow->Input.SetRawInput(true);
		}
		else
		{
			MainWindow->ShowCursor();
			MainWindow->Input.SetRawInput(false);
		}
	}
	if (MainWindow->Input.IsKeyPressed(VK_ESCAPE))
		PostQuitMessage(0);
}

Application::Application(int width, int height, HINSTANCE instance, const char* title)
	:MainWindow(std::make_unique<Window>(width, height, instance, title))
{
	assert(!Instance && "App instance not initialized");
	Instance = this;
	auto cursor = LoadCursor(nullptr, IDC_ARROW);

	MainWindow->SetEventCallbackFunction([this](Event& e)
										 {
											 OnEvent(e);
										 });
	GraphicsInterface = std::make_unique<Graphics>(*MainWindow);
}