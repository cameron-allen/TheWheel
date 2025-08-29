#pragma once
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>

class SDLWindow;

class Core 
{
private:
	vk::raii::Context  m_context;
	vk::raii::Instance m_instance = nullptr;
	vk::raii::DebugUtilsMessengerEXT m_debugMessenger = nullptr;

	static Core* mp_instance;
	SDLWindow* mp_window;

	//@brief Sets up debug messenger callback
	void setupDebugMessenger();

public:
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