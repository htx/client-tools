#ifndef INCLUDED_Direct3d11_VertexDeclarationMap_H
#define INCLUDED_Direct3d11_VertexDeclarationMap_H

// ======================================================================

struct ID3D11InputLayout;
class  VertexBufferFormat;

// ======================================================================

class Direct3d11_VertexDeclarationMap
{
public:

	static void                         install();
	static void                         remove();
	static ID3D11InputLayout *fetchVertexDeclaration(VertexBufferFormat const &vertexBufferFormat);
	static ID3D11InputLayout *fetchVertexDeclaration(VertexBufferFormat const * const * vertexBufferFormat, int count);
};

// ======================================================================

#endif
