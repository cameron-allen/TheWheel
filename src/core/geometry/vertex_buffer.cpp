#include "core/geometry/buffers.h"

void VertexBuffer::initBuffer(
    vk::raii::Device& device, 
    std::vector<Vertex> const* vertices)
{
    VmaAllocator& allocator = Allocator::getAllocator();
    vk::DeviceSize bufferSize = sizeof((*vertices)[0]) * vertices->size();
    VkBuffer stagingBuffer({});

    VmaAllocation allocation = Buffer::Create(
        device,
        bufferSize,
        VkBufferUsageFlagBits::VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VkMemoryPropertyFlagBits::VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
        VkMemoryPropertyFlagBits::VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        stagingBuffer,
        VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT);
    
    void* data = nullptr;
    vmaMapMemory(allocator, allocation, &data);
    memcpy(data, vertices->data(), (size_t)bufferSize);
    vmaUnmapMemory(allocator, allocation);
        
    Buffer::Create(
        device, 
        bufferSize, 
        VkBufferUsageFlagBits::VK_BUFFER_USAGE_TRANSFER_DST_BIT |
        VkBufferUsageFlagBits::VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        VkMemoryPropertyFlagBits::VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        m_buffer,
        VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT);

    Buffer::Copy(device, stagingBuffer, m_buffer, bufferSize);
    vmaDestroyBuffer(allocator, stagingBuffer, allocation);
}
