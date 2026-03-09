#include "core/geometry/buffers.h"
#include <ktx.h>
#include <ktxvulkan.h>

void ImageBuffer::initBuffer(vk::raii::Device& device, const char* ktx2ImagePath) 
{
    VmaAllocator& allocator = Allocator::GetAllocator();
    ktxTexture2* kTexture = nullptr;

    assert(ktx_error_code_e::KTX_SUCCESS == 
        ktxTexture2_CreateFromNamedFile(
            ktx2ImagePath,
            KTX_TEXTURE_CREATE_NO_FLAGS,
            &kTexture));

    ktx_size_t buffSize = ktxTexture_GetDataSize(ktxTexture(kTexture));
    std::vector<ktx_uint8_t> buff(buffSize);
    
    VkBuffer stagingBuffer({});
    VmaAllocation allocation = Buffer::Create(
        device,
        stagingBuffer,
        static_cast<vk::DeviceSize>(buffSize),
        VkBufferUsageFlagBits::VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT);

    assert(ktx_error_code_e::KTX_SUCCESS ==
        ktxTexture2_LoadImageData(
            kTexture,
            buff.data(),
            buffSize));

    void* data = nullptr;
    vmaMapMemory(allocator, allocation, &data);
    memcpy(data, buff.data(), (size_t)buffSize);
    vmaUnmapMemory(allocator, allocation);

    m_allocation = ImageBuffer::Create(
        device,
        m_image,
        kTexture,
        VkImageTiling::VK_IMAGE_TILING_OPTIMAL,
        VkImageUsageFlagBits::VK_IMAGE_USAGE_TRANSFER_DST_BIT |
        VkImageUsageFlagBits::VK_IMAGE_USAGE_SAMPLED_BIT,
        0,
        VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE);

    vk::BufferImageCopy region{ 
        .bufferOffset = 0, 
        .bufferRowLength = 0, 
        .bufferImageHeight = 0,
        .imageSubresource = { vk::ImageAspectFlagBits::eColor, 0, 0, 1 }, 
        .imageOffset = {0, 0, 0}, 
        .imageExtent = {kTexture->baseWidth, kTexture->baseHeight, 1} };
    
    ktxTexture2_Destroy(kTexture);

    ImageBuffer::Copy(
        device,
        stagingBuffer,
        m_image,
        region
    );

    vmaDestroyBuffer(allocator, stagingBuffer, allocation);
}

void ImageBuffer::destroy() 
{
    if (m_image != VK_NULL_HANDLE) {
        vmaDestroyImage(Allocator::GetAllocator(), m_image, m_allocation);
        m_image = VK_NULL_HANDLE;
        m_allocation = VK_NULL_HANDLE;
    }
}