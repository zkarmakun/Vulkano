#pragma once
#include <string>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include "vulkan/vulkan_core.h"

struct FStaticVertex
{
public:
    glm::vec3 Position = glm::vec3(0);
    glm::vec3 Normal = glm::vec3(0);
    glm::vec2 UV0 = glm::vec2(0);
    glm::vec3 Color = glm::vec3(0);
};

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
