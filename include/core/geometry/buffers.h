#pragma once
#include <vector>
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

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

class Buffer 
{
protected:
	vk::raii::Buffer m_buffer = nullptr;
	vk::raii::DeviceMemory m_bufferMemory = nullptr;

public:

	//@brief Gets buffer
	const vk::Buffer& getBuffer()
	{ return *m_buffer; }


	// -----Helpers-----

	//@brief Creates a buffer from a set of specifications (i.e. size, usage, properties)
	static void Create(
		vk::raii::Device& device,
		vk::DeviceSize size,
		vk::BufferUsageFlags usage,
		vk::MemoryPropertyFlags properties,
		vk::raii::Buffer& buffer,
		vk::raii::DeviceMemory& bufferMemory);

	//@brief Copies source buffer into destination buffer
	static void Copy(
		vk::raii::Device& device,
		vk::raii::Buffer& srcBuffer,
		vk::raii::Buffer& dstBuffer,
		vk::DeviceSize size);

	//@brief Queries and finds buffer memory requirements
	static uint32_t FindMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties);
};

class IndexBuffer : public Buffer
{
	friend class Mesh;

private:
	// TODO: adjust mp_indices to be std::vector<uint16_t> if there are < 65535 unique vertices
	std::vector<uint32_t> const* mp_indices;

public:
	IndexBuffer() : mp_indices(nullptr) {}
	IndexBuffer(std::vector<uint32_t> const* _indices) : mp_indices(_indices) {}
	// Creates deep copy when initBuffer is called
	IndexBuffer(const IndexBuffer& _ib) : mp_indices(_ib.mp_indices) {}

	//@brief Initializes index buffer
	void initBuffer(vk::raii::Device& device);

	void setIndices(std::vector<uint32_t> const* indices)
	{ mp_indices = indices; }

	size_t getIndexSize()
	{ return mp_indices ? mp_indices->size() : 0; }
};

class VertexBuffer : public Buffer
{
private:
	std::vector<Vertex> const* mp_vertices;

public:
	VertexBuffer() : mp_vertices(nullptr) {}
	VertexBuffer(std::vector<Vertex> const* _vertices) : mp_vertices(_vertices) {}
	// Creates deep copy when initBuffer is called
	VertexBuffer(const VertexBuffer& _vb) : mp_vertices(_vb.mp_vertices) {}

	//@brief Initializes vertex buffer
	void initBuffer(vk::raii::Device& device);

	//@brief Sets vertex data for vertex buffer
	void setVertices(std::vector<Vertex> const* vertices)
	{ mp_vertices = vertices; }

	//@brief Gets buffer
	const vk::Buffer& getBuffer() 
	{ return *m_buffer; }
};