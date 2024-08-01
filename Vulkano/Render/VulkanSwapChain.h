#pragma once
#include <vector>
#include "RenderResources.h"
#include "vulkan/vulkan_core.h"

class FVulkanSwapChain
{
public:
    void CreateSwapChain(const VkSurfaceKHR& SurfaceKHR, int SizeX, int SizeY);
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
    VkExtent2D ViewportSize;
    VkSwapchainKHR SwapChain;
    uint32_t SwapChainImageCount;

    std::vector<VkImage> SwapChainImages;
    std::vector<VkImageView> SwapChainImagesViews;

    FVulkanTexture DepthStencil;


    VkFormat DepthFormat;
    VkRenderPass RenderPass;
    std::vector<VkFramebuffer> SwapChainFrameBuffers;
    VkCommandPool CommandPool;
    std::vector<VkCommandBuffer> CommandBuffers;

    VkSemaphore ImageAvailableSemaphore;
    VkSemaphore RenderingFinishedSemaphore;

    std::vector<VkFence> Fences;
    uint32_t FrameIndex;

    VkQueue GraphicsQueue;
    VkQueue PresentQueue;
};
