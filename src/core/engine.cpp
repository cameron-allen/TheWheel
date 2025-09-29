#include "pch.h"
#include "core/engine.h"
#include "core/window.h"

// More info for Vulkan debug configuration at the bottom of this page:
// https://docs.vulkan.org/tutorial/latest/03_Drawing_a_triangle/00_Setup/02_Validation_layers.html

Core* Core::mp_instance = nullptr;

const std::vector<const char*> VALIDATION_LAYERS = {
    "VK_LAYER_KHRONOS_validation"
};

static VKAPI_ATTR vk::Bool32 VKAPI_CALL DebugCallback(vk::DebugUtilsMessageSeverityFlagBitsEXT severity, vk::DebugUtilsMessageTypeFlagsEXT type, const vk::DebugUtilsMessengerCallbackDataEXT* pCallbackData, void*) {
    std::cerr << "validation layer: type " << to_string(type) << " msg: " << pCallbackData->pMessage << std::endl;
    return vk::False;
}

void Core::setupDebugMessenger()
{
    vk::DebugUtilsMessageSeverityFlagsEXT severityFlags(vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose | vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning | vk::DebugUtilsMessageSeverityFlagBitsEXT::eError);
    vk::DebugUtilsMessageTypeFlagsEXT    messageTypeFlags(vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation);
    vk::DebugUtilsMessengerCreateInfoEXT debugUtilsMessengerCreateInfoEXT{
        .messageSeverity = severityFlags,
        .messageType = messageTypeFlags,
        .pfnUserCallback = &DebugCallback
    };
    m_debugMessenger = m_instance.createDebugUtilsMessengerEXT(debugUtilsMessengerCreateInfoEXT);
}

void Core::selectPhysicalDevices()
{
    vk::raii::PhysicalDevice physicalDevice = m_instance.enumeratePhysicalDevices().front();
    auto devices = m_instance.enumeratePhysicalDevices();

    try 
    {
        if (devices.empty()) {
            throw std::runtime_error("failed to find GPUs with Vulkan support!");
        }

        if constexpr (DISPLAY_VULKAN_INFO)
            std::cout << "-----Devices--------" << std::endl;
        for (const auto& device : devices)
            if (suitableDiscreteGPU(device)) 
                break;
        if constexpr (DISPLAY_VULKAN_INFO)
            std::cout << "--------------------" << std::endl;
        
        vk::raii::Device device(physicalDevice, vk::DeviceCreateInfo{});
        // Use Vulkan objects
        vk::raii::Buffer buffer(device, vk::BufferCreateInfo{});

    }
    catch (const std::exception& err) {
        std::cerr << "Device Setup Error: " << err.what() << std::endl;
        assert(0);
    }
}


// TDO: Modify this to create a presentation queue too
void Core::setupLogicalDevice()
{
    std::vector<vk::QueueFamilyProperties> queueFamilyProperties = m_dGPU.getQueueFamilyProperties();
    
    // get the first index into queueFamilyProperties which supports graphics
    auto graphicsQueueFamilyProperty = std::ranges::find_if(queueFamilyProperties, [](auto const& qfp)
        { return (qfp.queueFlags & vk::QueueFlagBits::eGraphics) != static_cast<vk::QueueFlags>(0); });
    
    float queuePriority = 0.0f;
    unsigned int graphicsIndex = static_cast<uint32_t>(std::distance(queueFamilyProperties.begin(), graphicsQueueFamilyProperty));

    // Create a chain of feature structures
    vk::StructureChain<vk::PhysicalDeviceFeatures2, vk::PhysicalDeviceVulkan13Features, vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT> featureChain = {
        {},                               // vk::PhysicalDeviceFeatures2 (empty for now)
        {.dynamicRendering = true },      // Enable dynamic rendering from Vulkan 1.3
        {.extendedDynamicState = true }   // Enable extended dynamic state from the extension
    };

    std::vector<const char*> deviceExtensions = {
        vk::KHRSwapchainExtensionName,
        vk::KHRSpirv14ExtensionName,
        vk::KHRSynchronization2ExtensionName,
        vk::KHRCreateRenderpass2ExtensionName
    };

    // determine a queueFamilyIndex that supports present
    // first check if the graphicsIndex is good enough
    unsigned int presentIndex = m_dGPU.getSurfaceSupportKHR(graphicsIndex, *m_surface)
        ? graphicsIndex
        : static_cast<uint32_t>(queueFamilyProperties.size());

    if (presentIndex == queueFamilyProperties.size())
    {
        // the graphicsIndex doesn't support present -> look for another family index that supports both
        // graphics and present
        for (size_t i = 0; i < queueFamilyProperties.size(); i++)
        {
            if ((queueFamilyProperties[i].queueFlags & vk::QueueFlagBits::eGraphics) &&
                m_dGPU.getSurfaceSupportKHR(static_cast<uint32_t>(i), *m_surface))
            {
                graphicsIndex = static_cast<uint32_t>(i);
                presentIndex = graphicsIndex;
                break;
            }
        }
        if (presentIndex == queueFamilyProperties.size())
        {
            // there's nothing like a single family index that supports both graphics and present -> look for another
            // family index that supports present
            for (size_t i = 0; i < queueFamilyProperties.size(); i++)
            {
                if (m_dGPU.getSurfaceSupportKHR(static_cast<uint32_t>(i), *m_surface))
                {
                    presentIndex = static_cast<uint32_t>(i);
                    break;
                }
            }
        }
    }
    if ((graphicsIndex == queueFamilyProperties.size()) || (presentIndex == queueFamilyProperties.size()))
    {
        throw std::runtime_error("Could not find a queue for graphics or present -> terminating");
    }

    // Iterate over the queue families to find one with graphics capabilities
    /*for (int i = 0; i < queueFamilyProperties.size(); ++i) {
        if (queueFamilyProperties[i].queueFlags & vk::QueueFlagBits::eGraphics) {
            graphicsIndex = i;
            break;
        }
    }*/

    /*vk::DeviceQueueCreateInfo deviceQueueCreateInfo{
        .queueFamilyIndex = graphicsIndex,
        .queueCount = 1,
        .pQueuePriorities = &queuePriority
    };

    vk::DeviceCreateInfo deviceCreateInfo{
        .pNext = &featureChain.get<vk::PhysicalDeviceFeatures2>(),
        .queueCreateInfoCount = 1,
        .pQueueCreateInfos = &deviceQueueCreateInfo,
        .enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size()),
        .ppEnabledExtensionNames = deviceExtensions.data()
    };*/

    // query for Vulkan 1.3 features
    auto features = m_dGPU.getFeatures2();
    vk::PhysicalDeviceVulkan13Features vulkan13Features;
    vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT extendedDynamicStateFeatures;
    vulkan13Features.dynamicRendering = vk::True;
    extendedDynamicStateFeatures.extendedDynamicState = vk::True;
    vulkan13Features.pNext = &extendedDynamicStateFeatures;
    features.pNext = &vulkan13Features;
    
    // create a Device
    vk::DeviceQueueCreateInfo deviceQueueCreateInfo{ .queueFamilyIndex = graphicsIndex, .queueCount = 1, .pQueuePriorities = &queuePriority };
    vk::DeviceCreateInfo      deviceCreateInfo{ .pNext = &features, .queueCreateInfoCount = 1, .pQueueCreateInfos = &deviceQueueCreateInfo };
    deviceCreateInfo.enabledExtensionCount = deviceExtensions.size();
    deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions.data();

    m_device = vk::raii::Device(m_dGPU, deviceCreateInfo);
    m_graphicsQueue = vk::raii::Queue(m_device, graphicsIndex, 0);
    m_presentQueue = vk::raii::Queue(m_device, presentIndex, 0);
}

bool Core::suitableDiscreteGPU(const vk::raii::PhysicalDevice& device)
{
    auto deviceProperties = device.getProperties();
    auto deviceFeatures = device.getFeatures();
    if (deviceProperties.deviceType == vk::PhysicalDeviceType::eDiscreteGpu &&
        deviceFeatures.geometryShader) {
        if constexpr (DISPLAY_VULKAN_INFO)
            std::cout << "Selected: " << deviceProperties.deviceName << std::endl;
        m_dGPU = device;
        return true;
    }
    return false;
}

void Core::createSurface()
{
    VkSurfaceKHR       _surface;

    if (!SDL_Vulkan_CreateSurface(mp_window->GetWindow(),
        static_cast<VkInstance>(*m_instance),
        nullptr,
        &_surface)) {
        throw std::runtime_error("SDL_Vulkan_CreateSurface failed: " +
            std::string(SDL_GetError()));
    }
    
    m_surface = vk::raii::SurfaceKHR(m_instance, _surface);
}

Core& Core::GetInstance()
{
    if (!mp_instance)
        mp_instance = new Core();
    return *mp_instance;
}

std::vector<const char*> Core::GetRequiredExtensions()
{
    Uint32 sdlExtensionCount = 0;
    const char* const* sdlExtensions = SDL_Vulkan_GetInstanceExtensions(&sdlExtensionCount);

    std::vector extensions(sdlExtensions, sdlExtensions + sdlExtensionCount);

#ifndef NDEBUG
    extensions.push_back(vk::EXTDebugUtilsExtensionName);
#endif

    if constexpr (DISPLAY_VULKAN_INFO) 
    {
        std::cout << "-----Extensions-----\n";
        for (int i = 0; i < extensions.size(); i++)
        {
            std::cout << extensions[i] << std::endl;
        }
        std::cout << "--------------------\n";
    }

    return extensions;
}

void Core::run()
{
	init();
	loop();
	clean();
}

void Core::init()
{
    mp_window = new SDLWindow();
    mp_window->init();

    vk::ApplicationInfo appInfo {
        .sType = vk::StructureType::eApplicationInfo,
        .pApplicationName = "The Wheel",
        .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
        .pEngineName = "No Engine",
        .engineVersion = VK_MAKE_VERSION(1, 0, 0),
        .apiVersion = vk::ApiVersion14
    };

    std::vector<char const*> requiredLayers, requiredExtensions = Core::GetRequiredExtensions();

#ifndef NDEBUG
        requiredLayers.assign(VALIDATION_LAYERS.begin(), VALIDATION_LAYERS.end());
#endif

    try 
    {
        // Check if the required layers are supported by the Vulkan implementation.
        auto layerProperties = m_context.enumerateInstanceLayerProperties();
        if (std::ranges::any_of(requiredLayers, [&layerProperties](auto const& requiredLayer) {
            return std::ranges::none_of(layerProperties,
                [requiredLayer](auto const& layerProperty)
                { return strcmp(layerProperty.layerName, requiredLayer) == 0; });
            }))
        {
            throw std::runtime_error("One or more required layers are not supported!");
        }

        auto extensionProperties = m_context.enumerateInstanceExtensionProperties();
        for (auto const& requiredExtension : requiredExtensions)
        {
            if (std::ranges::none_of(extensionProperties,
                [requiredExtension](auto const& extensionProperty)
                { return strcmp(extensionProperty.extensionName, requiredExtension) == 0; }))
            {
                throw std::runtime_error("Required extension not supported: " + std::string(requiredExtension));
            }
        }

        if constexpr (DISPLAY_VULKAN_INFO) 
        {
            std::cout << "-----Layers---------\n";
            for (int i = 0; i < requiredLayers.size(); i++)
            {
                std::cout << requiredLayers[i] << std::endl;
            }
            std::cout << "--------------------\n";
        }

        vk::InstanceCreateInfo createInfo{
            .sType = vk::StructureType::eInstanceCreateInfo,
            .pApplicationInfo = &appInfo,
            .enabledLayerCount = static_cast<uint32_t>(requiredLayers.size()),
            .ppEnabledLayerNames = requiredLayers.data(),
            .enabledExtensionCount = static_cast<uint32_t>(requiredExtensions.size()),
            .ppEnabledExtensionNames = requiredExtensions.data(),
        };

        m_instance = vk::raii::Instance{ m_context, createInfo };

#ifndef NDEBUG
        setupDebugMessenger();
#endif

        selectPhysicalDevices();
        createSurface();
        setupLogicalDevice();
    }
    catch (const vk::SystemError& err) {
        std::cerr << "Vulkan error: " << err.what() << std::endl;
        assert(0);
    }
    catch (const std::exception& err) {
        std::cerr << "Error: " << err.what() << std::endl;
        assert(0);
    }
}

void Core::loop()
{
    while (mp_window->isOpen()) 
    {
        mp_window->checkEvents();
    }
}

void Core::clean()
{
    delete(mp_window);
}
