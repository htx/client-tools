#ifndef BLOCK_ALLOCATOR_H_
#define BLOCK_ALLOCATOR_H_

#include <stdio.h>
#include <stdlib.h>

#ifdef EXTERNAL_DISTRO
namespace NAMESPACE 
{

#endif
namespace Base 
{
	class BlockAllocator
	{
	public:
		BlockAllocator();
		~BlockAllocator();
		void *getBlock(uintptr_t accum);
		void returnBlock(uintptr_t *handle);

	private:
		uintptr_t  *m_blocks[31]{};
	};
};
#ifdef EXTERNAL_DISTRO
};
#endif

#endif
