#include "pch.h"
#include "core/engine.h"
#include "core/window.h"

// More info for Vulkan debug configuration at the bottom of this page:
// https://docs.vulkan.org/tutorial/latest/03_Drawing_a_triangle/00_Setup/02_Validation_layers.html

constexpr int MAX_FRAMES_IN_FLIGHT = 2;

Core* Core::mp_instance = nullptr;
bool Core::m_framebufferResized = false;

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
    // MAYBE
    m_commandBuffers[m_currentFrame].pipelineBarrier2(dependencyInfo);
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
    std::vector<vk::QueueFamilyProperties> queueFamilyProperties = m_dGPU.getQueueFamilyProperties();
    
    // get the first index into queueFamilyProperties which supports graphics
    auto graphicsQueueFamilyProperty = std::ranges::find_if(queueFamilyProperties, [](auto const& qfp)
        { return (qfp.queueFlags & vk::QueueFlagBits::eGraphics) != static_cast<vk::QueueFlags>(0); });

    // Query surface capabilities, available formats and presentation modes
    auto surfaceCapabilities = m_dGPU.getSurfaceCapabilitiesKHR(m_surface);
    std::vector<vk::SurfaceFormatKHR> availableFormats = m_dGPU.getSurfaceFormatsKHR(m_surface);
    std::vector<vk::PresentModeKHR> availablePresentModes = m_dGPU.getSurfacePresentModesKHR(m_surface);
    
    float queuePriority = 1.0f;
    m_gFamilyIndex = static_cast<uint32_t>(std::distance(queueFamilyProperties.begin(), graphicsQueueFamilyProperty));

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
    m_pFamilyIndex = m_dGPU.getSurfaceSupportKHR(m_gFamilyIndex, *m_surface)
        ? m_gFamilyIndex
        : static_cast<uint32_t>(queueFamilyProperties.size());

    if (m_pFamilyIndex == queueFamilyProperties.size())
    {
        // the graphicsIndex doesn't support present -> look for another family index that supports both
        // graphics and present
        for (size_t i = 0; i < queueFamilyProperties.size(); i++)
        {
            if ((queueFamilyProperties[i].queueFlags & vk::QueueFlagBits::eGraphics) &&
                m_dGPU.getSurfaceSupportKHR(static_cast<uint32_t>(i), *m_surface))
            {
                m_gFamilyIndex = static_cast<uint32_t>(i);
                m_pFamilyIndex = m_gFamilyIndex;
                break;
            }
        }
        if (m_pFamilyIndex == queueFamilyProperties.size())
        {
            // if there's nothing isn't a single family index that supports both graphics and present -> look for another
            // family index that supports present
            for (size_t i = 0; i < queueFamilyProperties.size(); i++)
            {
                if (m_dGPU.getSurfaceSupportKHR(static_cast<uint32_t>(i), *m_surface))
                {
                    m_pFamilyIndex = static_cast<uint32_t>(i);
                    break;
                }
            }
        }
    }
    if ((m_gFamilyIndex == queueFamilyProperties.size()) || (m_pFamilyIndex == queueFamilyProperties.size()))
    {
        throw std::runtime_error("Could not find a queue for graphics or present -> terminating");
    }

    // query for Vulkan 1.3 features
    auto features = m_dGPU.getFeatures2();
    vk::PhysicalDeviceVulkan13Features vulkan13Features;
    vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT extendedDynamicStateFeatures;
    vulkan13Features.dynamicRendering = vk::True;
    extendedDynamicStateFeatures.extendedDynamicState = vk::True;
    vulkan13Features.synchronization2 = vk::True;
    vulkan13Features.pNext = &extendedDynamicStateFeatures;
    features.pNext = &vulkan13Features;
    
    // create a Device
    vk::DeviceQueueCreateInfo deviceQueueCreateInfo
    {
        .queueFamilyIndex = m_gFamilyIndex,
        .queueCount = 1,
        .pQueuePriorities = &queuePriority,
    };

    vk::DeviceCreateInfo deviceCreateInfo 
    {
        .pNext = &features,
        .queueCreateInfoCount = 1,
        .pQueueCreateInfos = &deviceQueueCreateInfo,
        .enabledExtensionCount = static_cast<unsigned int>(deviceExtensions.size()),
        .ppEnabledExtensionNames = deviceExtensions.data()
    };

    m_device = vk::raii::Device(m_dGPU, deviceCreateInfo);
    m_graphicsQueue = vk::raii::Queue(m_device, m_gFamilyIndex, 0);
    m_presentQueue = vk::raii::Queue(m_device, m_pFamilyIndex, 0);
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
    m_device.waitIdle();
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

    vk::PipelineVertexInputStateCreateInfo vertexInputInfo{};
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
        .frontFace = vk::FrontFace::eClockwise, 
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
    vk::raii::PipelineLayout pipelineLayout = nullptr;
    vk::PipelineLayoutCreateInfo pipelineLayoutInfo{ 
        .setLayoutCount = 0, 
        .pushConstantRangeCount = 0 
    };
    pipelineLayout = vk::raii::PipelineLayout(m_device, pipelineLayoutInfo);

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
        .pDynamicState = &dynamicState, .layout = pipelineLayout, .renderPass = nullptr ,
        .basePipelineHandle = VK_NULL_HANDLE, .basePipelineIndex = -1
    };

    //pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
    //pipelineInfo.basePipelineIndex = -1; // Optional

    m_graphicsPipeline = vk::raii::Pipeline(m_device, nullptr, pipelineInfo);
}

void Core::createCommandPool()
{
    vk::CommandPoolCreateInfo poolInfo{
        .flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
        .queueFamilyIndex = m_gFamilyIndex
    };

    m_commandPool = vk::raii::CommandPool(m_device, poolInfo);
}

void Core::createCommandBuffers()
{
    m_commandBuffers.clear();

    vk::CommandBufferAllocateInfo allocInfo{ 
        .commandPool = m_commandPool, 
        .level = vk::CommandBufferLevel::ePrimary, 
        .commandBufferCount = MAX_FRAMES_IN_FLIGHT 
    };

    m_commandBuffers = vk::raii::CommandBuffers(m_device, allocInfo);
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

void Core::recordCommandBuffer(uint32_t imageIndex)
{
    m_commandBuffers[m_currentFrame].begin({});
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

    m_commandBuffers[m_currentFrame].beginRendering(renderingInfo);
    m_commandBuffers[m_currentFrame].bindPipeline(vk::PipelineBindPoint::eGraphics, *m_graphicsPipeline);
    m_commandBuffers[m_currentFrame].setViewport(0, vk::Viewport(0.0f, 0.0f, static_cast<float>(m_swapChainExtent.width), static_cast<float>(m_swapChainExtent.height), 0.0f, 1.0f));
    m_commandBuffers[m_currentFrame].setScissor(0, vk::Rect2D(vk::Offset2D(0, 0), m_swapChainExtent));
    m_commandBuffers[m_currentFrame].draw(3, 1, 0, 0);
    m_commandBuffers[m_currentFrame].endRendering();
    
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

    m_commandBuffers[m_currentFrame].end();
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
    
    SDL_GetWindowSizeInPixels(mp_window->GetWindow(), &width, &height);

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

void Core::draw()
{
    while (vk::Result::eTimeout == m_device.waitForFences(*m_inFlightFences[m_currentFrame], vk::True, UINT64_MAX));

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

    m_device.resetFences(*m_inFlightFences[m_currentFrame]);
    m_commandBuffers[m_currentFrame].reset();
    recordCommandBuffer(imageIndex);

    vk::PipelineStageFlags waitDestinationStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput);
    const vk::SubmitInfo   submitInfo{ 
        .waitSemaphoreCount = 1, 
        .pWaitSemaphores = &*m_presentCompleteSemaphores[m_semaphoreIndex],
        .pWaitDstStageMask = &waitDestinationStageMask, 
        .commandBufferCount = 1, 
        .pCommandBuffers = &*m_commandBuffers[m_currentFrame],
        .signalSemaphoreCount = 1, 
        .pSignalSemaphores = &*m_renderFinishedSemaphores[imageIndex]
    };
    m_presentQueue.submit(submitInfo, *m_inFlightFences[m_currentFrame]);

    try
    {
        const vk::PresentInfoKHR presentInfoKHR{ 
            .waitSemaphoreCount = 1, 
            .pWaitSemaphores = &*m_renderFinishedSemaphores[imageIndex],
            .swapchainCount = 1, 
            .pSwapchains = &*m_swapChain,
            .pImageIndices = &imageIndex 
        };

        result = m_presentQueue.presentKHR(presentInfoKHR);
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
    m_currentFrame = (m_currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
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
        createSurface();
        selectPhysicalDevices();
        setupLogicalDevice();
        createSwapChain();
        createImageViews();
        createGraphicsPipeline();
        createCommandPool();
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
    cleanSwapChain();

    //TODO clean SD3

    mp_window->clean();
    delete(mp_window);
}
