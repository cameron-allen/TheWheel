#include "core/geometry/buffers.h"

void IndexBuffer::initBuffer(
    vk::raii::Device& device, 
    std::vector<uint32_t> const* indices)
{
    VmaAllocator& allocator = Allocator::GetAllocator();
    m_numIndices = indices->size();
    vk::DeviceSize iSize = sizeof((*indices)[0]) * m_numIndices;
    
    VkBuffer stagingBuffer({});
    
    VmaAllocation allocation = Buffer::Create(
        device,
        stagingBuffer,
        iSize,
        VkBufferUsageFlagBits::VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT);

    void* data = nullptr;
    vmaMapMemory(allocator, allocation, &data);
    memcpy(data, indices->data(), (size_t)iSize);
    vmaUnmapMemory(allocator, allocation);

    m_allocation = Buffer::Create(
        device,
        m_buffer,
        iSize, 
        VkBufferUsageFlagBits::VK_BUFFER_USAGE_TRANSFER_DST_BIT | 
        VkBufferUsageFlagBits::VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
        0,
        VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE);

    Buffer::Copy(device, stagingBuffer, m_buffer, iSize);
    vmaDestroyBuffer(allocator, stagingBuffer, allocation);
}