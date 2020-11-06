#ifndef INCLUDED_Direct3d11_DynamicIndexBufferData_H
#define INCLUDED_Direct3d11_DynamicIndexBufferData_H

// ======================================================================

struct ID3D11Buffer;
class  MemoryBlockManager;

#include "clientGraphics/DynamicIndexBuffer.h"

// ======================================================================

class Direct3d11_DynamicIndexBufferData : public DynamicIndexBufferGraphicsData
{
public:

	static void  install();
	static void  remove();
	static void  beginFrame();
	static void  lostDevice();
	static void  restoreDevice();
	static void  setSize(int numberOfIndices);
	static ID3D11Buffer *getIndexBuffer();

	void *operator new(size_t size);
	void  operator delete(void *memory);

	Direct3d11_DynamicIndexBufferData();
	virtual ~Direct3d11_DynamicIndexBufferData();

	virtual Index *lock(int numberOfIndices);
	virtual void   unlock();

	DWORD                  getOffset() const;
	int                    getNumberOfIndices() const;

private:

	// Disabled.
	Direct3d11_DynamicIndexBufferData(const Direct3d11_DynamicIndexBufferData &);

	// Disabled.
	Direct3d11_DynamicIndexBufferData &operator =(const Direct3d11_DynamicIndexBufferData &);

	static bool                    ms_newFrame;
	static MemoryBlockManager  *   ms_memoryBlockManager;
	static int                     ms_numberOfIndices;
	static int                     ms_usedNumberOfIndices;
	static ID3D11Buffer			 * ms_d3dIndexBuffer;

	static int                     ms_locksSinceBeginFrame;
	static int                     ms_discardsSinceBeginFrame;
	static int                     ms_locksSinceResourceCreation;
	static int                     ms_discardsSinceResourceCreation;
	static int                     ms_locksEver;
	static int                     ms_discardsEver;

	DWORD  m_offset;
	int    m_numberOfIndices;
};

// ======================================================================

inline ID3D11Buffer *Direct3d11_DynamicIndexBufferData::getIndexBuffer()
{
	return ms_d3dIndexBuffer;
}

// ----------------------------------------------------------------------

inline DWORD Direct3d11_DynamicIndexBufferData::getOffset() const
{
	return m_offset;
}

// ----------------------------------------------------------------------

inline int Direct3d11_DynamicIndexBufferData::getNumberOfIndices() const
{
	return m_numberOfIndices;
}

// ======================================================================

#endif
