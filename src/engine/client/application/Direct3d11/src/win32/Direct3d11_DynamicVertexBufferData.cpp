#include "FirstDirect3d11.h"
#include "Direct3d11_DynamicVertexBufferData.h"

#include "ConfigDirect3d11.h"
#include "Direct3d11.h"
#include "Direct3d11_VertexBufferDescriptorMap.h"
#include "Direct3d11_VertexDeclarationMap.h"

#include "clientGraphics/VertexBuffer.h"
#include "sharedFoundation/MemoryBlockManager.h"

// ======================================================================

bool                     Direct3d11_DynamicVertexBufferData::ms_newFrame;
int                      Direct3d11_DynamicVertexBufferData::ms_size;
int                      Direct3d11_DynamicVertexBufferData::ms_used;
ID3D11Buffer *			 Direct3d11_DynamicVertexBufferData::ms_d3dVertexBuffer;
MemoryBlockManager *     Direct3d11_DynamicVertexBufferData::ms_memoryBlockManager;
int                      Direct3d11_DynamicVertexBufferData::ms_locksSinceBeginFrame;
int                      Direct3d11_DynamicVertexBufferData::ms_discardsSinceBeginFrame;
int                      Direct3d11_DynamicVertexBufferData::ms_locksSinceResourceCreation;
int                      Direct3d11_DynamicVertexBufferData::ms_discardsSinceResourceCreation;
int                      Direct3d11_DynamicVertexBufferData::ms_locksEver;
int                      Direct3d11_DynamicVertexBufferData::ms_discardsEver;

// ======================================================================

void Direct3d11_DynamicVertexBufferData::install()
{
	DEBUG_FATAL(ms_memoryBlockManager, ("Already installed"));
	ms_memoryBlockManager  = new MemoryBlockManager("Direct3d11_DynamicVertexBufferData", true, sizeof(Direct3d11_DynamicVertexBufferData), 0, 0, 0);

	ms_size = 2048*1024;//ConfigDirect3d11::getDynamicVertexBufferSize() * 1024;
	ms_used = 0;
	
	restoreDevice();
}

// ----------------------------------------------------------------------

void Direct3d11_DynamicVertexBufferData::remove()
{
	lostDevice();

	delete ms_memoryBlockManager;
	ms_memoryBlockManager = nullptr;
}

// ----------------------------------------------------------------------

void Direct3d11_DynamicVertexBufferData::beginFrame()
{
	ms_newFrame = ConfigDirect3d11::getDiscardDynamicBuffersAtBeginningOfFrame();
	ms_locksSinceBeginFrame = 0;
	ms_discardsSinceBeginFrame = 0;
}

// ----------------------------------------------------------------------

void *Direct3d11_DynamicVertexBufferData::operator new(size_t size)
{
	UNREF(size);
	NOT_NULL(ms_memoryBlockManager);
	DEBUG_FATAL(size != sizeof(Direct3d11_DynamicVertexBufferData), ("wrong new called"));
	DEBUG_FATAL(size != static_cast<size_t> (ms_memoryBlockManager->getElementSize()), ("installed with bad size"));

	return ms_memoryBlockManager->allocate();
}

// ----------------------------------------------------------------------

void Direct3d11_DynamicVertexBufferData::operator delete(void *memory)
{
	NOT_NULL(ms_memoryBlockManager);
	ms_memoryBlockManager->free(memory);
}

// ----------------------------------------------------------------------

void Direct3d11_DynamicVertexBufferData::lostDevice()
{
	if (ms_d3dVertexBuffer)
	{
		IGNORE_RETURN(ms_d3dVertexBuffer->Release());
		ms_d3dVertexBuffer = nullptr;
		ms_locksSinceResourceCreation = 0;
		ms_discardsSinceResourceCreation = 0;
	}
}

// ----------------------------------------------------------------------

void Direct3d11_DynamicVertexBufferData::restoreDevice()
{
	ms_newFrame = ConfigDirect3d11::getDiscardDynamicBuffersAtBeginningOfFrame();

	D3D11_BUFFER_DESC mVertexBufferDesc;
	mVertexBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	mVertexBufferDesc.ByteWidth = ms_size;
	mVertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	mVertexBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	mVertexBufferDesc.MiscFlags = 0;
	mVertexBufferDesc.StructureByteStride = 0;

	Direct3d11::getDevice()->CreateBuffer(&mVertexBufferDesc, nullptr, &ms_d3dVertexBuffer);
	
	ms_used = 0;
	ms_locksSinceResourceCreation = 0;
	ms_discardsSinceResourceCreation = 0;
}

// ======================================================================

Direct3d11_DynamicVertexBufferData::Direct3d11_DynamicVertexBufferData(const VertexBuffer &vertexBuffer)
:
	m_vertexBufferDescriptor(Direct3d11_VertexBufferDescriptorMap::getDescriptor(vertexBuffer.getFormat())),
	m_numberOfVertices(0),
	m_offset(0),
	m_vertexDeclaration(Direct3d11_VertexDeclarationMap::fetchVertexDeclaration(vertexBuffer.getFormat()))
{
}

// ----------------------------------------------------------------------

Direct3d11_DynamicVertexBufferData::~Direct3d11_DynamicVertexBufferData()
{
	if(m_vertexDeclaration)
		m_vertexDeclaration->Release();
}

// ----------------------------------------------------------------------

const VertexBufferDescriptor &Direct3d11_DynamicVertexBufferData::getDescriptor() const
{
	return m_vertexBufferDescriptor;
}

// ----------------------------------------------------------------------

void Direct3d11_DynamicVertexBufferData::roundUpUsed() const
{
	const int vertexSize = getVertexSize();
	ms_used = ((ms_used + vertexSize - 1) / vertexSize) * vertexSize;
}

// ----------------------------------------------------------------------

void *Direct3d11_DynamicVertexBufferData::lock(int numberOfVertices, bool forceDiscard)
{
	roundUpUsed();

	const int vertexSize = getVertexSize();
	const int length = numberOfVertices * vertexSize;

	++ms_locksSinceBeginFrame;
	++ms_locksSinceResourceCreation;
	++ms_locksEver;

	// check for space
	//DWORD lockFlag = D3DLOCK_NOOVERWRITE;
	int discard = 0;
	
	if (ms_newFrame || forceDiscard || ms_used + length > ms_size)
	{
		ms_newFrame = false;
		++ms_discardsSinceBeginFrame;
		++ms_discardsSinceResourceCreation;
		++ms_discardsEver;
		discard = 1;

		// make sure this VB will fit even when the dynamic vb is empty
		DEBUG_FATAL(length > ms_size, ("Too many vertices %d/%d", numberOfVertices, ms_size / getVertexSize()));

		// lock with discard contents
		//lockFlag = D3DLOCK_DISCARD;
		ms_used = 0;
	}

	HRESULT hr = Direct3d11::getDeviceContext()->Map(ms_d3dVertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mMappedData);
	
	DEBUG_FATAL(FAILED(hr),("map fail"));

	// use up vertices from this dynamic vb
	m_numberOfVertices = numberOfVertices;
	m_offset = ms_used / vertexSize;

	return &mMappedData;
}

// ----------------------------------------------------------------------

void Direct3d11_DynamicVertexBufferData::unlock()
{
	Direct3d11_DynamicVertexBufferData::unlock(m_numberOfVertices);
}

// ----------------------------------------------------------------------

void Direct3d11_DynamicVertexBufferData::unlock(int numberOfVertices)
{
	Direct3d11::getDeviceContext()->Unmap(ms_d3dVertexBuffer, 0);

	m_numberOfVertices = numberOfVertices;
	const int vertexSize = getVertexSize();
	const int length = numberOfVertices * vertexSize;

	ms_used += length;
}

// ------------------------------------------------------------------	----

int Direct3d11_DynamicVertexBufferData::getNumberOfLockableDynamicVertices(bool withDiscard)
{
	roundUpUsed();

	// with a discard, they can get all the vertices
	// without discard, they can only get access to the remaining ones
	return (ms_size - (withDiscard ? 0 : ms_used)) / getVertexSize();
}

// ----------------------------------------------------------------------

intptr_t Direct3d11_DynamicVertexBufferData::getSortKey()
{
	return reinterpret_cast<intptr_t>(ms_d3dVertexBuffer);
}

// ----------------------------------------------------------------------

int Direct3d11_DynamicVertexBufferData::getVertexSize() const
{
	return m_vertexBufferDescriptor.vertexSize;
}

// ----------------------------------------------------------------------

ID3D11Buffer *Direct3d11_DynamicVertexBufferData::getVertexBuffer() const
{
	return ms_d3dVertexBuffer;
}

// ======================================================================

