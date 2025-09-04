#pragma once
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>

class SDLWindow;

class Core 
{
private:
	vk::raii::Context m_context;
	vk::raii::Instance m_instance = nullptr;
	vk::raii::DebugUtilsMessengerEXT m_debugMessenger = nullptr;
	const vk::raii::PhysicalDevice* mp_dGPU;

	static Core* mp_instance;
	SDLWindow* mp_window;

	//@brief Sets up debug messenger callback
	void setupDebugMessenger();
	//@brief Selects physical devices for Vulkan rendering
	void selectPhysicalDevices();
	//@brief Determines if a physical devices meets desired specifications for graphics purposes
	bool suitableDiscreteGPU(const vk::raii::PhysicalDevice& device);

public:
	Core() {}
	//@brief Gets static instance
	static Core& GetInstance();
	static std::vector<const char*> GetRequiredExtensions();

	//@brief Run engine
	void run();
	//@brief Init data members
	void init();
	//@brief Core engine loop
	void loop();
	//@brief Clean engine data members
	void clean();
};