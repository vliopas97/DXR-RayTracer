#pragma once

#include "Core.h"
#include "ErrorLog.h"

#include <exception>
#include <iostream>
#include <type_traits>

// Globals
constexpr const uint32_t NumUserDescriptorRanges = 16;
constexpr const uint32_t NumGlobalSRVDescriptorRanges = 7 + NumUserDescriptorRanges;

extern ID3D12CommandQueuePtr CmdQueue;
extern ID3D12GraphicsCommandList4Ptr CmdList;
extern ID3D12FencePtr Fence;
extern HANDLE FenceEvent;
extern uint64_t FenceValue;

// Structs

struct FrameData
{
	ID3D12CommandAllocatorPtr CmdAllocator;
	ID3D12ResourcePtr SwapChainBuffer;
	D3D12_CPU_DESCRIPTOR_HANDLE RTVHandle;
};

//
#define arraysize(a) (sizeof(a)/sizeof(a[0]))
#define align_to(_val, _alignment) (((_val + _alignment - 1) / _alignment) * _alignment)

std::wstring string_2_wstring(const std::string& s);
std::string wstring_2_string(const std::wstring& ws);

template<class BlotType>
std::string convertBlobToString(BlotType* pBlob)
{
	std::vector<char> infoLog(pBlob->GetBufferSize() + 1);
	memcpy(infoLog.data(), pBlob->GetBufferPointer(), pBlob->GetBufferSize());
	infoLog[pBlob->GetBufferSize()] = 0;
	return std::string(infoLog.data());
}

std::vector<glm::vec3> GenerateTextureData(const glm::uvec2& dims);

template<typename T>
concept IsContainer = requires(T container)
{
	{ container.size() } ->std::same_as<std::size_t>;
	container.data();
	typename T::value_type;
};

template <typename Fn, typename... Args>
concept ValidResourceFactory = requires(Fn fn, Args... args)
{
	std::is_invocable_v<Fn, Args...>&& IsContainer<std::invoke_result_t<Fn, Args...>>;
};

namespace D3D
{
	static const D3D12_HEAP_PROPERTIES DefaultHeapProps =
	{
		D3D12_HEAP_TYPE_DEFAULT,
		D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
		D3D12_MEMORY_POOL_UNKNOWN,
		0,
		0
	};

	static const D3D12_HEAP_PROPERTIES UploadHeapProps =
	{
		D3D12_HEAP_TYPE_UPLOAD,
		D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
		D3D12_MEMORY_POOL_UNKNOWN,
		0,
		0,
	};

	ID3D12DescriptorHeapPtr CreateDescriptorHeap(ID3D12Device5Ptr device,
												 uint32_t count,
												 D3D12_DESCRIPTOR_HEAP_TYPE type,
												 bool shaderVisible);

	ID3D12CommandQueuePtr CreateCommandQueue(ID3D12Device5Ptr device);

	D3D12_CPU_DESCRIPTOR_HANDLE CreateRTV(ID3D12Device5Ptr device,
										  ID3D12ResourcePtr resource,
										  ID3D12DescriptorHeapPtr heap,
										  uint32_t& usedHeapEntries,
										  DXGI_FORMAT format);

	void ResourceBarrier(ID3D12GraphicsCommandList4Ptr cmdList,
						 ID3D12ResourcePtr resource,
						 D3D12_RESOURCE_STATES prevState,
						 D3D12_RESOURCE_STATES nextState);

	uint64_t SubmitCommandList(ID3D12GraphicsCommandList4Ptr cmdList,
							   ID3D12CommandQueuePtr cmdQueue,
							   ID3D12FencePtr fence,
							   uint64_t fenceValue);

	ID3D12RootSignaturePtr CreateRootSignature(ID3D12Device5Ptr pDevice,
											   const D3D12_ROOT_SIGNATURE_DESC& desc);

	ID3D12ResourcePtr CreateBuffer(ID3D12Device5Ptr device,
								   uint64_t size,
								   D3D12_RESOURCE_FLAGS flags,
								   D3D12_RESOURCE_STATES initState,
								   const D3D12_HEAP_PROPERTIES& heapProperties);

	template<typename Fn, typename... Args>
	requires ValidResourceFactory<Fn, Args...>
	ID3D12ResourcePtr CreateAndInitializeBuffer(ID3D12Device5Ptr device,
												D3D12_RESOURCE_FLAGS flags,
												D3D12_RESOURCE_STATES initState,
												const D3D12_HEAP_PROPERTIES& heapProperties,
												Fn f, Args... args)
	{
		auto data = f(args...);

		using Type = decltype(data)::value_type;
		ID3D12ResourcePtr pBuffer = CreateBuffer(device, sizeof(Type) * data.size(), flags, initState, heapProperties);

		uint8_t* pData;
		pBuffer->Map(0, nullptr, (void**)&pData);
		std::memcpy(pData, data.data(), sizeof(Type) * data.size());
		pBuffer->Unmap(0, nullptr);
		return pBuffer;
	}

	template<IsContainer Container>
	void UploadTexture(ID3D12Device5Ptr device,
					   ID3D12CommandAllocatorPtr cmdAllocator,
					   ID3D12ResourcePtr destResource,
					   const Container& data)
	{
		using Type = Container::value_type;
		const uint64_t uploadBufferSize = GetRequiredIntermediateSize(destResource, 0, 1);

		auto textureDesc = destResource->GetDesc();

		// Create the GPU upload buffer.
		ID3D12ResourcePtr stagingBuffer;
		GRAPHICS_ASSERT(device->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&stagingBuffer)));

		D3D12_SUBRESOURCE_DATA textureData{};
		textureData.pData = data.data();
		textureData.RowPitch = textureDesc.Width * sizeof(Type);
		textureData.SlicePitch = textureData.RowPitch * textureDesc.Height;

		CmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(destResource, D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_COPY_DEST));
		UpdateSubresources(CmdList, destResource, stagingBuffer, 0, 0, 1, &textureData);
		CmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(destResource, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ));

		FenceValue = D3D::SubmitCommandList(CmdList, CmdQueue, Fence, FenceValue);
		Fence->SetEventOnCompletion(FenceValue, FenceEvent);
		WaitForSingleObject(FenceEvent, INFINITE);
		CmdList->Reset(cmdAllocator, nullptr);//CAREFUL - MUST TAKE BACKBUFFER INDEX!!!!
	}

}

struct RootSignatureDesc;
struct SphereComposite;

namespace DXR
{
	struct AccelerationStructureBuffers
	{
		ID3D12ResourcePtr Scratch;
		ID3D12ResourcePtr Result;
		ID3D12ResourcePtr InstanceDesc;
	};

	AccelerationStructureBuffers CreateBottomLevelAS(ID3D12Device5Ptr device,
													 ID3D12GraphicsCommandList4Ptr cmdList,
													 ID3D12ResourcePtr resource);

	AccelerationStructureBuffers CreateTopLevelAS(ID3D12Device5Ptr device,
												  ID3D12GraphicsCommandList4Ptr cmdList,
												  ID3D12ResourcePtr bottomLevelAS,
												  SphereComposite& spheres,
												  uint64_t& tLasSize);

}