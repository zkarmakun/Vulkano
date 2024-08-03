#include "VulkanSwapChain.h"
#include <vector>
#include <glm/common.hpp>
#include "VulkanInterface.h"
#include "Core/Assertion.h"

void FVulkanSwapChain::CreateSwapChain(HINSTANCE hInstance, HWND hwnd, int SizeX, int SizeY)
{
	SurfaceKHR = FVulkan::CreateSurface(hInstance, hwnd);
	if(SurfaceKHR != VK_NULL_HANDLE)
	{
		VK_LOG(LOG_INFO, "Creating Surface Win64");
	}
	
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(FVulkan::GetPhysicalDevice(), SurfaceKHR, &SurfaceCapabilitiesKHR);
   
    std::vector<VkSurfaceFormatKHR> surfaceFormats;
    uint32_t surfaceFormatsCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(FVulkan::GetPhysicalDevice(), SurfaceKHR, &surfaceFormatsCount, nullptr);
    surfaceFormats.resize(surfaceFormatsCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(FVulkan::GetPhysicalDevice(), SurfaceKHR, &surfaceFormatsCount, surfaceFormats.data());

    if(surfaceFormats[0].format != VK_FORMAT_B8G8R8A8_UNORM)
	{
        checkf(0, "Failed creating swap chain, unsupported surface format");
	}

    SurfaceFormatKHR = surfaceFormats[0];
    int width = SizeX;
    int height = SizeY;
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
	    checkf(0, "FVulkanSwapChain::CreateSwapChain Fail creating swap chain");
    }

    vkGetSwapchainImagesKHR(FVulkan::GetDevice(), SwapChain, &SwapChainImageCount, nullptr);
    SwapChainImages.resize(SwapChainImageCount);
    vkGetSwapchainImagesKHR(FVulkan::GetDevice(), SwapChain, &SwapChainImageCount, SwapChainImages.data());

    SwapChainImagesViews.resize(SwapChainImages.size());

    for(uint32_t i = 0; i < SwapChainImages.size(); i++)
    {
        SwapChainImagesViews[i] = FVulkan::CreateImageView(SwapChainImages[i], SurfaceFormatKHR.format, VK_IMAGE_ASPECT_COLOR_BIT);
    }
    
    SetupDepthStencil();
	CreateRenderPass();
	CreateFrameBuffers();
	CreateCommandPool();
	CreateCommandBuffers();
	CreateSemaphores();
	CreateFences();

	VK_LOG(LOG_INFO, "Creating swap chain");
}

void FVulkanSwapChain::DestroySwapChain()
{
	for(const VkImageView& ImageView : SwapChainImagesViews)
	{
		if(ImageView != VK_NULL_HANDLE)
		{
			vkDestroyImageView(FVulkan::GetDevice(), ImageView, nullptr);
		}
	}
	
	if(SwapChain != VK_NULL_HANDLE)
	{
		vkDestroySwapchainKHR(FVulkan::GetDevice(), SwapChain, nullptr);
	}

	if(SurfaceKHR != VK_NULL_HANDLE)
	{
		vkDestroySurfaceKHR(FVulkan::GetInstance(), SurfaceKHR, nullptr);
	}

	if(ImageAvailableSemaphore != VK_NULL_HANDLE)
	{
		vkDestroySemaphore(FVulkan::GetDevice(), ImageAvailableSemaphore, nullptr);
	}

	if(RenderingFinishedSemaphore != VK_NULL_HANDLE)
	{
		vkDestroySemaphore(FVulkan::GetDevice(), RenderingFinishedSemaphore, nullptr);
	}

	for(const VkCommandBuffer& CommandBuffer : CommandBuffers)
	{
		if (CommandBuffer != VK_NULL_HANDLE && CommandPool != VK_NULL_HANDLE)
		{
			vkFreeCommandBuffers(FVulkan::GetDevice(), CommandPool, 1, &CommandBuffer);
		}
	}

	if (CommandPool != VK_NULL_HANDLE)
	{
		vkDestroyCommandPool(FVulkan::GetDevice(), CommandPool, nullptr);
	}

	for(const VkFence& Fence : Fences)
	{
		if(Fence != VK_NULL_HANDLE)
		{
			vkDestroyFence(FVulkan::GetDevice(), Fence, nullptr);
		}
	}

	if(RenderPass != VK_NULL_HANDLE)
	{
		vkDestroyRenderPass(FVulkan::GetDevice(), RenderPass, nullptr);
	}

	FVulkan::ReleaseTexture(DepthStencil);

	for(const VkFramebuffer& Buffer : SwapChainFrameBuffers)
	{
		if(Buffer != VK_NULL_HANDLE)
		{
			vkDestroyFramebuffer(FVulkan::GetDevice(), Buffer, nullptr);
		}
	}

	VK_LOG(LOG_INFO, "Destroy swap chain");
}

void FVulkanSwapChain::AcquireNextImage()
{
	vkAcquireNextImageKHR(FVulkan::GetDevice(),
		SwapChain,
		UINT64_MAX,
		ImageAvailableSemaphore,
		VK_NULL_HANDLE,
		&FrameIndex);

	vkWaitForFences(FVulkan::GetDevice(), 1, &Fences[FrameIndex], VK_FALSE, UINT64_MAX);
	vkResetFences(FVulkan::GetDevice(), 1, &Fences[FrameIndex]);  

	VkCommandBuffer& CommandBuffer = CommandBuffers[FrameIndex];
	VkImage& Image = SwapChainImages[FrameIndex];

	vkResetCommandBuffer(CommandBuffer, 0);

	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
	vkBeginCommandBuffer(CommandBuffer, &beginInfo);
	
	{
		VkClearColorValue ClearColor = {0.1f, 0.1f, 0.1f, 1.0f};
		VkClearDepthStencilValue ClearDepthStencilValue = {1.0f, 0};

		VkRenderPassBeginInfo render_pass_info = {};
		render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		render_pass_info.renderPass        = RenderPass;
		render_pass_info.framebuffer       = SwapChainFrameBuffers[FrameIndex];
		render_pass_info.renderArea.offset = {0, 0};
		render_pass_info.renderArea.extent = ViewportSize;

		std::vector<VkClearValue> clearValues(2);
		clearValues[0].color = ClearColor;
		clearValues[1].depthStencil = ClearDepthStencilValue;

		render_pass_info.clearValueCount = static_cast<uint32_t>(clearValues.size());
		render_pass_info.pClearValues = clearValues.data();
	
		vkCmdBeginRenderPass(CommandBuffer, &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);


		vkCmdEndRenderPass(CommandBuffer);
	}

	VkPipelineStageFlags waitDestStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
	vkEndCommandBuffer(CommandBuffer);
	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = &ImageAvailableSemaphore;
	submitInfo.pWaitDstStageMask = &waitDestStageMask;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &CommandBuffer;
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = &RenderingFinishedSemaphore;
	vkQueueSubmit(FVulkan::GetGraphicsQueue(), 1, &submitInfo, Fences[FrameIndex]);

	VkPresentInfoKHR presentInfo = {};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = &RenderingFinishedSemaphore;
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = &SwapChain;
	presentInfo.pImageIndices = &FrameIndex;
	vkQueuePresentKHR(FVulkan::GetPresentQueue(), &presentInfo);

	vkQueueWaitIdle(FVulkan::GetPresentQueue());
}

void FVulkanSwapChain::SetupDepthStencil()
{
	DepthStencil = FVulkan::CreateTexture(
		ViewportSize.width,
		ViewportSize.height,
		VK_FORMAT_D32_SFLOAT_S8_UINT,
		VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		"DepthStencil",
		VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_ASPECT_DEPTH_BIT);
}

void FVulkanSwapChain::CreateRenderPass()
{
    FVulkan::GetSupportedDepthFormat(&DepthFormat);
	std::vector<VkAttachmentDescription> attachments(2);

	attachments[0].format = SurfaceFormatKHR.format;
	attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
	attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	attachments[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	attachments[1].format = DepthFormat;
	attachments[1].samples = VK_SAMPLE_COUNT_1_BIT;
	attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachments[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	attachments[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentReference colorReference = {};
	colorReference.attachment = 0;
	colorReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentReference depthReference = {};
	depthReference.attachment = 1;
	depthReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpassDescription = {};
	subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpassDescription.colorAttachmentCount = 1;
	subpassDescription.pColorAttachments = &colorReference;
	subpassDescription.pDepthStencilAttachment = &depthReference;
	subpassDescription.inputAttachmentCount = 0;
	subpassDescription.pInputAttachments = nullptr;
	subpassDescription.preserveAttachmentCount = 0;
	subpassDescription.pPreserveAttachments = nullptr;
	subpassDescription.pResolveAttachments = nullptr;

	std::vector<VkSubpassDependency> dependencies(1);

	dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
	dependencies[0].dstSubpass = 0;
	dependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
	dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

	VkRenderPassCreateInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
	renderPassInfo.pAttachments = attachments.data();
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpassDescription;
	renderPassInfo.dependencyCount = static_cast<uint32_t>(dependencies.size());
	renderPassInfo.pDependencies = dependencies.data();

	vkCreateRenderPass(FVulkan::GetDevice(), &renderPassInfo, nullptr, &RenderPass);
}

void FVulkanSwapChain::CreateFrameBuffers()
{
	SwapChainFrameBuffers.resize(SwapChainImagesViews.size());

	for (size_t i = 0; i < SwapChainImagesViews.size(); i++)
	{
		std::vector<VkImageView> attachments(2);
		attachments[0] = SwapChainImagesViews[i];
		attachments[1] = DepthStencil.ImageView;

		VkFramebufferCreateInfo framebufferInfo = {};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass = RenderPass;
		framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
		framebufferInfo.pAttachments = attachments.data();
		framebufferInfo.width = ViewportSize.width;
		framebufferInfo.height = ViewportSize.height;
		framebufferInfo.layers = 1;

		if (vkCreateFramebuffer(FVulkan::GetDevice(), &framebufferInfo, nullptr, &SwapChainFrameBuffers[i]) != VK_SUCCESS)
		{
			checkf(0, "CreateFramebuffers: failed to create framebuffer");
		}
	}
}

void FVulkanSwapChain::CreateCommandPool()
{
	VkCommandPoolCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	createInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT | VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
	createInfo.queueFamilyIndex = 0;
	vkCreateCommandPool(FVulkan::GetDevice(), &createInfo, nullptr, &CommandPool);
}

void FVulkanSwapChain::CreateCommandBuffers()
{
	VkCommandBufferAllocateInfo allocateInfo = {};
	allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocateInfo.commandPool = CommandPool;
	allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocateInfo.commandBufferCount = SwapChainImageCount;

	CommandBuffers.resize(SwapChainImageCount);
	vkAllocateCommandBuffers(FVulkan::GetDevice(), &allocateInfo, CommandBuffers.data());
}

void FVulkanSwapChain::CreateSemaphores()
{
	VkSemaphoreCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	vkCreateSemaphore(FVulkan::GetDevice(), &createInfo, nullptr, &ImageAvailableSemaphore);


	VkSemaphoreCreateInfo createInfo2 = {};
	createInfo2.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	vkCreateSemaphore(FVulkan::GetDevice(), &createInfo2, nullptr, &RenderingFinishedSemaphore);
}

void FVulkanSwapChain::CreateFences()
{
	uint32_t i;
	Fences.resize(SwapChainImageCount);
	for(i = 0; i < SwapChainImageCount; i++)
	{
		VkResult result;

		VkFenceCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		createInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
		vkCreateFence(FVulkan::GetDevice(), &createInfo, nullptr, &Fences[i]);
	}
}
