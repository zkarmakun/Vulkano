#pragma once
#include <array>
#include <memory>
#include <string>
#include <vector>
#include "vulkan/vulkan_core.h"

class FVertexInput;
class FShader;

class FGPUResource
{
public:
    FGPUResource(){}
    virtual ~FGPUResource(){}
    virtual bool IsValid() const { return false; }
    virtual void Release() {}
    
public:
    std::string ResourceName;
};

// Simple texture
class FVulkanTexture : public FGPUResource
{
public:
    FVulkanTexture();
    FVulkanTexture(const VkImage& InImage, const VkImageView& InImageView, std::string TextureName);
    virtual bool IsValid() const override;
    virtual void Release() override;
    
    uint32_t SizeX = 0;
    uint32_t SizeY = 0;
    VkFormat Format = VK_FORMAT_UNDEFINED;
    VkImageTiling ImageTilling = VK_IMAGE_TILING_LINEAR;
    VkImage Image = VK_NULL_HANDLE;
    VkDeviceMemory ImageMemory = VK_NULL_HANDLE;
    VkImageView ImageView = VK_NULL_HANDLE;
};

using FVulkanTextureRef = std::shared_ptr<FVulkanTexture>;

// Simple buffer
class FVulkanBuffer : public FGPUResource
{
public:
    virtual bool IsValid() const override;
    virtual void Release() override;
    uint32_t GetElemNum() const;
    void SetNumberOfElements(uint32_t Size);
    
    VkBuffer Buffer = VK_NULL_HANDLE;
    VkDeviceMemory BufferMemory = VK_NULL_HANDLE;

private:
    uint32_t NumberOfElements = 0;
};

enum
{
    MaxRenderTargets = 8
};

struct FRenderPassInfo
{
    struct FColorAttachment
    {
        std::shared_ptr<FVulkanTexture> Target = nullptr;
        VkAttachmentLoadOp Load = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        VkAttachmentStoreOp Store = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        VkClearColorValue ClearColor = {0.0f, 0.0f, 0.0f, 1.0f};
    };

    struct FDepthStencilAttachment
    {
        std::shared_ptr<FVulkanTexture> Target = nullptr;
        VkAttachmentLoadOp Load = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        VkAttachmentStoreOp Store = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        VkClearDepthStencilValue StencilClearColor = {1.0f, 0};
    };
    
    std::array<FColorAttachment, MaxRenderTargets> ColorRenderTargets;
    FDepthStencilAttachment DepthStencilRenderTarget;

    FRenderPassInfo(
        std::vector<std::shared_ptr<FVulkanTexture>> RenderTargets,
        VkAttachmentLoadOp Load = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        VkAttachmentStoreOp Store = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        std::shared_ptr<FVulkanTexture> DepthStencil = nullptr,
        VkAttachmentLoadOp StencilLoad = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        VkAttachmentStoreOp StencilStore = VK_ATTACHMENT_STORE_OP_DONT_CARE);
};

class FRenderPass
{
public:
    bool Valid() const;
    void Release() const;
    
    std::string RenderPassName;
    VkRenderPass RenderPass = VK_NULL_HANDLE;
    VkFramebuffer FrameBuffer = VK_NULL_HANDLE;
};

struct FGraphicsPipelineInitializer
{
    std::shared_ptr<FShader> VertexShader = nullptr;
    std::shared_ptr<FShader> PixelShader = nullptr;
    VkPrimitiveTopology PrimitiveTopology = VK_PRIMITIVE_TOPOLOGY_MAX_ENUM;
    std::shared_ptr<FVertexInput> VertexInput;
    FRenderPass* RenderPass = nullptr;
};

class FGraphicsPipeline
{
public:
    FGraphicsPipeline(const VkPipeline& Pipeline, const VkPipelineLayout& Layout);
    bool Valid() const;
    void Release();

    const VkPipeline& GetGraphicsPipeline() const;

private:
    VkPipeline GraphicsPipeline = VK_NULL_HANDLE;
    VkPipelineLayout PipeLineLayout = VK_NULL_HANDLE;
};
