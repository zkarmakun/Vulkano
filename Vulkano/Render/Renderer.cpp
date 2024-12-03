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
		ViewSize.width,
		ViewSize.height,
		VK_FORMAT_R8G8B8A8_SRGB,
		VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		"GBufferA");
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
			// Acquire the next image from the swapchain
			uint32_t imageIndex;
			vkAcquireNextImageKHR(FVulkan::GetDevice(), SwapChain, UINT64_MAX,  ImageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);
			vkWaitForFences(FVulkan::GetDevice(), 1, &Fence, VK_FALSE, UINT64_MAX);
			vkResetFences(FVulkan::GetDevice(), 1, &Fence);  
			
			FVulkan::ResetGraphicsCommandBuffer();
			
			FRenderPassInfo RenderPassInfo({GBuffer.GBufferA});
			FRenderPass* RenderPass = FVulkan::BeginRenderPass(RenderPassInfo, ViewportSize, "Render Quad");
			{
				std::shared_ptr<FDefaultVertexShader> VertexShader = FShaderCompiler::Get()->FindShader<FDefaultVertexShader>();
				std::shared_ptr<FDefaultPixelShader> PixelShader = FShaderCompiler::Get()->FindShader<FDefaultPixelShader>();
				
				FGraphicsPipelineInitializer GraphicsPSOInit;
				GraphicsPSOInit.VertexShader = VertexShader;
				GraphicsPSOInit.PixelShader = PixelShader;
				GraphicsPSOInit.PrimitiveTopology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
				GraphicsPSOInit.VertexInput = VKGlobals::GSimpleVertexInput;
				GraphicsPSOInit.RenderPass = RenderPass;
				FVulkan::SetGraphicsPipeline(GraphicsPSOInit);

				FVulkan::SetScissorRect(false, 0, 0, 0, 0);
				FVulkan::SetViewport(0.0f, 0.0f, 0.0f, static_cast<float>(ViewportSize.width), static_cast<float>(ViewportSize.height), 1.0f);

				FVulkan::BindStreamResource(0, VKGlobals::GQuadVertexBuffer, 0);
				FVulkan::DrawPrimitive(0, VKGlobals::GQuadVertexBuffer->GetElemNum(), 1);
			}
			FVulkan::EndRenderPass();

			// Copy to swap chain
			FVulkan::TransitionBarrier(GBuffer.GBufferA, SwapChainTextures[imageIndex]);
			FVulkan::CopyTexture(GBuffer.GBufferA, SwapChainTextures[imageIndex]);

			/*FVulkan::EndGraphicsCommandBuffer();
			
			PresetImage();*/
			vkEndCommandBuffer(FVulkan::GetGraphicsBuffer());

			VkSubmitInfo submitInfo{};
			submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
			
			VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
			submitInfo.waitSemaphoreCount = 1;
			submitInfo.pWaitSemaphores = &ImageAvailableSemaphore;
			submitInfo.pWaitDstStageMask = waitStages;

			submitInfo.commandBufferCount = 1;
			submitInfo.pCommandBuffers = &FVulkan::GetGraphicsBuffer();
			
			submitInfo.signalSemaphoreCount = 1;
			submitInfo.pSignalSemaphores = &RenderFinishedSemaphore;

			vkQueueSubmit(FVulkan::GetGraphicsQueue(), 1, &submitInfo, VK_NULL_HANDLE);

			// Present the image
			VkPresentInfoKHR presentInfo{};
			presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
			presentInfo.waitSemaphoreCount = 1;
			presentInfo.pWaitSemaphores = &RenderFinishedSemaphore;
			
			presentInfo.swapchainCount = 1;
			presentInfo.pSwapchains = &SwapChain;
			presentInfo.pImageIndices = &imageIndex;

			vkQueuePresentKHR(FVulkan::GetPresentQueue(), &presentInfo);
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
	for(std::shared_ptr<FVulkanTexture>& Texture : SwapChainTextures)
	{
		if(Texture->ImageView != VK_NULL_HANDLE)
		{
			vkDestroyImageView(FVulkan::GetDevice(), Texture->ImageView, nullptr);
		}
		Texture.reset();
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

	if(ImageAvailableSemaphore != VK_NULL_HANDLE)
	{
		vkDestroySemaphore(FVulkan::GetDevice(), ImageAvailableSemaphore, nullptr);
	}

	if(RenderFinishedSemaphore != VK_NULL_HANDLE)
	{
		vkDestroySemaphore(FVulkan::GetDevice(), RenderFinishedSemaphore, nullptr);
	}

	if(Fence != VK_NULL_HANDLE)
	{
		vkDestroyFence(FVulkan::GetDevice(), Fence, nullptr);
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
    	SwapChainTextures.push_back(std::make_shared<FVulkanTexture>(SwapChainImages[i], NewView, "SwapChainTexture"));
    }

	// Create phores
	VkSemaphoreCreateInfo SemaphoreCreateInfo = {};
	SemaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	vkCreateSemaphore(FVulkan::GetDevice(), &SemaphoreCreateInfo, nullptr, &ImageAvailableSemaphore);
	VkSemaphoreCreateInfo SemaphoreCreateInfo2 = {};
	SemaphoreCreateInfo2.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	vkCreateSemaphore(FVulkan::GetDevice(), &SemaphoreCreateInfo2, nullptr, &RenderFinishedSemaphore);

	VkFenceCreateInfo FenceCreateInfo = {};
	FenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	FenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
	if(vkCreateFence(FVulkan::GetDevice(), &FenceCreateInfo, nullptr, &Fence) != VK_SUCCESS)
	{
		fatal("FVulkanSwapChain::CreateFences Fail creating fences");
	}
}

std::shared_ptr<FVulkanTexture> FRenderer::GetSwapChainTexture()
{
	vkAcquireNextImageKHR(FVulkan::GetDevice(),
		SwapChain,
		UINT64_MAX,
		ImageAvailableSemaphore,
		VK_NULL_HANDLE,
		&FrameIndex);

	return SwapChainTextures[FrameIndex];

	/*vkWaitForFences(FVulkan::GetDevice(), 1, &Fences[FrameIndex], VK_FALSE, UINT64_MAX);
	vkResetFences(FVulkan::GetDevice(), 1, &Fences[FrameIndex]);  */
}

void FRenderer::PresetImage() const
{
	VkPresentInfoKHR presentInfo{};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = &RenderFinishedSemaphore;
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = &SwapChain;
	presentInfo.pImageIndices = &FrameIndex;
	vkQueuePresentKHR(FVulkan::GetPresentQueue(), &presentInfo);
}
