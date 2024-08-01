#include "Renderer.h"
#include <set>
#include <sstream>
#include "VulkanInterface.h"
#include "Core/Assertion.h"
#include "glm/glm.hpp"

FRenderer::FRenderer()
{
}

void FRenderer::Init(FRenderWindow* RenderWindow)
{
    pRenderWindow = RenderWindow;
	FVulkan::CreateVulkanInstance("Vulkano");
	FVulkan::CreateVulkanDebugLayer();
	SurfaceKHR = FVulkan::CreateSurface(pRenderWindow->GetHInstance(), pRenderWindow->GetWindow());
	FVulkan::CreateVulkanDevice(SurfaceKHR);
	/*vkGetDeviceQueue(Device, graphics_QueueFamilyIndex, 0, &GraphicsQueue);
	vkGetDeviceQueue(Device, present_QueueFamilyIndex, 0, &PresentQueue);*/
	SwapChain2.CreateSwapChain(SurfaceKHR, pRenderWindow->GetWidth(), pRenderWindow->GetHeight());

	bInitialized = true;
}

void FRenderer::RenderLoop()
{
	MSG msg;
	bool quitMessageReceived = false;
	while (!quitMessageReceived)
	{
		while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			if (msg.message == WM_QUIT)
			{
				quitMessageReceived = true;
				break;
			}
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		
		if (bInitialized && !IsIconic(pRenderWindow->GetWindow()))
		{
			SwapChain2.AcquireNextImage();
			// AcquireNextImage();
		}
	}
}

void FRenderer::Shutdown()
{
	if(!bInitialized)
	{
		return;
	}
    
	vkDestroySurfaceKHR(FVulkan::GetInstance(), SurfaceKHR, nullptr);
	vkDestroyInstance(FVulkan::GetInstance(), nullptr);
}
