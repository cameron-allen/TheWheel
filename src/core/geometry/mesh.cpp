#include "core/geometry/mesh.h"
#include "core/engine.h"

void Mesh::init(vk::raii::Device& device)
{
	// TODO: change copyBuffer function to not use waitIdle and wait for fences instead 
	// (this should be done after this init method is called)
	m_vBuff.initBuffer(device);
	m_iBuff.initBuffer(device);
}
