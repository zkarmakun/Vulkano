#include "Renderer.h"
#include <set>
#include <sstream>

#include "Shader.h"
#include "VertexInputs.h"
#include "VulkanInterface.h"
#include "Core/Assertion.h"
#include "glm/glm.hpp"

void FVulkanGBuffer::CreateGBuffer(VkExtent2D ViewSize)
{
	GBufferA = FVulkan::CreateTexture(
		ViewSize.width, ViewSize.height, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
}

void FVulkanGBuffer::ReleaseGBuffer()
{
	FVulkan::ReleaseTexture(GBufferA);
}

FRenderer::FRenderer()
{
}

void FRenderer::Init(FRenderWindow* RenderWindow)
{
    pRenderWindow = RenderWindow;
	CreateSwapChain();
	GBuffer.CreateGBuffer(ViewportSize);
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
			FRenderPassInfo RenderPassInfo({GBuffer.GBufferA});
			FRenderPassRef RenderPass = FVulkan::BeginRenderPass(RenderPassInfo, ViewportSize, "Render triangle");
			{
				std::shared_ptr<FDefaultVertexShader> VertexShader = FShaderCompiler::Get()->FindShader<FDefaultVertexShader>();
				std::shared_ptr<FDefaultPixelShader> PixelShader = FShaderCompiler::Get()->FindShader<FDefaultPixelShader>();
				
				FGraphicsPipelineInitializer GraphicsPSOInit;
				GraphicsPSOInit.VertexShader = VertexShader;
				GraphicsPSOInit.PixelShader = PixelShader;
				GraphicsPSOInit.PrimitiveTopology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
				GraphicsPSOInit.VertexInput = VKGlobals::GBasicVertexInput;
				GraphicsPSOInit.RenderPass = RenderPass;
				FVulkan::SetGraphicsPipeline(GraphicsPSOInit);

				FVulkan::SetScissorRect(false, 0, 0, 0, 0);
				FVulkan::SetViewport(0.0f, 0.0f, 0.0f, static_cast<float>(ViewportSize.width), static_cast<float>(ViewportSize.height), 1.0f);
			}
			FVulkan::EndRenderPass();
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

	// This is released manually since the SwapChain owns the VkImages and the Memories
	for(FVulkanTexture& Texture : SwapChainTextures)
	{
		if(Texture.ImageView != VK_NULL_HANDLE)
		{
			vkDestroyImageView(FVulkan::GetDevice(), Texture.ImageView, nullptr);
		}
	}
	
	if(SwapChain != VK_NULL_HANDLE)
	{
		vkDestroySwapchainKHR(FVulkan::GetDevice(), SwapChain, nullptr);
		VK_LOG(LOG_INFO, "Destroyed swap chain");
	}

	if(SurfaceKHR != VK_NULL_HANDLE)
	{
		vkDestroySurfaceKHR(FVulkan::GetInstance(), SurfaceKHR, nullptr);
		VK_LOG(LOG_INFO, "Destroyed KHR surface");
	}

	GBuffer.ReleaseGBuffer();
}

void FRenderer::CreateSwapChain()
{
	// Create swap chain
	SurfaceKHR = FVulkan::CreateSurface(pRenderWindow->GetHInstance(), pRenderWindow->GetWindow());
	if(SurfaceKHR != VK_NULL_HANDLE)
	{
		VK_LOG(LOG_INFO, "Creating Surface Win64");
	}

	VkSurfaceCapabilitiesKHR SurfaceCapabilitiesKHR;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(FVulkan::GetPhysicalDevice(), SurfaceKHR, &SurfaceCapabilitiesKHR);
   
    std::vector<VkSurfaceFormatKHR> surfaceFormats;
    uint32_t surfaceFormatsCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(FVulkan::GetPhysicalDevice(), SurfaceKHR, &surfaceFormatsCount, nullptr);
    surfaceFormats.resize(surfaceFormatsCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(FVulkan::GetPhysicalDevice(), SurfaceKHR, &surfaceFormatsCount, surfaceFormats.data());

    if(surfaceFormats[0].format != VK_FORMAT_B8G8R8A8_UNORM)
	{
        fatal("Failed creating swap chain, unsupported surface format");
	}
	
    VkSurfaceFormatKHR SurfaceFormatKHR = surfaceFormats[0];
    int width = pRenderWindow->GetWidth();
    int height = pRenderWindow->GetHeight();
    width = glm::clamp(width, static_cast<int>(SurfaceCapabilitiesKHR.minImageExtent.width), static_cast<int>(SurfaceCapabilitiesKHR.maxImageExtent.width));
    height = glm::clamp(height, static_cast<int>(SurfaceCapabilitiesKHR.minImageExtent.height), static_cast<int>(SurfaceCapabilitiesKHR.maxImageExtent.height));
    ViewportSize.width = width;
    ViewportSize.height = height;

    VkSwapchainCreateInfoKHR createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = SurfaceKHR;
    createInfo.minImageCount = SurfaceCapabilitiesKHR.minImageCount;
    createInfo.imageFormat = SurfaceFormatKHR.format;
    createInfo.imageColorSpace = SurfaceFormatKHR.colorSpace;
    createInfo.imageExtent = ViewportSize;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    createInfo.preTransform = SurfaceCapabilitiesKHR.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = VK_PRESENT_MODE_FIFO_KHR;
    createInfo.clipped = VK_TRUE;

    if(vkCreateSwapchainKHR(FVulkan::GetDevice(), &createInfo, nullptr, &SwapChain) != VK_SUCCESS)
    {
	    fatal("FVulkanSwapChain::CreateSwapChain Fail creating swap chain");
    }

	uint32_t SwapChainImageCount = 0;
	std::vector<VkImage> SwapChainImages;
    vkGetSwapchainImagesKHR(FVulkan::GetDevice(), SwapChain, &SwapChainImageCount, nullptr);
    SwapChainImages.resize(SwapChainImageCount);
    vkGetSwapchainImagesKHR(FVulkan::GetDevice(), SwapChain, &SwapChainImageCount, SwapChainImages.data());

    for(uint32_t i = 0; i < SwapChainImages.size(); i++)
    {
        VkImageView NewView = FVulkan::CreateImageView(SwapChainImages[i], SurfaceFormatKHR.format, VK_IMAGE_ASPECT_COLOR_BIT);
    	SwapChainTextures.push_back(FVulkanTexture(SwapChainImages[i], NewView, "SwapChainTexture"));
    }
}
