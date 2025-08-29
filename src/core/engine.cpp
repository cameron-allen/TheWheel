#include "pch.h"
#include "core/engine.h"
#include "core/window.h"

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
        .pApplicationName = "Hello Triangle",
        .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
        .pEngineName = "No Engine",
        .engineVersion = VK_MAKE_VERSION(1, 0, 0),
        .apiVersion = vk::ApiVersion14
    };

    Uint32 sdlExtensionCount = 0;
    const char* const* sdlExtensions = SDL_Vulkan_GetInstanceExtensions(&sdlExtensionCount);
    auto extensionProperties = m_context.enumerateInstanceExtensionProperties();

    try 
    {
        for (uint32_t i = 0; i < sdlExtensionCount; i++)
        {
            if (std::ranges::none_of(extensionProperties,
                [sdlExtension = sdlExtensions[i]](auto const& extensionProperty)
                { return strcmp(extensionProperty.extensionName, sdlExtension) == 0; }))
            {
                throw std::runtime_error("Required SDL3 extension not supported: " + std::string(sdlExtensions[i]));
            }
        }

        vk::InstanceCreateInfo createInfo{
            .sType = vk::StructureType::eInstanceCreateInfo,
            .pApplicationInfo = &appInfo,
            .enabledExtensionCount = sdlExtensionCount,
            .ppEnabledExtensionNames = sdlExtensions
        };

        m_instance = vk::raii::Instance{ m_context, createInfo };
        vk::raii::PhysicalDevice physicalDevice = m_instance.enumeratePhysicalDevices().front();
        vk::raii::Device device(physicalDevice, vk::DeviceCreateInfo{});

        // Use Vulkan objects
        vk::raii::Buffer buffer(device, vk::BufferCreateInfo{});
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
