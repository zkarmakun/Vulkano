#include "VertexInputs.h"

FVertexInput::FVertexInput()
{
    PipelineVertexInputStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    PipelineVertexInputStateCreateInfo.vertexBindingDescriptionCount = 0;
    PipelineVertexInputStateCreateInfo.vertexAttributeDescriptionCount = 0;
}

const VkPipelineVertexInputStateCreateInfo& FVertexInput::GetInputVertexState() const
{
    return PipelineVertexInputStateCreateInfo;
}

void FVertexInput::InitVertexInput(uint32_t Binding)
{
    // If components make an actual vertex input, otherwise just pass an empty declaration
    if(!Components.empty())
    {
        PipelineVertexInputStateCreateInfo.vertexBindingDescriptionCount = 1;
        PipelineVertexInputStateCreateInfo.pVertexBindingDescriptions = &VertexInputBindingDescription;
        PipelineVertexInputStateCreateInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(Components.size());
        PipelineVertexInputStateCreateInfo.pVertexAttributeDescriptions = Components.data();
    }
}

namespace VKGlobals
{
    std::shared_ptr<FBasicVertexInput> GBasicVertexInput = nullptr;
    
    void InitGlobalResources()
    {
        GBasicVertexInput = std::make_unique<FBasicVertexInput>();
        GBasicVertexInput->InitVertexInput(0);
    }

    void CleanupGlobalResources()
    {
        GBasicVertexInput.reset();
    }
}
