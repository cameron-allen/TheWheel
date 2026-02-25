#include "core/geometry/buffers.h"
#include <vma/vk_mem_alloc.h>

template<IndexDataTypes IndexDataType>
void VIBuffer::initBuffer(
    vk::raii::Device& device,
    std::vector<Vertex> const* vertices,
    std::vector<IndexDataType> const* indices) 
{
    if (std::is_same_v<IndexDataType, uint32_t>)
        m_indicesAre16bits = false;

    VmaAllocator& allocator = Allocator::getAllocator();
    m_numIndices = indices->size();
    VkBuffer stagingBuffer({});
    m_indexOffset = sizeof((*vertices)[0]) * vertices->size();
    vk::DeviceSize indexBuffSize = sizeof((*indices)[0]) * m_numIndices;

    m_allocation = Buffer::Create(
        device,
        m_indexOffset + indexBuffSize,
        VkBufferUsageFlagBits::VK_BUFFER_USAGE_TRANSFER_DST_BIT |
        VkBufferUsageFlagBits::VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
        VkBufferUsageFlagBits::VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
        VkMemoryPropertyFlagBits::VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        m_buffer,
        VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT);

    //-----VERTEX-----
    VmaAllocation allocation = Buffer::Create(
        device,
        m_indexOffset,
        VkBufferUsageFlagBits::VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VkMemoryPropertyFlagBits::VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
        VkMemoryPropertyFlagBits::VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        stagingBuffer,
        VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT);

    void* data = nullptr;
    vmaMapMemory(allocator, allocation, &data);
    memcpy(data, vertices->data(), (size_t)m_indexOffset);
    vmaUnmapMemory(allocator, allocation);
    
    Buffer::Copy(device, stagingBuffer, m_buffer, m_indexOffset);
    vmaDestroyBuffer(allocator, stagingBuffer, allocation);

    //-----INDEX-----
    allocation = Buffer::Create(
        device,
        indexBuffSize,
        VkBufferUsageFlagBits::VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VkMemoryPropertyFlagBits::VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
        VkMemoryPropertyFlagBits::VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        stagingBuffer,
        VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT);

    vmaMapMemory(allocator, allocation, &data);
    memcpy(data, indices->data(), (size_t)indexBuffSize);
    vmaUnmapMemory(allocator, allocation);

    Buffer::Copy(device, stagingBuffer, m_buffer, indexBuffSize, m_indexOffset);
    vmaDestroyBuffer(allocator, stagingBuffer, allocation);
}

template void VIBuffer::initBuffer(vk::raii::Device&,
    std::vector<Vertex> const*,
    std::vector<uint16_t> const*);

template void VIBuffer::initBuffer(vk::raii::Device&,
    std::vector<Vertex> const*,
    std::vector<uint32_t> const*);

void VIBuffer::bind(vk::raii::CommandBuffer& cmd)
{
    cmd.bindVertexBuffers(0, { m_buffer }, { 0 });
    cmd.bindIndexBuffer(
        m_buffer, 
        m_indexOffset, 
        m_indicesAre16bits ? vk::IndexType::eUint16 : vk::IndexType::eUint32);
}
