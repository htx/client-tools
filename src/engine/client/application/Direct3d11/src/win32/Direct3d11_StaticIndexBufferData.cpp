#include "FirstDirect3d11.h"
#include "Direct3d11_StaticIndexBufferData.h"

#include "Direct3d11.h"
#include "sharedFoundation/MemoryBlockManager.h"

// ======================================================================

MemoryBlockManager *  Direct3d11_StaticIndexBufferData::ms_memoryBlockManager;

// ======================================================================

void Direct3d11_StaticIndexBufferData::install()
{
	ms_memoryBlockManager = new MemoryBlockManager("Direct3d11_StaticIndexBufferData", true, sizeof(Direct3d11_StaticIndexBufferData), 0, 0, 0);
}

// ----------------------------------------------------------------------

void Direct3d11_StaticIndexBufferData::remove()
{
	delete ms_memoryBlockManager;
	ms_memoryBlockManager = nullptr;
}

// ----------------------------------------------------------------------

void *Direct3d11_StaticIndexBufferData::operator new(size_t size)
{
	UNREF(size);
	NOT_NULL(ms_memoryBlockManager);
	DEBUG_FATAL(size != sizeof (Direct3d11_StaticIndexBufferData), ("bad size"));
	DEBUG_FATAL(size != static_cast<size_t> (ms_memoryBlockManager->getElementSize()), ("installed with bad size"));
	return ms_memoryBlockManager->allocate();
}

// ----------------------------------------------------------------------

void Direct3d11_StaticIndexBufferData::operator delete(void *memory)
{
	NOT_NULL(ms_memoryBlockManager);
	ms_memoryBlockManager->free(memory);
}

// ======================================================================

Direct3d11_StaticIndexBufferData::Direct3d11_StaticIndexBufferData(const StaticIndexBuffer &indexBuffer)
:
	m_indexBuffer(indexBuffer),
	m_d3dIndexBuffer(nullptr)
{
	mIndexBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	mIndexBufferDesc.ByteWidth = sizeof(Index) * indexBuffer.getNumberOfIndices();
	mIndexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	mIndexBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	mIndexBufferDesc.MiscFlags = 0;
	mIndexBufferDesc.StructureByteStride = 0;

	m_indexBuffer.m_indexData = new Index[indexBuffer.getNumberOfIndices()];
	
	mIndexData.pSysMem = m_indexBuffer.m_indexData;
	mIndexData.SysMemPitch = 0;
	mIndexData.SysMemSlicePitch = 0;
		
	const HRESULT hr = Direct3d11::getDevice()->CreateBuffer(&mIndexBufferDesc, &mIndexData, &m_d3dIndexBuffer);

	FATAL(FAILED(hr), ("create index buffer fail"));
}

// ----------------------------------------------------------------------

Direct3d11_StaticIndexBufferData::~Direct3d11_StaticIndexBufferData()
{
	IGNORE_RETURN(m_d3dIndexBuffer->Release());
}

// ----------------------------------------------------------------------

Index *Direct3d11_StaticIndexBufferData::lock(bool readOnly)
{
	Direct3d11::getDeviceContext()->IASetIndexBuffer(m_d3dIndexBuffer, DXGI_FORMAT_R16_UINT, 0);
	Direct3d11::getDeviceContext()->Map(m_d3dIndexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mMappedData);

	return reinterpret_cast<Index *>(mMappedData.pData);
}

// ----------------------------------------------------------------------

void Direct3d11_StaticIndexBufferData::unlock()
{
	Direct3d11::getDeviceContext()->Unmap(m_d3dIndexBuffer, 0);
}

// ======================================================================
