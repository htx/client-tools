#ifndef INCLUDED_Direct3d11_DynamicVertexBufferData_H
#define INCLUDED_Direct3d11_DynamicVertexBufferData_H

// ======================================================================

struct ID3D11Buffer;
struct ID3D11InputLayout;
class  MemoryBlockManager;

#include "clientGraphics/DynamicVertexBuffer.h"
#include "Direct3d11_VertexBufferDescriptorMap.h"
#include <cstdint>
#include <d3d11.h>

// ======================================================================

class Direct3d11_DynamicVertexBufferData: public DynamicVertexBufferGraphicsData
{
public:

	static void install();
	static void remove();
	static void beginFrame();
	static void lostDevice();
	static void restoreDevice();

	void *operator new(size_t size);
	void  operator delete(void *memory);

	explicit Direct3d11_DynamicVertexBufferData(const VertexBuffer &vertexBuffer);
	virtual ~Direct3d11_DynamicVertexBufferData();

	virtual void                            *lock(int numberOfVertices, bool forceDiscard);
	virtual void                             unlock();
	virtual void                             unlock(int numberOfVertices);
	virtual const VertexBufferDescriptor    &getDescriptor() const;
	virtual int                              getNumberOfLockableDynamicVertices(bool withDiscard);
	intptr_t								getSortKey() override;

	int                          getNumberOfVertices() const;
	ID3D11Buffer				*getVertexBuffer() const;
	int                          getVertexSize() const;
	ID3D11InputLayout			*getVertexDeclaration() const;
	int                          getOffset() const;

private:

	void roundUpUsed() const;

	// Disabled.
	Direct3d11_DynamicVertexBufferData(void);
	Direct3d11_DynamicVertexBufferData(const Direct3d11_DynamicVertexBufferData &);
	Direct3d11_DynamicVertexBufferData &operator =(const Direct3d11_DynamicVertexBufferData &);

	static bool                    ms_newFrame;
	static int                     ms_size;
	static int                     ms_used;
	static ID3D11Buffer			  *ms_d3dVertexBuffer;
	static MemoryBlockManager     *ms_memoryBlockManager;

	static int                     ms_locksSinceBeginFrame;
	static int                     ms_discardsSinceBeginFrame;
	static int                     ms_locksSinceResourceCreation;
	static int                     ms_discardsSinceResourceCreation;
	static int                     ms_locksEver;
	static int                     ms_discardsEver;
	
	const VertexBufferDescriptor  &m_vertexBufferDescriptor;
	int                            m_numberOfVertices;
	int                            m_offset;
	ID3D11InputLayout			  *m_vertexDeclaration;
	static D3D11_SUBRESOURCE_DATA  mVertexData;
	D3D11_MAPPED_SUBRESOURCE	   mMappedData;
};

// ======================================================================

inline int Direct3d11_DynamicVertexBufferData::getNumberOfVertices() const
{
	return m_numberOfVertices;
}

// ----------------------------------------------------------------------

inline int Direct3d11_DynamicVertexBufferData::getOffset() const
{
	return m_offset;
}

// ----------------------------------------------------------------------

inline ID3D11InputLayout *Direct3d11_DynamicVertexBufferData::getVertexDeclaration() const
{
	return m_vertexDeclaration;
}

// ======================================================================

#endif
