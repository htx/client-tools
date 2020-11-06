#ifndef INCLUDED_Direct3d11_VertexBufferVectorData_H
#define INCLUDED_Direct3d11_VertexBufferVectorData_H

// ======================================================================

#include "Direct3d11.h"
#include "clientGraphics/VertexBufferVector.h"

// ======================================================================

class Direct3d11_VertexBufferVectorData : public VertexBufferVectorGraphicsData
{
public:

	Direct3d11_VertexBufferVectorData(VertexBufferVector const & vertexBufferVector);
	virtual ~Direct3d11_VertexBufferVectorData();

	ID3D11InputLayout * getVertexDeclaration();

private:

	ID3D11InputLayout * m_vertexDeclaration;
};

// ======================================================================

inline ID3D11InputLayout * Direct3d11_VertexBufferVectorData::getVertexDeclaration()
{
	return m_vertexDeclaration;
}

// ======================================================================

#endif
