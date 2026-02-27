#pragma once
#include <vector>
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <type_traits>
#include <vma/vk_mem_alloc.h>


namespace Allocator 
{
	void init(vk::raii::Instance& instance,
		vk::raii::PhysicalDevice& physicalDevice,
		vk::raii::Device& device);

	VmaAllocator& getAllocator();

	void clean();
};

struct Vertex 
{
	glm::vec4 pos;
	glm::vec4 color;

	Vertex(const glm::vec2& _p, const glm::vec3& _c) : pos(_p.x, _p.y, 0.0f, 1.0f), color(_c, 1.0f) {}
	Vertex(const glm::vec2& _p, const glm::vec4& _c) : pos(_p.x, _p.y, 0.0f, 1.0f), color(_c) {}
	Vertex(const glm::vec3& _p, const glm::vec3& _c) : pos(_p, 1.0f), color(_c, 1.0f) {}
	Vertex(const glm::vec3& _p, const glm::vec4& _c) : pos(_p, 1.0f), color(_c) {}

	static vk::VertexInputBindingDescription getBindingDesc() {
		return { 0, sizeof(Vertex), vk::VertexInputRate::eVertex };
	}

	static std::pair<vk::VertexInputAttributeDescription, vk::VertexInputAttributeDescription> getAttribDesc() {
		return {
			vk::VertexInputAttributeDescription(0, 0, vk::Format::eR32G32B32A32Sfloat, offsetof(Vertex, pos)),
			vk::VertexInputAttributeDescription(1, 0, vk::Format::eR32G32B32A32Sfloat, offsetof(Vertex, color))
		};
	}
};

struct CopyInfo
{
	vk::raii::CommandBuffer cmd;
	vk::raii::Fence fence;
};

class Buffer 
{
protected:
	VkBuffer m_buffer = VK_NULL_HANDLE;
	VmaAllocation m_allocation = VK_NULL_HANDLE;
	
public:
	//@brief Gets buffer
	const VkBuffer& getBuffer()
	{ return m_buffer; }

	//@brief Deallocates buffer CPU and GPU memory
	void destroy() 
	{
		if (m_buffer != VK_NULL_HANDLE) {
			vmaDestroyBuffer(Allocator::getAllocator(), m_buffer, m_allocation);
			m_buffer = VK_NULL_HANDLE;
			m_allocation = VK_NULL_HANDLE;
		}
	}


	// -----Helpers-----

	//@brief Creates a buffer from a set of specifications (i.e. size, usage, properties)
	//@return Mapped data if VMA_ALLOCATION_CREATE_MAPPED_BIT allocation flag is present
	static VmaAllocation Create(
		vk::raii::Device& device,
		vk::DeviceSize size,
		VkBufferUsageFlags usage,
		VkMemoryPropertyFlags properties,
		VkBuffer& buffer,
		unsigned int allocFlags = 0);

	//@brief Copies source buffer into destination buffer
	static void Copy(
		vk::raii::Device& device,
		VkBuffer& srcBuffer,
		VkBuffer& dstBuffer,
		vk::DeviceSize size,
		vk::DeviceSize dstOffset = 0);

	//@brief Copies source buffer into destination buffer and returns struct that contains the copy CommandBuffer and Fence
	static CopyInfo CopyAndReturn(
		vk::raii::Device& device,
		VkBuffer& srcBuffer,
		VkBuffer& dstBuffer,
		vk::DeviceSize size,
		vk::DeviceSize dstOffset = 0);

	//@brief Queries and finds buffer memory requirements
	static uint32_t FindMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties);
};

class IndexBuffer : public Buffer
{
private:
	size_t m_numIndices = 0;

public:
	IndexBuffer() {}

	//@brief Initializes index buffer
	void initBuffer(vk::raii::Device& device, std::vector<uint32_t> const* indices);

	size_t getIndicesSize() const
	{ return m_numIndices; }
};

class VertexBuffer : public Buffer
{
public:
	VertexBuffer() {}

	//@brief Initializes vertex buffer
	void initBuffer(vk::raii::Device& device, std::vector<Vertex> const* vertices);
};

template<typename T>
concept IndexDataTypes = std::is_same_v<T, uint16_t> || std::is_same_v<T, uint32_t>;

class VIBuffer : public Buffer 
{
private:
	vk::DeviceSize m_indexOffset = 0;
	uint32_t m_numIndices = 0;
	bool m_indicesAre16bits = true;

public:
	VIBuffer() {}

	//@brief Initializes vertex and index buffers into one buffer
	template<IndexDataTypes IndexDataType>
	void initBuffer(
		vk::raii::Device& device,
		std::vector<Vertex> const* vertices,
		std::vector<IndexDataType> const* indices);

	//@brief Binds vertex and index buffers
	void bind(vk::raii::CommandBuffer& cmd);

	//@brief Gets indices count
	uint32_t getIndicesCount() const
	{ return m_numIndices; }
};