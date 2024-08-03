#include "RenderResources.h"

bool FVulkanTexture::Valid() const
{
    return Image != VK_NULL_HANDLE && ImageMemory != VK_NULL_HANDLE && ImageView != VK_NULL_HANDLE;
}
