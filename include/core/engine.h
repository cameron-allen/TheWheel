#pragma once
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>

class SDLWindow;

class Core 
{
private:
	vk::raii::Context  m_context;
	vk::raii::Instance m_instance = nullptr;
	SDLWindow* mp_window;

public:
	//@brief Run engine
	void run();
	//@brief Init data members
	void init();
	//@brief Core engine loop
	void loop();
	//@brief Clean engine data members
	void clean();
};