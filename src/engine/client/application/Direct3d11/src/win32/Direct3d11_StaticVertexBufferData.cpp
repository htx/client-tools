#include "FirstDirect3d11.h"
#include "Direct3d11_StaticVertexBufferData.h"

#include "Direct3d11.h"
#include "Direct3d11_VertexDeclarationMap.h"

#include "sharedFoundation/MemoryBlockManager.h"

// ======================================================================

MemoryBlockManager * Direct3d11_StaticVertexBufferData::ms_memoryBlockManager;

// ======================================================================

void Direct3d11_StaticVertexBufferData::install()
{
	ms_memoryBlockManager  = new MemoryBlockManager("Direct3d11_StaticVertexBufferData", true, sizeof(Direct3d11_StaticVertexBufferData), 0, 0, 0);
}

// ----------------------------------------------------------------------

void Direct3d11_StaticVertexBufferData::remove()
{
	delete ms_memoryBlockManager;
	ms_memoryBlockManager = nullptr;
}

// ----------------------------------------------------------------------

void *Direct3d11_StaticVertexBufferData::operator new(size_t size)
{
	UNREF(size);
	NOT_NULL(ms_memoryBlockManager);
	DEBUG_FATAL(size != sizeof(Direct3d11_StaticVertexBufferData), ("wrong new called"));
	DEBUG_FATAL(size != static_cast<size_t> (ms_memoryBlockManager->getElementSize()), ("installed with bad size"));
	return ms_memoryBlockManager->allocate();
}

// ----------------------------------------------------------------------

void Direct3d11_StaticVertexBufferData::operator delete(void *memory)
{
	NOT_NULL(ms_memoryBlockManager);
	ms_memoryBlockManager->free(memory);
}

// ======================================================================

Direct3d11_StaticVertexBufferData::Direct3d11_StaticVertexBufferData(const StaticVertexBuffer &vertexBuffer)
:
	m_vertexBuffer(vertexBuffer),
	m_descriptor(Direct3d11_VertexBufferDescriptorMap::getDescriptor(vertexBuffer.getFormat())),
	m_d3dVertexBuffer(nullptr),
	m_vertexDeclaration(Direct3d11_VertexDeclarationMap::fetchVertexDeclaration(vertexBuffer.getFormat()))
{
	mVertexBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	mVertexBufferDesc.ByteWidth = m_descriptor.vertexSize * m_vertexBuffer.getNumberOfVertices();
	mVertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	mVertexBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	mVertexBufferDesc.MiscFlags = 0;
	mVertexBufferDesc.StructureByteStride = 0;
	
	mVertexData.pSysMem = m_vertexBuffer.m_data;
	mVertexData.SysMemPitch = 0;
	mVertexData.SysMemSlicePitch = 0; 

	HRESULT hr = Direct3d11::getDevice()->CreateBuffer(&mVertexBufferDesc, nullptr, &m_d3dVertexBuffer);

	FATAL(FAILED(hr), ("create staticvertexbuffer fail."));
}

// ----------------------------------------------------------------------

Direct3d11_StaticVertexBufferData::~Direct3d11_StaticVertexBufferData()
{
	IGNORE_RETURN(m_vertexDeclaration->Release());
	IGNORE_RETURN(m_d3dVertexBuffer->Release());
}

// ----------------------------------------------------------------------

const VertexBufferDescriptor  &Direct3d11_StaticVertexBufferData::getDescriptor() const
{
	return m_descriptor;
}

// ----------------------------------------------------------------------

void *Direct3d11_StaticVertexBufferData::lock(bool readOnly)
{
	NOT_NULL(m_d3dVertexBuffer);

	Direct3d11::getDeviceContext()->Map(m_d3dVertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mMappedData);

	return mMappedData.pData;
}

// ----------------------------------------------------------------------

void Direct3d11_StaticVertexBufferData::unlock()
{
	Direct3d11::getDeviceContext()->Unmap(m_d3dVertexBuffer, 0);
}

// ----------------------------------------------------------------------

intptr_t Direct3d11_StaticVertexBufferData::getSortKey()
{
	return reinterpret_cast<intptr_t>(m_d3dVertexBuffer);
}

// ======================================================================
