#include "core/geometry/buffers.h"

void VertexBuffer::initBuffer(
    vk::raii::Device& device, 
    std::vector<Vertex> const* vertices)
{
    vk::DeviceSize bufferSize = sizeof((*vertices)[0]) * vertices->size();

    vk::raii::Buffer stagingBuffer({});
    vk::raii::DeviceMemory stagingBufferMemory({});
    Buffer::Create(
        device,
        bufferSize,
        vk::BufferUsageFlagBits::eTransferSrc,
        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
        stagingBuffer,
        stagingBufferMemory);
        
    void* data = stagingBufferMemory.mapMemory(0, bufferSize);
    memcpy(data, vertices->data(), (size_t)bufferSize);
    stagingBufferMemory.unmapMemory();
        
    Buffer::Create(device, bufferSize, vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer, vk::MemoryPropertyFlagBits::eDeviceLocal, m_buffer, m_bufferMemory);
    Buffer::Copy(device, stagingBuffer, m_buffer, bufferSize);
}
