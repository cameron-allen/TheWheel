#pragma once
#include "core/geometry/buffers.h"

// vertices, edges, faces
class Mesh 
{
private:
	VertexBuffer m_vBuff;
	IndexBuffer m_iBuff;
	
public:
	Mesh() {}

	Mesh(const std::vector<Vertex>& _vertices)
	{ setVBufferVerts(_vertices); }

	//@brief Sets vertex data for the vertex buffer
	void setVBufferVerts(const std::vector<Vertex>& _vertices) 
	{ m_vBuff.setVertices(_vertices); }

	//@brief Initializes mesh
	void init(vk::raii::Device& device);

	//@brief Gets vertex buffer
	const vk::Buffer& getVertexBuffer() 
	{ return m_vBuff.getBuffer(); }
};