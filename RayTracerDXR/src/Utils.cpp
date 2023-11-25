#include "Utils.h"
#include "Shader.h"
#include "Sphere.h"

#include <locale>
#include <codecvt>

#include <random>

#pragma warning(disable: 4996)

// Globals
uint64_t FenceValue = 0;
ID3D12CommandQueuePtr CmdQueue{};
ID3D12GraphicsCommandList4Ptr CmdList{};
ID3D12FencePtr Fence{};
HANDLE FenceEvent{};

//
std::wstring string_2_wstring(const std::string& s)
{
	std::wstring_convert<std::codecvt_utf8<WCHAR>> cvt;
	std::wstring ws = cvt.from_bytes(s);
	return ws;
}

std::string wstring_2_string(const std::wstring& ws)
{
	std::wstring_convert<std::codecvt_utf8<wchar_t>> cvt;
	std::string s = cvt.to_bytes(ws);
	return s;
}

std::vector<glm::vec3> GenerateTextureData(const glm::uvec2& dims)
{
	std::random_device r;
	std::mt19937 gen(r());
	std::uniform_real_distribution<float> distr(-1.0f, 1.0f);
	std::vector<glm::vec3> v(dims.x * dims.y);
	for (auto& element : v)
	{
		element = { distr(gen), distr(gen), distr(gen) };
	}
	return v;
};

namespace D3D
{
	ID3D12DescriptorHeapPtr CreateDescriptorHeap(ID3D12Device5Ptr device,
												 uint32_t count,
												 D3D12_DESCRIPTOR_HEAP_TYPE type,
												 bool shaderVisible)
	{
		D3D12_DESCRIPTOR_HEAP_DESC desc{};
		desc.NumDescriptors = count;
		desc.Type = type;
		desc.Flags = shaderVisible ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE :
			D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

		ID3D12DescriptorHeapPtr heap;
		GRAPHICS_ASSERT(device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&heap)));
		return heap;
	}

	ID3D12CommandQueuePtr CreateCommandQueue(ID3D12Device5Ptr device)
	{
		ID3D12CommandQueuePtr queue;
		D3D12_COMMAND_QUEUE_DESC desc{};
		desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
		desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
		GRAPHICS_ASSERT(device->CreateCommandQueue(&desc, IID_PPV_ARGS(&queue)));
		return queue;
	}

	D3D12_CPU_DESCRIPTOR_HANDLE CreateRTV(ID3D12Device5Ptr device,
										  ID3D12ResourcePtr resource,
										  ID3D12DescriptorHeapPtr heap,
										  uint32_t& usedHeapEntries,
										  DXGI_FORMAT format)
	{
		D3D12_RENDER_TARGET_VIEW_DESC desc{};
		desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
		desc.Format = format;
		desc.Texture2D.MipSlice = 0;
		auto rtvHandle = heap->GetCPUDescriptorHandleForHeapStart();
		rtvHandle.ptr += (usedHeapEntries++) * device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
		device->CreateRenderTargetView(resource, &desc, rtvHandle);
		return rtvHandle;
	}

	void ResourceBarrier(ID3D12GraphicsCommandList4Ptr cmdList,
						 ID3D12ResourcePtr resource,
						 D3D12_RESOURCE_STATES prevState,
						 D3D12_RESOURCE_STATES nextState)
	{
		D3D12_RESOURCE_BARRIER barrier{};
		barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barrier.Transition.pResource = resource;
		barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		barrier.Transition.StateBefore = prevState;
		barrier.Transition.StateAfter = nextState;
		cmdList->ResourceBarrier(1, &barrier);
	}

	uint64_t SubmitCommandList(ID3D12GraphicsCommandList4Ptr cmdList,
							   ID3D12CommandQueuePtr cmdQueue,
							   ID3D12FencePtr fence,
							   uint64_t fenceValue)
	{
		cmdList->Close();
		ID3D12CommandList* list = cmdList.GetInterfacePtr();
		cmdQueue->ExecuteCommandLists(1, &list);
		cmdQueue->Signal(fence, ++fenceValue);
		return fenceValue;
	}

	ID3D12RootSignaturePtr CreateRootSignature(ID3D12Device5Ptr pDevice,
											   const D3D12_ROOT_SIGNATURE_DESC& desc)
	{
		ID3DBlobPtr sigBlob;
		ID3DBlobPtr errorBlob;
		HRESULT hr = D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1, &sigBlob, &errorBlob);
		if (FAILED(hr))
		{
			std::string msg = convertBlobToString(errorBlob.GetInterfacePtr());
			MessageBoxA(NULL, msg.c_str(), "Error", MB_OK);
			return nullptr;
		}
		ID3D12RootSignaturePtr rootSig;
		GRAPHICS_ASSERT(pDevice->CreateRootSignature(0, sigBlob->GetBufferPointer(), sigBlob->GetBufferSize(), IID_PPV_ARGS(&rootSig)));
		return rootSig;
	}

	ID3D12ResourcePtr CreateBuffer(ID3D12Device5Ptr device,
								   uint64_t size,
								   D3D12_RESOURCE_FLAGS flags,
								   D3D12_RESOURCE_STATES initState,
								   const D3D12_HEAP_PROPERTIES& heapProperties)
	{
		D3D12_RESOURCE_DESC desc{};
		desc.Alignment = 0;
		desc.DepthOrArraySize = 1;
		desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		desc.Flags = flags;
		desc.Format = DXGI_FORMAT_UNKNOWN;
		desc.Height = 1;
		desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		desc.MipLevels = 1;
		desc.SampleDesc.Count = 1;
		desc.SampleDesc.Quality = 0;
		desc.Width = size;

		ID3D12ResourcePtr buffer;
		GRAPHICS_ASSERT(device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &desc,
														initState, nullptr, IID_PPV_ARGS(&buffer)));
		return buffer;
	}
}

namespace DXR
{
	AccelerationStructureBuffers CreateBottomLevelAS(ID3D12Device5Ptr device,
													 ID3D12GraphicsCommandList4Ptr cmdList,
													 ID3D12ResourcePtr resource)
	{
		D3D12_RAYTRACING_GEOMETRY_DESC desc{};
		desc.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_PROCEDURAL_PRIMITIVE_AABBS;
		desc.AABBs.AABBs.StartAddress = resource->GetGPUVirtualAddress();
		desc.AABBs.AABBs.StrideInBytes = sizeof(D3D12_RAYTRACING_AABB);
		desc.AABBs.AABBCount = 1;
		desc.Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;

		D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS inputs{};
		inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
		inputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_NONE;
		inputs.NumDescs = 1;
		inputs.pGeometryDescs = &desc;
		inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;

		D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO info{};
		device->GetRaytracingAccelerationStructurePrebuildInfo(&inputs, &info);

		AccelerationStructureBuffers buffers{};
		buffers.Scratch = D3D::CreateBuffer(device, info.ScratchDataSizeInBytes, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D::DefaultHeapProps);

		buffers.Result = D3D::CreateBuffer(device, info.ResultDataMaxSizeInBytes, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE, D3D::DefaultHeapProps);

		D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC asDesc{};
		asDesc.Inputs = inputs;
		asDesc.DestAccelerationStructureData = buffers.Result->GetGPUVirtualAddress();
		asDesc.ScratchAccelerationStructureData = buffers.Scratch->GetGPUVirtualAddress();

		cmdList->BuildRaytracingAccelerationStructure(&asDesc, 0, nullptr);

		D3D12_RESOURCE_BARRIER barrier{};
		barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
		barrier.UAV.pResource = buffers.Result;
		cmdList->ResourceBarrier(1, &barrier);

		return buffers;
	}

	AccelerationStructureBuffers CreateTopLevelAS(ID3D12Device5Ptr device,
												  ID3D12GraphicsCommandList4Ptr cmdList,
												  ID3D12ResourcePtr bottomLevelAS,
												  SphereComposite& spheres,
												  uint64_t& tLasSize)
	{
		D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS inputs{};
		inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
		inputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_NONE;
		inputs.NumDescs = spheres.Size();
		inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;

		D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO info{};
		device->GetRaytracingAccelerationStructurePrebuildInfo(&inputs, &info);

		AccelerationStructureBuffers buffers;
		buffers.Scratch = D3D::CreateBuffer(device, info.ScratchDataSizeInBytes, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D::DefaultHeapProps);
		buffers.Result = D3D::CreateBuffer(device, info.ResultDataMaxSizeInBytes, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE, D3D::DefaultHeapProps);
		tLasSize = info.ResultDataMaxSizeInBytes;

		buffers.InstanceDesc = D3D::CreateAndInitializeBuffer(device, D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_GENERIC_READ,
															  D3D::UploadHeapProps,
															  [&spheres, &bottomLevelAS]()
															  {
																  std::vector<D3D12_RAYTRACING_INSTANCE_DESC> instanceDesc(spheres.Size(),
																  D3D12_RAYTRACING_INSTANCE_DESC{});

																  for (uint32_t i = 0; i < spheres.Size(); i++)
																  {
																	instanceDesc[i].InstanceID = i;
																	instanceDesc[i].InstanceContributionToHitGroupIndex = 0;
																	instanceDesc[i].Flags = D3D12_RAYTRACING_INSTANCE_FLAG_NONE;
																	glm::mat4 m = spheres[i].GetInstanceTransform();
																	std::memcpy(instanceDesc[i].Transform, &m, sizeof(instanceDesc[i].Transform));
																	instanceDesc[i].AccelerationStructure = bottomLevelAS->GetGPUVirtualAddress();
																	instanceDesc[i].InstanceMask = 0xFF;
																  }

																  return instanceDesc;
															  });

		D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC asDesc{};
		asDesc.Inputs = inputs;
		asDesc.Inputs.InstanceDescs = buffers.InstanceDesc->GetGPUVirtualAddress();
		asDesc.DestAccelerationStructureData = buffers.Result->GetGPUVirtualAddress();
		asDesc.ScratchAccelerationStructureData = buffers.Scratch->GetGPUVirtualAddress();

		cmdList->BuildRaytracingAccelerationStructure(&asDesc, 0, nullptr);

		D3D12_RESOURCE_BARRIER barrier{};
		barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
		barrier.UAV.pResource = buffers.Result;
		cmdList->ResourceBarrier(1, &barrier);

		return buffers;
	}

}