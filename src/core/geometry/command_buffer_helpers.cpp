#include "core/geometry/buffers.h"
#include "core/engine.h"

vk::raii::CommandBuffer CommandBuffer::BeginSingleUse(vk::raii::Device& device, QType qType)
{
    vk::CommandBufferAllocateInfo allocInfo{ 
        .commandPool = Core::GetInstance().getCommandPool(qType),
        .level = vk::CommandBufferLevel::ePrimary, 
        .commandBufferCount = 1};
    vk::raii::CommandBuffer commandBuffer = std::move(device.allocateCommandBuffers(allocInfo).front());

    vk::CommandBufferBeginInfo beginInfo{ .flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit };
    commandBuffer.begin(beginInfo);
    return commandBuffer;
}

void CommandBuffer::EndSingleUse(
    vk::raii::CommandBuffer& commandBuffer,
    QType qType,
    vk::raii::Fence* pFence)
{
    commandBuffer.end();
    vk::SubmitInfo submitInfo{ .commandBufferCount = 1, .pCommandBuffers = &*commandBuffer };
    Core::GetInstance().getQueue(qType).submit(submitInfo, pFence ? **pFence : nullptr);
}