#pragma once
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>

class SDLWindow;

//@brief Contains core engine logic
class Core 
{
private:
	std::vector<vk::Image> m_swapChainImages;
	std::vector<vk::raii::ImageView> swapChainImageViews;
	vk::raii::Context m_context;
	vk::raii::Instance m_instance = nullptr;
	vk::raii::DebugUtilsMessengerEXT m_debugMessenger = nullptr;
	vk::raii::Queue m_graphicsQueue = nullptr, m_presentQueue = nullptr;
	vk::raii::Device m_device = nullptr;
	vk::raii::PhysicalDevice m_dGPU = nullptr;
	vk::raii::SurfaceKHR m_surface = nullptr; 
	vk::raii::SwapchainKHR m_swapChain = nullptr;
	vk::Format m_swapChainSurfaceFormat = vk::Format::eUndefined;
	vk::Extent2D m_swapChainExtent;
	SDLWindow* mp_window;
	//			 graphics			 present
	unsigned int m_gFamilyIndex = 0, m_pFamilyIndex = 0;
	
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
	//@brief Creates Vulkan rendering surface
	void createSurface();
	//@brief Creates surface swap chain
	void createSwapChain();
	//@brief Creates image views
	void createImageViews();

	//@brief Chooses swap chain surface format
	vk::SurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& availableFormats);
	//@brief Chooses swap chain present mode
	vk::PresentModeKHR chooseSwapPresentMode(const std::vector<vk::PresentModeKHR>& availablePresentModes);
	//@brief Chooses swap chain surface extent
	vk::Extent2D chooseSwapExtent(const vk::SurfaceCapabilitiesKHR& capabilities);

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