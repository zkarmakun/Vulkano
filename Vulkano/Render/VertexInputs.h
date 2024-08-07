#pragma once
#include "memory"
#include "vulkan/vulkan_core.h"
#include "vector"
#include "glm/glm.hpp"

struct FStaticVertex
{
public:
    glm::vec3 Position = glm::vec3(0);
    glm::vec3 Normal = glm::vec3(0);
    glm::vec2 UV0 = glm::vec2(0);
    glm::vec3 Color = glm::vec3(0);
};

class FVertexInput
{
public:
    FVertexInput();
    const VkPipelineVertexInputStateCreateInfo& GetInputVertexState() const; 
    virtual void InitVertexInput(uint32_t Binding);
    VkPipelineVertexInputStateCreateInfo PipelineVertexInputStateCreateInfo;
    VkVertexInputBindingDescription VertexInputBindingDescription;

protected:
    std::vector<VkVertexInputAttributeDescription> Components;
};

class FBasicVertexInput : public FVertexInput
{
public:
    virtual void InitVertexInput(uint32_t Binding) override
    {
        Components.push_back({0, Binding, VK_FORMAT_R32G32B32_SFLOAT, offsetof(FStaticVertex, Position)});
        Components.push_back({1, Binding, VK_FORMAT_R32G32B32_SFLOAT, offsetof(FStaticVertex, Normal)});
        Components.push_back({2, Binding, VK_FORMAT_R32G32_SFLOAT, offsetof(FStaticVertex, UV0)});
        Components.push_back({3, Binding, VK_FORMAT_R32G32B32_SFLOAT, offsetof(FStaticVertex, Color)});
        VertexInputBindingDescription = { Binding, sizeof(FStaticVertex), VK_VERTEX_INPUT_RATE_VERTEX };
        FVertexInput::InitVertexInput(Binding);
    }
};

namespace VKGlobals
{
    extern std::shared_ptr<FBasicVertexInput> GBasicVertexInput;

    void InitGlobalResources();
    void CleanupGlobalResources();
}
