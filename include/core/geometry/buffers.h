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

class IndexBuffer {};

class VertexBuffer 
{
private:
	std::vector<Vertex> m_vertices;
	vk::raii::Buffer m_buffer = nullptr;
	vk::raii::DeviceMemory m_bufferMemory = nullptr;

	uint32_t findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties);

public:
	VertexBuffer(const std::vector<Vertex>& _vertices = {}) : m_vertices(_vertices) {}
	VertexBuffer(const VertexBuffer& _vb) : m_vertices(_vb.m_vertices) {}

	//@brief Initializes vertex buffer
	void initBuffer(vk::raii::Device& device);

	//@brief Sets vertex data for vertex buffer
	void setVertices(const std::vector<Vertex>& vertices)
	{ m_vertices = vertices; }

	//@brief Gets buffer
	const vk::Buffer& getBuffer() 
	{ return *m_buffer; }
};