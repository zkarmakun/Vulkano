#include "RenderResources.h"

#include "Shader.h"
#include "VulkanInterface.h"
#include "Core/VulkanoLog.h"


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

bool FRenderPass::Valid() const
{
    return RenderPass != VK_NULL_HANDLE;
}

void FRenderPass::Release() const
{
    if(Valid())
    {
        vkDestroyRenderPass(FVulkan::GetDevice(), RenderPass, nullptr);
    }
}
