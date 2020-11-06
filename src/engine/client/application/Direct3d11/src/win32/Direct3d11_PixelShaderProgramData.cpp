#include "FirstDirect3d11.h"
#include "Direct3d11_PixelShaderProgramData.h"

#include "Direct3d11.h"
#include "ConfigDirect3d11.h"
#include "clientGraphics/ShaderCapability.h"

// ======================================================================

Direct3d11_PixelShaderProgramData::Direct3d11_PixelShaderProgramData(ShaderImplementation::Pass::PixelShader::Program const & pixelShaderProgram)
: ShaderImplementationPassPixelShaderProgramGraphicsData(),
	m_pixelShader(NULL)
{
	LPTSTR errorText = NULL;
	HRESULT const hresult = Direct3d11::getDevice()->CreatePixelShader(pixelShaderProgram.m_exe, sizeof(pixelShaderProgram.m_exe), nullptr, &m_pixelShader);

	FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM|FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_IGNORE_INSERTS, NULL, hresult, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&errorText, 0, NULL); 
	FATAL(FAILED(hresult), ("create pixelshader fail. %s", errorText));
}

// ----------------------------------------------------------------------

Direct3d11_PixelShaderProgramData::~Direct3d11_PixelShaderProgramData()
{
	if (m_pixelShader)
		m_pixelShader->Release();
}

// ======================================================================
