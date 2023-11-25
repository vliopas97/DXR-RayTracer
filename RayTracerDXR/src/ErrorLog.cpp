#include "ErrorLog.h"

#include <iomanip>
#include <sstream>


ExceptionBase::ExceptionBase(uint32_t line, const char* file) noexcept
	: Line(line), File(file)
{}

const char* ExceptionBase::what() const noexcept
{
	std::ostringstream oss;
	oss << GetType() << " | " << GetRawMessage();
	ErrorMessage = oss.str();
	return ErrorMessage.c_str();
}

const char* ExceptionBase::GetType() const noexcept
{
	return "Exception Base";
}

std::string ExceptionBase::GetRawMessage() const noexcept
{
	std::ostringstream oss;
	oss << "File: " << std::left << std::setw(File.size() + 2) << File << std::endl <<
		"Line:" << std::setw(std::to_string(Line).length() + 1) << std::right << Line << std::endl;
	return oss.str();
}

uint32_t ExceptionBase::GetLine() const noexcept
{
	return Line;
}

const std::string& ExceptionBase::GetFile() const noexcept
{
	return File;
}

std::string ExceptionBase::TranslateErrorCode(HRESULT result) noexcept
{
	char* pMsgBuf = nullptr;
	DWORD nMsgLen = FormatMessageA(
		FORMAT_MESSAGE_ALLOCATE_BUFFER |
		FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		nullptr, result, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		reinterpret_cast<LPSTR>(&pMsgBuf), 0, nullptr);

	if (nMsgLen == 0)
	{
		return "Unidentified error code";
	}
	std::string errorString = pMsgBuf;
	LocalFree(pMsgBuf);
	return errorString;
}

DXGIInfoManager::DXGIInfoManager()
{
	using DXGIGetDebugInterface = HRESULT(WINAPI*)(REFIID, void**);

	const auto modDxgiDebug = LoadLibraryEx(L"dxgidebug.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
	if (!modDxgiDebug)
		MessageBoxA(NULL, "Can't init Info manager", "Error", MB_OK);

	const auto dxgiGetDebugInterface = reinterpret_cast<DXGIGetDebugInterface>(
		reinterpret_cast<void*>(GetProcAddress(modDxgiDebug, "DXGIGetDebugInterface"))
		);
	if (!dxgiGetDebugInterface)
		MessageBoxA(NULL, "Can't init Info manager", "Error", MB_OK);

	GRAPHICS_ASSERT_NO_INFO(dxgiGetDebugInterface(__uuidof(IDXGIInfoQueue), reinterpret_cast<void**>(&InfoQueue)));
}

DXGIInfoManager::~DXGIInfoManager()
{
	if (!InfoQueue)
		InfoQueue->Release();
}

void DXGIInfoManager::Reset() noexcept
{
	Next = InfoQueue->GetNumStoredMessages(DXGI_DEBUG_ALL);
}

std::vector<std::string> DXGIInfoManager::GetMessages() const
{
	std::vector<std::string> messages;
	const auto end = InfoQueue->GetNumStoredMessages(DXGI_DEBUG_ALL);
	for (auto i = Next; i < end; i++)
	{
		SIZE_T messageLength = 0;

		GRAPHICS_ASSERT_NO_INFO(InfoQueue->GetMessageW(DXGI_DEBUG_ALL, i, nullptr, &messageLength));

		auto bytes = std::make_unique<byte[]>(messageLength);
		auto message = reinterpret_cast<DXGI_INFO_QUEUE_MESSAGE*>(bytes.get());

		GRAPHICS_ASSERT_NO_INFO(InfoQueue->GetMessageW(DXGI_DEBUG_ALL, i, message, &messageLength));
		messages.emplace_back(message->pDescription);
	}
	return messages;
}

void d3d_trace_hr_no_info(const std::string& msg, const char* file, int line, HRESULT hr)
{
	char hr_msg[512];
	FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM, nullptr, hr, 0, hr_msg, ARRAYSIZE(hr_msg), nullptr);
	std::string fileLineInfo = "\n[ File: " + std::string(file) + " | Line: " + std::to_string(line) + " ]";
	std::string error_msg = msg + fileLineInfo + ".\nError: " + hr_msg;
	MessageBoxA(NULL, error_msg.c_str(), "Error", MB_OK);
}

void d3d_trace_hr(const std::string& msg, const char* file, int line, HRESULT hr)
{
	DXGIInfoManager infoManager;
	char hr_msg[512];
	FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM, nullptr, hr, 0, hr_msg, ARRAYSIZE(hr_msg), nullptr);
	std::string fileLineInfo = "\n[ File: " + std::string(file) + " | Line: " + std::to_string(line) + " ]";
	std::string error_msg = msg + fileLineInfo + ".\nError: " + hr_msg + '\n';
	for (auto& m : infoManager.GetMessages())
	{
		error_msg += m;
		error_msg.push_back('\n');
	}
	MessageBoxA(NULL, error_msg.c_str(), "Error", MB_OK);
}
