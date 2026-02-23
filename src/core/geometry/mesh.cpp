#include "core/geometry/mesh.h"

void Mesh::draw(vk::raii::CommandBuffer& cmd)
{
	m_buffer.bind(cmd);
	cmd.drawIndexed(m_buffer.getIndicesCount(), 1, 0, 0, 0);
}