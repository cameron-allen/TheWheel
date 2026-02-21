#include "core/geometry/buffers.h"
#include "core/engine.h"

void VertexBuffer::initBuffer(vk::raii::Device& device)
{
    vk::DeviceSize bufferSize = sizeof(mp_vertices[0]) * mp_vertices->size();

    vk::BufferCreateInfo stagingInfo{ .size = bufferSize, .usage = vk::BufferUsageFlagBits::eTransferSrc, .sharingMode = vk::SharingMode::eExclusive };
    vk::raii::Buffer stagingBuffer(device, stagingInfo);
    vk::MemoryRequirements memRequirementsStaging = stagingBuffer.getMemoryRequirements();
    vk::MemoryAllocateInfo memoryAllocateInfoStaging{ .allocationSize = memRequirementsStaging.size, 
        .memoryTypeIndex = Buffer::FindMemoryType(memRequirementsStaging.memoryTypeBits, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent) };
    vk::raii::DeviceMemory stagingBufferMemory(device, memoryAllocateInfoStaging);

    stagingBuffer.bindMemory(stagingBufferMemory, 0);
    void* dataStaging = stagingBufferMemory.mapMemory(0, stagingInfo.size);
    memcpy(dataStaging, mp_vertices->data(), stagingInfo.size);
    stagingBufferMemory.unmapMemory();

    vk::BufferCreateInfo bufferInfo{ .size = bufferSize, .usage = vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst, .sharingMode = vk::SharingMode::eExclusive };
    m_buffer = vk::raii::Buffer(device, bufferInfo);

    vk::MemoryRequirements memRequirements = m_buffer.getMemoryRequirements();
    vk::MemoryAllocateInfo memoryAllocateInfo{ .allocationSize = memRequirements.size, .memoryTypeIndex = Buffer::FindMemoryType(memRequirements.memoryTypeBits, vk::MemoryPropertyFlagBits::eDeviceLocal) };
    m_bufferMemory = vk::raii::DeviceMemory(device, memoryAllocateInfo);

    m_buffer.bindMemory(*m_bufferMemory, 0);
    Buffer::Copy(device, stagingBuffer, m_buffer, stagingInfo.size);
}
