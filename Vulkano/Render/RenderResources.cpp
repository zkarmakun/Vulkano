#include "RenderResources.h"

#include "Shader.h"
#include "VulkanInterface.h"
#include "Core/Assertion.h"
#include "Core/VulkanoLog.h"


FVulkanTexture::FVulkanTexture()
{
}

FVulkanTexture::FVulkanTexture(const VkImage& InImage, const VkImageView& InImageView, std::string TextureName)
{
    Image = InImage;
    ImageView = InImageView;
    Name = TextureName; 
}

bool FVulkanTexture::Valid() const
{
    return ImageView != VK_NULL_HANDLE;
}

FGraphicsPipeline::FGraphicsPipeline(const VkPipeline& Pipeline, const VkPipelineLayout& Layout)
{
    GraphicsPipeline = Pipeline;
    PipeLineLayout = Layout;
}

bool FGraphicsPipeline::Valid() const
{
    return GraphicsPipeline != VK_NULL_HANDLE;
}

void FGraphicsPipeline::Release()
{
    if(Valid())
    {
        vkDestroyPipeline(FVulkan::GetDevice(), GraphicsPipeline, nullptr);
        GraphicsPipeline = VK_NULL_HANDLE;
        vkDestroyPipelineLayout(FVulkan::GetDevice(), PipeLineLayout, nullptr);
        PipeLineLayout = VK_NULL_HANDLE;
    }
}

FRenderPassInfo::FRenderPassInfo(std::vector<std::shared_ptr<FVulkanTexture>> RenderTargets, VkAttachmentLoadOp Load,
    VkAttachmentStoreOp Store, std::shared_ptr<FVulkanTexture> DepthStencil, VkAttachmentLoadOp StencilLoad,
    VkAttachmentStoreOp StencilStore)
{
    if(RenderTargets.size() > MaxRenderTargets)
    {
        fatal("FRenderPassInfo::FRenderPassInfo, RenderTarget size cannot be greater than 8, MRT maximum");
    }
    
    for (size_t i = 0; i < RenderTargets.size(); ++i)
    {
        ColorRenderTargets[i].Load = Load;
        ColorRenderTargets[i].Store = Store;
        ColorRenderTargets[i].Target = RenderTargets[i];
    }

    if(DepthStencil)
    {
        DepthStencilRenderTarget.Load = StencilLoad;
        DepthStencilRenderTarget.Store = StencilStore;
        DepthStencilRenderTarget.Target = DepthStencil;
    }
}

bool FRenderPass::Valid() const
{
    return RenderPass != VK_NULL_HANDLE;
}

void FRenderPass::Release() const
{
    if(Valid())
    {
        vkDestroyFramebuffer(FVulkan::GetDevice(), FrameBuffer, nullptr);
        vkDestroyRenderPass(FVulkan::GetDevice(), RenderPass, nullptr);
    }
}
