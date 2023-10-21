#pragma once

#include "Core.h"
#include "Window.h"

struct Graphics
{
    Graphics(Window& window);
    ~Graphics();

    void Tick();
private:
    void Init();
    void Shutdown();
    void EndFrame(UINT index);

    void CreateDevice();
    void CreateSwapChain();


private:
    HWND WinHandle{ nullptr };

    IDXGIFactory4Ptr Factory;
    ID3D12DebugPtr Debug;

    ID3D12Device5Ptr Device;
    ID3D12CommandQueuePtr CmdQueue;
    IDXGISwapChain3Ptr SwapChain;
    glm::uvec2 SwapChainSize;
    ID3D12GraphicsCommandList4Ptr CmdList;
    ID3D12FencePtr Fence;
    HANDLE FenceEvent;
    uint64_t FenceValue = 0;

    struct
    {
        ID3D12CommandAllocatorPtr CmdAllocator;
        ID3D12ResourcePtr SwapChainBuffer;
        D3D12_CPU_DESCRIPTOR_HANDLE RTVHandle;
    } FrameObjects[kDefaultSwapChainBuffers];

    struct HeapData
    {
        ID3D12DescriptorHeapPtr Heap;
        uint32_t UsedEntries = 0;
    };
    HeapData RTVHeap;
    static const uint32_t RTVHeapSize = 3;
};