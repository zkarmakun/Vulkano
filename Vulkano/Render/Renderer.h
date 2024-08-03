#pragma once
#include <vector>

#include "RenderWindow.h"
#include "VulkanSwapChain.h"
#include "vulkan/vulkan_core.h"

class FRenderer
{
public:
    FRenderer();
    void Init(FRenderWindow* RenderWindow);
    void RenderLoop();
    void Shutdown();

private:
    FVulkanSwapChain SwapChain2;
    
private:
    bool bInitialized = false;
    FRenderWindow* pRenderWindow = nullptr;
};
