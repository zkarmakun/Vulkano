#pragma once
#include <vector>
#include "RenderResources.h"
#include "vulkan/vulkan_core.h"
#include "Windows.h"

class FVulkanSwapChain
{
public:
    void CreateSwapChain(HINSTANCE hInstance, HWND hwnd, int SizeX, int SizeY);
    void DestroySwapChain();
    void AcquireNextImage();

private:
    void SetupDepthStencil();
    void CreateRenderPass();
    void CreateFrameBuffers();
    void CreateCommandPool();
    void CreateCommandBuffers();
    void CreateSemaphores();
    void CreateFences();

private:
    VkSurfaceCapabilitiesKHR SurfaceCapabilitiesKHR;
    VkSurfaceFormatKHR SurfaceFormatKHR;
    VkExtent2D ViewportSize = {0, 0};
    VkSwapchainKHR SwapChain = VK_NULL_HANDLE;
    VkSurfaceKHR SurfaceKHR = VK_NULL_HANDLE;
    uint32_t SwapChainImageCount = 0;

    std::vector<VkImage> SwapChainImages;
    std::vector<VkImageView> SwapChainImagesViews;

    FVulkanTexture DepthStencil;
    
    VkFormat DepthFormat;
    VkRenderPass RenderPass = VK_NULL_HANDLE;
    std::vector<VkFramebuffer> SwapChainFrameBuffers;
    VkCommandPool CommandPool = VK_NULL_HANDLE;
    std::vector<VkCommandBuffer> CommandBuffers;

    VkSemaphore ImageAvailableSemaphore;
    VkSemaphore RenderingFinishedSemaphore;

    std::vector<VkFence> Fences;
    uint32_t FrameIndex;
};
