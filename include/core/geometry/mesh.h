#pragma once
#include "core/geometry/buffers.h"

class Mesh
{
private:
	VIBuffer m_buffer;

public:
	//@brief Initializes mesh
	template<IndexDataTypes IndexDataType>
	void init(vk::raii::Device& device,
		std::vector<Vertex> const* vertices,
		std::vector<IndexDataType> const* indices) 
	{ m_buffer.initBuffer(device, vertices, indices); }

	//brief Binds vertex and index buffers
	void bind(vk::raii::CommandBuffer& cmd);

	//@brief Draws
	void draw(vk::raii::CommandBuffer& cmd);

	void destroy()
	{ m_buffer.destroy(); }
};