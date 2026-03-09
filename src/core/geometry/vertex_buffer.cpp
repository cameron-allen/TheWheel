#include "core/geometry/buffers.h"

void VertexBuffer::initBuffer(
    vk::raii::Device& device, 
    std::vector<Vertex> const* vertices)
{
    VmaAllocator& allocator = Allocator::GetAllocator();
    vk::DeviceSize bufferSize = sizeof((*vertices)[0]) * vertices->size();
    VkBuffer stagingBuffer({});

    VmaAllocation allocation = Buffer::Create(
        device,
        stagingBuffer,
        bufferSize,
        VkBufferUsageFlagBits::VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT);
    
    void* data = nullptr;
    vmaMapMemory(allocator, allocation, &data);
    memcpy(data, vertices->data(), (size_t)bufferSize);
    vmaUnmapMemory(allocator, allocation);
        
    m_allocation = Buffer::Create(
        device, 
        m_buffer,
        bufferSize, 
        VkBufferUsageFlagBits::VK_BUFFER_USAGE_TRANSFER_DST_BIT |
        VkBufferUsageFlagBits::VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        0,
        VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE);

    Buffer::Copy(device, stagingBuffer, m_buffer, bufferSize);
    vmaDestroyBuffer(allocator, stagingBuffer, allocation);
}
