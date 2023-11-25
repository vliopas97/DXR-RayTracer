#include "Window.h"

#include "Application.h"
#include "Utils.h"

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nShowCmd)
{
	Application::Init(800, 600, GetModuleHandle(nullptr), "Direct X Ray Tracing");

	int msg{ 0 };
	EXCEPTION_WRAP(
		auto & application = Application::GetApp();
		msg = application.Run();
	);

	return msg;
}