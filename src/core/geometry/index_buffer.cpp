#include "core/geometry/buffers.h"

void IndexBuffer::initBuffer(
    vk::raii::Device& device, 
    std::vector<uint32_t> const* indices)
{
    VmaAllocator& allocator = Allocator::getAllocator();
    m_numIndices = indices->size();
    vk::DeviceSize iSize = sizeof((*indices)[0]) * m_numIndices;
    
    VkBuffer stagingBuffer({});
    
    VmaAllocation allocation = Buffer::Create(
        device,
        iSize,
        VkBufferUsageFlagBits::VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 
        VkMemoryPropertyFlagBits::VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | 
        VkMemoryPropertyFlagBits::VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
        stagingBuffer,
        VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT);

    void* data = nullptr;
    vmaMapMemory(allocator, allocation, &data);
    memcpy(data, indices->data(), (size_t)iSize);
    vmaUnmapMemory(allocator, allocation);

    Buffer::Create(
        device, 
        iSize, 
        VkBufferUsageFlagBits::VK_BUFFER_USAGE_TRANSFER_DST_BIT | 
        VkBufferUsageFlagBits::VK_BUFFER_USAGE_INDEX_BUFFER_BIT, 
        VkMemoryPropertyFlagBits::VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 
        m_buffer,
        VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT);

    Buffer::Copy(device, stagingBuffer, m_buffer, iSize);
    vmaDestroyBuffer(allocator, stagingBuffer, allocation);
}