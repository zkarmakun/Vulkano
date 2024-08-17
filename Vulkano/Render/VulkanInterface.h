#pragma once
#include <map>
#include <string>
#include <vector>
#include "vulkan/vulkan_core.h"
#include "RenderResources.h"
#include <Windows.h>

class FVulkan
{
public:
    static HWND CreateDummyWindow(HINSTANCE hInstance);
    static void CreateVulkanInstance(const std::string& ApplicationName);
    static void CreateVulkanDebugLayer();
    static void DestroyVulkanDebugLayer();
    static void CreateVulkanDevice(HINSTANCE hInstance);
    static void ExitVulkan();
    
    static VkSurfaceKHR CreateSurface(HINSTANCE hInstance, HWND hwnd);
    static VkBool32 GetSupportedDepthFormat(VkFormat* depthFormat);
    static VkQueue GetGraphicsQueue();
    static VkQueue GetPresentQueue();

    static std::vector<std::string> GetSupportedExtensions();
    static VkInstance& GetInstance();
    static VkDevice& GetDevice();
    static VkPhysicalDevice& GetPhysicalDevice();
    static uint32_t GetMajorVersion();
    static uint32_t GetMinorVersion();
    static uint32_t FindMemoryType(const VkPhysicalDevice& PhysicalDevice, uint32_t TypeFilter, VkMemoryPropertyFlags MemoryPropertyFlags);
    
    // Resources
    static void CreateImage(uint32_t Width, uint32_t Height, VkFormat Format, VkImageTiling Tiling, VkImageUsageFlags ImageUsageFlags, VkMemoryPropertyFlags MemoryPropertyFlags, VkImage& Image, VkDeviceMemory& ImageMemory);
    static VkImageView CreateImageView(VkImage Image, VkFormat Format, VkImageAspectFlags AspectFlags);
    
    static std::shared_ptr<FVulkanTexture> CreateTexture(int X, int Y, VkFormat Format, VkImageUsageFlags Flags, VkMemoryPropertyFlags MemoryFlags, std::string TextureName = "Texture", VkImageTiling Tilling = VK_IMAGE_TILING_LINEAR, VkImageAspectFlags AspectFlags = VK_IMAGE_ASPECT_COLOR_BIT);
    static void ReleaseTexture(std::shared_ptr<FVulkanTexture>& Texture);
    
    template<typename StructType>
    static FVulkanBuffer CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties);
    static void SetBufferData(FVulkanBuffer& Buffer, const void* BufferData, size_t BufferSize);

    static std::shared_ptr<FRenderPass> BeginRenderPass(const FRenderPassInfo& RenderPassInfo, VkExtent2D ViewSize, const std::string& RenderPassName);
    static std::shared_ptr<FGraphicsPipeline> SetGraphicsPipeline(const FGraphicsPipelineInitializer& PSOInitializer);
    static void SetScissorRect(bool bEnabled, float MinX, float MinY, float MaxX, float MaxY);
    static void SetViewport(float MinX, float MinY, float MinZ, float MaxX, float MaxY, float MaxZ);
    static void EndRenderPass();

private:
    static std::shared_ptr<FRenderPass> GetOrCreateRenderPass(const FRenderPassInfo& RenderPassInfo,  VkExtent2D ViewSize, const std::string& RenderPassName);
    static void SelectPhysicalDevice();
    
private:
    static std::map<std::size_t, std::shared_ptr<FRenderPass>> RenderPasses;
    static std::map<std::size_t, std::shared_ptr<FGraphicsPipeline>> PSOs;

    static VkInstance Instance;
    static VkDevice Device;
    static VkPhysicalDevice PhysicalDevice;

    static PFN_vkCreateDebugUtilsMessengerEXT vkCreateDebugUtilsMessengerEXT;
    static PFN_vkDestroyDebugUtilsMessengerEXT vkDestroyDebugUtilsMessengerEXT;
    static VkDebugUtilsMessengerEXT DebugUtilsMessenger;

    static uint32_t GraphicsIndex;
    static uint32_t ComputeIndex;
    static uint32_t PresentIndex;
    static VkQueue GraphicsQueue;
    static VkQueue PresentQueue;
    static VkQueue ComputeQueue;
    static uint32_t MajorVersion;
    static uint32_t MinorVersion;
};
