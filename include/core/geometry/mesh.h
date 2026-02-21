#pragma once
#include "core/geometry/buffers.h"

// vertices, edges, faces
class Mesh 
{
private:
	VertexBuffer m_vBuff;
	IndexBuffer m_iBuff;
	
public:
	Mesh() : m_vBuff(), m_iBuff() {}

	Mesh(std::vector<Vertex> const * const _vertices)
	{ setVBufferVerts(_vertices); }

	Mesh(std::vector<Vertex> const* _vertices, std::vector<unsigned int> const* _indices)
		: m_vBuff(_vertices), m_iBuff(_indices) {}

	Mesh(const Mesh& _mesh) : m_vBuff(_mesh.m_vBuff), m_iBuff(_mesh.m_iBuff) {}

	//@brief Sets vertex data for the vertex buffer
	void setVBufferVerts(std::vector<Vertex> const * const _vertices) 
	{ m_vBuff.setVertices(_vertices); }

	//@brief Initializes mesh
	void init(vk::raii::Device& device);

	//@brief Gets vertex buffer
	VertexBuffer& getVertexBuffer() 
	{ return m_vBuff; }

	//@brief Gets index buffer
	IndexBuffer& getIndexBuffer()
	{ return m_iBuff; }
};