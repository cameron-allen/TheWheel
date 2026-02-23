#include "core/geometry/buffers.h"

void IndexBuffer::initBuffer(
    vk::raii::Device& device, 
    std::vector<uint32_t> const* indices)
{
    m_numIndices = indices->size();
    vk::DeviceSize iSize = sizeof((*indices)[0]) * m_numIndices;

    vk::raii::Buffer stagingBuffer({});
    vk::raii::DeviceMemory stagingBufferMemory({});
    Buffer::Create(
        device,
        iSize,
        vk::BufferUsageFlagBits::eTransferSrc, 
        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, 
        stagingBuffer, 
        stagingBufferMemory);

    void* data = stagingBufferMemory.mapMemory(0, iSize);
    memcpy(data, indices->data(), (size_t)iSize);
    stagingBufferMemory.unmapMemory();

    Buffer::Create(device, iSize, vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer, vk::MemoryPropertyFlagBits::eDeviceLocal, m_buffer, m_bufferMemory);

    Buffer::Copy(device, stagingBuffer, m_buffer, iSize);
}