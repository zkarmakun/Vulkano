#include "VulkanInterface.h"

#include <set>
#include <sstream>
#include <vector>
#include <vulkan/vulkan_win32.h>

#include "Core/Assertion.h"
#include "Core/VulkanoLog.h"

VkInstance          FVulkan::Instance = { VK_NULL_HANDLE };
VkDevice            FVulkan::Device = { VK_NULL_HANDLE };
VkPhysicalDevice    FVulkan::PhysicalDevice = { VK_NULL_HANDLE };
uint32_t            FVulkan::GraphicsIndex = UINT32_MAX;
uint32_t            FVulkan::PresentIndex = UINT32_MAX;
uint32_t            FVulkan::ComputeIndex = UINT32_MAX;
VkQueue             FVulkan::GraphicsQueue = VK_NULL_HANDLE;
VkQueue             FVulkan::PresentQueue = VK_NULL_HANDLE;
VkQueue             FVulkan::ComputeQueue = VK_NULL_HANDLE;

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
    std::stringstream debugMessage;

    if (pCallbackData->pMessageIdName)
    {
        debugMessage << "[" << pCallbackData->messageIdNumber << "][" << pCallbackData->pMessageIdName << "] : " << pCallbackData->pMessage;
    }
    else
    {
        debugMessage <<  "[" << pCallbackData->messageIdNumber << "] : " << pCallbackData->pMessage;
    }

    if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT)
    {
        VK_LOG(LOG_INFO, "%s", debugMessage.str().c_str());
    }
    else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT)
    {
        VK_LOG(LOG_INFO, "%s", debugMessage.str().c_str());
    }
    else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
    {
        VK_LOG(LOG_WARNING, "%s", debugMessage.str().c_str());
    }
    else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
    {
        VK_LOG(LOG_ERROR, "%s", debugMessage.str().c_str());
    }
    
    return VK_FALSE;
}

LRESULT CALLBACK DummyWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    return DefWindowProc(hWnd, message, wParam, lParam);
}

HWND FVulkan::CreateDummyWindow(HINSTANCE hInstance)
{
    WNDCLASS wc = {};
    wc.lpfnWndProc = DummyWndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"DummyWindowClass";

    RegisterClass(&wc);

    HWND hWnd = CreateWindowExA(
        0,                              // Optional window styles.
        "DummyWindowClass",             // Window class
        "Dummy Window",                 // Window text
        WS_OVERLAPPEDWINDOW,            // Window style

        // Size and position
        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,

        NULL,       // Parent window
        NULL,       // Menu
        hInstance,  // Instance handle
        NULL        // Additional application data
    );

    if (hWnd == NULL)
    {
        checkf(0, "Failed to create dummy window");
    }

    return hWnd;
}

bool CheckValidationLayerSupport(const std::vector<const char*>& validationLayers) {
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    for (const char* layerName : validationLayers) {
        bool layerFound = false;

        for (const auto& layerProperties : availableLayers) {
            if (strcmp(layerName, layerProperties.layerName) == 0) {
                layerFound = true;
                break;
            }
        }

        if (!layerFound) {
            return false;
        }
    }

    return true;
}

void FVulkan::CreateVulkanInstance(const std::string& ApplicationName)
{
    if(Instance != VK_NULL_HANDLE)
    {
        printf("Vulkan instance already exist");
        return;
    }

#ifdef _DEBUG
    std::vector<const char*> validationLayers = {
        "VK_LAYER_KHRONOS_validation"
    };
    // Check if the validation layers are supported
    if (!CheckValidationLayerSupport(validationLayers))
    {
        VK_LOG(LOG_WARNING, "Validation layer is not available in your machine, please install the vulkan SDK from lunar");
        validationLayers.clear();
    }
#else
    std::vector<const char*> validationLayers;
#endif

    
    std::vector<const char*> instanceExtensions = { VK_KHR_SURFACE_EXTENSION_NAME };
    instanceExtensions.push_back("VK_KHR_win32_surface");
    instanceExtensions.push_back("VK_EXT_debug_utils");
    
    VkApplicationInfo appInfo = {};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = ApplicationName.c_str();
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = ApplicationName.c_str();
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_0;

    VkInstanceCreateInfo instanceCreateInfo = {};
    instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instanceCreateInfo.pApplicationInfo = &appInfo;
    instanceCreateInfo.enabledLayerCount =  static_cast<uint32_t>(validationLayers.size());
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

void FVulkan::DestroyVulkanDebugLayer()
{
    if(DebugUtilsMessenger != VK_NULL_HANDLE)
    {
        vkDestroyDebugUtilsMessengerEXT(Instance, DebugUtilsMessenger, nullptr);
    }
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

void FVulkan::CreateVulkanDevice(HINSTANCE hInstance)
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

    
    HWND DummyWindow = CreateDummyWindow(hInstance);
    VkSurfaceKHR TempSurface = CreateSurface(hInstance, DummyWindow);

    int i = 0;
    for(const auto& queueFamily : QueueFamilyProps)
    {
        // Support graphics API
        if(queueFamily.queueCount > 0 && queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
        {
            GraphicsIndex = i;
        }

        if(queueFamily.queueCount > 0 && queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT)
        {
            ComputeIndex = i;
        }
            

        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(PhysicalDevice, i, TempSurface, &presentSupport);
        if(queueFamily.queueCount > 0 && presentSupport)
        {
            PresentIndex = i;
        }

        if(PresentIndex != UINT32_MAX && GraphicsIndex != UINT32_MAX && ComputeIndex != UINT32_MAX)
        {
            break;
        }

        i++;
    }
    

    // Destroy temp surface
    vkDestroySurfaceKHR(Instance, TempSurface, nullptr);
    DestroyWindow(DummyWindow);
    
    std::vector<const char*> validationLayers;
    const std::vector<const char*> deviceExtensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
    
    std::set<uint32_t> uniqueQueueFamilies = { GraphicsIndex, PresentIndex, ComputeIndex };
    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    float queuePriority = 1.0f;

    for (uint32_t queueFamily : uniqueQueueFamilies) {
        VkDeviceQueueCreateInfo queueCreateInfo{};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queueFamily;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(queueCreateInfo);
    }
    
    
    VkPhysicalDeviceFeatures deviceFeatures = {};
    deviceFeatures.samplerAnisotropy = VK_TRUE;

    VkDeviceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    createInfo.pQueueCreateInfos = queueCreateInfos.data();
    createInfo.pEnabledFeatures = &deviceFeatures;
    createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
    createInfo.ppEnabledExtensionNames = deviceExtensions.data();
    
    createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
    createInfo.ppEnabledLayerNames = validationLayers.data();

    if(vkCreateDevice(PhysicalDevice, &createInfo, nullptr, &Device) != VK_SUCCESS)
    {
        checkf(0, "FVulkan::CreateVulkanDevice Fail creating logical device");
    }

    VK_LOG(LOG_INFO, "Logical device created");

    vkGetDeviceQueue(Device, GraphicsIndex, 0, &GraphicsQueue);
    vkGetDeviceQueue(Device, PresentIndex, 0, &PresentQueue);
    vkGetDeviceQueue(Device, ComputeIndex, 0, &ComputeQueue);
}

void FVulkan::ExitVulkan()
{
    if (Device != VK_NULL_HANDLE)
    {
        vkDestroyDevice(Device, nullptr);
        Device = VK_NULL_HANDLE;
        VK_LOG(LOG_INFO, "Destroy Device");
    }

    if (Instance != VK_NULL_HANDLE)
    {
        vkDestroyInstance(Instance, nullptr);
        Instance = VK_NULL_HANDLE;
        VK_LOG(LOG_INFO, "Destroy Instance");
    }

    VK_LOG(LOG_INFO, "Existing engine... return 1 success");
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

VkQueue FVulkan::GetGraphicsQueue()
{
    return GraphicsQueue;
}

VkQueue FVulkan::GetPresentQueue()
{
    return PresentQueue;
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

void FVulkan::ReleaseTexture(FVulkanTexture& Texture)
{
    // Destroy Image View
    if (Texture.ImageView != VK_NULL_HANDLE)
    {
        vkDestroyImageView(Device, Texture.ImageView, nullptr);
        Texture.ImageView = VK_NULL_HANDLE;
    }

    // Destroy Image
    if (Texture.Image != VK_NULL_HANDLE)
    {
        vkDestroyImage(Device, Texture.Image, nullptr);
        Texture.Image = VK_NULL_HANDLE;
    }

    // Free Memory
    if (Texture.ImageMemory != VK_NULL_HANDLE)
    {
        vkFreeMemory(Device, Texture.ImageMemory, nullptr);
        Texture.ImageMemory = VK_NULL_HANDLE;
    }
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
