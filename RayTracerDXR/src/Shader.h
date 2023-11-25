#pragma once

#include "Core.h"
#include "Utils.h"
#include "Shaders/HLSLCompat.h"

#include <functional>

static dxc::DxcDllSupport gDxcDllHelper;
MAKE_SMART_COM_PTR(IDxcCompiler);
MAKE_SMART_COM_PTR(IDxcLibrary);
MAKE_SMART_COM_PTR(IDxcBlobEncoding);
MAKE_SMART_COM_PTR(IDxcOperationResult);

ID3DBlobPtr compileShader(const WCHAR* filename, const WCHAR* entryPoint);

struct ShaderProgram
{
	ShaderProgram(const std::string& file, const WCHAR* entryPoint);

	D3D12_STATE_SUBOBJECT Subobject{};
	D3D12_DXIL_LIBRARY_DESC LibDesc{};
	D3D12_EXPORT_DESC ExportDesc{};
	std::wstring ExportName;
	ID3DBlobPtr Blob;
};

struct HitGroup
{
	HitGroup(LPCWSTR intersectionEntryPoint, LPCWSTR ahsEntryPoint,
			 LPCWSTR chsEntryPoint, const std::wstring& name);

	std::wstring Name;
	D3D12_HIT_GROUP_DESC Desc{};
	D3D12_STATE_SUBOBJECT Subobject{};
};

struct RootSignatureDesc
{
	D3D12_ROOT_SIGNATURE_DESC Desc = {};
	std::vector<D3D12_DESCRIPTOR_RANGE> Range;
	std::vector<D3D12_ROOT_PARAMETER> RootParams;
};

struct LocalRootSignature
{
	LocalRootSignature(ID3D12Device5Ptr device);

	template <typename Fn, typename... Args>
	requires std::is_invocable_v<Fn, Args...> && std::is_same_v<std::invoke_result_t<Fn, Args...>, RootSignatureDesc>
	LocalRootSignature(ID3D12Device5Ptr device, Fn f, Args... args)
	{
		RootSignatureDesc desc = f(args...);

		RootSignature = D3D::CreateRootSignature(device, desc.Desc);
		Interface = RootSignature.GetInterfacePtr();
		Subobject.Type = D3D12_STATE_SUBOBJECT_TYPE_LOCAL_ROOT_SIGNATURE;
		Subobject.pDesc = &Interface;
	}

	ID3D12RootSignaturePtr RootSignature;
	ID3D12RootSignature* Interface = nullptr;
	D3D12_STATE_SUBOBJECT Subobject{};
};

struct ExportAssociation
{
	ExportAssociation(const WCHAR* names[], uint32_t size, const D3D12_STATE_SUBOBJECT* relatedSubobject);

	D3D12_STATE_SUBOBJECT Subobject{};
	D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION Association{};
};

struct ShaderConfig
{
	ShaderConfig(uint32_t maxPayloadSizeBytes,
				 uint32_t maxAttributeSizeBytes = D3D12_RAYTRACING_MAX_ATTRIBUTE_SIZE_IN_BYTES);

	D3D12_RAYTRACING_SHADER_CONFIG Config{};
	D3D12_STATE_SUBOBJECT Subobject{};
};

struct PipelineConfig
{
	PipelineConfig(uint32_t maxTraceRecursionDepth);

	D3D12_RAYTRACING_PIPELINE_CONFIG Config{};
	D3D12_STATE_SUBOBJECT Subobject{};
};

enum GlobalRootParams : uint32_t
{
	StandardDescriptors,
	SceneDescriptor,
	UAVDescriptor,
	CBuffer,
	LightCBuffer,
	AppSettings,

	NumRTRootParams
};

template <typename Fn, typename... Args>
concept ValidRootSignatureFactory = requires(Fn fn, Args... args)
{
	std::is_invocable_v<Fn, Args...>&&
		std::is_same_v<std::invoke_result_t<Fn, Args...>, RootSignatureDesc>;
};

struct GlobalRootSignature
{
	GlobalRootSignature(ID3D12Device5Ptr device);
	GlobalRootSignature(ID3D12Device5Ptr device, ID3D12RootSignaturePtr rootSig);
	template <typename Fn, typename... Args>
	requires ValidRootSignatureFactory<Fn, Args...>
	GlobalRootSignature(ID3D12Device5Ptr device, Fn f, Args... args)
	{
		RootSignatureDesc desc = f(args...);

		RootSignature = D3D::CreateRootSignature(device, desc.Desc);
		Interface = RootSignature.GetInterfacePtr();
		Subobject.Type = D3D12_STATE_SUBOBJECT_TYPE_GLOBAL_ROOT_SIGNATURE;
		Subobject.pDesc = &Interface;
	}

	ID3D12RootSignaturePtr RootSignature;
	ID3D12RootSignature* Interface = nullptr;
	D3D12_STATE_SUBOBJECT Subobject = {};
};

struct GlobalBindings
{
	using GlobalRootSignaturePtr = std::unique_ptr<GlobalRootSignature>;
	GlobalBindings() = default;
	~GlobalBindings() = default;

	void Initialize(ID3D12Device5Ptr device, GlobalRootSignature rootSignature);
	void Initialize(ID3D12Device5Ptr device);
	
	void SetSceneAccelerationStructures(ID3D12ResourcePtr bottomLevelAS, ID3D12ResourcePtr toplevelAS, uint64_t size);
	void Bind(ID3D12GraphicsCommandList4Ptr cmdList);

	void Tick();
private:
	void Setup(ID3D12Device5Ptr device);
	ID3D12RootSignaturePtr InitializeImpl(ID3D12Device5Ptr device);
	void BindDescriptorHeap(ID3D12GraphicsCommandList4Ptr cmdList, ID3D12DescriptorHeapPtr heap);

public:
	ID3D12DescriptorHeapPtr SRVHeap;
	ID3D12DescriptorHeapPtr UAVHeap;
	ID3D12DescriptorHeapPtr CBVHeap;//for temp objects
	GlobalRootSignaturePtr RootSignatureData;

	ID3D12ResourcePtr TopLevelAS;
	ID3D12ResourcePtr BottomLevelAS;
	uint64_t TopLevelASSize = 0;

	// Temp objects
	RayTracingConstants RTConstantsData{};
private:
	ID3D12ResourcePtr RTConstants;
	uint8_t* RTDataBridgePtr;
};

