#pragma once
#include <memory>
#include <string>
#include "vulkan/vulkan_core.h"

class FVertexInput;
class FShader;

// Simple texture
class FVulkanTexture
{
public:
    bool Valid() const;
    
    uint32_t SizeX = 0;
    uint32_t SizeY = 0;
    VkFormat Format = VK_FORMAT_UNDEFINED;
    VkImageTiling ImageTilling = VK_IMAGE_TILING_LINEAR;
    VkImage Image = VK_NULL_HANDLE;
    VkDeviceMemory ImageMemory = VK_NULL_HANDLE;
    VkImageView ImageView = VK_NULL_HANDLE;
    std::string Name;
};

// Simple buffer
class FVulkanBuffer
{
public:
    VkBuffer Buffer = VK_NULL_HANDLE;
    VkDeviceMemory BufferMemory = VK_NULL_HANDLE;
};

struct FGraphicsPipelineInitializer
{
    std::shared_ptr<FShader> VertexShader = nullptr;
    std::shared_ptr<FShader> PixelShader = nullptr;
    VkPrimitiveTopology PrimitiveTopology = VK_PRIMITIVE_TOPOLOGY_MAX_ENUM;
    std::shared_ptr<FVertexInput> VertexInput;
};

class FGraphicsPipeline
{
public:
    FGraphicsPipeline(const VkPipeline& Pipeline, const VkPipelineLayout& Layout);
    bool Valid() const;
    void Release();

private:
    VkPipeline GraphicsPipeline = VK_NULL_HANDLE;
    VkPipelineLayout PipeLineLayout = VK_NULL_HANDLE;
};

class FRenderPass
{
public:
    bool Valid() const;
    void Release() const;
    
    std::string RenderPassName;
    uint32_t Id;
    VkRenderPass RenderPass = VK_NULL_HANDLE;
};
