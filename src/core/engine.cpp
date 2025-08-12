#include "pch.h"
#include "core/engine.h"
#include "core/window.h"
#include <vulkan/vulkan.hpp>

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

    VkInstance instance;
    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Hello Triangle";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "No Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_0;

    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;
    createInfo.enabledExtensionCount = 0;
    createInfo.ppEnabledExtensionNames = nullptr;

    createInfo.enabledLayerCount = 0;

    if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
        throw std::runtime_error("failed to create instance!");
    }

    vkDestroyInstance(instance, nullptr);
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
