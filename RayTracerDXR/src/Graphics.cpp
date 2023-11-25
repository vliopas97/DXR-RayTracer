#include "Graphics.h"
#include "Utils.h"

#include <random>

static void InitializeRTConstants(RayTracingConstants& rtConstants)
{
	rtConstants.CameraPosition = vec3(0.0f);
	rtConstants.ViewProjectionInv = mat4x4(1.0f);

	rtConstants.TexturesOffset = 0;
	rtConstants.SpheresOfsset = 0;
	rtConstants.MaterialsOffset = 0;
	rtConstants.RandomNumbersIndex = 0;
}

Graphics::Graphics(Window& window)
	:WinHandle(window.Handle), SceneCamera()
{
	Init();
	SwapChainSize = glm::vec2(window.Width, window.Height);


	SceneCamera.SetPosition(glm::vec3(0, 0, -4));
	InitializeRTConstants(GlobalResources.RTConstantsData);
	GlobalResources.Initialize(Device);

	Spheres.SetDevice(Device);
	Spheres.AddSphere(Sphere{ glm::vec3(0), 1, glm::vec3(0.8f, 0.0f, 0.0f) });
	Spheres.AddSphere(Sphere{ glm::vec3(0, -101, 0), 100, glm::vec3(0.3f, 0.4f, 0.8f) });
	Spheres.AddSphere(Sphere(glm::vec3(-2, 0, 0), 1, glm::vec3(1.0f, 1.0f, 1.0f), MaterialType::Dielectric));
	Spheres.AddSphere(Sphere(glm::vec3(2, 0, 0), 1, glm::vec3(0.8f, 0.6f, 0.2f), MaterialType::Metal));

	InitializeMaterials();

	CreateAccelerationStructures();
	CreateRTPipelaneState();
	CreateShaderResources();
	CreateShaderTable();
}

Graphics::~Graphics()
{
	Shutdown();
}

void Graphics::Tick(float delta)
{
	uint32_t frameIndex = SwapChain->GetCurrentBackBufferIndex();

	D3D::ResourceBarrier(CmdList, OutputTexture, D3D12_RESOURCE_STATE_COPY_SOURCE,
						 D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	UpdateTexture();

	GlobalResources.RTConstantsData.CameraPosition = SceneCamera.GetPosition();
	GlobalResources.RTConstantsData.ViewProjectionInv = (glm::inverse(SceneCamera.GetViewProjection()));
	GlobalResources.Tick();
	SceneCamera.Tick(delta);

	D3D12_DISPATCH_RAYS_DESC rayTraceDesc{};
	rayTraceDesc.Width = SwapChainSize.x;
	rayTraceDesc.Height = SwapChainSize.y;
	rayTraceDesc.Depth = 1;

	rayTraceDesc.RayGenerationShaderRecord.StartAddress = ShaderTable->GetGPUVirtualAddress() + 0 * ShaderTableEntrySize;
	rayTraceDesc.RayGenerationShaderRecord.SizeInBytes = ShaderTableEntrySize;

	size_t missOffset = ShaderTableEntrySize;
	rayTraceDesc.MissShaderTable.StartAddress = ShaderTable->GetGPUVirtualAddress() + missOffset;
	rayTraceDesc.MissShaderTable.StrideInBytes = ShaderTableEntrySize;
	rayTraceDesc.MissShaderTable.SizeInBytes = ShaderTableEntrySize;

	size_t hitOffset = 2 * ShaderTableEntrySize;
	rayTraceDesc.HitGroupTable.StartAddress = ShaderTable->GetGPUVirtualAddress() + hitOffset;
	rayTraceDesc.HitGroupTable.StrideInBytes = ShaderTableEntrySize;
	rayTraceDesc.HitGroupTable.SizeInBytes = ShaderTableEntrySize;

	GlobalResources.Bind(CmdList);

	CmdList->SetPipelineState1(PipelineState.GetInterfacePtr());
	CmdList->DispatchRays(&rayTraceDesc);

	D3D::ResourceBarrier(CmdList, OutputTexture, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_SOURCE);
	D3D::ResourceBarrier(CmdList, FrameObjects[frameIndex].SwapChainBuffer, D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_COPY_DEST);
	CmdList->CopyResource(FrameObjects[frameIndex].SwapChainBuffer, OutputTexture);

	EndFrame(frameIndex);
}

void Graphics::Init()
{
#ifndef NDEBUG
	if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&Debug))))
	{
		Debug->EnableDebugLayer();
		Debug->QueryInterface(IID_PPV_ARGS(&Debug1));
		//Debug1->SetEnableGPUBasedValidation(true);
	}
#endif // !NDEBUG

	GRAPHICS_ASSERT(CreateDXGIFactory1(IID_PPV_ARGS(&Factory)));

	CreateDevice();
	CmdQueue = D3D::CreateCommandQueue(Device);
	CreateSwapChain();
	RTVHeap.Heap = D3D::CreateDescriptorHeap(Device, RTVHeapSize, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, false);

#ifndef NDEBUG
	ID3D12InfoQueue* InfoQueue = nullptr;
	Device->QueryInterface(IID_PPV_ARGS(&InfoQueue));

	InfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);
	InfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);
	InfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, false);

#endif // !NDEBUG

	for (uint32_t i = 0; i < kDefaultSwapChainBuffers; i++)
	{
		GRAPHICS_ASSERT(Device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,
													   IID_PPV_ARGS(&FrameObjects[i].CmdAllocator)));
		GRAPHICS_ASSERT(SwapChain->GetBuffer(i, IID_PPV_ARGS(&FrameObjects[i].SwapChainBuffer)));
		FrameObjects[i].RTVHandle = D3D::CreateRTV(Device, FrameObjects[i].SwapChainBuffer,
												   RTVHeap.Heap, RTVHeap.UsedEntries,
												   DXGI_FORMAT_R8G8B8A8_UNORM_SRGB);
	}

	GRAPHICS_ASSERT(Device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT,
											  FrameObjects[0].CmdAllocator, nullptr, IID_PPV_ARGS(&CmdList)));
	GRAPHICS_ASSERT(Device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&Fence)));
	FenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
}

void Graphics::Shutdown()
{
	CmdQueue->Signal(Fence, ++FenceValue);
	Fence->SetEventOnCompletion(FenceValue, FenceEvent);
	WaitForSingleObject(FenceEvent, INFINITE);
}

void Graphics::EndFrame(UINT index)
{
	D3D::ResourceBarrier(CmdList, FrameObjects[index].SwapChainBuffer,
						 D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PRESENT);
	FenceValue = D3D::SubmitCommandList(CmdList, CmdQueue, Fence, FenceValue);
	if (SwapChain->Present(0, 0) == DXGI_ERROR_DEVICE_REMOVED) GRAPHICS_ASSERT(Device->GetDeviceRemovedReason());

	uint32_t bufferIndex = SwapChain->GetCurrentBackBufferIndex();

	if (FenceValue > kDefaultSwapChainBuffers)
	{
		Fence->SetEventOnCompletion(FenceValue - kDefaultSwapChainBuffers + 1, FenceEvent);
		WaitForSingleObject(FenceEvent, INFINITE);
	}

	FrameObjects[bufferIndex].CmdAllocator->Reset();
	CmdList->Reset(FrameObjects[bufferIndex].CmdAllocator, nullptr);
}

void Graphics::CreateDevice()
{
	IDXGIAdapter1Ptr adapter;

	for (uint32_t i = 0; DXGI_ERROR_NOT_FOUND != Factory->EnumAdapters1(i, &adapter); i++)
	{
		DXGI_ADAPTER_DESC1 desc{};
		adapter->GetDesc1(&desc);

		if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) continue;

		ID3D12Device5Ptr device;
		GRAPHICS_ASSERT(D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_12_1, IID_PPV_ARGS(&device)));

		D3D12_FEATURE_DATA_D3D12_OPTIONS5 features{};
		HRESULT hr = device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5, &features,
												 sizeof(D3D12_FEATURE_DATA_D3D12_OPTIONS5));
		if (SUCCEEDED(hr) && features.RaytracingTier != D3D12_RAYTRACING_TIER_NOT_SUPPORTED)
		{
			Device = device;
			return;
		}
	}

	MessageBoxA(NULL, "Raytracing is not supported on this device", "Error", MB_OK);
	exit(1);
}

void Graphics::CreateSwapChain()
{
	DXGI_SWAP_CHAIN_DESC1 desc{};
	desc.BufferCount = kDefaultSwapChainBuffers;
	desc.Width = SwapChainSize.x;
	desc.Height = SwapChainSize.y;
	desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	desc.SampleDesc.Count = 1;

	MAKE_SMART_COM_PTR(IDXGISwapChain1);
	IDXGISwapChain1Ptr swapChain;

	GRAPHICS_ASSERT(Factory->CreateSwapChainForHwnd(CmdQueue, WinHandle, &desc, nullptr, nullptr, &swapChain));

	GRAPHICS_ASSERT(swapChain->QueryInterface(IID_PPV_ARGS(&SwapChain)));
}

void Graphics::CreateRTPipelaneState()
{
	std::vector<D3D12_STATE_SUBOBJECT> subobjects;
	subobjects.reserve(12);

	ShaderProgram rayGenShader("RayGen.hlsl", L"rayGen");
	subobjects.push_back(rayGenShader.Subobject);

	ShaderProgram missShader("Miss.hlsl", L"miss");
	subobjects.push_back(missShader.Subobject);

	ShaderProgram intersectionShader("Intersection.hlsl", L"intersection");
	subobjects.push_back(intersectionShader.Subobject);

	ShaderProgram closestHitShader("ClosestHit.hlsl", L"chs");
	subobjects.push_back(closestHitShader.Subobject);

	HitGroup hitGroup(L"intersection", nullptr, L"chs", L"HitGroup");
	subobjects.push_back(hitGroup.Subobject);

	ShaderConfig shaderConfig(10 * sizeof(float));
	subobjects.push_back(shaderConfig.Subobject);

	const WCHAR* shaderConfigExportNames[] = { L"rayGen", L"miss", L"HitGroup" };
	ExportAssociation shaderConfigAssociation(shaderConfigExportNames, arraysize(shaderConfigExportNames), &subobjects.back());
	subobjects.push_back(shaderConfigAssociation.Subobject);

	PipelineConfig pipelineConfig(maxTraceRecursionDepth);
	subobjects.push_back(pipelineConfig.Subobject);

	subobjects.push_back(GlobalResources.RootSignatureData->Subobject);

	D3D12_STATE_OBJECT_DESC desc{};
	desc.NumSubobjects = static_cast<UINT>(subobjects.size());
	desc.pSubobjects = subobjects.data();
	desc.Type = D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE;

	GRAPHICS_ASSERT(Device->CreateStateObject(&desc, IID_PPV_ARGS(&PipelineState)));
}

void Graphics::CreateAccelerationStructures()
{
	//vertex buffer setup
	uint64_t tLasSize = 0;
	//ID3D12ResourcePtr vertexBuffer = createTriangleVB(Device);
	ID3D12ResourcePtr vertexBuffer = Sphere::CreateSphereAABB(Device);
	DXR::AccelerationStructureBuffers bottomLevelBuffers = DXR::CreateBottomLevelAS(Device, CmdList, vertexBuffer);
	DXR::AccelerationStructureBuffers topLevelBuffers = DXR::CreateTopLevelAS(Device, CmdList, bottomLevelBuffers.Result,
																			  Spheres, tLasSize);

	FenceValue = D3D::SubmitCommandList(CmdList, CmdQueue, Fence, FenceValue);
	Fence->SetEventOnCompletion(FenceValue, FenceEvent);
	WaitForSingleObject(FenceEvent, INFINITE);
	uint32_t bufferIndex = SwapChain->GetCurrentBackBufferIndex();
	CmdList->Reset(FrameObjects[0].CmdAllocator, nullptr);

	GlobalResources.SetSceneAccelerationStructures(bottomLevelBuffers.Result, topLevelBuffers.Result, tLasSize);
}

void Graphics::CreateShaderResources()
{
	D3D12_RESOURCE_DESC resDesc{};
	resDesc.DepthOrArraySize = 1;
	resDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	resDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	resDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
	resDesc.Width = SwapChainSize.x;
	resDesc.Height = SwapChainSize.y;
	resDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	resDesc.MipLevels = 1;
	resDesc.SampleDesc.Count = 1;
	resDesc.SampleDesc.Quality = 0;
	GRAPHICS_ASSERT(Device->CreateCommittedResource(&D3D::DefaultHeapProps, D3D12_HEAP_FLAG_NONE,
													&resDesc, D3D12_RESOURCE_STATE_COPY_SOURCE, nullptr, IID_PPV_ARGS(&OutputTexture)));

	auto uavHandle = GlobalResources.UAVHeap->GetCPUDescriptorHandleForHeapStart();
	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc{};
	uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
	Device->CreateUnorderedAccessView(OutputTexture, nullptr, &uavDesc,
									  uavHandle);

	auto srvHandle = GlobalResources.SRVHeap->GetCPUDescriptorHandleForHeapStart();

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
	srvDesc.Format = DXGI_FORMAT_UNKNOWN;
	srvDesc.Buffer.FirstElement = 0;
	srvDesc.Buffer.NumElements = Spheres.Size();
	srvDesc.Buffer.StructureByteStride = sizeof(decltype(Spheres)::ValueType);
	srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
	Device->CreateShaderResourceView(Spheres.GetBufferPtr(), &srvDesc, srvHandle);

	srvHandle.ptr += Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	GlobalResources.RTConstantsData.TexturesOffset = 1;

	// Describe and create a Texture2D.
	resDesc.Format = DXGI_FORMAT_R32G32B32_FLOAT;
	resDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

	GRAPHICS_ASSERT(Device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&resDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&Texture)));

	// Describe and create a SRV for the texture.
	srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = resDesc.Format;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1;
	Device->CreateShaderResourceView(Texture, &srvDesc, srvHandle);

	std::vector<glm::vec3> texture = GenerateTextureData(SwapChainSize);
	D3D::UploadTexture(Device, FrameObjects[SwapChain->GetCurrentBackBufferIndex()].CmdAllocator, Texture, texture);
	GlobalResources.RTConstantsData.RandomNumbersIndex = 0;

	srvHandle.ptr += Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	GlobalResources.RTConstantsData.MaterialsOffset = 2;

	Materials = D3D::CreateAndInitializeBuffer(Device, D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_GENERIC_READ,
											   D3D::UploadHeapProps, [&materials = MatArray]() { return materials; });

	srvDesc = D3D12_SHADER_RESOURCE_VIEW_DESC{};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
	srvDesc.Format = DXGI_FORMAT_UNKNOWN;
	srvDesc.Buffer.FirstElement = 0;
	srvDesc.Buffer.NumElements = MatArray.size();
	srvDesc.Buffer.StructureByteStride = sizeof(decltype(MatArray)::value_type);
	srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
	Device->CreateShaderResourceView(Materials, &srvDesc, srvHandle);
	
	// -----------------------------------------------------------------------------
	// GPU-CPU synchronization for not releasing stack-allocated resources too early
	FenceValue = D3D::SubmitCommandList(CmdList, CmdQueue, Fence, FenceValue);
	Fence->SetEventOnCompletion(FenceValue, FenceEvent);
	WaitForSingleObject(FenceEvent, INFINITE);
	CmdList->Reset(FrameObjects[0].CmdAllocator, nullptr);
}

void Graphics::CreateShaderTable()
{
	ShaderTableEntrySize = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
	ShaderTableEntrySize += 8;
	ShaderTableEntrySize = align_to(ShaderTableEntrySize, D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT);

	uint32_t ShaderTableSize = 3 * ShaderTableEntrySize;

	ShaderTable = D3D::CreateBuffer(Device, ShaderTableSize, D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_GENERIC_READ, D3D::UploadHeapProps);

	uint8_t* data;
	GRAPHICS_ASSERT(ShaderTable->Map(0, nullptr, (void**)&data));

	MAKE_SMART_COM_PTR(ID3D12StateObjectProperties);
	ID3D12StateObjectPropertiesPtr properties;
	PipelineState->QueryInterface(IID_PPV_ARGS(&properties));

	std::memcpy(data, properties->GetShaderIdentifier(L"rayGen"), D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);

	std::memcpy(data + ShaderTableEntrySize, properties->GetShaderIdentifier(L"miss"), D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
	std::memcpy(data + 2 * ShaderTableEntrySize, properties->GetShaderIdentifier(L"HitGroup"), D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);

	ShaderTable->Unmap(0, nullptr);
}

void Graphics::UpdateTexture()
{
	auto data = GenerateTextureData(SwapChainSize);
	D3D::UploadTexture(Device, FrameObjects[SwapChain->GetCurrentBackBufferIndex()].CmdAllocator, Texture, data);
}

void Graphics::InitializeMaterials()
{
	MatArray[MaterialType::Diffuse] = Material{ .Roughness = 0.0f, .Eta = 0.0f };
	MatArray[MaterialType::Metal] = Material{ .Roughness = 0.2f, .Eta = 0.0f };
	MatArray[MaterialType::Dielectric] = Material{ .Roughness = 0.0f, .Eta = 1.52f };
}
