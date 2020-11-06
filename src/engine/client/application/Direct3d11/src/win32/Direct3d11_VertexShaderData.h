#ifndef INCLUDED_Direct3d11_VertexShaderData_H
#define INCLUDED_Direct3d11_VertexShaderData_H

// ======================================================================

struct ID3D11VertexShader;

#include "clientGraphics/ShaderImplementation.h"

// ======================================================================

class Direct3d11_VertexShaderData : public ShaderImplementationPassVertexShaderGraphicsData
{
public:

	typedef stdvector<Tag>::fwd TextureCoordinateSetTags;

	static void install();
	static void remove();

	Direct3d11_VertexShaderData(ShaderImplementationPassVertexShader const & VertexShader);
	virtual ~Direct3d11_VertexShaderData();

	ID3D11VertexShader * getVertexShader(uint32 textureCoordinateSetKey) const;

	TextureCoordinateSetTags const * getTextureCoordinateSetTags() const;

private:

	typedef stdmap<uint32, ID3D11VertexShader *>::fwd Container;

	ID3D11VertexShader * createVertexShader(uint32 textureCoordinateSetKey) const;

	ShaderImplementation::Pass::VertexShader const * m_vertexShader;

	bool                                             m_hlsl;
	char                                             m_hlslTarget[8];
	const char *                                     m_compileText;
	int                                              m_compileTextLength;
	TextureCoordinateSetTags *                       m_textureCoordinateSetTags;

	mutable Container *                              m_container;
	mutable ID3D11VertexShader *                 m_nonPatchedVertexShader;

	mutable uint32                                   m_lastRequestedKey;
	mutable ID3D11VertexShader *                 m_lastReturnedValue;
};

// ======================================================================

inline Direct3d11_VertexShaderData::TextureCoordinateSetTags const * Direct3d11_VertexShaderData::getTextureCoordinateSetTags() const
{
	return m_textureCoordinateSetTags;
}

// ======================================================================

#endif
