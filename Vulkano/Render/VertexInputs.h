#pragma once
#include "memory"
#include "vulkan/vulkan_core.h"
#include "vector"
#include "glm/glm.hpp"

class FVulkanBuffer;

struct FSimpleVertex
{
    glm::vec2 Position = glm::vec3(0);
    glm::vec2 UV = glm::vec2(0);
};

struct FStaticMeshVertex
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

class FSimpleVertexInput : public FVertexInput
{
public:
    virtual void InitVertexInput(uint32_t Binding) override
    {
        Components.push_back({0, Binding, VK_FORMAT_R32G32_SFLOAT, offsetof(FSimpleVertex, Position)});
        Components.push_back({1, Binding, VK_FORMAT_R32G32_SFLOAT, offsetof(FSimpleVertex, UV)});
        VertexInputBindingDescription = { Binding, sizeof(FSimpleVertex), VK_VERTEX_INPUT_RATE_VERTEX };
        FVertexInput::InitVertexInput(Binding);
    }
};

class FStaticVertexInput : public FVertexInput
{
public:
    virtual void InitVertexInput(uint32_t Binding) override
    {
        Components.push_back({0, Binding, VK_FORMAT_R32G32B32_SFLOAT, offsetof(FStaticMeshVertex, Position)});
        Components.push_back({1, Binding, VK_FORMAT_R32G32B32_SFLOAT, offsetof(FStaticMeshVertex, Normal)});
        Components.push_back({2, Binding, VK_FORMAT_R32G32_SFLOAT, offsetof(FStaticMeshVertex, UV0)});
        Components.push_back({3, Binding, VK_FORMAT_R32G32B32_SFLOAT, offsetof(FStaticMeshVertex, Color)});
        VertexInputBindingDescription = { Binding, sizeof(FStaticMeshVertex), VK_VERTEX_INPUT_RATE_VERTEX };
        FVertexInput::InitVertexInput(Binding);
    }
};

namespace VKGlobals
{
    extern std::shared_ptr<FSimpleVertexInput> GSimpleVertexInput;
    extern std::shared_ptr<FStaticVertexInput> GStaticMeshVertexInput;
    extern std::shared_ptr<FVulkanBuffer> GQuadVertexBuffer;

    void InitGlobalResources();
    void CleanupGlobalResources();
}
