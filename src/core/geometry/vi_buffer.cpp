#include "core/geometry/buffers.h"

template<IndexDataTypes IndexDataType>
void VIBuffer::initBuffer(
    vk::raii::Device& device,
    std::vector<Vertex> const* vertices,
    std::vector<IndexDataType> const* indices) 
{
    if (std::is_same_v<IndexDataType, uint32_t>)
        m_indicesAre16bytes = false;

    m_numIndices = indices->size();
    vk::DeviceSize iElementSize = sizeof((*indices)[0]);
    vk::DeviceSize typeSize = sizeof(IndexDataType);
    vk::raii::Buffer stagingBuffer({});
    vk::raii::DeviceMemory stagingBufferMemory({});
    m_indexOffset = sizeof((*vertices)[0]) * vertices->size();
    vk::DeviceSize indexBuffSize = sizeof((*indices)[0]) * m_numIndices;

    Buffer::Create(
        device,
        m_indexOffset + indexBuffSize,
        vk::BufferUsageFlagBits::eTransferDst |
        vk::BufferUsageFlagBits::eVertexBuffer |
        vk::BufferUsageFlagBits::eIndexBuffer,
        vk::MemoryPropertyFlagBits::eDeviceLocal,
        m_buffer,
        m_bufferMemory);

    //-----VERTEX-----
    Buffer::Create(
        device,
        m_indexOffset,
        vk::BufferUsageFlagBits::eTransferSrc,
        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
        stagingBuffer,
        stagingBufferMemory);

    void* data = stagingBufferMemory.mapMemory(0, m_indexOffset);
    memcpy(data, vertices->data(), (size_t)m_indexOffset);
    stagingBufferMemory.unmapMemory();

    Buffer::Copy(device, stagingBuffer, m_buffer, m_indexOffset);

    //-----INDEX-----
    Buffer::Create(
        device,
        indexBuffSize,
        vk::BufferUsageFlagBits::eTransferSrc,
        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
        stagingBuffer,
        stagingBufferMemory);

    data = stagingBufferMemory.mapMemory(0, indexBuffSize);
    memcpy(data, indices->data(), (size_t)indexBuffSize);
    stagingBufferMemory.unmapMemory();

    Buffer::Copy(device, stagingBuffer, m_buffer, indexBuffSize, m_indexOffset);
}

template void VIBuffer::initBuffer(vk::raii::Device&,
    std::vector<Vertex> const*,
    std::vector<uint16_t> const*);

template void VIBuffer::initBuffer(vk::raii::Device&,
    std::vector<Vertex> const*,
    std::vector<uint32_t> const*);

void VIBuffer::bind(vk::raii::CommandBuffer& cmd)
{
    cmd.bindVertexBuffers(0, { *m_buffer }, { 0 });
    cmd.bindIndexBuffer(
        *m_buffer, 
        m_indexOffset, 
        m_indicesAre16bytes ? vk::IndexType::eUint16 : vk::IndexType::eUint32);
}
