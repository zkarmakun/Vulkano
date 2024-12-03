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
    static VkCommandBuffer& GetGraphicsBuffer();

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
    
    static std::shared_ptr<FVulkanTexture> CreateTexture(uint32_t X, uint32_t Y, VkFormat Format, VkImageUsageFlags Flags, VkMemoryPropertyFlags MemoryFlags, const std::string& TextureName = "Texture", VkImageTiling Tilling = VK_IMAGE_TILING_LINEAR, VkImageAspectFlags AspectFlags = VK_IMAGE_ASPECT_COLOR_BIT);
    static void ReleaseTexture(std::shared_ptr<FVulkanTexture>& Texture);
    
    static std::shared_ptr<FVulkanBuffer> CreateBuffer(VkDeviceSize BufferSize, uint32_t ElemNumber, VkBufferUsageFlags BufferUsage, VkMemoryPropertyFlags MemoryProperties, const std::string& BufferName = "Buffer");
    static void UpdateBuffer(const std::shared_ptr<FVulkanBuffer>& Buffer, const void* BufferData, size_t BufferSize);

    static FRenderPass* BeginRenderPass(const FRenderPassInfo& RenderPassInfo, VkExtent2D ViewSize, const std::string& RenderPassName);
    static FGraphicsPipeline* SetGraphicsPipeline(const FGraphicsPipelineInitializer& PSOInitializer);
    static void BindStreamResource(int Index, std::shared_ptr<FVulkanBuffer> Buffer, uint64_t Offset);
    static void DrawPrimitive(uint32_t BaseVertexIndex, uint32_t VertexCount, uint32_t NumInstances);
    static void SetScissorRect(bool bEnabled, int32_t MinX, int32_t MinY, uint32_t MaxX, uint32_t MaxY);
    static void SetViewport(float MinX, float MinY, float MinZ, float MaxX, float MaxY, float MaxZ);
    static void EndRenderPass();
    static void ResetGraphicsCommandBuffer();
    static void EndGraphicsCommandBuffer();
    static void TransitionBarrier(const std::shared_ptr<FVulkanTexture> Input, const std::shared_ptr<FVulkanTexture> TransitionTo);
    static void CopyTexture(const std::shared_ptr<FVulkanTexture> Source, const std::shared_ptr<FVulkanTexture> Target);


private:
    static FRenderPass* GetOrCreateRenderPass(const FRenderPassInfo& RenderPassInfo,  VkExtent2D ViewSize, const std::string& RenderPassName);
    static void SelectPhysicalDevice();
    
private:
    static std::map<std::uint32_t, FRenderPass*> RenderPasses;
    static std::map<std::uint32_t, FGraphicsPipeline*> PSOs;

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
    static VkCommandPool GraphicsCommandPool;
    static VkCommandBuffer GraphicsCommandBuffer;
};
