#include "VulkanInterface.h"

#include <set>
#include <sstream>
#include <vector>
#include <vulkan/vulkan_win32.h>

#include "Core/Assertion.h"
#include "Core/VulkanoLog.h"

VkInstance          FVulkan::Instance = { VK_NULL_HANDLE };;
VkDevice            FVulkan::Device;
VkPhysicalDevice    FVulkan::PhysicalDevice;

PFN_vkCreateDebugUtilsMessengerEXT  FVulkan::vkCreateDebugUtilsMessengerEXT;
PFN_vkDestroyDebugUtilsMessengerEXT FVulkan::vkDestroyDebugUtilsMessengerEXT;
VkDebugUtilsMessengerEXT            FVulkan::DebugUtilsMessenger;

VkBool32 VKAPI_CALL DebugVulkanCallback2(
            VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
            VkDebugUtilsMessageTypeFlagsEXT messageType,
            const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
            void* pUserData)
{
    // Select prefix depending on flags passed to the callback
    std::string prefix;

    if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT)
    {
        prefix = "VERBOSE: ";
    }
    else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT)
    {
        prefix = "INFO: ";
    }
    else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
    {
        prefix = "WARNING: ";
    }
    else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
    {
        prefix = "ERROR: ";
    }


    // Display message to default output (console/logcat)
    std::stringstream debugMessage;
    if (pCallbackData->pMessageIdName)
    {
        debugMessage << prefix << "[" << pCallbackData->messageIdNumber << "][" << pCallbackData->pMessageIdName << "] : " << pCallbackData->pMessage;
    }
    else
    {
        debugMessage << prefix << "[" << pCallbackData->messageIdNumber << "] : " << pCallbackData->pMessage;
    }
	
    if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
    {
        std::cerr << debugMessage.str() << "\n\n";
    }
    else
    {
        std::cout << debugMessage.str() << "\n\n";
    }

    fflush(stdout);
    return VK_FALSE;
}

void FVulkan::CreateVulkanInstance(const std::string& ApplicationName)
{
    if(Instance != VK_NULL_HANDLE)
    {
        printf("Vulkan instance already exist");
        return;
    }
    
    std::vector<const char*> validationLayers;
    std::vector<const char*> instanceExtensions = { VK_KHR_SURFACE_EXTENSION_NAME };
    instanceExtensions.push_back("VK_KHR_win32_surface");
    instanceExtensions.push_back("VK_EXT_debug_utils");
    
    VkApplicationInfo appInfo = {};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = ApplicationName.c_str();
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "MyEngine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_0;

    VkInstanceCreateInfo instanceCreateInfo = {};
    instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instanceCreateInfo.pApplicationInfo = &appInfo;
    instanceCreateInfo.enabledLayerCount = validationLayers.size();
    instanceCreateInfo.ppEnabledLayerNames = validationLayers.data();
    instanceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(instanceExtensions.size());
    instanceCreateInfo.ppEnabledExtensionNames = instanceExtensions.data();

    if(vkCreateInstance(&instanceCreateInfo, nullptr, &Instance) != VK_SUCCESS)
    {
        checkf(0, "Failed creating vulkan instance");
        return;
    }
    VK_LOG(LOG_INFO, "Vulkan Instance Created");
}

void FVulkan::CreateVulkanDebugLayer()
{
    vkCreateDebugUtilsMessengerEXT = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(Instance, "vkCreateDebugUtilsMessengerEXT"));
    vkDestroyDebugUtilsMessengerEXT = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(Instance, "vkDestroyDebugUtilsMessengerEXT"));

    VkDebugUtilsMessengerCreateInfoEXT debugUtilsMessengerCI{};
    debugUtilsMessengerCI.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    debugUtilsMessengerCI.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    debugUtilsMessengerCI.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;
    debugUtilsMessengerCI.pfnUserCallback = DebugVulkanCallback2;
    VkResult result = vkCreateDebugUtilsMessengerEXT(Instance, &debugUtilsMessengerCI, nullptr, &DebugUtilsMessenger);
    checkf(result == VK_SUCCESS, "Fail creating vulkan debug");
    VK_LOG(LOG_INFO, "Vulkan Debug Layer Created");
}

void FVulkan::SelectPhysicalDevice()
{
    std::vector<VkPhysicalDevice> PhysicalDevices;
    uint32_t physicalDeviceCount = 0;

    vkEnumeratePhysicalDevices(Instance, &physicalDeviceCount, nullptr);
    PhysicalDevices.resize(physicalDeviceCount);
    vkEnumeratePhysicalDevices(Instance, &physicalDeviceCount, PhysicalDevices.data());

    if(!PhysicalDevices.empty())
    {
        PhysicalDevice = PhysicalDevices[0];
        for(const auto& Devices : PhysicalDevices)
        {
            VkPhysicalDeviceProperties deviceProperties;
            vkGetPhysicalDeviceProperties(Devices, &deviceProperties);
            VK_LOG(LOG_INFO, "Selected device [%s] Type: %i API: %i", deviceProperties.deviceName, static_cast<int>(deviceProperties.deviceType), deviceProperties.apiVersion);
        }
        return;
    }
    checkf(0, "Failed selecting device");
}

void FVulkan::CreateVulkanDevice(const VkSurfaceKHR& Surface)
{
    if(Device != VK_NULL_HANDLE)
    {
        VK_LOG(LOG_INFO, "Vulkan device already exist");
        return;
    }
    
    SelectPhysicalDevice();

    std::vector<VkQueueFamilyProperties> QueueFamilyProps;
    uint32_t QueueCount;

    vkGetPhysicalDeviceQueueFamilyProperties(PhysicalDevice, &QueueCount, nullptr);
    QueueFamilyProps.resize(QueueCount);
    vkGetPhysicalDeviceQueueFamilyProperties(PhysicalDevice, &QueueCount, QueueFamilyProps.data());

    int graphicIndex = -1;
    int presentIndex = -1;

    int i = 0;
    for(const auto& queueFamily : QueueFamilyProps)
    {
        if(queueFamily.queueCount > 0 && queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
        {
            graphicIndex = i;
        }

        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(PhysicalDevice, i, Surface, &presentSupport);
        if(queueFamily.queueCount > 0 && presentSupport)
        {
            presentIndex = i;
        }

        if(graphicIndex != -1 && presentIndex != -1)
        {
            break;
        }

        i++;
    }

    uint32_t graphics_QueueFamilyIndex = graphicIndex;
    uint32_t present_QueueFamilyIndex = presentIndex;
    
    std::vector<const char*> validationLayers;
    const std::vector<const char*> deviceExtensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
    const float queue_priority[] = { 1.0f };

    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::set<uint32_t> uniqueQueueFamilies = { graphics_QueueFamilyIndex, present_QueueFamilyIndex };

    float queuePriority = queue_priority[0];
    for(int queueFamily : uniqueQueueFamilies)
    {
        VkDeviceQueueCreateInfo queueCreateInfo = {};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queueFamily;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(queueCreateInfo);
    }

    VkDeviceQueueCreateInfo queueCreateInfo = {};
    queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfo.queueFamilyIndex = graphics_QueueFamilyIndex;
    queueCreateInfo.queueCount = 1;
    queueCreateInfo.pQueuePriorities = &queuePriority;
    
    VkPhysicalDeviceFeatures deviceFeatures = {};
    deviceFeatures.samplerAnisotropy = VK_TRUE;

    VkDeviceCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.pQueueCreateInfos = &queueCreateInfo;
    createInfo.queueCreateInfoCount = queueCreateInfos.size();
    createInfo.pQueueCreateInfos = queueCreateInfos.data();
    createInfo.pEnabledFeatures = &deviceFeatures;
    createInfo.enabledExtensionCount = deviceExtensions.size();
    createInfo.ppEnabledExtensionNames = deviceExtensions.data();

    createInfo.enabledLayerCount = validationLayers.size();
    createInfo.ppEnabledLayerNames = validationLayers.data();

    vkCreateDevice(PhysicalDevice, &createInfo, nullptr, &Device);

    /*vkGetDeviceQueue(Device, graphics_QueueFamilyIndex, 0, &GraphicsQueue);
    vkGetDeviceQueue(Device, present_QueueFamilyIndex, 0, &PresentQueue);*/
}

VkSurfaceKHR FVulkan::CreateSurface(HINSTANCE hInstance, HWND hwnd)
{
    VkSurfaceKHR SurfaceKHR;
    VkWin32SurfaceCreateInfoKHR surfaceCreateInfo = {};
    surfaceCreateInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    surfaceCreateInfo.hinstance = hInstance;
    surfaceCreateInfo.hwnd = hwnd;
    if(vkCreateWin32SurfaceKHR(Instance, &surfaceCreateInfo, nullptr, &SurfaceKHR) != VK_SUCCESS)
    {
        checkf(0, "Fail creating surface");
    }
    VK_LOG(LOG_INFO, "Creating Surface Win64");
    return SurfaceKHR;
}

VkBool32 FVulkan::GetSupportedDepthFormat(VkFormat* depthFormat)
{
    std::vector<VkFormat> depthFormats = {
        VK_FORMAT_D32_SFLOAT_S8_UINT,
        VK_FORMAT_D32_SFLOAT,
        VK_FORMAT_D24_UNORM_S8_UINT,
        VK_FORMAT_D16_UNORM_S8_UINT,
        VK_FORMAT_D16_UNORM
    };

    for (auto& format : depthFormats)
    {
        VkFormatProperties formatProps;
        vkGetPhysicalDeviceFormatProperties(PhysicalDevice, format, &formatProps);
        if (formatProps.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)
        {
            *depthFormat = format;
            return true;
        }
    }

    return false;
}

std::vector<std::string> FVulkan::GetSupportedExtensions()
{
    static std::vector<std::string> Result;
    if(Result.empty())
    {
        uint32_t extCount = 0;
        vkEnumerateInstanceExtensionProperties(nullptr, &extCount, nullptr);
        if (extCount > 0)
        {
            std::vector<VkExtensionProperties> extensions(extCount);
            if (vkEnumerateInstanceExtensionProperties(nullptr, &extCount, &extensions.front()) == VK_SUCCESS)
            {
                for (VkExtensionProperties& extension : extensions)
                {
                    Result.push_back(extension.extensionName);
                }
            }
        }
    }
    return Result;
}

VkInstance& FVulkan::GetInstance()
{
    return Instance;
}

VkDevice& FVulkan::GetDevice()
{
    return Device;
}

VkPhysicalDevice& FVulkan::GetPhysicalDevice()
{
    return PhysicalDevice;
}

uint32_t FVulkan::FindMemoryType(const VkPhysicalDevice& PhysicalDevice, uint32_t TypeFilter, VkMemoryPropertyFlags MemoryPropertyFlags)
{
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(PhysicalDevice, &memProperties);

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
    {
        if ((TypeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & MemoryPropertyFlags) == MemoryPropertyFlags)
        {
            return i;
        }
    }

    return 0;
}

void FVulkan::CreateImage(uint32_t Width, uint32_t Height, VkFormat Format, VkImageTiling Tiling,
    VkImageUsageFlags ImageUsageFlags, VkMemoryPropertyFlags MemoryPropertyFlags, VkImage& Image,
    VkDeviceMemory& ImageMemory)
{
    VkImageCreateInfo imageInfo = {};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = Width;
    imageInfo.extent.height = Height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = Format;
    imageInfo.tiling = Tiling;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = ImageUsageFlags;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateImage(Device, &imageInfo, nullptr, &Image) != VK_SUCCESS)
    {
        check(0);
    }

    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(Device, Image, &memRequirements);

    VkMemoryAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = FindMemoryType(PhysicalDevice, memRequirements.memoryTypeBits, MemoryPropertyFlags);

    if (vkAllocateMemory(Device, &allocInfo, nullptr, &ImageMemory) != VK_SUCCESS)
    {
        check(0);
    }

    vkBindImageMemory(Device, Image, ImageMemory, 0);
}

VkImageView FVulkan::CreateImageView(VkImage Image, VkFormat Format, VkImageAspectFlags AspectFlags)
{
    VkImageViewCreateInfo viewInfo = {};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = Image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = Format;
    viewInfo.subresourceRange.aspectMask = AspectFlags;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    VkImageView imageView;
    if (vkCreateImageView(Device, &viewInfo, nullptr, &imageView) != VK_SUCCESS)
    {
        checkf(0, "Fail creating image view");
    }

    return imageView;
}

FVulkanTexture FVulkan::CreateTexture(int X, int Y, VkFormat Format, VkImageUsageFlags Flags, VkMemoryPropertyFlags MemoryFlags, std::string TextureName, VkImageTiling Tilling, VkImageAspectFlags AspectFlags)
{
    FVulkanTexture Texture;
    Texture.Format = Format;
    Texture.SizeX = X;
    Texture.SizeY = Y;
    Texture.ImageTilling = Tilling;
    Texture.Name = TextureName;
    
    VkImageCreateInfo imageInfo = {};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = X;
    imageInfo.extent.height = Y;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = Format;
    imageInfo.tiling = Tilling;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = Flags;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateImage(Device, &imageInfo, nullptr, &Texture.Image) != VK_SUCCESS)
    {
        checkf(0, "Fail creating texture");
    }

    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(Device, Texture.Image, &memRequirements);

    VkMemoryAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = FindMemoryType(PhysicalDevice, memRequirements.memoryTypeBits, MemoryFlags);

    if (vkAllocateMemory(Device, &allocInfo, nullptr, &Texture.ImageMemory) != VK_SUCCESS)
    {
        checkf(0, "Fail allocation memory for texture");
    }

    vkBindImageMemory(Device, Texture.Image, Texture.ImageMemory, 0);

    VkImageViewCreateInfo viewInfo = {};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = Texture.Image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = Format;
    viewInfo.subresourceRange.aspectMask = AspectFlags;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;
    
    if (vkCreateImageView(Device, &viewInfo, nullptr, &Texture.ImageView) != VK_SUCCESS)
    {
        checkf(0, "Fail creating image view");
    }

    return Texture;
}

template <typename StructType>
FVulkanBuffer FVulkan::CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties)
{
    FVulkanBuffer Result;
    
    VkBufferCreateInfo BufferCreateInfo{};
    BufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    BufferCreateInfo.size = size;
    BufferCreateInfo.usage = usage;
    BufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(Device, &BufferCreateInfo, nullptr, &Result.Buffer) != VK_SUCCESS)
    {
        checkf(0, "Fail creating buffer");
    }

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(Device, Result.Buffer, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = FindMemoryType(PhysicalDevice, memRequirements.memoryTypeBits, properties);

    if (vkAllocateMemory(Device, &allocInfo, nullptr, &Result.BufferMemory) != VK_SUCCESS)
    {
        checkf(0, "Fail allocating memory for buffer");
    }

    vkBindBufferMemory(Device, Result.Buffer, Result.BufferMemory, 0);
    return Result;
}

void FVulkan::SetBufferData(FVulkanBuffer& Buffer, const void* BufferData, size_t BufferSize)
{
    void* data;
    vkMapMemory(Device, Buffer.BufferMemory, 0, BufferSize, 0, &data);
    memcpy(data, BufferData, BufferSize);
    vkUnmapMemory(Device, Buffer.BufferMemory);
}
