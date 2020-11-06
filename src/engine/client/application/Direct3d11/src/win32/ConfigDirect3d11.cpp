#include "FirstDirect3d11.h"
#include "ConfigDirect3d11.h"

#include "SharedFoundation/ConfigFile.h"
#include "SharedFoundation/CrashReportInformation.h"

// ======================================================================

namespace ConfigDirect3d11Namespace
{
	int  ms_adapter;
	bool ms_useReferenceRasterizer;
	bool ms_usePureDevice;

	bool ms_disableVertexAndPixelShaders;
	int  ms_shaderCapabilityOverride;

	bool ms_allowTearing;
	int  ms_fullscreenRefreshRate;

	bool ms_disableDynamicTextures;

	bool ms_validateShaderImplementations;
	bool ms_disableMultiStreamVertexBuffers;
	bool ms_doNotLockBackBuffer;
	bool ms_screenShotBackBuffer;

	bool ms_createShaders;

	int  ms_dynamicVertexBufferSize;
	int  ms_dynamicIndexBufferSize;

	int  ms_maxVertexShaderVersion;
	int  ms_maxPixelShaderVersion;

	bool ms_discardDynamicBuffersAtBeginningOfFrame;

	bool ms_antiAlias;

	ConfigDirect3d11::VertexProcessingMode ms_vertexProcessingMode = ConfigDirect3d11::VPM_default;
}
using namespace ConfigDirect3d11Namespace;

// ======================================================================

#define KEY_INT(a,b)     (ms_ ## a = ConfigFile::getKeyInt("Direct3d11", #a, b))
#define KEY_BOOL(a,b)    (ms_ ## a = ConfigFile::getKeyBool("Direct3d11", #a, b))

// ======================================================================

void ConfigDirect3d11::install()
{
	KEY_INT (adapter, -1);
	KEY_BOOL(useReferenceRasterizer, false);
	KEY_BOOL(usePureDevice, false);

	KEY_BOOL(disableVertexAndPixelShaders, false);
	KEY_INT (shaderCapabilityOverride, 0);

	KEY_BOOL(allowTearing, false);
	KEY_INT (fullscreenRefreshRate, 0);

	KEY_BOOL(disableDynamicTextures, false);

	KEY_BOOL(disableMultiStreamVertexBuffers, false);
	KEY_BOOL(screenShotBackBuffer, false);
	KEY_BOOL(doNotLockBackBuffer, false);

	KEY_BOOL(createShaders, true);

	KEY_INT (dynamicVertexBufferSize, 256);
	KEY_INT (dynamicIndexBufferSize, 64);

	KEY_INT (maxVertexShaderVersion, 0xffff);
	KEY_INT (maxPixelShaderVersion, 0xffff);

	KEY_BOOL(discardDynamicBuffersAtBeginningOfFrame, false);
	
	KEY_BOOL(antiAlias, true);

	int const vertexProcessingMode = ConfigFile::getKeyInt("Direct3d11", "vertexProcessingMode", 0);
	FATAL(vertexProcessingMode < 0 || vertexProcessingMode > 2, ("vertexProcessingMode setting invalid %d [0..2]", vertexProcessingMode));
	ms_vertexProcessingMode = static_cast<VertexProcessingMode>(vertexProcessingMode);

	CrashReportInformation::addStaticText("DiscardDynamicBuffersAtBeginningOfFrame: %d\n", ms_discardDynamicBuffersAtBeginningOfFrame ? 1 : 0);
}

// ----------------------------------------------------------------------

int  ConfigDirect3d11::getAdapter()
{
	return ms_adapter;
}

// ----------------------------------------------------------------------

bool ConfigDirect3d11::getUseReferenceRasterizer()
{
	return ms_useReferenceRasterizer;
}

// ----------------------------------------------------------------------

ConfigDirect3d11::VertexProcessingMode ConfigDirect3d11::getVertexProcessingMode()
{
	return ms_vertexProcessingMode;
}

// ----------------------------------------------------------------------

bool ConfigDirect3d11::getUsePureDevice()
{
	return ms_usePureDevice;
}

// ----------------------------------------------------------------------

bool ConfigDirect3d11::getDisableVertexAndPixelShaders()
{
	return ms_disableVertexAndPixelShaders;
}

// ----------------------------------------------------------------------

int ConfigDirect3d11::getShaderCapabilityOverride()
{
	return ms_shaderCapabilityOverride;
}

// ----------------------------------------------------------------------

bool ConfigDirect3d11::getAllowTearing()
{
	return ms_allowTearing;
}

// ----------------------------------------------------------------------

bool ConfigDirect3d11::getDisableDynamicTextures()
{
	return ms_disableDynamicTextures;
}

// ----------------------------------------------------------------------

int  ConfigDirect3d11::getFullscreenRefreshRate()
{
	return ms_fullscreenRefreshRate;
}

// ----------------------------------------------------------------------

bool ConfigDirect3d11::getDisableMultiStreamVertexBuffers()
{
	return ms_disableMultiStreamVertexBuffers;
}

// ----------------------------------------------------------------------

bool ConfigDirect3d11::getScreenShotBackBuffer()
{
	return ms_screenShotBackBuffer;
}

// ----------------------------------------------------------------------

bool ConfigDirect3d11::getDoNotLockBackBuffer()
{
	return ms_doNotLockBackBuffer;
}

// ----------------------------------------------------------------------

bool ConfigDirect3d11::getCreateShaders()
{
	return ms_createShaders;
}

// ----------------------------------------------------------------------

int ConfigDirect3d11::getDynamicVertexBufferSize()
{
	return ms_dynamicVertexBufferSize;
}

// ----------------------------------------------------------------------

bool ConfigDirect3d11::getAntiAlias()
{
	return ms_antiAlias;
}

// ----------------------------------------------------------------------

int ConfigDirect3d11::getDynamicIndexBufferSize()
{
	return ms_dynamicIndexBufferSize;
}

// ----------------------------------------------------------------------

int ConfigDirect3d11::getMaxVertexShaderVersion()
{
	return ms_maxVertexShaderVersion;
}

// ----------------------------------------------------------------------

int ConfigDirect3d11::getMaxPixelShaderVersion()
{
	return ms_maxPixelShaderVersion;
}

// ----------------------------------------------------------------------

bool ConfigDirect3d11::getDiscardDynamicBuffersAtBeginningOfFrame()
{
	return ms_discardDynamicBuffersAtBeginningOfFrame;
}

// ======================================================================
