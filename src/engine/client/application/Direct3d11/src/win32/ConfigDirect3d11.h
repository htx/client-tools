#ifndef INCLUDED_ConfigDirect3d11_H
#define INCLUDED_ConfigDirect3d11_H

// ======================================================================

class ConfigDirect3d11
{
public:

	enum VertexProcessingMode
	{
		VPM_default,
		VPM_hardware,
		VPM_software
	};

	static void install();

	static int  getAdapter();
	static bool getUseReferenceRasterizer();
	static bool getUsePureDevice();

	static bool getDisableVertexAndPixelShaders();
	static int  getShaderCapabilityOverride();

	static bool getAllowTearing();
	static int  getFullscreenRefreshRate();

	static bool getDisableDynamicTextures();

	static bool getDisableMultiStreamVertexBuffers();
	static bool getScreenShotBackBuffer();
	static bool getDoNotLockBackBuffer();

	static bool getCreateShaders();

	static int  getDynamicVertexBufferSize();
	static int  getDynamicIndexBufferSize();

	static int  getMaxVertexShaderVersion();
	static int  getMaxPixelShaderVersion();

	static bool getDiscardDynamicBuffersAtBeginningOfFrame();

	static VertexProcessingMode getVertexProcessingMode();

	static bool getAntiAlias();

};

// ======================================================================

#endif
