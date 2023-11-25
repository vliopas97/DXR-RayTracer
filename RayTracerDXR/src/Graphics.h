#pragma once

#include "Core.h"

#include "Camera.h"
#include "Window.h"
#include "Shader.h"
#include "Sphere.h"

#include "Shaders/HLSLCompat.h"

struct Graphics
{
    Graphics(Window& window);
    ~Graphics();

    void Tick(float delta);

private:
    void Init();
    void Shutdown();
    void EndFrame(UINT index);

    void CreateDevice();
    void CreateSwapChain();
    void CreateRTPipelaneState();

    void CreateAccelerationStructures();
    void CreateShaderResources();
    void CreateShaderTable();

    void UpdateTexture();

    void InitializeMaterials();
private:
    HWND WinHandle{ nullptr };
    SphereComposite Spheres;

    IDXGIFactory4Ptr Factory;
    ID3D12DebugPtr Debug;
    ID3D12Debug1Ptr Debug1;

    ID3D12Device5Ptr Device;
    IDXGISwapChain3Ptr SwapChain;
    glm::uvec2 SwapChainSize;

    FrameData FrameObjects[kDefaultSwapChainBuffers];

    struct HeapData
    {
        ID3D12DescriptorHeapPtr Heap;
        uint32_t UsedEntries = 0;
    };
    HeapData RTVHeap;
    static const uint32_t RTVHeapSize = 3;

    ID3D12StateObjectPtr PipelineState;

    ID3D12ResourcePtr ShaderTable;
    uint32_t ShaderTableEntrySize = 0;

    ID3D12ResourcePtr OutputTexture;
    GlobalBindings GlobalResources;
    static const uint32_t HeapSize = 2;

    ID3D12ResourcePtr SpheresBuffer;

    ID3D12ResourcePtr Texture;
    std::array<Material, MaterialType::Count> MatArray = {};
    ID3D12ResourcePtr Materials;

    Camera SceneCamera;
};