#pragma once
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>

class SDLWindow;

//@brief Contains core logic for engine
class Core 
{
private:
	vk::raii::Context m_context;
	vk::raii::Instance m_instance = nullptr;
	vk::raii::DebugUtilsMessengerEXT m_debugMessenger = nullptr;
	vk::raii::Queue m_graphicsQueue = nullptr, m_presentQueue = nullptr;
	vk::raii::Device m_device = nullptr;
	vk::raii::PhysicalDevice m_dGPU = nullptr;
	vk::raii::SurfaceKHR m_surface = nullptr;
	SDLWindow* mp_window;

	static Core* mp_instance;

	//@brief Sets up debug messenger callback
	void setupDebugMessenger();
	//@brief Selects physical devices for Vulkan rendering
	void selectPhysicalDevices();
	//@brief Creates logical device and specifies queues to use
	void setupLogicalDevice();
	//@brief Determines if a physical devices meets desired specifications for graphics purposes
	//@return True if physical device is a discrete GPU with geometry shader capabilities
	bool suitableDiscreteGPU(const vk::raii::PhysicalDevice& device);

	void createSurface();

public:
	//@brief Gets static instance
	static Core& GetInstance();
	//@brief Retrieves required Vulkan extensions
	static std::vector<const char*> GetRequiredExtensions();

	//@brief Runs engine
	void run();
	//@brief Inits data members
	void init();
	//@brief Core engine loop
	void loop();
	//@brief Cleans engine data members
	void clean();
};