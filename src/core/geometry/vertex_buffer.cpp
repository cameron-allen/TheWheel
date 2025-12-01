#include "core/geometry/buffers.h"
#include "core/engine.h"

uint32_t VertexBuffer::findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties) 
{
	vk::PhysicalDeviceMemoryProperties memProperties = Core::GetInstance().getGPUMemoryProperties();
	for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
		if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
			return i;
		}
	}

	throw std::runtime_error("failed to find suitable memory type!");
}

void VertexBuffer::initBuffer(vk::raii::Device& device)
{
	Core& c = Core::GetInstance();
	vk::BufferCreateInfo bufferInfo{
		.size = sizeof(m_vertices[0]) * m_vertices.size(),
		.usage = vk::BufferUsageFlagBits::eVertexBuffer
	};
	
	if (c.getQueueFamilyIndex(QType::Graphics) == c.getQueueFamilyIndex(QType::Transfer))
	{
		bufferInfo.setSharingMode(vk::SharingMode::eExclusive);
		m_buffer = vk::raii::Buffer(device, bufferInfo);
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
		m_buffer = vk::raii::Buffer(device, bufferInfo);
	}

	vk::MemoryRequirements memRequirements = m_buffer.getMemoryRequirements();

	vk::MemoryAllocateInfo memoryAllocateInfo{
		.allocationSize = memRequirements.size,
		.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits,
			vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent)
	};

	m_bufferMemory = vk::raii::DeviceMemory(device, memoryAllocateInfo);
	m_buffer.bindMemory(*m_bufferMemory, 0);

	void* data = m_bufferMemory.mapMemory(0, bufferInfo.size);
	memcpy(data, m_vertices.data(), bufferInfo.size);
	m_bufferMemory.unmapMemory();
}
