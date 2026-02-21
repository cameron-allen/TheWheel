#pragma once
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>

// Queue Types
enum QType 
{
	Graphics,
	Compute,
	Transfer,
	Present
};

class SDLWindow;

//@brief Contains core engine logic
class Core 
{
	friend class SDLWindow;

private:
	vk::PhysicalDeviceMemoryProperties m_pDMemoryProperties;
	vk::raii::CommandPool m_commandPools[3] = { nullptr, nullptr, nullptr };
	std::vector<vk::raii::CommandBuffer> m_commandBuffers[3];
	std::vector<vk::Image> m_swapChainImages;
	std::vector<vk::raii::ImageView> m_swapChainImageViews;
	std::vector<vk::raii::Semaphore> m_presentCompleteSemaphores;
	std::vector<vk::raii::Semaphore> m_renderFinishedSemaphores;
	std::vector<vk::raii::Fence> m_inFlightFences;
	vk::raii::Queue m_queues[4] = { nullptr, nullptr, nullptr, nullptr };
	vk::raii::Context m_context{};
	vk::raii::Instance m_instance = nullptr;
	vk::raii::DebugUtilsMessengerEXT m_debugMessenger = nullptr;
	vk::raii::Device m_device = nullptr;
	vk::raii::PhysicalDevice m_dGPU = nullptr;
	vk::raii::Pipeline m_graphicsPipeline = nullptr;
	vk::raii::SurfaceKHR m_surface = nullptr; 
	vk::raii::SwapchainKHR m_swapChain = nullptr;

	vk::Format m_swapChainSurfaceFormat = vk::Format::eUndefined;
	vk::Extent2D m_swapChainExtent;
	SDLWindow* mp_window = nullptr;
	//		 graphics, compute, transfer, present
	uint32_t m_familyIndices[4] = { 0, 0, 0, 0 };
	uint32_t m_currentFrame = 0;
	uint32_t m_semaphoreIndex = 0;

	bool m_framebufferResized = false;
	
	static inline Core* mp_instance = nullptr;

	//@brief Transition image layout to specified params
	void transitionImageLayout(
		uint32_t imageIndex,
		vk::ImageLayout oldLayout,
		vk::ImageLayout newLayout,
		vk::AccessFlags2 srcAccessMask,
		vk::AccessFlags2 dstAccessMask,
		vk::PipelineStageFlags2 srcStageMask,
		vk::PipelineStageFlags2 dstStageMask
	);

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
	//@brief Recreates swap chain
	void recreateSwapChain();
	//@brief Cleans swap chain
	void cleanSwapChain();
	//@brief Creates image views
	void createImageViews();
	//@brief Creates graphics pipeline
	void createGraphicsPipeline();
	//@brief Initializes vk::raii::CommandPool 
	void createCommandPools();
	//@brief Initializes vk::raii::CommandBuffer 
	void createCommandBuffers();
	//@brief Initializes rendering semaphores and fences
	void createSyncObjects();

	void createMeshes();

	//@brief Writes commands into vk::raii::CommandBuffer
	void recordCommandBuffer(uint32_t imageIndex);

	//@brief Encapsulates code into a vk::raii::ShaderModule object
	//@param code:	binary data from compiled .slang file (i.e. .spv file)
	[[nodiscard]] vk::raii::ShaderModule createShaderModule(const std::vector<char>& code) const;

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
	std::vector<const char*> getRequiredExtensions();

	//@brief Runs engine
	void run();
	//@brief Executes rendering logic called each frame
	void draw();
	//@brief Initializes data members
	void init();
	//@brief Core engine loop
	void loop();
	//@brief Cleans engine data members
	void clean();

	const vk::PhysicalDeviceMemoryProperties& getGPUMemoryProperties() const
	{ return m_pDMemoryProperties; }

	bool getFramebufferResized() const 
	{ return m_framebufferResized; }

	void setFramebufferResized(bool state) 
	{ m_framebufferResized = state; }

	//@brief Gets family index of Vulkan queue
	//@param  queueType:	enum value representing queue type
	//@return unsigned int
	unsigned int getQueueFamilyIndex(QType queueType)
	{ return m_familyIndices[queueType]; }

	//@brief Gets command pool for a given queueType
	//@param  queueType:	enum value representing queue type
	//@return vk::raii::CommandPool&
	vk::raii::CommandPool& getCommandPool(QType queueType)
	{ return m_commandPools[queueType]; }

	//@brief Gets queue for a given queueType
	//@param  queueType:	enum value representing queue type
	//@return vk::raii::Queue&
	vk::raii::Queue& getQueue(QType queueType)
	{ return m_queues[queueType]; }
};