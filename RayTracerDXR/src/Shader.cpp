#include "Shader.h"

#include <source_location>
#include <sstream>


ID3DBlobPtr compileShader(const WCHAR* filename, const WCHAR* entryPoint)
{
	const WCHAR* targetProfile = L"lib_6_3";
	IDxcCompilerPtr compiler;
	IDxcLibraryPtr library;
	GRAPHICS_ASSERT(gDxcDllHelper.Initialize());
	GRAPHICS_ASSERT(gDxcDllHelper.CreateInstance(CLSID_DxcCompiler, &compiler));
	GRAPHICS_ASSERT(gDxcDllHelper.CreateInstance(CLSID_DxcLibrary, &library));

	MAKE_SMART_COM_PTR(IDxcIncludeHandler);
	IDxcIncludeHandlerPtr dxcIncludeHandler;
	GRAPHICS_ASSERT(library->CreateIncludeHandler(&dxcIncludeHandler));

	std::ifstream shaderFile(filename);
	if (shaderFile.good() == false)
	{
		std::string errorMessage = "Can't open file " + wstring_2_string(std::wstring(filename));
		MessageBoxA(NULL, errorMessage.c_str(), "Error", MB_OK);
	}

	std::stringstream strStream;
	strStream << shaderFile.rdbuf();
	std::string shader = strStream.str();

	IDxcBlobEncodingPtr textBlob;
	GRAPHICS_ASSERT(library->CreateBlobWithEncodingFromPinned((LPBYTE)shader.c_str(), (uint32_t)shader.size(), 0, &textBlob));

	LPCWSTR args[] = { L"/Zi", L"/Zss", L"/Zpr" };

	IDxcOperationResultPtr result;
	GRAPHICS_ASSERT(compiler->Compile(textBlob, filename, entryPoint, targetProfile, args, arraysize(args), nullptr, 0, dxcIncludeHandler, &result));

	HRESULT resultCode;
	GRAPHICS_ASSERT(result->GetStatus(&resultCode));
	if (FAILED(resultCode))
	{
		IDxcBlobEncodingPtr error;
		GRAPHICS_ASSERT(result->GetErrorBuffer(&error));
		std::string errorMessage = convertBlobToString(error.GetInterfacePtr());
		MessageBoxA(NULL, errorMessage.c_str(), "Error", MB_OK);
		__debugbreak();
		return nullptr;
	}

	MAKE_SMART_COM_PTR(IDxcBlob);
	IDxcBlobPtr blob;
	GRAPHICS_ASSERT(result->GetResult(&blob));
	return blob;
}

ShaderProgram::ShaderProgram(const std::string& file, const WCHAR* entryPoint)
	:ExportName(entryPoint)
{
	std::string currentDir = std::string(std::source_location::current().file_name());
	currentDir = currentDir.substr(0, currentDir.find_last_of("\\/")) + "\\Shaders\\" + file;

	auto filename = string_2_wstring(currentDir);
	Blob = compileShader(filename.c_str(), entryPoint);

	if (Blob)
	{
		ExportDesc.Name = ExportName.c_str();
		ExportDesc.Flags = D3D12_EXPORT_FLAG_NONE;
		ExportDesc.ExportToRename = nullptr;

		LibDesc.DXILLibrary.pShaderBytecode = Blob->GetBufferPointer();
		LibDesc.DXILLibrary.BytecodeLength = Blob->GetBufferSize();
		LibDesc.NumExports = 1;
		LibDesc.pExports = &ExportDesc;
	}

	Subobject.Type = D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY;
	Subobject.pDesc = &LibDesc;
}

HitGroup::HitGroup(LPCWSTR intersectionEntryPoint, LPCWSTR ahsEntryPoint, LPCWSTR chsEntryPoint, const std::wstring& name)
	:Name(name)
{
	Desc.Type = D3D12_HIT_GROUP_TYPE_PROCEDURAL_PRIMITIVE;
	Desc.IntersectionShaderImport = intersectionEntryPoint;
	Desc.AnyHitShaderImport = ahsEntryPoint;
	Desc.ClosestHitShaderImport = chsEntryPoint;
	Desc.HitGroupExport = Name.c_str();

	Subobject.Type = D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP;
	Subobject.pDesc = &Desc;
}

LocalRootSignature::LocalRootSignature(ID3D12Device5Ptr device)
{
	D3D12_ROOT_SIGNATURE_DESC desc{};
	desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE;

	RootSignature = D3D::CreateRootSignature(device, desc);
	Interface = RootSignature.GetInterfacePtr();
	Subobject.Type = D3D12_STATE_SUBOBJECT_TYPE_LOCAL_ROOT_SIGNATURE;
	Subobject.pDesc = &Interface;
}

ExportAssociation::ExportAssociation(const WCHAR* names[], uint32_t size, const D3D12_STATE_SUBOBJECT* relatedSubobject)
{
	Association.NumExports = size;
	Association.pExports = names;
	Association.pSubobjectToAssociate = relatedSubobject;

	Subobject.Type = D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION;
	Subobject.pDesc = &Association;
}

ShaderConfig::ShaderConfig(uint32_t maxPayloadSizeBytes, uint32_t maxAttributeSizeBytes)
{
	Config.MaxPayloadSizeInBytes = maxPayloadSizeBytes;
	Config.MaxAttributeSizeInBytes = maxAttributeSizeBytes;

	Subobject.Type = D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_SHADER_CONFIG;
	Subobject.pDesc = &Config;
}

PipelineConfig::PipelineConfig(uint32_t maxTraceRecursionDepth)
{
	Config.MaxTraceRecursionDepth = maxTraceRecursionDepth;

	Subobject.Type = D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_PIPELINE_CONFIG;
	Subobject.pDesc = &Config;
}

GlobalRootSignature::GlobalRootSignature(ID3D12Device5Ptr device)
{
	D3D12_ROOT_SIGNATURE_DESC desc{};
	RootSignature = D3D::CreateRootSignature(device, desc);
	Interface = RootSignature.GetInterfacePtr();
	Subobject.pDesc = &Interface;
	Subobject.Type = D3D12_STATE_SUBOBJECT_TYPE_GLOBAL_ROOT_SIGNATURE;
}

GlobalRootSignature::GlobalRootSignature(ID3D12Device5Ptr device, ID3D12RootSignaturePtr rootSig)
{
	RootSignature = rootSig;
	Interface = RootSignature.GetInterfacePtr();
	Subobject.pDesc = &Interface;
	Subobject.Type = D3D12_STATE_SUBOBJECT_TYPE_GLOBAL_ROOT_SIGNATURE;
}

void GlobalBindings::Initialize(ID3D12Device5Ptr device, GlobalRootSignature rootSignature)
{
	*RootSignatureData = rootSignature;
	Setup(device);
}

void GlobalBindings::Initialize(ID3D12Device5Ptr device)
{
	auto data = InitializeImpl(device);
	RootSignatureData = std::make_unique<GlobalRootSignature>(device, data);
	Setup(device);
}

void GlobalBindings::SetSceneAccelerationStructures(ID3D12ResourcePtr bottomLevelAS, ID3D12ResourcePtr toplevelAS, uint64_t size)
{
	BottomLevelAS = bottomLevelAS;
	TopLevelAS = toplevelAS;
	TopLevelASSize = size;
}

void GlobalBindings::Bind(ID3D12GraphicsCommandList4Ptr cmdList)
{
	cmdList->SetComputeRootSignature(RootSignatureData->Interface);

	BindDescriptorHeap(cmdList, SRVHeap);
	cmdList->SetComputeRootDescriptorTable(StandardDescriptors,
										   SRVHeap->GetGPUDescriptorHandleForHeapStart());
	cmdList->SetComputeRootShaderResourceView(SceneDescriptor, TopLevelAS->GetGPUVirtualAddress());

	BindDescriptorHeap(cmdList, UAVHeap);
	cmdList->SetComputeRootDescriptorTable(UAVDescriptor, UAVHeap->GetGPUDescriptorHandleForHeapStart());

	BindDescriptorHeap(cmdList, CBVHeap);
	cmdList->SetComputeRootConstantBufferView(CBuffer, RTConstants->GetGPUVirtualAddress());
}

void GlobalBindings::Tick()
{
	std::memcpy(RTDataBridgePtr, &RTConstantsData, sizeof(RTConstantsData));
}

void GlobalBindings::Setup(ID3D12Device5Ptr device)
{
	SRVHeap = D3D::CreateDescriptorHeap(device, NumGlobalSRVDescriptorRanges + 1,
										D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, true);
	UAVHeap = D3D::CreateDescriptorHeap(device, 2, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, true);
	CBVHeap = D3D::CreateDescriptorHeap(device, 1, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, true);

	//Create constant buffer for temp data
	const UINT constantBufferSize = align_to(sizeof(RTConstantsData), D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);    // CB size is required to be 256-byte aligned.

	GRAPHICS_ASSERT(device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(constantBufferSize),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&RTConstants)));

	// Describe and create a constant buffer view.
	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
	cbvDesc.BufferLocation = RTConstants->GetGPUVirtualAddress();
	cbvDesc.SizeInBytes = constantBufferSize;
	device->CreateConstantBufferView(&cbvDesc, CBVHeap->GetCPUDescriptorHandleForHeapStart());

	GRAPHICS_ASSERT(RTConstants->Map(0, nullptr, reinterpret_cast<void**>(&RTDataBridgePtr)));
	std::memcpy(RTDataBridgePtr, &RTConstantsData, sizeof(RTConstantsData));
}

ID3D12RootSignaturePtr GlobalBindings::InitializeImpl(ID3D12Device5Ptr device)
{
	std::array<D3D12_DESCRIPTOR_RANGE, 1> uavRanges = {};
	std::array<D3D12_DESCRIPTOR_RANGE, NumGlobalSRVDescriptorRanges> srvRanges = {};
	std::array<D3D12_DESCRIPTOR_RANGE, 1> cbvRanges = {};

	uavRanges[0].BaseShaderRegister = 0;
	uavRanges[0].NumDescriptors = 2;
	uavRanges[0].RegisterSpace = 0;
	uavRanges[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;

	uint32_t userStart = NumGlobalSRVDescriptorRanges - NumUserDescriptorRanges;
	for (uint32_t i = 0; i < NumGlobalSRVDescriptorRanges; ++i)
	{
		auto& descElement = srvRanges[i];
		descElement.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		descElement.NumDescriptors = UINT_MAX;
		descElement.BaseShaderRegister = 0;
		descElement.RegisterSpace = i;
		descElement.OffsetInDescriptorsFromTableStart = 0;
		if (i >= userStart)
			descElement.RegisterSpace = (i - userStart) + 100;
	}

	std::array<D3D12_ROOT_PARAMETER, 4> rootParams;

	rootParams[StandardDescriptors].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParams[StandardDescriptors].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	rootParams[StandardDescriptors].DescriptorTable.NumDescriptorRanges = static_cast<UINT>(srvRanges.size());
	rootParams[StandardDescriptors].DescriptorTable.pDescriptorRanges =srvRanges.data();

	// gRtScene
	rootParams[SceneDescriptor].ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV;
	rootParams[SceneDescriptor].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	rootParams[SceneDescriptor].Descriptor.ShaderRegister = 0;
	rootParams[SceneDescriptor].Descriptor.RegisterSpace = 200;

	rootParams[UAVDescriptor].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParams[UAVDescriptor].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	rootParams[UAVDescriptor].DescriptorTable.NumDescriptorRanges = static_cast<UINT>(uavRanges.size());
	rootParams[UAVDescriptor].DescriptorTable.pDescriptorRanges = uavRanges.data();

	rootParams[CBuffer].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	rootParams[CBuffer].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	rootParams[CBuffer].Descriptor.ShaderRegister = 0;
	rootParams[CBuffer].Descriptor.RegisterSpace = 0;

	D3D12_ROOT_SIGNATURE_DESC desc{};
	desc.NumParameters = static_cast<UINT>(rootParams.size());
	desc.pParameters = rootParams.data();

	return D3D::CreateRootSignature(device, desc);

}

void GlobalBindings::BindDescriptorHeap(ID3D12GraphicsCommandList4Ptr cmdList, ID3D12DescriptorHeapPtr heap)
{
	std::array<ID3D12DescriptorHeap*, 1> heaps = { heap };
	cmdList->SetDescriptorHeaps(heaps.size(), heaps.data());
}
