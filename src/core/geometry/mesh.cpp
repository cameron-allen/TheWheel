#include "core/geometry/mesh.h"


void Mesh::init(vk::raii::Device& device) 
{
	m_vBuff.initBuffer(device);
}
