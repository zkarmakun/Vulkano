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
	SwapChain2.CreateSwapChain(pRenderWindow->GetHInstance(), pRenderWindow->GetWindow(), pRenderWindow->GetWidth(), pRenderWindow->GetHeight());
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
		}
	}
}

void FRenderer::Shutdown()
{
	if(!bInitialized)
	{
		return;
	}
	bInitialized = false;
	SwapChain2.DestroySwapChain();
}
