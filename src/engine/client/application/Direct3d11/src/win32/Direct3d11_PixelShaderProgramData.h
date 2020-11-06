#ifndef INCLUDED_Direct3d11_PixelShaderProgramData_H
#define INCLUDED_Direct3d11_PixelShaderProgramData_H

// ======================================================================

struct ID3D11PixelShader;

#include "clientGraphics/ShaderImplementation.h"

// ======================================================================

class Direct3d11_PixelShaderProgramData : public ShaderImplementationPassPixelShaderProgramGraphicsData
{
public:

	Direct3d11_PixelShaderProgramData(ShaderImplementation::Pass::PixelShader::Program const & pixelShaderProgram);
	virtual ~Direct3d11_PixelShaderProgramData();

	ID3D11PixelShader * getPixelShader() const;

private:

	ID3D11PixelShader * m_pixelShader;
};

// ======================================================================

inline ID3D11PixelShader * Direct3d11_PixelShaderProgramData::getPixelShader() const
{
	return m_pixelShader;
}

// ======================================================================

#endif
