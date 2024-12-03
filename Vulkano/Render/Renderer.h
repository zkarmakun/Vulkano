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
    std::shared_ptr<FVulkanTexture> GetSwapChainTexture();
    void PresetImage() const; 
    
private:
    bool bInitialized = false;
    FRenderWindow* pRenderWindow = nullptr;

    VkSwapchainKHR SwapChain = VK_NULL_HANDLE;
    uint32_t FrameIndex = 0;
    VkSemaphore ImageAvailableSemaphore = VK_NULL_HANDLE;
    VkSemaphore RenderFinishedSemaphore = VK_NULL_HANDLE;
    VkFence Fence;
    VkSurfaceKHR SurfaceKHR = VK_NULL_HANDLE;
    VkExtent2D ViewportSize = {0, 0};

    std::vector<std::shared_ptr<FVulkanTexture>> SwapChainTextures;
    FVulkanGBuffer GBuffer;
};
