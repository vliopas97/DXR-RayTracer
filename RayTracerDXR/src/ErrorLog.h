#pragma once

#include "Core.h"


class ExceptionBase : public std::exception
{
public:
	ExceptionBase(uint32_t line, const char* file) noexcept;
	virtual ~ExceptionBase() = default;
	const char* what() const noexcept override;
	virtual const char* GetType() const noexcept;

	std::string GetRawMessage() const noexcept;

	uint32_t GetLine() const noexcept;
	const std::string& GetFile() const noexcept;

protected:
	static std::string TranslateErrorCode(HRESULT result) noexcept;
private:
	uint32_t Line;
	std::string File;

protected:
	mutable std::string ErrorMessage;
};

struct DXGIInfoManager
{
	DXGIInfoManager();
	~DXGIInfoManager();
	DXGIInfoManager(const DXGIInfoManager&) = delete;
	DXGIInfoManager& operator=(const DXGIInfoManager) = delete;

	void Reset() noexcept;
	std::vector <std::string> GetMessages() const;
private:
	unsigned long long Next = 0u;
	struct IDXGIInfoQueue* InfoQueue = nullptr;
};

#define EXCEPTION_WRAP(x)\
	try\
	{\
		x\
	}\
	catch( const ExceptionBase& e )\
	{\
		MessageBoxA(nullptr, e.what(), e.GetType(), MB_OK | MB_ICONEXCLAMATION);\
	}\
	catch (const std::exception& e)\
	{\
		MessageBoxA(nullptr, e.what(), "Standard Exception", MB_OK | MB_ICONEXCLAMATION);\
	}\
	catch (...)\
	{\
		MessageBoxA(nullptr, "No details available", "Unknown Exception", MB_OK | MB_ICONEXCLAMATION);\
	}\

void d3d_trace_hr_no_info(const std::string& msg, const char* file, int line, HRESULT hr);
void d3d_trace_hr(const std::string& msg, const char* file, int line, HRESULT hr);

#define GRAPHICS_ASSERT_NO_INFO(hrCall) \
{ \
    HRESULT hr = (hrCall); \
    if (FAILED(hr)) \
        {d3d_trace_hr_no_info(#hrCall, __FILE__, __LINE__, hr); \
		 __debugbreak(); }\
}

#define GRAPHICS_ASSERT(hrCall) \
{ \
    HRESULT hr = (hrCall); \
    if (FAILED(hr)) \
        {d3d_trace_hr(#hrCall, __FILE__, __LINE__, hr); \
		 __debugbreak(); }\
}
