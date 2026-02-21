#include "core/geometry/buffers.h"
#include "core/engine.h"

void IndexBuffer::initBuffer(vk::raii::Device& device)
{
    vk::DeviceSize bufferSize{ sizeof(mp_indices[0]) * mp_indices->size() };

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
    memcpy(data, mp_indices->data(), (size_t)bufferSize);
    stagingBufferMemory.unmapMemory();

    Buffer::Create(device, bufferSize, vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer, vk::MemoryPropertyFlagBits::eDeviceLocal, m_buffer, m_bufferMemory);

    Buffer::Copy(device, stagingBuffer, m_buffer, bufferSize);
}