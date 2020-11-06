#ifndef INCLUDED_Direct3d11_VertexBufferDescriptorMap_H
#define INCLUDED_Direct3d11_VertexBufferDescriptorMap_H

// ======================================================================

class  VertexBufferFormat;
struct VertexBufferDescriptor;

// ======================================================================

class Direct3d11_VertexBufferDescriptorMap
{
public:

	static void install();
	static void remove();

	static const VertexBufferDescriptor &getDescriptor(const VertexBufferFormat &vertexFormat);
	static const VertexBufferDescriptor &getDescriptor(uint32 formatFlags);
};

// ======================================================================

#endif
