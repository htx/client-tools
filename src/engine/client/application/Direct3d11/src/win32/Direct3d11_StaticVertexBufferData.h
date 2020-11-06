#ifndef INCLUDED_Direct3d11_StaticVertexBufferData_H
#define INCLUDED_Direct3d11_StaticVertexBufferData_H
#include <d3d11.h>

// ======================================================================

struct ID3D11Buffer;
struct ID3D11InputLayout;
class  MemoryBlockManager;

#include "clientGraphics/StaticVertexBuffer.h"
#include "Direct3d11_VertexBufferDescriptorMap.h"
#include <cstdint>

// ======================================================================

class Direct3d11_StaticVertexBufferData: public StaticVertexBufferGraphicsData
{
public:

	void *operator new(size_t size);
	void  operator delete(void *memory);

	static void install();
	static void remove();

	explicit Direct3d11_StaticVertexBufferData(const StaticVertexBuffer &vertexBuffer);
	virtual ~Direct3d11_StaticVertexBufferData();

	void                            *lock(bool readOnly) override;
	void                             unlock() override;
	const VertexBufferDescriptor    &getDescriptor() const override;
	intptr_t                         getSortKey() override;

	ID3D11Buffer				*getVertexBuffer() const;
	int                          getVertexSize() const;
	ID3D11InputLayout		    *getVertexDeclaration() const;
	int                          getOffset() const;

private:

	/// disabled.
	Direct3d11_StaticVertexBufferData(void);
	Direct3d11_StaticVertexBufferData(const Direct3d11_StaticVertexBufferData &);
	Direct3d11_StaticVertexBufferData &operator =(const Direct3d11_StaticVertexBufferData &);

	static MemoryBlockManager     *ms_memoryBlockManager;

	const StaticVertexBuffer      &m_vertexBuffer;
	const VertexBufferDescriptor  &m_descriptor;
	ID3D11Buffer				  *m_d3dVertexBuffer;
	ID3D11InputLayout			  *m_vertexDeclaration;
	D3D11_BUFFER_DESC			   mVertexBufferDesc;
	D3D11_SUBRESOURCE_DATA		   mVertexData;
	D3D11_MAPPED_SUBRESOURCE	   mMappedData;
};

// ======================================================================

inline ID3D11Buffer *Direct3d11_StaticVertexBufferData::getVertexBuffer() const
{
	return m_d3dVertexBuffer;
}

// ----------------------------------------------------------------------

inline int Direct3d11_StaticVertexBufferData::getVertexSize() const
{
	return m_descriptor.vertexSize;
}

// ----------------------------------------------------------------------

inline ID3D11InputLayout *Direct3d11_StaticVertexBufferData::getVertexDeclaration() const
{
	return m_vertexDeclaration;
}

// ======================================================================

#endif
