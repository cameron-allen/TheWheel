#include "core/geometry/buffers.h"
#include "core/engine.h"

void Buffer::Create(
	vk::raii::Device& device,
	vk::DeviceSize size, 
	vk::BufferUsageFlags usage, 
	vk::MemoryPropertyFlags properties,
	vk::raii::Buffer& buffer, 
	vk::raii::DeviceMemory& bufferMemory)
{
	Core& c = Core::GetInstance();
	vk::BufferCreateInfo bufferInfo{ .size = size, .usage = usage};

	if (c.getQueueFamilyIndex(QType::Graphics) == c.getQueueFamilyIndex(QType::Transfer))
	{
		bufferInfo.setSharingMode(vk::SharingMode::eExclusive);
	}
	else
	{
		uint32_t qFamilyIndices[2] = {
			c.getQueueFamilyIndex(QType::Graphics),
			c.getQueueFamilyIndex(QType::Transfer)
		};
		bufferInfo.setSharingMode(vk::SharingMode::eConcurrent);
		bufferInfo.setQueueFamilyIndexCount(2);
		bufferInfo.setPQueueFamilyIndices(qFamilyIndices);
	}
	
	buffer = vk::raii::Buffer(device, bufferInfo);
	vk::MemoryRequirements memRequirements = buffer.getMemoryRequirements();
	vk::MemoryAllocateInfo allocInfo{ .allocationSize = memRequirements.size, .memoryTypeIndex = FindMemoryType(memRequirements.memoryTypeBits, properties) };
	bufferMemory = vk::raii::DeviceMemory(device, allocInfo);
	buffer.bindMemory(*bufferMemory, 0);
}

void Buffer::Copy(
	vk::raii::Device& device, 
	vk::raii::Buffer& srcBuffer, 
	vk::raii::Buffer& dstBuffer,
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