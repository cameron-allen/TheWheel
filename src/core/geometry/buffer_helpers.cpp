#include "core/geometry/buffers.h"
#include "core/engine.h"

#define VMA_IMPLEMENTATION
#include <vma/vk_mem_alloc.h>

VmaAllocator allocator = VK_NULL_HANDLE;

VmaAllocator& Allocator::getAllocator()
{
	return allocator;
}

void Allocator::init(vk::raii::Instance& instance,
	vk::raii::PhysicalDevice& physicalDevice,
	vk::raii::Device& device) 
{
	VmaAllocatorCreateInfo info{};
	info.instance = *instance;               // VkInstance
	info.physicalDevice = *physicalDevice;   // VkPhysicalDevice
	info.device = *device;                   // VkDevice

	VkResult r = vmaCreateAllocator(&info, &allocator);
	if (r != VK_SUCCESS) {
		throw std::runtime_error("vmaCreateAllocator failed");
	}
}

void Allocator::clean() 
{
	if (allocator) {
		vmaDestroyAllocator(allocator);
		allocator = VK_NULL_HANDLE;
	}
}

VmaAllocation Buffer::Create(
	vk::raii::Device& device,
	vk::DeviceSize size, 
	VkBufferUsageFlags usage, 
	VkMemoryPropertyFlags properties,
	VkBuffer& buffer,
	unsigned int allocFlags)
{
	Core& c = Core::GetInstance();
	VkBufferCreateInfo bufferInfo{ .size = size, .usage = usage};

	if (c.getQueueFamilyIndex(QType::Graphics) == c.getQueueFamilyIndex(QType::Transfer))
	{
		bufferInfo.sharingMode = VkSharingMode::VK_SHARING_MODE_EXCLUSIVE;
	}
	else
	{
		uint32_t qFamilyIndices[2] = {
			c.getQueueFamilyIndex(QType::Graphics),
			c.getQueueFamilyIndex(QType::Transfer)
		};
		bufferInfo.sharingMode = VkSharingMode::VK_SHARING_MODE_CONCURRENT;
		bufferInfo.queueFamilyIndexCount = 2;
		bufferInfo.pQueueFamilyIndices = qFamilyIndices;
	}

	bufferInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	//buffer = vk::raii::Buffer(device, bufferInfo);

	VmaAllocationCreateInfo allocCreateInfo = {};
	allocCreateInfo.usage = VMA_MEMORY_USAGE_AUTO;
	allocCreateInfo.flags = allocFlags;

	VmaAllocation allocation;
	vmaCreateBuffer(allocator, &bufferInfo, &allocCreateInfo, &buffer, &allocation, nullptr);
	return allocation;
}

void Buffer::Copy(
	vk::raii::Device& device, 
	VkBuffer& srcBuffer, 
	VkBuffer& dstBuffer,
	vk::DeviceSize size,
	vk::DeviceSize dstOffset)
{
	Core& core = Core::GetInstance();
	vk::raii::CommandPool& cPool = core.getCommandPool(QType::Transfer);
	vk::raii::Queue& queue = core.getQueue(QType::Transfer);
	vk::CommandBufferAllocateInfo allocInfo{ .commandPool = cPool, .level = vk::CommandBufferLevel::ePrimary, .commandBufferCount = 1 };
	vk::raii::CommandBuffer commandCopyBuffer = std::move(device.allocateCommandBuffers(allocInfo).front());
	commandCopyBuffer.begin(vk::CommandBufferBeginInfo{ .flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit });
	commandCopyBuffer.copyBuffer(srcBuffer, dstBuffer, vk::BufferCopy(0, dstOffset, size));
	commandCopyBuffer.end();
	queue.submit(vk::SubmitInfo{.commandBufferCount = 1, .pCommandBuffers = &*commandCopyBuffer}, nullptr);
	
	// NOTE: might want to use vkWaitForFences instead to allow for scheduling multiple copies at once
	queue.waitIdle();
}

uint32_t Buffer::FindMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties)
{
	vk::PhysicalDeviceMemoryProperties memProperties = Core::GetInstance().getGPUMemoryProperties();
	for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
		if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
			return i;
		}
	}

	throw std::runtime_error("failed to find suitable memory type!");
}