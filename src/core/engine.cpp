#include "pch.h"
#include "core/engine.h"

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include "core/window.h"

#include "core/geometry/mesh.h"
#include "core/renderer.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <chrono>

Renderer* pRenderer = nullptr;

const std::vector<Vertex> vertex_data =
{
    {{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}},
    {{0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},
    {{0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}},
    {{-0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}}
};

const std::vector<uint16_t> index_data =
{ 0, 1, 2, 2, 3, 0 };

Mesh triangle;

// More info for Vulkan debug configuration at the bottom of this page:
// https://docs.vulkan.org/tutorial/latest/03_Drawing_a_triangle/00_Setup/02_Validation_layers.html

constexpr int MAX_FRAMES_IN_FLIGHT = 2;

const std::vector<const char*> VALIDATION_LAYERS = {
    "VK_LAYER_KHRONOS_validation"
};

static VKAPI_ATTR vk::Bool32 VKAPI_CALL DebugCallback(vk::DebugUtilsMessageSeverityFlagBitsEXT severity, vk::DebugUtilsMessageTypeFlagsEXT type, const vk::DebugUtilsMessengerCallbackDataEXT* pCallbackData, void*) {
    std::cerr << "\nvalidation layer: type " << to_string(type) << " msg: " << pCallbackData->pMessage << std::endl;
    return vk::False;
}

static uint32_t chooseSwapMinImageCount(vk::SurfaceCapabilitiesKHR const& surfaceCapabilities)
{
    auto minImageCount = std::max(3u, surfaceCapabilities.minImageCount);
    if ((0 < surfaceCapabilities.maxImageCount) && (surfaceCapabilities.maxImageCount < minImageCount))
    {
        minImageCount = surfaceCapabilities.maxImageCount;
    }
    return minImageCount;
}

void Core::transitionImageLayout(
    uint32_t imageIndex,
    vk::ImageLayout oldLayout,
    vk::ImageLayout newLayout,
    vk::AccessFlags2 srcAccessMask,
    vk::AccessFlags2 dstAccessMask,
    vk::PipelineStageFlags2 srcStageMask,
    vk::PipelineStageFlags2 dstStageMask
) {
    vk::ImageMemoryBarrier2 barrier = {
        .srcStageMask = srcStageMask,
        .srcAccessMask = srcAccessMask,
        .dstStageMask = dstStageMask,
        .dstAccessMask = dstAccessMask,
        .oldLayout = oldLayout,
        .newLayout = newLayout,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = m_swapChainImages[imageIndex],
        .subresourceRange = {
            .aspectMask = vk::ImageAspectFlagBits::eColor,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1
        }
    };
    vk::DependencyInfo dependencyInfo = {
        .dependencyFlags = {},
        .imageMemoryBarrierCount = 1,
        .pImageMemoryBarriers = &barrier
    };
    m_commandBuffers[QType::Graphics][m_frameIndex].pipelineBarrier2(dependencyInfo);
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
    }
    catch (const std::exception& err) {
        std::cerr << "Device Setup Error: " << err.what() << std::endl;
        assert(0);
    }
}

void Core::setupLogicalDevice()
{
    std::vector<vk::DeviceQueueCreateInfo> deviceQueueCI;
    std::vector<vk::QueueFamilyProperties> queueFamilyProperties = m_dGPU.getQueueFamilyProperties();
    float queuePriorities[4] = { 1.0f, 0.0f, 0.0f, 0.0f };
    const float defPriority = 1.0f;
    unsigned int queueIndices[3] = { 0, 0, 0 };
    unsigned int currQueueIndex = 1;
    bool foundGraphicsBit = false;

    m_familyIndices[QType::Graphics] = m_familyIndices[QType::Compute] = 
        m_familyIndices[QType::Transfer] = m_familyIndices[QType::Present] = queueFamilyProperties.size();
    
    // Query surface capabilities, available formats and presentation modes
    auto surfaceCapabilities = m_dGPU.getSurfaceCapabilitiesKHR(m_surface);
    std::vector<vk::SurfaceFormatKHR> availableFormats = m_dGPU.getSurfaceFormatsKHR(m_surface);
    std::vector<vk::PresentModeKHR> availablePresentModes = m_dGPU.getSurfacePresentModesKHR(m_surface);

    // Find graphics, compute, and transfer queue index
    for (size_t i = 0; i < queueFamilyProperties.size(); i++)
    {
        if (!(queueFamilyProperties[i].queueFlags & vk::QueueFlagBits::eGraphics))
        {
            if (queueFamilyProperties[i].queueFlags & vk::QueueFlagBits::eCompute)
            {
                m_familyIndices[QType::Compute] = i;
            }
            else if (queueFamilyProperties[i].queueFlags & vk::QueueFlagBits::eTransfer)
            {
                m_familyIndices[QType::Transfer] = i;
            }
        }
        else if (!foundGraphicsBit)
        {
            foundGraphicsBit = true;
            m_familyIndices[QType::Graphics] = i;
        }
    }

    if (m_familyIndices[QType::Graphics] == queueFamilyProperties.size())
    {
        throw std::runtime_error("Could not find a queue for graphics -> terminating");
    }

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
        vk::KHRCreateRenderpass2ExtensionName,
        vk::KHRShaderDrawParametersExtensionName
    };

    // determine a queueFamilyIndex that supports present
    // first check if the graphicsIndex is good enough
    if (m_dGPU.getSurfaceSupportKHR(m_familyIndices[QType::Graphics], *m_surface))
        m_familyIndices[QType::Present] = m_familyIndices[QType::Graphics];

    if (m_familyIndices[QType::Present] == queueFamilyProperties.size())
    {
        // the graphicsIndex doesn't support present -> look for another family index that supports both
        // graphics and present
        for (size_t i = m_familyIndices[QType::Graphics] == queueFamilyProperties.size() ? 0 : m_familyIndices[QType::Graphics] + 1;
            i < queueFamilyProperties.size(); i++)
        {
            if ((queueFamilyProperties[i].queueFlags & vk::QueueFlagBits::eGraphics) &&
                m_dGPU.getSurfaceSupportKHR(static_cast<uint32_t>(i), *m_surface))
            {
                m_familyIndices[QType::Graphics] = static_cast<uint32_t>(i);
                m_familyIndices[QType::Present] = m_familyIndices[QType::Graphics];
                break;
            }
        }
        if (m_familyIndices[QType::Present] == queueFamilyProperties.size())
        {
            // if there isn't a single family index that supports both graphics and present -> look for another
            // family index that supports present
            for (size_t i = 0; i < queueFamilyProperties.size(); i++)
            {
                if (m_dGPU.getSurfaceSupportKHR(static_cast<uint32_t>(i), *m_surface))
                {
                    m_familyIndices[QType::Present] = static_cast<uint32_t>(i);
                    break;
                }
            }
        }
    }
    
    deviceQueueCI.push_back({
        .queueFamilyIndex = m_familyIndices[QType::Graphics],
        .queueCount = 1,
        .pQueuePriorities = queuePriorities
    });

    if (m_familyIndices[QType::Present] == queueFamilyProperties.size())
    {
        throw std::runtime_error("Could not find a queue for present -> terminating");
    }

    if (m_familyIndices[QType::Present] == m_familyIndices[QType::Graphics])
    {
        queueIndices[QType::Present - 1] = currQueueIndex;
        queuePriorities[currQueueIndex++] = 0.25f;
    }
    else 
    {
        deviceQueueCI.push_back({
            .queueFamilyIndex = m_familyIndices[QType::Present],
            .queueCount = 1,
            .pQueuePriorities = &defPriority
        });
    }

    if (m_familyIndices[QType::Compute] == queueFamilyProperties.size())
    {
        m_familyIndices[QType::Compute] = m_familyIndices[QType::Graphics];
        queueIndices[QType::Compute - 1] = currQueueIndex;
        queuePriorities[currQueueIndex++] = 1.0f;
    }
    else 
    {
        deviceQueueCI.push_back({
            .queueFamilyIndex = m_familyIndices[QType::Compute],
            .queueCount = 1,
            .pQueuePriorities = &defPriority
        });
    }

    if (m_familyIndices[QType::Transfer] == queueFamilyProperties.size())
    {
        m_familyIndices[QType::Transfer] = m_familyIndices[QType::Graphics];
        queueIndices[QType::Transfer - 1] = currQueueIndex;
        queuePriorities[currQueueIndex++] = 0.5f;
    }
    else 
    {
        deviceQueueCI.push_back({
            .queueFamilyIndex = m_familyIndices[QType::Transfer],
            .queueCount = 1,
            .pQueuePriorities = &defPriority
        });
    }

    deviceQueueCI[QType::Graphics].setQueueCount(currQueueIndex);

    // query for Vulkan 1.3 features
    auto features = m_dGPU.getFeatures2();
    vk::PhysicalDeviceVulkan13Features vulkan13Features;
    vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT extendedDynamicStateFeatures;
    vulkan13Features.dynamicRendering = vk::True;
    extendedDynamicStateFeatures.extendedDynamicState = vk::True;
    vulkan13Features.synchronization2 = vk::True;
    vulkan13Features.pNext = &extendedDynamicStateFeatures;
    features.pNext = &vulkan13Features;

    vk::DeviceCreateInfo deviceCreateInfo 
    {
        .pNext = &features,
        .queueCreateInfoCount = static_cast<unsigned int>(deviceQueueCI.size()),
        .pQueueCreateInfos = &deviceQueueCI[0],
        .enabledExtensionCount = static_cast<unsigned int>(deviceExtensions.size()),
        .ppEnabledExtensionNames = deviceExtensions.data()
    };

    m_device = vk::raii::Device(m_dGPU, deviceCreateInfo);
    m_queues[QType::Graphics] = vk::raii::Queue(m_device, m_familyIndices[QType::Graphics], 0);
    m_queues[QType::Compute] = vk::raii::Queue(m_device, m_familyIndices[QType::Compute], queueIndices[QType::Compute - 1]);
    m_queues[QType::Transfer] = vk::raii::Queue(m_device, m_familyIndices[QType::Transfer], queueIndices[QType::Transfer - 1]);
    m_queues[QType::Present] = vk::raii::Queue(m_device, m_familyIndices[QType::Present], queueIndices[QType::Present - 1]);

    VULKAN_HPP_DEFAULT_DISPATCHER.init(*m_device);
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
    VkSurfaceKHR _surface;

    if (!SDL_Vulkan_CreateSurface(mp_window->getWindow(),
        static_cast<VkInstance>(*m_instance),
        nullptr,
        &_surface)) {
        throw std::runtime_error("SDL_Vulkan_CreateSurface failed: " +
            std::string(SDL_GetError()));
    }
    
    m_surface = vk::raii::SurfaceKHR(m_instance, _surface);
}

void Core::createSwapChain()
{
    auto surfaceCapabilities = m_dGPU.getSurfaceCapabilitiesKHR(m_surface);
    m_swapChainExtent = chooseSwapExtent(surfaceCapabilities);
    vk::SurfaceFormatKHR swapChainSurfaceFormat = chooseSwapSurfaceFormat(m_dGPU.getSurfaceFormatsKHR(m_surface));

    vk::SwapchainCreateInfoKHR swapChainCreateInfo{
        .flags = vk::SwapchainCreateFlagsKHR(),
        .surface = m_surface, .minImageCount = chooseSwapMinImageCount(surfaceCapabilities),
        .imageFormat = swapChainSurfaceFormat.format, .imageColorSpace = swapChainSurfaceFormat.colorSpace,
        .imageExtent = m_swapChainExtent, .imageArrayLayers = 1,
        .imageUsage = vk::ImageUsageFlagBits::eColorAttachment, 
        .imageSharingMode = vk::SharingMode::eExclusive,
        .preTransform = surfaceCapabilities.currentTransform, 
        .compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque,
        .presentMode = chooseSwapPresentMode(m_dGPU.getSurfacePresentModesKHR(m_surface)),
        .clipped = true, .oldSwapchain = nullptr
    };

    m_swapChain = vk::raii::SwapchainKHR(m_device, swapChainCreateInfo);
    m_swapChainImages = m_swapChain.getImages();
    m_swapChainSurfaceFormat = swapChainSurfaceFormat.format;
}

void Core::recreateSwapChain()
{
    int w, h;
    SDL_GetWindowSize(mp_window->getWindow(), &w, &h);
    
    while (w == 0 || h == 0) 
    {
        SDL_GetWindowSize(mp_window->getWindow(), &w, &h);
        SDL_Event e;
        SDL_WaitEvent(&e);
    }

    m_device.waitIdle();

    cleanSwapChain();
    createSwapChain();
    createImageViews();
}

void Core::cleanSwapChain()
{
    m_swapChainImageViews.clear();
    m_swapChain = nullptr;
}

void Core::createImageViews()
{
    m_swapChainImageViews.clear();

    vk::ImageViewCreateInfo imageViewCreateInfo{ 
        .viewType = vk::ImageViewType::e2D, 
        .format = m_swapChainSurfaceFormat,
        .subresourceRange = { vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 } 
    };

    imageViewCreateInfo.components.r = vk::ComponentSwizzle::eIdentity;
    imageViewCreateInfo.components.g = vk::ComponentSwizzle::eIdentity;
    imageViewCreateInfo.components.b = vk::ComponentSwizzle::eIdentity;
    imageViewCreateInfo.components.a = vk::ComponentSwizzle::eIdentity;

    imageViewCreateInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
    imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
    imageViewCreateInfo.subresourceRange.levelCount = 1;
    imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
    imageViewCreateInfo.subresourceRange.layerCount = 1;

    for (auto& image : m_swapChainImages) {
        imageViewCreateInfo.image = image;
        m_swapChainImageViews.emplace_back(m_device, imageViewCreateInfo);
    }
}

void Core::createDescriptorLayout() 
{
    vk::DescriptorSetLayoutBinding uboLayoutBinding(0, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eVertex, nullptr);
    vk::DescriptorSetLayoutCreateInfo layoutInfo{ .bindingCount = 1, .pBindings = &uboLayoutBinding };
    m_descriptorSetLayout = vk::raii::DescriptorSetLayout(m_device, layoutInfo);
}

void Core::createGraphicsPipeline()
{   
    vk::raii::ShaderModule vertModule = createShaderModule(ReadFile("..\\..\\..\\out\\shaders\\triangle.vert.spv")),
                           fragModule = createShaderModule(ReadFile("..\\..\\..\\out\\shaders\\triangle.frag.spv"));

    vk::PipelineShaderStageCreateInfo vertShaderStageInfo{ 
        .stage = vk::ShaderStageFlagBits::eVertex, 
        .module = vertModule,  
        .pName = "vertMain" 
    };

    vk::PipelineShaderStageCreateInfo fragShaderStageInfo{ 
        .stage = vk::ShaderStageFlagBits::eFragment, 
        .module = fragModule, 
        .pName = "fragMain"
    };

    vk::PipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

    std::vector dynamicStates = {
        vk::DynamicState::eViewport,
        vk::DynamicState::eScissor
    };

    vk::PipelineDynamicStateCreateInfo dynamicState{ 
        .dynamicStateCount = static_cast<uint32_t>(dynamicStates.size()), 
        .pDynamicStates = dynamicStates.data()
    };

    auto bindingDescription = Vertex::getBindingDesc();
    auto attributeDescriptions = Vertex::getAttribDesc();
    vk::PipelineVertexInputStateCreateInfo vertexInputInfo{
        .vertexBindingDescriptionCount = 1, 
        .pVertexBindingDescriptions = &bindingDescription,
        .vertexAttributeDescriptionCount = 2, 
        .pVertexAttributeDescriptions = &attributeDescriptions.first
    };
    
    vk::PipelineInputAssemblyStateCreateInfo inputAssembly{ 
        .topology = vk::PrimitiveTopology::eTriangleList 
    };

    vk::PipelineViewportStateCreateInfo viewportState{ 
        .viewportCount = 1, 
        .scissorCount = 1 
    };

    vk::PipelineRasterizationStateCreateInfo rasterizer{ 
        .depthClampEnable = vk::False, 
        .rasterizerDiscardEnable = vk::False,
        .polygonMode = vk::PolygonMode::eFill, 
        .cullMode = vk::CullModeFlagBits::eBack,
        .frontFace = vk::FrontFace::eCounterClockwise, 
        .depthBiasEnable = vk::False,
        .depthBiasSlopeFactor = 1.0f, 
        .lineWidth = 1.0f 
    };

    // Useful for antialiasing
    vk::PipelineMultisampleStateCreateInfo multisampling{ 
        .rasterizationSamples = vk::SampleCountFlagBits::e1, 
        .sampleShadingEnable = vk::False 
    };

    vk::PipelineColorBlendAttachmentState colorBlendAttachment{
        .blendEnable = vk::True,
        .srcColorBlendFactor = vk::BlendFactor::eSrcAlpha,
        .dstColorBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha,
        .colorBlendOp = vk::BlendOp::eAdd,
        .srcAlphaBlendFactor = vk::BlendFactor::eOne,
        .dstAlphaBlendFactor = vk::BlendFactor::eZero,
        .alphaBlendOp = vk::BlendOp::eAdd,
        .colorWriteMask =
            vk::ColorComponentFlagBits::eR |
            vk::ColorComponentFlagBits::eG |
            vk::ColorComponentFlagBits::eB |
            vk::ColorComponentFlagBits::eA
    };

    vk::PipelineColorBlendStateCreateInfo colorBlending{ 
        .logicOpEnable = vk::False, 
        .logicOp = vk::LogicOp::eCopy, 
        .attachmentCount = 1, 
        .pAttachments = &colorBlendAttachment 
    };

    // For uniforms
    vk::PipelineLayoutCreateInfo pipelineLayoutInfo{ 
        .setLayoutCount = 1, 
        .pSetLayouts = &*m_descriptorSetLayout, 
        .pushConstantRangeCount = 0
    };
    m_pipelineLayout = vk::raii::PipelineLayout(m_device, pipelineLayoutInfo);

    vk::PipelineRenderingCreateInfo pipelineRenderingCreateInfo{ 
        .colorAttachmentCount = 1, 
        .pColorAttachmentFormats = &m_swapChainSurfaceFormat 
    };

    vk::GraphicsPipelineCreateInfo pipelineInfo{ 
        .pNext = &pipelineRenderingCreateInfo,
        .stageCount = 2, .pStages = shaderStages,
        .pVertexInputState = &vertexInputInfo, .pInputAssemblyState = &inputAssembly,
        .pViewportState = &viewportState, .pRasterizationState = &rasterizer,
        .pMultisampleState = &multisampling, .pColorBlendState = &colorBlending,
        .pDynamicState = &dynamicState, .layout = m_pipelineLayout, .renderPass = nullptr,
        .basePipelineHandle = VK_NULL_HANDLE, .basePipelineIndex = -1
    };

    //pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
    //pipelineInfo.basePipelineIndex = -1; // Optional

    m_graphicsPipeline = vk::raii::Pipeline(m_device, nullptr, pipelineInfo);
}

void Core::createCommandPools()
{
    m_commandPools[QType::Graphics] = vk::raii::CommandPool(m_device, {
        .flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
        .queueFamilyIndex = m_familyIndices[QType::Graphics]
    });

    m_commandPools[QType::Compute] = vk::raii::CommandPool(m_device, {
        .flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
        .queueFamilyIndex = m_familyIndices[QType::Compute]
    });

    m_commandPools[QType::Transfer] = vk::raii::CommandPool(m_device, {
        .flags = vk::CommandPoolCreateFlagBits::eTransient,
        .queueFamilyIndex = m_familyIndices[QType::Transfer]
    });
}

void Core::createCommandBuffers()
{
    m_commandBuffers[QType::Graphics].clear();
    m_commandBuffers[QType::Graphics] = vk::raii::CommandBuffers(m_device, {
        .commandPool = m_commandPools[QType::Graphics],
        .level = vk::CommandBufferLevel::ePrimary,
        .commandBufferCount = MAX_FRAMES_IN_FLIGHT 
    });

    m_commandBuffers[QType::Transfer].clear();
    m_commandBuffers[QType::Transfer] = vk::raii::CommandBuffers(m_device, {
        .commandPool = m_commandPools[QType::Transfer],
        .level = vk::CommandBufferLevel::ePrimary,
        .commandBufferCount = 1        // One for now
    });
}

void Core::createSyncObjects()
{
    m_presentCompleteSemaphores.clear();
    m_renderFinishedSemaphores.clear();
    m_inFlightFences.clear();

    for (size_t i = 0; i < m_swapChainImages.size(); i++)
    {
        m_presentCompleteSemaphores.emplace_back(m_device, vk::SemaphoreCreateInfo());
        m_renderFinishedSemaphores.emplace_back(m_device, vk::SemaphoreCreateInfo());
    }

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        m_inFlightFences.emplace_back(m_device, vk::FenceCreateInfo{ .flags = vk::FenceCreateFlagBits::eSignaled });
    }
}

void Core::createMeshes()
{
    Allocator::init(m_instance, m_dGPU, m_device);

    m_pDMemoryProperties = m_dGPU.getMemoryProperties();
    triangle.init(m_device, &vertex_data, &index_data);
}

void Core::createUBOs() 
{
    m_uniformBuffers.clear();
    m_uniformBufferAllocations.clear();
    m_uniformBuffersMapped.clear();
    VmaAllocator& allocator = Allocator::getAllocator();
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        vk::DeviceSize bufferSize = sizeof(UniformBufferObject);
        VkBuffer buffer({});
        VmaAllocationInfo allocInfo{};
        m_uniformBufferAllocations.push_back(
            Buffer::Create(
                m_device, 
                bufferSize, 
                VkBufferUsageFlagBits::VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, 
                VkMemoryPropertyFlagBits::VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VkMemoryPropertyFlagBits::VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
                buffer,
                VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT));
        
        void* data = nullptr;
        vmaMapMemory(allocator, m_uniformBufferAllocations[i], &data);
        m_uniformBuffers.emplace_back(vk::Buffer(std::move(buffer)));
        m_uniformBuffersMapped.emplace_back(data);
    }
}

void Core::recordCommandBuffer(uint32_t imageIndex)
{
    auto& cmd = m_commandBuffers[QType::Graphics][m_frameIndex];
    cmd.begin({});
    // Before starting rendering, transition the swapchain image to COLOR_ATTACHMENT_OPTIMAL
    transitionImageLayout(
        imageIndex,
        vk::ImageLayout::eUndefined,
        vk::ImageLayout::eColorAttachmentOptimal,
        {},                                                        // srcAccessMask (no need to wait for previous operations)
        vk::AccessFlagBits2::eColorAttachmentWrite,                // dstAccessMask
        vk::PipelineStageFlagBits2::eColorAttachmentOutput,        // srcStage
        vk::PipelineStageFlagBits2::eColorAttachmentOutput         // dstStage
    );
    vk::ClearValue clearColor = vk::ClearColorValue(0.0f, 0.0f, 0.0f, 1.0f);
    
    vk::RenderingAttachmentInfo attachmentInfo = {
        .imageView = m_swapChainImageViews[imageIndex],
        .imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
        .loadOp = vk::AttachmentLoadOp::eClear,
        .storeOp = vk::AttachmentStoreOp::eStore,
        .clearValue = clearColor 
    };
    
    vk::RenderingInfo renderingInfo = {
        .renderArea = {.offset = {0, 0}, .extent = m_swapChainExtent},
        .layerCount = 1,
        .colorAttachmentCount = 1,
        .pColorAttachments = &attachmentInfo 
    };

    cmd.beginRendering(renderingInfo);
    cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, *m_graphicsPipeline);
    cmd.setViewport(0, vk::Viewport(0.0f, 0.0f, static_cast<float>(m_swapChainExtent.width), static_cast<float>(m_swapChainExtent.height), 0.0f, 1.0f));
    cmd.setScissor(0, vk::Rect2D(vk::Offset2D(0, 0), m_swapChainExtent));
    
    // -----DRAW HERE-----
    triangle.bind(cmd);
    cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, m_pipelineLayout, 0, *m_descriptorSets[m_frameIndex], nullptr);
    triangle.draw(cmd);
    
    cmd.endRendering();
    
    // After rendering, transition the swapchain image to PRESENT_SRC
    transitionImageLayout(
        imageIndex,
        vk::ImageLayout::eColorAttachmentOptimal,
        vk::ImageLayout::ePresentSrcKHR,
        vk::AccessFlagBits2::eColorAttachmentWrite,                // srcAccessMask
        {},                                                        // dstAccessMask
        vk::PipelineStageFlagBits2::eColorAttachmentOutput,        // srcStage
        vk::PipelineStageFlagBits2::eBottomOfPipe                  // dstStage
    );

    cmd.end();
}

vk::raii::ShaderModule Core::createShaderModule(const std::vector<char>& code) const
{
    vk::ShaderModuleCreateInfo createInfo{ 
        .codeSize = code.size() * sizeof(char), 
        .pCode = reinterpret_cast<const uint32_t*>(code.data())
    };

    return vk::raii::ShaderModule{ m_device, createInfo };
}

vk::SurfaceFormatKHR Core::chooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& availableFormats) 
{
    for (const auto& availableFormat : availableFormats) {
        if (availableFormat.format == vk::Format::eB8G8R8A8Srgb && availableFormat.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear) {
            return availableFormat;
        }
    }
    return availableFormats[0];
}

vk::PresentModeKHR Core::chooseSwapPresentMode(const std::vector<vk::PresentModeKHR>& availablePresentModes) \
{
    for (const auto& availablePresentMode : availablePresentModes) {
        if (availablePresentMode == vk::PresentModeKHR::eMailbox) {
            return availablePresentMode;
        }
    }
    return vk::PresentModeKHR::eFifo;
}

vk::Extent2D Core::chooseSwapExtent(const vk::SurfaceCapabilitiesKHR& capabilities)
{
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
        return capabilities.currentExtent;
    }
    int width, height;
    
    SDL_GetWindowSizeInPixels(mp_window->getWindow(), &width, &height);

    return {
        std::clamp<uint32_t>(width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width),
        std::clamp<uint32_t>(height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height)
    };
}

Core& Core::GetInstance()
{
    
    if (!mp_instance)
        mp_instance = new Core();
    return *mp_instance;
}

std::vector<const char*> Core::getRequiredExtensions()
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

void Core::updateUniformBuffers() 
{
    static auto startTime = std::chrono::high_resolution_clock::now();

    auto currentTime = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

    UniformBufferObject ubo{};
    ubo.model = rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    ubo.view = lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    ubo.proj = glm::perspective(glm::radians(45.0f), static_cast<float>(m_swapChainExtent.width) / static_cast<float>(m_swapChainExtent.height), 0.1f, 10.0f);
    ubo.proj[1][1] *= -1;

    memcpy(m_uniformBuffersMapped[m_frameIndex], &ubo, sizeof(ubo));
}

void Core::createDescriptorPool() 
{
    vk::DescriptorPoolSize poolSize(vk::DescriptorType::eUniformBuffer, MAX_FRAMES_IN_FLIGHT);
    vk::DescriptorPoolCreateInfo poolInfo{ 
        .flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet, 
        .maxSets = MAX_FRAMES_IN_FLIGHT, 
        .poolSizeCount = 1, 
        .pPoolSizes = &poolSize };

    m_descriptorPool = vk::raii::DescriptorPool(m_device, poolInfo);
}

void Core::createDiscriptorSets() 
{
    std::vector<vk::DescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, *m_descriptorSetLayout);
   
    vk::DescriptorSetAllocateInfo allocInfo{ 
        .descriptorPool = m_descriptorPool, 
        .descriptorSetCount = static_cast<uint32_t>(layouts.size()), 
        .pSetLayouts = layouts.data() };

    m_descriptorSets.clear();
    m_descriptorSets = m_device.allocateDescriptorSets(allocInfo);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) 
    {
        vk::DescriptorBufferInfo bufferInfo{ 
            .buffer = m_uniformBuffers[i], 
            .offset = 0, 
            .range = sizeof(UniformBufferObject) };

        vk::WriteDescriptorSet descriptorWrite{ 
            .dstSet = m_descriptorSets[i], 
            .dstBinding = 0, 
            .dstArrayElement = 0, 
            .descriptorCount = 1, 
            .descriptorType = vk::DescriptorType::eUniformBuffer, 
            .pBufferInfo = &bufferInfo };

        m_device.updateDescriptorSets(descriptorWrite, {});
    }
}

void Core::draw()
{
    while (vk::Result::eTimeout == m_device.waitForFences(*m_inFlightFences[m_frameIndex], vk::True, UINT64_MAX));

    auto [result, imageIndex] = m_swapChain.acquireNextImage(UINT64_MAX, *m_presentCompleteSemaphores[m_semaphoreIndex], nullptr);

    if (result == vk::Result::eErrorOutOfDateKHR)
    {
        recreateSwapChain();
        return;
    }
    if (result != vk::Result::eSuccess && result != vk::Result::eSuboptimalKHR)
    {
        throw std::runtime_error("failed to acquire swap chain image!");
    }

    m_device.resetFences(*m_inFlightFences[m_frameIndex]);
    m_commandBuffers[QType::Graphics][m_frameIndex].reset();
    recordCommandBuffer(imageIndex);

    updateUniformBuffers();

    vk::PipelineStageFlags waitDestinationStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput);
    const vk::SubmitInfo   submitInfo{ 
        .waitSemaphoreCount = 1, 
        .pWaitSemaphores = &*m_presentCompleteSemaphores[m_semaphoreIndex],
        .pWaitDstStageMask = &waitDestinationStageMask, 
        .commandBufferCount = 1, 
        .pCommandBuffers = &*m_commandBuffers[QType::Graphics][m_frameIndex],
        .signalSemaphoreCount = 1, 
        .pSignalSemaphores = &*m_renderFinishedSemaphores[imageIndex]
    };
    m_queues[QType::Graphics].submit(submitInfo, *m_inFlightFences[m_frameIndex]);

    try
    {
        const vk::PresentInfoKHR presentInfoKHR{ 
            .waitSemaphoreCount = 1, 
            .pWaitSemaphores = &*m_renderFinishedSemaphores[imageIndex],
            .swapchainCount = 1, 
            .pSwapchains = &*m_swapChain,
            .pImageIndices = &imageIndex 
        };

        result = m_queues[QType::Present].presentKHR(presentInfoKHR);
        if (result == vk::Result::eErrorOutOfDateKHR || result == vk::Result::eSuboptimalKHR || m_framebufferResized)
        {
            m_framebufferResized = false;
            recreateSwapChain();
        }
        else if (result != vk::Result::eSuccess)
        {
            throw std::runtime_error("failed to present swap chain image!");
        }
    }
    catch (const vk::SystemError& e)
    {
        if (e.code().value() == static_cast<int>(vk::Result::eErrorOutOfDateKHR))
        {
            recreateSwapChain();
            return;
        }
        else
        {
            throw;
        }
    }
    
    m_semaphoreIndex = (m_semaphoreIndex + 1) % m_presentCompleteSemaphores.size();
    m_frameIndex = (m_frameIndex + 1) % MAX_FRAMES_IN_FLIGHT;
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

    std::vector<char const*> requiredLayers, requiredExtensions = getRequiredExtensions();

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

        VULKAN_HPP_DEFAULT_DISPATCHER.init(vkGetInstanceProcAddr);

        vk::InstanceCreateInfo createInfo{
            .sType = vk::StructureType::eInstanceCreateInfo,
            .pApplicationInfo = &appInfo,
            .enabledLayerCount = static_cast<uint32_t>(requiredLayers.size()),
            .ppEnabledLayerNames = requiredLayers.data(),
            .enabledExtensionCount = static_cast<uint32_t>(requiredExtensions.size()),
            .ppEnabledExtensionNames = requiredExtensions.data()
        };

        m_instance = vk::raii::Instance{ m_context, createInfo };
        VULKAN_HPP_DEFAULT_DISPATCHER.init(*m_instance);

#ifndef NDEBUG
        setupDebugMessenger();
#endif
        createSurface();
        selectPhysicalDevices();
        setupLogicalDevice();
        createSwapChain();
        createImageViews();
        createDescriptorLayout();
        createGraphicsPipeline();
        createCommandPools();
        createMeshes();
        createUBOs();
        createDescriptorPool();
        createDiscriptorSets();
        createCommandBuffers();
        createSyncObjects();
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
        draw();
    }

    m_device.waitIdle();
}

void Core::clean()
{
    VmaAllocator allocator = Allocator::getAllocator();
    cleanSwapChain();
    
    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) 
    {
        vmaUnmapMemory(allocator, m_uniformBufferAllocations[i]);
        vmaDestroyBuffer(allocator, static_cast<VkBuffer>(m_uniformBuffers[i]), m_uniformBufferAllocations[i]);
    }

    triangle.destroy();
    Allocator::clean();
    mp_window->clean();
    Renderer::GetInstance().clean();
    delete(mp_window);
}
