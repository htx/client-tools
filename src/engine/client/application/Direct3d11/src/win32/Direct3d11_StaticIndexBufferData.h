#ifndef INCLUDED_Direct3d11_StaticIndexBufferData_H
#define INCLUDED_Direct3d11_StaticIndexBufferData_H
#include <d3d11.h>

// ======================================================================

struct ID3D11Buffer;
class  MemoryBlockManager;

#include "clientGraphics/StaticIndexBuffer.h"

// ======================================================================

class Direct3d11_StaticIndexBufferData : public StaticIndexBufferGraphicsData
{
public:

	void *operator new(size_t size);
	void  operator delete(void *memory);

	static void install();
	static void remove();

	explicit Direct3d11_StaticIndexBufferData(const StaticIndexBuffer &indexBuffer);
	virtual ~Direct3d11_StaticIndexBufferData();

	virtual Index *lock(bool readOnly);
	virtual void   unlock();

	ID3D11Buffer *getIndexBuffer() const;

private:

	/// Disabled.
	Direct3d11_StaticIndexBufferData();
	Direct3d11_StaticIndexBufferData(const Direct3d11_StaticIndexBufferData &);
	Direct3d11_StaticIndexBufferData &operator =(const Direct3d11_StaticIndexBufferData &);

	static MemoryBlockManager	*ms_memoryBlockManager;
	const StaticIndexBuffer		&m_indexBuffer;
	ID3D11Buffer				*m_d3dIndexBuffer;
	D3D11_BUFFER_DESC			mIndexBufferDesc;
	D3D11_SUBRESOURCE_DATA		mIndexData;
	D3D11_MAPPED_SUBRESOURCE	mMappedData;
};

// ======================================================================

inline ID3D11Buffer *Direct3d11_StaticIndexBufferData::getIndexBuffer() const
{
	return m_d3dIndexBuffer;
}

// ======================================================================

#endif
