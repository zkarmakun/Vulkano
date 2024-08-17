#pragma once
#include <vector>

#include "RenderWindow.h"
#include "VulkanSwapChain.h"
#include "vulkan/vulkan_core.h"

class FVulkanGBuffer
{
public:
    void CreateGBuffer(VkExtent2D ViewSize);
    void ReleaseGBuffer();
    
    FVulkanTextureRef GBufferA;
    FVulkanTextureRef GBufferB;
    FVulkanTextureRef GBufferC;
    FVulkanTextureRef GBufferD;
};

class FRenderer
{
public:
    FRenderer();
    void Init(FRenderWindow* RenderWindow);
    void RenderLoop();
    void Shutdown();

private:
    void CreateSwapChain();

private:
    bool bInitialized = false;
    FRenderWindow* pRenderWindow = nullptr;

    VkSwapchainKHR SwapChain = VK_NULL_HANDLE;
    VkSurfaceKHR SurfaceKHR = VK_NULL_HANDLE;
    VkExtent2D ViewportSize = {0, 0};

    std::vector<FVulkanTexture> SwapChainTextures;
    FVulkanTexture DepthStencil;

    FVulkanGBuffer GBuffer;
};
