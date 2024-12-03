#include "VertexInputs.h"

#include "RenderResources.h"
#include "VulkanInterface.h"

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
    std::shared_ptr<FSimpleVertexInput> GSimpleVertexInput = nullptr;
    std::shared_ptr<FStaticVertexInput> GStaticMeshVertexInput = nullptr;
    std::shared_ptr<FVulkanBuffer> GQuadVertexBuffer = nullptr;
    
    void InitGlobalResources()
    {
        GSimpleVertexInput = std::make_unique<FSimpleVertexInput>();
        GSimpleVertexInput->InitVertexInput(0);
        
        GStaticMeshVertexInput = std::make_unique<FStaticVertexInput>();
        GStaticMeshVertexInput->InitVertexInput(0);
        
        std::vector<FSimpleVertex> Vertices = {
            {{-1.0f, -1.0f}, {0.0f, 0.0f}},  // Bottom-left
            {{1.0f, -1.0f}, {1.0f, 0.0f}},   // Bottom-right
            {{-1.0f, 1.0f}, {0.0f, 1.0f}},   // Top-left

            {{1.0f, -1.0f}, {1.0f, 0.0f}},   // Bottom-right
            {{1.0f, 1.0f}, {1.0f, 1.0f}},    // Top-right
            {{-1.0f, 1.0f}, {0.0f, 1.0f}}    // Top-left
        };

        VkDeviceSize ByteSize = sizeof(FSimpleVertex) * Vertices.size();
        GQuadVertexBuffer = FVulkan::CreateBuffer(
            ByteSize,
            static_cast<uint32_t>(Vertices.size()),
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            "SimpleQuadBuffer");
        FVulkan::UpdateBuffer(GQuadVertexBuffer, Vertices.data(), ByteSize);
    }

    void CleanupGlobalResources()
    {
        GSimpleVertexInput.reset();
        GStaticMeshVertexInput.reset();
        GQuadVertexBuffer->Release();
        GQuadVertexBuffer.reset();
    }
}
