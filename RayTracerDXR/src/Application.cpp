#include "Application.h"

#include <cassert>

Application* Application::Instance = nullptr;

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

void Application::Tick()
{
	GraphicsInterface->Tick();
}

Application::Application(int width, int height, HINSTANCE instance, const char* title)
	:MainWindow(std::make_unique<Window>(width, height, instance, title)),
	GraphicsInterface(std::make_unique<Graphics>(*MainWindow))
{
	assert(!Instance && "App instance not initialized");
	Instance = this;
	auto cursor = LoadCursor(nullptr, IDC_ARROW);
}