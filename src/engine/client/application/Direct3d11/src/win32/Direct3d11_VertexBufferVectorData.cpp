#include "FirstDirect3d11.h"
#include "Direct3d11_VertexBufferVectorData.h"
#include "Direct3d11_VertexBufferDescriptorMap.h"
#include "Direct3d11_VertexDeclarationMap.h"

#include "clientGraphics/HardwareVertexBuffer.h"

#include <vector>

// ======================================================================

namespace Direct3d11_VertexBufferVectorDataNamespace
{
	const int MAX_VERTEX_BUFFERS = 2;
}
using namespace Direct3d11_VertexBufferVectorDataNamespace;

// ======================================================================

Direct3d11_VertexBufferVectorData::Direct3d11_VertexBufferVectorData(VertexBufferVector const & vertexBufferVector)
: VertexBufferVectorGraphicsData(),
	m_vertexDeclaration(0)
{
	DEBUG_FATAL(vertexBufferVector.m_vertexBufferList->size() > MAX_VERTEX_BUFFERS, ("too many vertex buffers in a vector"));

	int j = 0;
	VertexBufferFormat const * vertexBufferFormat[MAX_VERTEX_BUFFERS];
	VertexBufferVector::VertexBufferList::const_iterator iEnd = vertexBufferVector.m_vertexBufferList->end();
	for (VertexBufferVector::VertexBufferList::const_iterator i = vertexBufferVector.m_vertexBufferList->begin(); i != iEnd; ++i, ++j)
		vertexBufferFormat[j] = & (*i)->getFormat();

	m_vertexDeclaration = Direct3d11_VertexDeclarationMap::fetchVertexDeclaration(vertexBufferFormat, j);
}

// ----------------------------------------------------------------------

Direct3d11_VertexBufferVectorData::~Direct3d11_VertexBufferVectorData()
{
	m_vertexDeclaration->Release();
}

// ======================================================================
