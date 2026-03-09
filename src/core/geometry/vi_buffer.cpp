#include "core/geometry/buffers.h"

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE

template<IndexDataTypes IndexDataType>
void VIBuffer::initBuffer(
    vk::raii::Device& device,
    std::vector<Vertex> const* vertices,
    std::vector<IndexDataType> const* indices) 
{
    if (std::is_same_v<IndexDataType, uint32_t>)
        m_indicesAre16bits = false;

    VmaAllocation allocs[2];
    VmaAllocator& allocator = Allocator::GetAllocator();
    m_numIndices = indices->size();
    VkBuffer sBuffer1({}), sBuffer2({});
    m_indexOffset = sizeof((*vertices)[0]) * vertices->size();
    vk::DeviceSize indexBuffSize = sizeof((*indices)[0]) * m_numIndices;

    m_allocation = Buffer::Create(
        device,
        m_buffer,
        m_indexOffset + indexBuffSize,
        VkBufferUsageFlagBits::VK_BUFFER_USAGE_TRANSFER_DST_BIT |
        VkBufferUsageFlagBits::VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
        VkBufferUsageFlagBits::VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
        0,
        VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE);

    //-----VERTEX-----
    allocs[0] = Buffer::Create(
        device,
        sBuffer1,
        m_indexOffset,
        VkBufferUsageFlagBits::VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT);

    void* data = nullptr;
    vmaMapMemory(allocator, allocs[0], &data);
    memcpy(data, vertices->data(), (size_t)m_indexOffset);
    vmaUnmapMemory(allocator, allocs[0]);
    
    CommandInfo stagingBufferCmdInfo = Buffer::CopyAndReturn(device, sBuffer1, m_buffer, m_indexOffset);

    //-----INDEX-----
    allocs[1] = Buffer::Create(
        device,
        sBuffer2,
        indexBuffSize,
        VkBufferUsageFlagBits::VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT);

    vmaMapMemory(allocator, allocs[1], &data);
    memcpy(data, indices->data(), (size_t)indexBuffSize);
    vmaUnmapMemory(allocator, allocs[1]);

    CommandInfo bufferCmdInfo = Buffer::CopyAndReturn(device, sBuffer2, m_buffer, indexBuffSize, m_indexOffset);

    vk::Fence fences[2] = {*stagingBufferCmdInfo.fence, *bufferCmdInfo.fence};
    vk::Result res = device.waitForFences(fences, VK_TRUE, UINT64_MAX);
    vmaDestroyBuffer(allocator, sBuffer1, allocs[0]);
    vmaDestroyBuffer(allocator, sBuffer2, allocs[1]);
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
