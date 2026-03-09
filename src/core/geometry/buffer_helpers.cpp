#include "core/geometry/buffers.h"
#include "core/engine.h"
#include <ktx.h>
#include <ktxvulkan.h>

#define VMA_IMPLEMENTATION
#include <vma/vk_mem_alloc.h>

VmaAllocator allocator = VK_NULL_HANDLE;
uint32_t qFamilyIndices[2] = { 0, 0 };

uint32_t* pQueueFamilyIndices = nullptr;
uint32_t queueFamilyIndexCount = 0;
VkSharingMode sharingMode = VkSharingMode::VK_SHARING_MODE_EXCLUSIVE;


VmaAllocator& Allocator::GetAllocator()
{
	return allocator;
}

void Allocator::Init(vk::raii::Instance& instance,
	vk::raii::PhysicalDevice& physicalDevice,
	vk::raii::Device& device) 
{
	Core& c = Core::GetInstance();
	VmaAllocatorCreateInfo info{};
	info.instance = *instance;               // VkInstance
	info.physicalDevice = *physicalDevice;   // VkPhysicalDevice
	info.device = *device;                   // VkDevice

	VkResult r = vmaCreateAllocator(&info, &allocator);
	if (r != VK_SUCCESS) {
		throw std::runtime_error("vmaCreateAllocator failed");
	}

	if (c.getQueueFamilyIndex(QType::Graphics) != c.getQueueFamilyIndex(QType::Transfer))
	{
		qFamilyIndices[0] = c.getQueueFamilyIndex(QType::Graphics);
		qFamilyIndices[1] = c.getQueueFamilyIndex(QType::Transfer);
		sharingMode = VkSharingMode::VK_SHARING_MODE_CONCURRENT;
		queueFamilyIndexCount = 2;
		pQueueFamilyIndices = qFamilyIndices;
	}
}

void Allocator::Clean() 
{
	if (allocator) {
		vmaDestroyAllocator(allocator);
		allocator = VK_NULL_HANDLE;
	}
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

VmaAllocation Buffer::Create(
	vk::raii::Device& device,
	VkBuffer& buffer,
	vk::DeviceSize size,
	VkBufferUsageFlags buffUsage,
	VmaAllocationCreateFlags allocFlags,
	VmaMemoryUsage memUsage)
{
	VkBufferCreateInfo bufferInfo{
		.sType = VkStructureType::VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		.size = size, 
		.usage = buffUsage,
		.sharingMode = sharingMode,
		.queueFamilyIndexCount = queueFamilyIndexCount,
		.pQueueFamilyIndices = pQueueFamilyIndices
	};

	VmaAllocationCreateInfo allocCreateInfo = {};
	allocCreateInfo.usage = memUsage;
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
	vk::raii::CommandBuffer commandCopyBuffer = CommandBuffer::BeginSingleUse(device, QType::Transfer);
	commandCopyBuffer.copyBuffer(srcBuffer, dstBuffer, vk::BufferCopy(0, dstOffset, size));
	CommandBuffer::EndSingleUse(commandCopyBuffer, QType::Transfer);
	Core::GetInstance().getQueue(QType::Transfer).waitIdle();
}

CommandInfo Buffer::CopyAndReturn(
	vk::raii::Device& device,
	VkBuffer& srcBuffer,
	VkBuffer& dstBuffer,
	vk::DeviceSize size,
	vk::DeviceSize dstOffset) 
{
	/*vk::raii::CommandBuffer commandCopyBuffer = CommandBuffer::BeginSingleUse(device, QType::Transfer);
	commandCopyBuffer.copyBuffer(srcBuffer, dstBuffer, vk::BufferCopy(0, dstOffset, size));
	vk::raii::Fence fence(device, vk::FenceCreateInfo{});
	CommandBuffer::EndSingleUse(commandCopyBuffer, QType::Transfer, &fence);
	return CommandInfo{ .cmd = std::move(commandCopyBuffer), .fence = std::move(fence) };*/
	Core& core = Core::GetInstance();
	vk::raii::CommandPool& cPool = core.getCommandPool(QType::Transfer);
	vk::raii::Queue& queue = core.getQueue(QType::Transfer);
	vk::CommandBufferAllocateInfo allocInfo{ .commandPool = cPool, .level = vk::CommandBufferLevel::ePrimary, .commandBufferCount = 1 };
	vk::raii::CommandBuffer commandCopyBuffer = std::move(device.allocateCommandBuffers(allocInfo).front());
	vk::raii::Fence fence(device, vk::FenceCreateInfo{});
	commandCopyBuffer.begin(vk::CommandBufferBeginInfo{ .flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit });
	commandCopyBuffer.copyBuffer(srcBuffer, dstBuffer, vk::BufferCopy(0, dstOffset, size));
	commandCopyBuffer.end();

	queue.submit(vk::SubmitInfo{ .commandBufferCount = 1, .pCommandBuffers = &*commandCopyBuffer }, *fence);
	return CommandInfo{ .cmd = std::move(commandCopyBuffer), .fence = std::move(fence) };
}

VmaAllocation ImageBuffer::Create(
	vk::raii::Device& device,
	VkImage& image,
	ktxTexture2* texture,
	VkImageTiling tiling,
	VkImageUsageFlags usageFlags,
	VmaAllocationCreateFlags allocFlags,
	VmaMemoryUsage memUsage)
{
	VkImageCreateInfo imageInfo{
		.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
		.imageType = VkImageType::VK_IMAGE_TYPE_2D,
		.format = VkFormat::VK_FORMAT_R8G8B8A8_SRGB,
		.extent = { texture->baseWidth, texture->baseHeight, 1 },
		.mipLevels = texture->numLevels,
		.arrayLayers = texture->numLayers,
		.samples = VkSampleCountFlagBits::VK_SAMPLE_COUNT_1_BIT,
		.tiling = tiling,
		.usage = usageFlags,
		.sharingMode = sharingMode,
		.queueFamilyIndexCount = queueFamilyIndexCount,
		.pQueueFamilyIndices = pQueueFamilyIndices,
		.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED
	};

	VmaAllocationCreateInfo allocCreateInfo = {};
	allocCreateInfo.usage = memUsage;
	allocCreateInfo.flags = allocFlags;

	VmaAllocation allocation;
	vmaCreateImage(allocator, &imageInfo, &allocCreateInfo, &image, &allocation, nullptr);
	return allocation;
}

void ImageBuffer::Copy(
	vk::raii::Device& device,
	VkBuffer& srcBuffer,
	VkImage& dstImage,
	vk::BufferImageCopy biCopy)
{
	vk::raii::CommandBuffer commandCopyBuffer = CommandBuffer::BeginSingleUse(device, QType::Transfer);
	
	TransitionLayout(
		device, 
		dstImage, 
		VkImageLayout::VK_IMAGE_LAYOUT_UNDEFINED, 
		VkImageLayout::VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
	
	commandCopyBuffer.copyBufferToImage(
		static_cast<vk::Buffer>(srcBuffer),
		static_cast<vk::Image>(dstImage),
		vk::ImageLayout::eTransferDstOptimal,
		biCopy
	);

	CommandBuffer::EndSingleUse(commandCopyBuffer, QType::Transfer);
	Core::GetInstance().getQueue(QType::Transfer).waitIdle();
}

void ImageBuffer::TransitionLayout(
	vk::raii::Device& device,
	const VkImage& image,
	VkImageLayout oldLayout,
	VkImageLayout newLayout) 
{
	vk::raii::CommandBuffer commandBuffer = CommandBuffer::BeginSingleUse(device, QType::Graphics);

	VkImageMemoryBarrier mb{
		.oldLayout = oldLayout,
		.newLayout = newLayout,
		.image = image,
		.subresourceRange = {  VkImageAspectFlagBits::VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 }
	};

	CommandBuffer::EndSingleUse(commandBuffer, QType::Graphics);
	Core::GetInstance().getQueue(QType::Graphics).waitIdle();
}