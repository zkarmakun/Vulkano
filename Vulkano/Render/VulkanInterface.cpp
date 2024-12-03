#include "VulkanInterface.h"

#include <set>
#include <sstream>
#include <vector>
#include <vulkan/vulkan_win32.h>

#include "Shader.h"
#include "VertexInputs.h"
#include "Core/Assertion.h"
#include "Core/VulkanoLog.h"

std::map<std::uint32_t, FRenderPass*>         FVulkan::RenderPasses;
std::map<std::uint32_t, FGraphicsPipeline*>   FVulkan::PSOs;


VkInstance          FVulkan::Instance = { VK_NULL_HANDLE };
VkDevice            FVulkan::Device = { VK_NULL_HANDLE };
VkPhysicalDevice    FVulkan::PhysicalDevice = { VK_NULL_HANDLE };
uint32_t            FVulkan::GraphicsIndex = UINT32_MAX;
uint32_t            FVulkan::PresentIndex = UINT32_MAX;
uint32_t            FVulkan::ComputeIndex = UINT32_MAX;
VkQueue             FVulkan::GraphicsQueue = VK_NULL_HANDLE;
VkQueue             FVulkan::PresentQueue = VK_NULL_HANDLE;
VkQueue             FVulkan::ComputeQueue = VK_NULL_HANDLE;
uint32_t            FVulkan::MajorVersion = UINT32_MAX;
uint32_t            FVulkan::MinorVersion = UINT32_MAX;
VkCommandPool       FVulkan::GraphicsCommandPool = VK_NULL_HANDLE;
VkCommandBuffer     FVulkan::GraphicsCommandBuffer = VK_NULL_HANDLE;

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
    appInfo.apiVersion = VK_API_VERSION_1_3;

    VkInstanceCreateInfo instanceCreateInfo = {};
    instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instanceCreateInfo.pApplicationInfo = &appInfo;
    instanceCreateInfo.enabledLayerCount =  static_cast<uint32_t>(validationLayers.size());
    instanceCreateInfo.ppEnabledLayerNames = validationLayers.data();
    instanceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(instanceExtensions.size());
    instanceCreateInfo.ppEnabledExtensionNames = instanceExtensions.data();

    if(vkCreateInstance(&instanceCreateInfo, nullptr, &Instance) != VK_SUCCESS)
    {
        fatal("Failed creating vulkan instance");
    }

    // Query the Vulkan version
    uint32_t apiVersion;
    vkEnumerateInstanceVersion(&apiVersion);

    uint32_t major = VK_VERSION_MAJOR(apiVersion);
    uint32_t minor = VK_VERSION_MINOR(apiVersion);
    uint32_t patch = VK_VERSION_PATCH(apiVersion);

    VK_LOG(LOG_INFO, "Vulkan Instance Created, API version: %i.%i.%i", major, minor, patch);
    MajorVersion = major;
    MinorVersion = minor;
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
        fatal("FVulkan::CreateVulkanDevice Fail creating logical device");
    }

    VK_LOG(LOG_INFO, "Logical device created");

    vkGetDeviceQueue(Device, GraphicsIndex, 0, &GraphicsQueue);
    vkGetDeviceQueue(Device, PresentIndex, 0, &PresentQueue);
    vkGetDeviceQueue(Device, ComputeIndex, 0, &ComputeQueue);

    // Create command pools and command buffers
    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.queueFamilyIndex = GraphicsIndex;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;  // Allows command buffers to be reset
    if(vkCreateCommandPool(Device, &poolInfo, nullptr, &GraphicsCommandPool) != VK_SUCCESS)
    {
        fatal("FVulkan::CreateVulkanDevice Fail creating Graphics command pool");
    }

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = GraphicsCommandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = 1;
    if(vkAllocateCommandBuffers(Device, &allocInfo, &GraphicsCommandBuffer) != VK_SUCCESS)
    {
        fatal("FVulkan::CreateVulkanDevice Fail creating Graphics command buffer");
    }

    VKGlobals::InitGlobalResources();
}

void FVulkan::ExitVulkan()
{
    for(auto& Elem : PSOs)
    {
        if(FGraphicsPipeline* It = Elem.second)
        {
            It->Release();
            delete It;
        }
    }
    PSOs.clear();

    for(auto& Elem : RenderPasses)
    {
        if(FRenderPass* It = Elem.second)
        {
            It->Release();
            delete It;
        }
    }
    RenderPasses.clear();

    // Destroy commands
    if(GraphicsCommandBuffer != VK_NULL_HANDLE)
    {
        vkFreeCommandBuffers(Device, GraphicsCommandPool, 1, &GraphicsCommandBuffer);
        vkDestroyCommandPool(Device, GraphicsCommandPool, nullptr);
        GraphicsCommandPool = VK_NULL_HANDLE;
    }
    
    VKGlobals::CleanupGlobalResources();
    
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

VkCommandBuffer& FVulkan::GetGraphicsBuffer()
{
    return GraphicsCommandBuffer;
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

uint32_t FVulkan::GetMajorVersion()
{
    return MajorVersion;
}

uint32_t FVulkan::GetMinorVersion()
{
    return MinorVersion;
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

std::shared_ptr<FVulkanTexture> FVulkan::CreateTexture(uint32_t X, uint32_t Y, VkFormat Format, VkImageUsageFlags Flags, VkMemoryPropertyFlags MemoryFlags, const std::string& TextureName, VkImageTiling Tilling, VkImageAspectFlags AspectFlags)
{
    std::shared_ptr<FVulkanTexture> Texture = std::make_shared<FVulkanTexture>();
    Texture->Format = Format;
    Texture->SizeX = X;
    Texture->SizeY = Y;
    Texture->ImageTilling = Tilling;
    Texture->ResourceName = TextureName;
    
    VkImageCreateInfo ImageCreateInfo = {};
    ImageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    ImageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
    ImageCreateInfo.extent.width = X;
    ImageCreateInfo.extent.height = Y;
    ImageCreateInfo.extent.depth = 1;
    ImageCreateInfo.mipLevels = 1;
    ImageCreateInfo.arrayLayers = 1;
    ImageCreateInfo.format = Format;
    ImageCreateInfo.tiling = Tilling;
    ImageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    ImageCreateInfo.usage = Flags;
    ImageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    ImageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateImage(Device, &ImageCreateInfo, nullptr, &Texture->Image) != VK_SUCCESS)
    {
        checkf(0, "Fail creating texture");
    }

    VkMemoryRequirements MemRequirements;
    vkGetImageMemoryRequirements(Device, Texture->Image, &MemRequirements);

    VkMemoryAllocateInfo MemoryAllocateInfo = {};
    MemoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    MemoryAllocateInfo.allocationSize = MemRequirements.size;
    MemoryAllocateInfo.memoryTypeIndex = FindMemoryType(PhysicalDevice, MemRequirements.memoryTypeBits, MemoryFlags);

    if (vkAllocateMemory(Device, &MemoryAllocateInfo, nullptr, &Texture->ImageMemory) != VK_SUCCESS)
    {
        checkf(0, "Fail allocation memory for texture");
    }

    vkBindImageMemory(Device, Texture->Image, Texture->ImageMemory, 0);

    VkImageViewCreateInfo ImageViewCreateInfo = {};
    ImageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    ImageViewCreateInfo.image = Texture->Image;
    ImageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    ImageViewCreateInfo.format = Format;
    ImageViewCreateInfo.subresourceRange.aspectMask = AspectFlags;
    ImageViewCreateInfo.subresourceRange.baseMipLevel = 0;
    ImageViewCreateInfo.subresourceRange.levelCount = 1;
    ImageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
    ImageViewCreateInfo.subresourceRange.layerCount = 1;
    
    if (vkCreateImageView(Device, &ImageViewCreateInfo, nullptr, &Texture->ImageView) != VK_SUCCESS)
    {
        checkf(0, "Fail creating image view");
    }

    return Texture;
}

void FVulkan::ReleaseTexture(std::shared_ptr<FVulkanTexture>& Texture)
{
    if(!Texture)
    {
        return;
    }
    
    Texture->Release();
    Texture.reset();
}

std::shared_ptr<FVulkanBuffer> FVulkan::CreateBuffer(VkDeviceSize BufferSize, uint32_t ElemNumber, VkBufferUsageFlags BufferUsage, VkMemoryPropertyFlags MemoryProperties, const std::string& BufferName /*= "Buffer"*/)
{
    std::shared_ptr<FVulkanBuffer> Result = std::make_shared<FVulkanBuffer>();
    Result->ResourceName = BufferName;
    Result->SetNumberOfElements(ElemNumber);
    VkBufferCreateInfo BufferCreateInfo{};
    BufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    BufferCreateInfo.size = BufferSize;
    BufferCreateInfo.usage = BufferUsage;
    BufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(Device, &BufferCreateInfo, nullptr, &Result->Buffer) != VK_SUCCESS)
    {
        fatal("FVulkan::CreateBuffer Fail creating buffer size: %i, name: %s", BufferSize, BufferName.c_str());
    }

    VkMemoryRequirements MemRequirements;
    vkGetBufferMemoryRequirements(Device, Result->Buffer, &MemRequirements);

    VkMemoryAllocateInfo MemoryAllocateInfo{};
    MemoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    MemoryAllocateInfo.allocationSize = MemRequirements.size;
    MemoryAllocateInfo.memoryTypeIndex = FindMemoryType(PhysicalDevice, MemRequirements.memoryTypeBits, MemoryProperties);

    if (vkAllocateMemory(Device, &MemoryAllocateInfo, nullptr, &Result->BufferMemory) != VK_SUCCESS)
    {
        fatal("FVulkan::CreateBuffer Fail allocating memory for buffer, size: %i, name: %s", BufferSize, BufferName.c_str());
    }

    vkBindBufferMemory(Device, Result->Buffer, Result->BufferMemory, 0);
    VK_LOG(LOG_INFO, "Buffer created success, byte size: %i, name: %s", BufferSize, BufferName.c_str());
    return Result;
}

void FVulkan::UpdateBuffer(const std::shared_ptr<FVulkanBuffer>& Buffer, const void* BufferData, size_t BufferSize)
{
    if(Buffer)
    {
        void* data;
        vkMapMemory(Device, Buffer->BufferMemory, 0, BufferSize, 0, &data);
        memcpy(data, BufferData, BufferSize);
        vkUnmapMemory(Device, Buffer->BufferMemory);
    }
}

std::uint32_t GenerateUniqueId(const std::string& input)
{
    return static_cast<uint32_t>(std::hash<std::string>{}(input));
}

FRenderPass* FVulkan::BeginRenderPass(const FRenderPassInfo& RenderPassInfo, VkExtent2D ViewSize, const std::string& RenderPassName)
{
    FRenderPass* RenderPass = GetOrCreateRenderPass(RenderPassInfo, ViewSize, RenderPassName);
    
    std::vector<VkClearValue> ClearValues;
    for(const FRenderPassInfo::FColorAttachment& ColorTarget : RenderPassInfo.ColorRenderTargets)
    {
        VkClearValue& Elem = ClearValues.emplace_back();
        Elem.color = ColorTarget.ClearColor;
    }

    if(RenderPassInfo.DepthStencilRenderTarget.Target)
    {
        VkClearValue& Elem = ClearValues.emplace_back();
        Elem.depthStencil = RenderPassInfo.DepthStencilRenderTarget.StencilClearColor;
    }
    
    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = RenderPass->RenderPass;
    renderPassInfo.framebuffer = RenderPass->FrameBuffer;
    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = ViewSize;
    renderPassInfo.clearValueCount = static_cast<uint32_t>(ClearValues.size());
    renderPassInfo.pClearValues = ClearValues.data();
    
    vkCmdBeginRenderPass(GraphicsCommandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    return RenderPass;
}

FRenderPass* FVulkan::GetOrCreateRenderPass(const FRenderPassInfo& RenderPassInfo,  VkExtent2D ViewSize, const std::string& RenderPassName)
{
    uint32_t PassId = GenerateUniqueId(RenderPassName);
    auto it = RenderPasses.find(PassId);
    if (it != RenderPasses.end())
    {
        return it->second; // Return the shared_ptr to FRenderPass
    }

    auto SetupAttachment_Lambda([](VkAttachmentDescription& Attach, VkAttachmentLoadOp LoadOp, VkAttachmentStoreOp StoreOp, VkFormat Format, VkImageLayout FinalLayout)
    {
        Attach.samples = VK_SAMPLE_COUNT_1_BIT;
        Attach.loadOp = LoadOp;
        Attach.storeOp = StoreOp;
        Attach.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        Attach.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        Attach.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        Attach.finalLayout = FinalLayout;
        Attach.format = Format;
    });

    std::vector<VkAttachmentDescription> AttachmentDescriptions;
    std::vector<VkAttachmentReference> ColorReferences;
    std::vector<VkImageView> Attachments;
    for (uint32_t i = 0; i < RenderPassInfo.ColorRenderTargets.size(); ++i)
    {
        if(!RenderPassInfo.ColorRenderTargets[i].Target)
        {
            continue;
        }
        
        VkAttachmentDescription& Attach = AttachmentDescriptions.emplace_back();
        SetupAttachment_Lambda(
            Attach,
            RenderPassInfo.ColorRenderTargets[i].Load,
            RenderPassInfo.ColorRenderTargets[i].Store,
            RenderPassInfo.ColorRenderTargets[i].Target->Format,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        
        VkAttachmentReference& Reference = ColorReferences.emplace_back();
        Reference.attachment = i;
        Reference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        Attachments.push_back(RenderPassInfo.ColorRenderTargets[i].Target->ImageView);
    }

    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.pColorAttachments = ColorReferences.data();
    subpass.colorAttachmentCount = static_cast<uint32_t>(ColorReferences.size());
    
    if(RenderPassInfo.DepthStencilRenderTarget.Target)
    {
        VkAttachmentReference depthReference = {};
        VkAttachmentDescription& Attach = AttachmentDescriptions.emplace_back();
        SetupAttachment_Lambda(
            Attach,
            RenderPassInfo.DepthStencilRenderTarget.Load,
            RenderPassInfo.DepthStencilRenderTarget.Store,
            RenderPassInfo.DepthStencilRenderTarget.Target->Format,
            VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
        depthReference.attachment = static_cast<uint32_t>(AttachmentDescriptions.size()) - 1;
        depthReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        subpass.pDepthStencilAttachment = &depthReference;

        Attachments.push_back(RenderPassInfo.DepthStencilRenderTarget.Target->ImageView);
    }

    VkRenderPassCreateInfo renderPassInfo = {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.pAttachments = AttachmentDescriptions.data();
    renderPassInfo.attachmentCount = static_cast<uint32_t>(AttachmentDescriptions.size());
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;

    VkRenderPass RenderPass = VK_NULL_HANDLE;
    if(vkCreateRenderPass(Device, &renderPassInfo, nullptr, &RenderPass) == VK_SUCCESS)
    {
        FRenderPass* NewRenderPass = new FRenderPass();
        NewRenderPass->RenderPassName = RenderPassName;
        NewRenderPass->RenderPass = RenderPass;
        
        VkFramebufferCreateInfo framebufferInfo = {};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = NewRenderPass->RenderPass;
        framebufferInfo.attachmentCount = static_cast<uint32_t>(Attachments.size());
        framebufferInfo.pAttachments = Attachments.data();
        framebufferInfo.width = ViewSize.width;
        framebufferInfo.height = ViewSize.height;
        framebufferInfo.layers = 1;

        if (vkCreateFramebuffer(GetDevice(), &framebufferInfo, nullptr, &NewRenderPass->FrameBuffer) != VK_SUCCESS)
        {
            fatal("Failed to create framebuffer %s", RenderPassName.c_str());
        }

        RenderPasses[PassId] = NewRenderPass;
        VK_LOG(LOG_SUCCESS, "Creating render pass: %s", RenderPassName.c_str());
        return NewRenderPass;
    }

    fatal("Fail creating render pass %s", RenderPassName.c_str());
}

FGraphicsPipeline* FVulkan::SetGraphicsPipeline(const FGraphicsPipelineInitializer& PSOInitializer)
{
    // We need at least vertex and pixel shader
    if(!PSOInitializer.VertexShader || !PSOInitializer.PixelShader || !PSOInitializer.RenderPass)
    {
        fatal("FVulkan::SetGraphicsPipeline Failed creating graphics pipelines, invalid shader or render pass");
    }

    // Will use the same id as the render pass
    uint32_t Id = GenerateUniqueId(PSOInitializer.RenderPass->RenderPassName);
    auto it = PSOs.find(Id);
    if (it != PSOs.end())
    {
        vkCmdBindPipeline(GraphicsCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, it->second->GetGraphicsPipeline());
        return it->second;
    }

    std::vector<VkPipelineShaderStageCreateInfo> ShaderStages;
    VkPipelineShaderStageCreateInfo& VertexStageInfo = ShaderStages.emplace_back();
    VertexStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    VertexStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    VertexStageInfo.module = PSOInitializer.VertexShader->GetShader();
    VertexStageInfo.pName = PSOInitializer.VertexShader->GetEntryPoint().c_str();
    
    VkPipelineShaderStageCreateInfo& PixelStageInfo = ShaderStages.emplace_back();
    PixelStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    PixelStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    PixelStageInfo.module = PSOInitializer.PixelShader->GetShader();
    PixelStageInfo.pName = PSOInitializer.PixelShader->GetEntryPoint().c_str();

    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = PSOInitializer.PrimitiveTopology;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;

    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;

    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = VK_LOGIC_OP_COPY;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;
    colorBlending.blendConstants[0] = 0.0f;
    colorBlending.blendConstants[1] = 0.0f;
    colorBlending.blendConstants[2] = 0.0f;
    colorBlending.blendConstants[3] = 0.0f;

    std::vector<VkDynamicState> dynamicStates = {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR
    };

    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
    dynamicState.pDynamicStates = dynamicStates.data();

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 0;
    pipelineLayoutInfo.pushConstantRangeCount = 0;

    VkPipelineLayout PipeLineLayout = VK_NULL_HANDLE;
    if (vkCreatePipelineLayout(FVulkan::GetDevice(), &pipelineLayoutInfo, nullptr, &PipeLineLayout) != VK_SUCCESS)
    {
        fatal("FVulkan::SetGraphicsPipeline Failed creating pipeline layout");
    }

    if(!PSOInitializer.VertexInput)
    {
        fatal("FVulkan::SetGraphicsPipeline No valid vertex input creating the graphics pipeline");
    }

    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = static_cast<uint32_t>(ShaderStages.size());
    pipelineInfo.pStages = ShaderStages.data();
    pipelineInfo.pVertexInputState = &PSOInitializer.VertexInput->GetInputVertexState();
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = nullptr; // Optional
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = &dynamicState;
    pipelineInfo.layout = PipeLineLayout;
    pipelineInfo.renderPass = PSOInitializer.RenderPass->RenderPass;
    pipelineInfo.subpass = 0;

    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
    pipelineInfo.basePipelineIndex = -1; // Optional

    VkPipeline GraphicsPipeline = VK_NULL_HANDLE;
    if (vkCreateGraphicsPipelines(FVulkan::GetDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &GraphicsPipeline) != VK_SUCCESS)
    {
        fatal("FVulkan::SetGraphicsPipeline failed to create graphics pipeline");
    }

    FGraphicsPipeline* NewGraphics = new FGraphicsPipeline(GraphicsPipeline, PipeLineLayout);
    PSOs[Id] = NewGraphics;
    VK_LOG(LOG_SUCCESS, "Creating graphics PSO render pass: %s", PSOInitializer.RenderPass->RenderPassName.c_str());

    vkCmdBindPipeline(GraphicsCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, NewGraphics->GetGraphicsPipeline());
    
    return NewGraphics;
}

void FVulkan::BindStreamResource(int Index, std::shared_ptr<FVulkanBuffer> Buffer, uint64_t Offset)
{
    if(Buffer)
    {
        VkDeviceSize offsets[] = { Offset };
        vkCmdBindVertexBuffers(GraphicsCommandBuffer, Index, 1, &Buffer->Buffer, offsets);
    }
}

void FVulkan::DrawPrimitive(uint32_t BaseVertexIndex, uint32_t VertexCount, uint32_t NumInstances)
{
    vkCmdDraw(GraphicsCommandBuffer, VertexCount, NumInstances, BaseVertexIndex, 0);
}

void FVulkan::SetScissorRect(bool bEnabled, int32_t MinX, int32_t MinY, uint32_t MaxX, uint32_t MaxY)
{
    // Set the scissor rectangle dynamically
    VkRect2D scissor{};
    scissor.offset.x = MinX;
    scissor.offset.y = MinY;
    scissor.extent.width = MaxX;
    scissor.extent.height = MaxY;
    vkCmdSetScissor(GraphicsCommandBuffer, 0, 1, &scissor);
}

void FVulkan::SetViewport(float MinX, float MinY, float MinZ, float MaxX, float MaxY, float MaxZ)
{
    VkViewport viewport{};
    viewport.x = MinX;
    viewport.y = MinY;
    viewport.width = MaxX;
    viewport.height = MaxY;
    viewport.minDepth = MinZ;
    viewport.maxDepth = MaxZ;
    vkCmdSetViewport(GraphicsCommandBuffer, 0, 1, &viewport);
}

void FVulkan::EndRenderPass()
{
    vkCmdEndRenderPass(GraphicsCommandBuffer);
}

void FVulkan::ResetGraphicsCommandBuffer()
{
    vkResetCommandBuffer(GraphicsCommandBuffer, 0);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    vkBeginCommandBuffer(GraphicsCommandBuffer, &beginInfo);
}

void FVulkan::EndGraphicsCommandBuffer()
{
    vkEndCommandBuffer(GraphicsCommandBuffer);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &GraphicsCommandBuffer;

    vkQueueSubmit(GraphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(GraphicsQueue); 
}

void FVulkan::TransitionBarrier(const std::shared_ptr<FVulkanTexture> Input, const std::shared_ptr<FVulkanTexture> TransitionTo)
{
    VkImageMemoryBarrier barrier1 = {};
    barrier1.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier1.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    barrier1.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    barrier1.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    barrier1.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    barrier1.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier1.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier1.image = Input->Image;
    barrier1.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier1.subresourceRange.baseMipLevel = 0;
    barrier1.subresourceRange.levelCount = 1;
    barrier1.subresourceRange.baseArrayLayer = 0;
    barrier1.subresourceRange.layerCount = 1;

    vkCmdPipelineBarrier(
        GraphicsCommandBuffer,
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        0,
        0, nullptr,
        0, nullptr,
        1, &barrier1
    );

    VkImageMemoryBarrier barrier2 = {};
    barrier2.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier2.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier2.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    barrier2.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    barrier2.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    barrier2.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier2.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier2.image = TransitionTo->Image;
    barrier2.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier2.subresourceRange.baseMipLevel = 0;
    barrier2.subresourceRange.levelCount = 1;
    barrier2.subresourceRange.baseArrayLayer = 0;
    barrier2.subresourceRange.layerCount = 1;

    vkCmdPipelineBarrier(
        GraphicsCommandBuffer,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
        0,
        0, nullptr,
        0, nullptr,
        1, &barrier2
    );
}

void FVulkan::CopyTexture(const std::shared_ptr<FVulkanTexture> Source, const std::shared_ptr<FVulkanTexture> Target)
{
    VkImageCopy CopyRegion{};
    CopyRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    CopyRegion.srcSubresource.mipLevel = 0;
    CopyRegion.srcSubresource.baseArrayLayer = 0;
    CopyRegion.srcSubresource.layerCount = 1;
    CopyRegion.srcOffset = {0, 0, 0};
    CopyRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    CopyRegion.dstSubresource.mipLevel = 0;
    CopyRegion.dstSubresource.baseArrayLayer = 0;
    CopyRegion.dstSubresource.layerCount = 1;
    CopyRegion.dstOffset = {0, 0, 0};
    CopyRegion.extent.width = Source->SizeX;
    CopyRegion.extent.height = Source->SizeY;
    CopyRegion.extent.depth = 1;

    vkCmdCopyImage(
        GraphicsCommandBuffer,
        Source->Image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        Target->Image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        1, &CopyRegion
    );
}

