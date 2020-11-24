#include "FirstDirect3d11.h"
#include "Direct3d11.h"

#include "ConfigDirect3d11.h"
#include "Direct3d11_DynamicIndexBufferData.h"
#include "Direct3d11_DynamicVertexBufferData.h"
#include "Direct3d11_LightManager.h"
#include "Direct3d11_PixelShaderConstantRegisters.h"
#include "Direct3d11_PixelShaderProgramData.h"
#include "Direct3d11_RenderTarget.h"
#include "Direct3d11_ShaderImplementationData.h"
#include "Direct3d11_StateCache.h"
#include "Direct3d11_StaticIndexBufferData.h"
#include "Direct3d11_StaticShaderData.h"
#include "Direct3d11_StaticVertexBufferData.h"
#include "Direct3d11_TextureData.h"
#include "Direct3d11_VertexBufferVectorData.h"
#include "Direct3d11_VertexDeclarationMap.h"
#include "Direct3d11_VertexShaderConstantRegisters.h"
#include "Direct3d11_VertexShaderData.h"
#include "PaddedVector.h"
#include "SetupDll.h"
#include "WriteTGA.h"

#include "clientGraphics/Camera.h"
#include "clientGraphics/ConfigClientGraphics.h"
#include "clientGraphics/Gl_dll.def"
#include "clientGraphics/Graphics.h"
#include "clientGraphics/Material.h"
#include "clientGraphics/ShaderCapability.h"
#include "clientGraphics/VertexBuffer.h"
#include "sharedDebug/DebugFlags.h"
#include "sharedDebug/DebugKey.h"
#include "sharedDebug/PerformanceTimer.h"
#include "sharedFoundation/Clock.h"
#include "sharedFoundation/ConfigSharedFoundation.h"
#include "sharedFoundation/CrashReportInformation.h"
#include "sharedFoundation/Os.h"
#include "sharedFoundation/Production.h"
#include "sharedMath/Rectangle2d.h"
#include "sharedMath/Transform.h"
#include "sharedMath/Vector.h"
#include "sharedMath/VectorRgba.h"

#include <ddraw.h>
#include <cstdio>

#pragma warning (disable: 4201)
#include <d3dcompiler.h>
#include <memory>
#include <wrl/client.h>
#include <mmsystem.h>
#pragma warning (default: 4201)

extern "C"
{
#include "jpeglib.h"


#if _MSC_VER < 1300
// from WINUSER.H
typedef struct tagMONITORINFO
{
	DWORD   cbSize;
	RECT    rcMonitor;
	RECT    rcWork;
	DWORD   dwFlags;
} MONITORINFO, *LPMONITORINFO;
#endif

WINUSERAPI BOOL WINAPI GetMonitorInfoA(HMONITOR hMonitor, LPMONITORINFO lpmi);
WINUSERAPI BOOL WINAPI GetMonitorInfoW(HMONITOR hMonitor, LPMONITORINFO lpmi);
#ifdef UNICODE
#define GetMonitorInfo  GetMonitorInfoW
#else
#define GetMonitorInfo  GetMonitorInfoA
#endif // !UNICODE

}

// ======================================================================

namespace Direct3d11Namespace
{
	// ----------------------------------------------------------------------
	// types

	typedef void (*CallbackFunction)();
	typedef std::vector<CallbackFunction> CallbackFunctions;

	class AdapterData
	{
	public:
		AdapterData(IDXGIAdapter * pAdapter)
		{
			this->pAdapter = pAdapter;
			pAdapter->GetDesc(&this->description);
		}
		IDXGIAdapter * pAdapter = nullptr;
		DXGI_ADAPTER_DESC description;
	};
	
	// ----------------------------------------------------------------------
	// functions

	bool                               verify();
	void                               remove();
	void                               displayModeChanged();
	bool                               checkDisplayMode();
	void                               lostDevice();
	void                               restoreDevice();
	bool                               isGdiVisible();
	bool                               wasDeviceReset();

	void                               flushResources(bool fullReset);

	bool                               requiresVertexAndPixelShaders();

	void                               updateWindowSettings();

	bool                               supportsScissorRect();
	bool                               supportsHardwareMouseCursor();

	char const *                       getFormatName(DXGI_FORMAT format);
	DXGI_FORMAT                        convertToAdapterFormat(DXGI_FORMAT backBufferFormat);
	DXGI_FORMAT const *                getMatchingColorAlphaFormats(int color, int alpha);
	DXGI_FORMAT const *                getMatchingDepthStencilFormats(int z, int stencil);
	int                                getDepthBufferBitDepth(DXGI_FORMAT depthStencilFormat);
	int                                getStencilBufferBitDepth(DXGI_FORMAT depthStencilFormat);

	bool                               supportsMipmappedCubeMaps();
	int                                getMaximumVertexBufferStreamCount();
	void                               getOtherAdapterRects(std::vector<RECT> &otherAdapterRects);
	void                               setBadVertexShaderStaticShader(const StaticShader *shader);

	void                               resize(int newWidth, int newHeight);
	void                               setWindowedMode(bool windowed, bool force);
	void                               setWindowedMode(bool windowed);
	void                               setWindow(HWND window, int width, int height);
	void                               removeWindow(HWND window);
	void                               setPresentParameters();

	void                               setBrightnessContrastGamma(float brightness, float contrast, float gamma);

	void                               setFillMode(GlFillMode newFillMode);
	void                               setCullMode(GlCullMode newCullMode);

	void                               setPointSize(float size);
	void                               setPointSizeMin( float min );
	void                               setPointSizeMax( float max );
	void                               setPointScaleEnable( bool bEnable );
	void                               setPointScaleFactor( float A, float B, float C);
	void                               setPointSpriteEnable( bool bEnable );

	void                               clearViewport(bool clearColor, uint32 colorValue, bool clearDepth, float depthValue, bool clearStencil, uint32 stencilValue);

	bool                               screenShot(GlScreenShotFormat screenShotFormat, int quality, const char *fileName);

	void                               update(float elapsedTime);
	void                               beginScene();
	void                               endScene();
	bool                               getBackBuffer();
	bool                               lockBackBuffer(Gl_pixelRect &o_pixels, const RECT *i_lockRect);
	bool                               unlockBackBuffer();
	void                               releaseBackBuffer();
	bool                               present(bool windowed, HWND window, int width, int height);
	bool                               present();
	bool                               present(HWND window, int width, int height);
	void                               setRenderTarget(Texture *texture, CubeFace cubeFace, int mipmapLevel);
	bool                               copyRenderTargetToNonRenderTargetTexture();

	bool                               setMouseCursor(Texture const & mouseCursorTexture, int hotSpotX, int hotSpotY);
	bool                               showMouseCursor(bool cursorVisible);

	void                               setViewport(int x0, int y0, int x1, int y1, float minZ, float maxZ);
	void                               setScissorRect(bool enabled, int x, int y, int width, int height);
	void                               setWorldToCameraTransform(const Transform &transform, const Vector &cameraPosition);
	void                               setProjectionMatrix(const GlMatrix4x4 &projectionMatrix);
	void                               setFog(bool enabled, float density, const PackedArgb &color);
	void                               setObjectToWorldTransformAndScale(const Transform &transform, const Vector &scale);
	void                               setGlobalTexture(Tag tag, const Texture &texture);
	void                               releaseAllGlobalTextures();
	void                               setTextureTransform(int stage, bool enabled, int dimension, bool projected, const float *transform);
	void                               setVertexShaderUserConstants(int index, float c0, float c1, float c2, float c3);
	void                               setPixelShaderUserConstants(VectorRgba const * constants, int count);

	ShaderImplementationGraphicsData * createShaderImplementationGraphicsData(const ShaderImplementation &shaderImplementation);
	StaticShaderGraphicsData *         createStaticShaderGraphicsData(const StaticShader &shader);

	ShaderImplementationPassVertexShaderGraphicsData       * createVertexShaderData(ShaderImplementationPassVertexShader const &vertexShader);
	ShaderImplementationPassPixelShaderProgramGraphicsData * createPixelShaderProgramData(ShaderImplementationPassPixelShaderProgram const &pixelShaderProgram);

	void                               setAlphaFadeOpacity(bool enabled, float opacity);
	void                               noSetAlphaFadeOpacity(bool enabled, float opacity);

//	VectorRgba const &                 getAlphaFadeAndBloomSettings();

	void                               setLights(const stdvector<const Light*>::fwd &lightList);

	StaticVertexBufferGraphicsData *   createVertexBufferData(const StaticVertexBuffer &vertexBuffer);
	DynamicVertexBufferGraphicsData *  createVertexBufferData(const DynamicVertexBuffer &vertexBuffer);
	VertexBufferVectorGraphicsData *   createVertexBufferVectorData(VertexBufferVector const & vertexBufferVector);

	StaticIndexBufferGraphicsData *    createIndexBufferData(const StaticIndexBuffer &indexBuffer);
	DynamicIndexBufferGraphicsData *   createIndexBufferData();
	void                               setDynamicIndexBufferSize(int numberOfIndices);

	void                               getOneToOneUVMapping(int textureWidth, int textureHeight, float &u0, float &v0, float &u1, float &v1);
	TextureGraphicsData *              createTextureData(const Texture &texture, const TextureFormat *runtimeFormats, int numberOfRuntimeFormats);

	void                               drawPointList();
	void                               drawLineList();
	void                               drawLineStrip();
	void                               drawTriangleList();
	void                               drawTriangleStrip();
	void                               drawTriangleFan();
	void                               drawQuadList();

	void                               drawIndexedPointList();
	void                               drawIndexedLineList();
	void                               drawIndexedLineStrip();
	void                               drawIndexedTriangleList();
	void                               drawIndexedTriangleStrip();
	void                               drawIndexedTriangleFan();

	void                               drawPointList(int startVertex, int primitiveCount);
	void                               drawLineList(int startVertex, int primitiveCount);
	void                               drawLineStrip(int startVertex, int primitiveCount);
	void                               drawTriangleList(int startVertex, int primitiveCount);
	void                               drawTriangleStrip(int startVertex, int primitiveCount);
	void                               drawTriangleFan(int startVertex, int primitiveCount);

	void                               drawIndexedPointList(int baseInex, int minimumVertexIndex, int numberOfVertices, int startIndex, int primitiveCount);
	void                               drawIndexedLineList(int baseIndex, int minimumVertexIndex, int numberOfVertices, int startIndex, int primitiveCount);
	void                               drawIndexedLineStrip(int baseIndex, int minimumVertexIndex, int numberOfVertices, int startIndex, int primitiveCount);
	void                               drawIndexedTriangleList(int baseIndex, int minimumVertexIndex, int numberOfVertices, int startIndex, int primitiveCount);
	void                               drawIndexedTriangleStrip(int baseIndex, int minimumVertexIndex, int numberOfVertices, int startIndex, int primitiveCount);
	void                               drawIndexedTriangleFan(int baseIndex, int minimumVertexIndex, int numberOfVertices, int startIndex, int primitiveCount);

	void                               resizeQuadListIndexBuffer(int numberOfQuads);

	void                               addDeviceLostCallback(CallbackFunction callbackFunction);
	void                               removeDeviceLostCallback(CallbackFunction callbackFunction);
	void                               addDeviceRestoredCallback(CallbackFunction callbackFunction);
	void                               removeDeviceRestoredCallback(CallbackFunction callbackFunction);

	void                               optimizeIndexBuffer(WORD *indices, int numIndices);

	void                               setBloomEnabled(bool enabled);

	void                               pixSetMarker(WCHAR const * markerName);
	void                               pixBeginEvent(WCHAR const * eventName);
	void                               pixEndEvent(WCHAR const * eventName);
	bool                               writeImage(char const * file, int const width, int const height, int const pitch, int const * pixelsARGB, bool const alphaExtend, Gl_imageFormat const imageFormat, Rectangle2d const * subRect);

	void                               _queryVideoMemory();

	// ----------------------------------------------------------------------

	bool                                       ms_installed;
	bool                                       ms_windowed;
	bool                                       ms_engineOwnsWindow;
	bool                                       ms_borderlessWindow;
	int                                        ms_windowX;
	int                                        ms_windowY;
	void                                       (*ms_windowedModeChanged)(bool windowed);
	CallbackFunctions                          ms_deviceLostCallbacks;
	CallbackFunctions                          ms_deviceRestoredCallbacks;
	bool                                       ms_displayModeChanged;
	bool                                       ms_deviceReset;
	int                                        ms_frameNumber;
	float                                      ms_currentTime;
	HWND                                       ms_window;

	Gl_api                                     ms_glApi;
	
	Microsoft::WRL::ComPtr<ID3D11Device>			ms_device;
	Microsoft::WRL::ComPtr<ID3D11DeviceContext>		ms_deviceContext;
	Microsoft::WRL::ComPtr<ID3D11DepthStencilState> ms_depthStencilState;
	Microsoft::WRL::ComPtr<ID3D11DepthStencilView>	ms_depthStencilView;
	Microsoft::WRL::ComPtr<ID3D11RasterizerState>	ms_rasterState;
	Microsoft::WRL::ComPtr<ID3D11SamplerState>		ms_samplerState;
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView>	ms_renderTargetView;
	Microsoft::WRL::ComPtr<IDXGISwapChain>			ms_swapChain;
	Microsoft::WRL::ComPtr<IDXGIFactory>            ms_dxgiFactory;
	Microsoft::WRL::ComPtr<ID3D11Texture2D>			ms_backBuffer;
	Microsoft::WRL::ComPtr<ID3D11Texture2D>		    ms_depthStencilBuffer;
	std::vector<AdapterData>						ms_adapters;
	
	ID3D11Buffer*							   ms_constantBufferTime;
	ID3D11Buffer*							   ms_constantBufferViewport;
	ID3D11Buffer*							   ms_constantBufferCameraPosition;
	ID3D11Buffer*							   ms_constantBufferFog;
	ID3D11Buffer*							   ms_constantBufferUserConstants;
	ID3D11Buffer*							   ms_constantBufferObjectWorldCameraPM;

	D3D_FEATURE_LEVEL						   ms_featureLevel;
	D3D11_TEXTURE2D_DESC					   ms_depthBufferDesc;
	D3D11_VIEWPORT							   ms_viewport;
	DXGI_MODE_DESC                             ms_displayMode;
	DXGI_FORMAT                                ms_adapterFormat;
	IDXGIAdapter*							   ms_adapter;
	IDXGIOutput*							   ms_adapterOutput;
	DXGI_FORMAT                                ms_backBufferFormat;
	DXGI_FORMAT                                ms_depthStencilFormat;
	DXGI_ADAPTER_DESC						   ms_adapterIdentifier;
	bool                                       ms_backBufferLocked;
	bool                                       ms_hasDepthBuffer;
	bool                                       ms_hasStencilBuffer;
	bool                                       ms_supportsStreamOffsets;
	bool                                       ms_supportsDynamicTextures;
	bool                                       ms_supportsMultiSample;
	DWORD                                      ms_multiSampleQualityLevels;

	bool                                       ms_alphaBlendEnable;
	uint8                                      ms_alphaTestReferenceValue;
	uint8                                      ms_colorWriteEnable;

	bool                                       ms_alphaFadeOpacityEnabled;
	bool                                       ms_alphaFadeOpacityDirty;
	VectorRgba                                 ms_alphaFadeOpacity;

	BYTE ms_colorCorrectionTable[256];
	bool applyGammaCorrectionToXRGBSurface( ID3D11Texture2D *surface );

	unsigned int                               ms_width;
	unsigned int                               ms_height;

	int                                        ms_viewportX;
	int                                        ms_viewportY;
	int                                        ms_viewportWidth;
	int                                        ms_viewportHeight;
	float                                      ms_viewportMinimumZ;
	float                                      ms_viewportMaximumZ;

	int                                        ms_sliceNumberOfVertices;
	int                                        ms_sliceFirstVertex;

	ID3D11Buffer *							   ms_savedIndexBuffer;
	int                                        ms_sliceNumberOfIndices;
	int                                        ms_sliceFirstIndex;

	int                                        ms_lastVertexBufferCount;

	DWORD                                      ms_fogColor;
	DWORD                                      ms_fogModeValue;

	bool                                       ms_antialiasEnabled = false;

	bool                                ms_transformDirty;

	DirectX::XMFLOAT4X4                 ms_cachedObjectToWorldMatrix;
	DirectX::XMFLOAT4X4                 ms_cachedWorldToCameraMatrix;
	DirectX::XMFLOAT4X4                 ms_cachedProjectionMatrix;
	DirectX::XMFLOAT4X4                 ms_cachedWorldToProjectionMatrix;

	const StaticShader                 *ms_badVertexShaderStaticShader;

	void                               *ms_temporaryBuffer;
	int                                 ms_temporaryBufferSize;
	int   ms_shaderCapability;
	int   ms_videoMemoryInMegabytes = 32;

	int                ms_quadListIndexBufferNumberOfQuads;
	StaticIndexBuffer *ms_quadListIndexBuffer;

	/*const D3DCUBEMAP_FACES ms_cubeFaceLookup[] =
	{
		D3DCUBEMAP_FACE_POSITIVE_X,  // CF_positiveX
		D3DCUBEMAP_FACE_NEGATIVE_X,  // CF_negativeX
		D3DCUBEMAP_FACE_POSITIVE_Y,  // CF_positiveY
		D3DCUBEMAP_FACE_NEGATIVE_Y,  // CF_negativeY
		D3DCUBEMAP_FACE_POSITIVE_Z,  // CF_positiveZ
		D3DCUBEMAP_FACE_NEGATIVE_Z   // CF_negativeZ
	};*/

	PerformanceTimer *ms_performanceTimer;

	int nextPowerOfTwo(int x);

	// video capture variables
	ID3D11Texture2D *ms_videoSurface;             // Surface to StretchRect the backbuffer to
	ID3D11Texture2D *ms_videoOffScreenSurface;    // Surface to GetRenderData to
}
using namespace Direct3d11Namespace;

// ======================================================================

extern "C" __declspec(dllexport) Gl_api const * GetApi();

// ======================================================================

Gl_api const * GetApi()
{
	ms_glApi.verify  = verify;
	ms_glApi.install = Direct3d11::install;
	return &ms_glApi;
}

// ----------------------------------------------------------------------

bool Direct3d11Namespace::verify()
{
	return true;
}

// ----------------------------------------------------------------------

IDXGIFactory *Direct3d11::getDXGIFactory()
{
	return ms_dxgiFactory.Get();
}

// ----------------------------------------------------------------------

ID3D11Device *Direct3d11::getDevice()
{
	return ms_device.Get();
}

// ----------------------------------------------------------------------

float Direct3d11::getCurrentTime()
{
	return ms_currentTime;
}

// ----------------------------------------------------------------------

int Direct3d11::getFrameNumber()
{
	return ms_frameNumber;
}

// ----------------------------------------------------------------------

ID3D11DeviceContext* Direct3d11::getDeviceContext()
{
	return ms_deviceContext.Get();
}

// ----------------------------------------------------------------------

DXGI_FORMAT Direct3d11::getAdapterFormat()
{
	return ms_adapterFormat;
}

// ----------------------------------------------------------------------

int Direct3d11::getShaderCapability()
{
	return ms_shaderCapability;
}

// ----------------------------------------------------------------------

int Direct3d11::getVideoMemoryInMegabytes()
{
	return ms_videoMemoryInMegabytes;
}

// ----------------------------------------------------------------------

bool Direct3d11Namespace::requiresVertexAndPixelShaders()
{
	return true;
}

// ----------------------------------------------------------------------

DWORD Direct3d11::getFogColor()
{
	return ms_fogColor;
}

// ----------------------------------------------------------------------

DXGI_FORMAT Direct3d11::getDepthStencilFormat()
{
	return ms_depthStencilFormat;
}

// ----------------------------------------------------------------------

/*D3DCUBEMAP_FACES DireDirect3d11ct3d9::getD3dCubeFace(CubeFace const cubeFace)
{
	DEBUG_FATAL(cubeFace == CF_none, ("cannot look up CF_none"));
	return ms_cubeFaceLookup[cubeFace];
}*/

// ----------------------------------------------------------------------

const char *Direct3d11Namespace::getFormatName(DXGI_FORMAT format)
{
#define CASE(a) case a: return #a
	switch (format)
	{
		CASE(DXGI_FORMAT_UNKNOWN);

		CASE(DXGI_FORMAT_B8G8R8A8_UNORM);
		CASE(DXGI_FORMAT_B8G8R8A8_UNORM_SRGB);
		
		CASE(DXGI_FORMAT_B8G8R8X8_UNORM);
		CASE(DXGI_FORMAT_B8G8R8X8_UNORM_SRGB);
		
		CASE(DXGI_FORMAT_B5G6R5_UNORM);
		CASE(DXGI_FORMAT_B5G5R5A1_UNORM);
		CASE(DXGI_FORMAT_B4G4R4A4_UNORM);
		CASE(DXGI_FORMAT_A8_UNORM);
		CASE(DXGI_FORMAT_R8G8B8A8_UNORM);
		CASE(DXGI_FORMAT_R8G8B8A8_UNORM_SRGB);
		CASE(DXGI_FORMAT_R16G16_UNORM);
		CASE(DXGI_FORMAT_R16G16B16A16_UNORM);
		CASE(DXGI_FORMAT_R8_UNORM);
		CASE(DXGI_FORMAT_R8G8_UNORM);
		CASE(DXGI_FORMAT_R8G8_SNORM);
		CASE(DXGI_FORMAT_R8G8B8A8_SNORM);
		CASE(DXGI_FORMAT_R16G16_SNORM);

		CASE(DXGI_FORMAT_G8R8_G8B8_UNORM);
		CASE(DXGI_FORMAT_R8G8_B8G8_UNORM);
		CASE(DXGI_FORMAT_BC1_UNORM);
		CASE(DXGI_FORMAT_BC1_UNORM_SRGB);
		CASE(DXGI_FORMAT_BC2_UNORM);
		CASE(DXGI_FORMAT_BC2_UNORM_SRGB);
		CASE(DXGI_FORMAT_BC3_UNORM);
		CASE(DXGI_FORMAT_BC3_UNORM_SRGB);
		CASE(DXGI_FORMAT_D16_UNORM);
		CASE(DXGI_FORMAT_D32_FLOAT);
		CASE(DXGI_FORMAT_R16_UNORM);

		CASE(DXGI_FORMAT_R16_UINT);
		CASE(DXGI_FORMAT_R32_UINT);

		CASE(DXGI_FORMAT_R16G16B16A16_SNORM);

		CASE(DXGI_FORMAT_R16_FLOAT);
		CASE(DXGI_FORMAT_R16G16_FLOAT);
		CASE(DXGI_FORMAT_R16G16B16A16_FLOAT);

		CASE(DXGI_FORMAT_R32_FLOAT);
		CASE(DXGI_FORMAT_R32G32_FLOAT);
		CASE(DXGI_FORMAT_R32G32B32A32_FLOAT);
	}

#undef CASE

	return "bad format";
}

// ----------------------------------------------------------------------

DXGI_FORMAT Direct3d11Namespace::convertToAdapterFormat(DXGI_FORMAT backBufferFormat)
{
	switch (backBufferFormat)
	{
		case DXGI_FORMAT_B8G8R8A8_UNORM:  return DXGI_FORMAT_B8G8R8X8_UNORM;
		case DXGI_FORMAT_B8G8R8X8_UNORM:  return DXGI_FORMAT_B8G8R8X8_UNORM;
		case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:  return DXGI_FORMAT_B8G8R8X8_UNORM_SRGB;
		case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:  return DXGI_FORMAT_B8G8R8X8_UNORM_SRGB;
		case DXGI_FORMAT_B5G6R5_UNORM:    return DXGI_FORMAT_B5G6R5_UNORM;
	}

	DEBUG_FATAL(true, ("Unknown back buffer format"));
	return DXGI_FORMAT_UNKNOWN;
}

// ----------------------------------------------------------------------

const DXGI_FORMAT *Direct3d11Namespace::getMatchingColorAlphaFormats(int color, int alpha)
{
	switch (color)
	{
		case -1:
			switch (alpha)
			{
				case -1:
					{
						static const DXGI_FORMAT bbf[] = { DXGI_FORMAT_B8G8R8A8_UNORM, DXGI_FORMAT_B8G8R8X8_UNORM, DXGI_FORMAT_B5G5R5A1_UNORM, DXGI_FORMAT_B5G6R5_UNORM, DXGI_FORMAT_UNKNOWN };
						return bbf;
					}
				case  0:
					{
						static const DXGI_FORMAT bbf[] = { DXGI_FORMAT_B8G8R8X8_UNORM, DXGI_FORMAT_B5G6R5_UNORM, DXGI_FORMAT_UNKNOWN };
						return bbf;
					}
				case  1:
					{
						static const DXGI_FORMAT bbf[] = { DXGI_FORMAT_B5G5R5A1_UNORM, DXGI_FORMAT_UNKNOWN };
						return bbf;
					}
				case  8:
					{
						static const DXGI_FORMAT bbf[] = { DXGI_FORMAT_B8G8R8A8_UNORM, DXGI_FORMAT_UNKNOWN };
						return bbf;
					}
			}
			break;

		case 15:
			switch (alpha)
			{
				case -1:
					{
						static const DXGI_FORMAT bbf[] = { DXGI_FORMAT_B5G5R5A1_UNORM, DXGI_FORMAT_UNKNOWN };
						return bbf;
					}
				case  0:
					{
						static const DXGI_FORMAT bbf[] = { DXGI_FORMAT_UNKNOWN };
						return bbf;
					}
				case  1:
					{
						static const DXGI_FORMAT bbf[] = { DXGI_FORMAT_B5G5R5A1_UNORM, DXGI_FORMAT_UNKNOWN };
						return bbf;
					}
			}
			break;

		case 16:
			switch (alpha)
			{
				case -1:
				case  0:
				{
					static const DXGI_FORMAT bbf[] = { DXGI_FORMAT_B5G6R5_UNORM, DXGI_FORMAT_UNKNOWN };
					return bbf;
				}
			}
			break;

		case 24:
			switch (alpha)
			{
				case -1:
					{
						static const DXGI_FORMAT bbf[] = { DXGI_FORMAT_B8G8R8A8_UNORM, DXGI_FORMAT_B8G8R8X8_UNORM, DXGI_FORMAT_UNKNOWN };
						return bbf;
					}
				case 0:
					{
						static const DXGI_FORMAT bbf[] = { DXGI_FORMAT_B8G8R8X8_UNORM, DXGI_FORMAT_UNKNOWN };
						return bbf;
					}
				case 8:
					{
						static const DXGI_FORMAT bbf[] = { DXGI_FORMAT_B8G8R8A8_UNORM, DXGI_FORMAT_UNKNOWN };
						return bbf;
					}
			}
			break;
	}

	return NULL;
}

// ----------------------------------------------------------------------

int Direct3d11Namespace::getDepthBufferBitDepth(DXGI_FORMAT depthStencilFormat)
{
	switch (depthStencilFormat)
	{
		case DXGI_FORMAT_D32_FLOAT:			return 32;
		case DXGI_FORMAT_D24_UNORM_S8_UINT: return 24;
		case DXGI_FORMAT_D16_UNORM:			return 16;
		default:
			DEBUG_FATAL(true, ("Not a depth/stencil format"));
	}

	return 0;
}

// ----------------------------------------------------------------------

int Direct3d11Namespace::getStencilBufferBitDepth(DXGI_FORMAT depthStencilFormat)
{
	switch (depthStencilFormat)
	{
		case DXGI_FORMAT_D24_UNORM_S8_UINT: return 8;
		case DXGI_FORMAT_D16_UNORM:			return 0;
		default:
			DEBUG_FATAL(true, ("Not a depth/stencil format"));
	}

	return 0;
}

// ----------------------------------------------------------------------

const DXGI_FORMAT *Direct3d11Namespace::getMatchingDepthStencilFormats(int z, int stencil)
{
	switch (z)
	{
		case -1:
			switch (stencil)
			{
				case -1:
				{
					static const DXGI_FORMAT dsf[] = { DXGI_FORMAT_D16_UNORM, DXGI_FORMAT_UNKNOWN };
					return dsf;
				}

				case  0:
				{
					static const DXGI_FORMAT dsf[] = {  DXGI_FORMAT_D16_UNORM, DXGI_FORMAT_UNKNOWN };
					return dsf;
				}

				case  1:
				{
					static const DXGI_FORMAT dsf[] = { DXGI_FORMAT_UNKNOWN };
					return dsf;
				}

				case  4:
				{
					static const DXGI_FORMAT dsf[] = { DXGI_FORMAT_UNKNOWN };
					return dsf;
				}

				case  8:
				{
					static const DXGI_FORMAT dsf[] = { DXGI_FORMAT_UNKNOWN };
					return dsf;
				}
			}
			break;

		case 15:
			switch (stencil)
			{
				case -1:
				case  1:
					{
						static const DXGI_FORMAT dsf[] = { DXGI_FORMAT_UNKNOWN };
						return dsf;
					}
			}
			break;

		case 16:
			switch (stencil)
			{
				case -1:
				case  0:
					{
						static const DXGI_FORMAT dsf[] = { DXGI_FORMAT_D16_UNORM, DXGI_FORMAT_UNKNOWN };
						return dsf;
					}
			}
			break;

		case 24:
			switch (stencil)
			{
				case -1:
					{
						static const DXGI_FORMAT dsf[] = { DXGI_FORMAT_UNKNOWN };
						return dsf;
					}
				case  0:
					{
						static const DXGI_FORMAT dsf[] = { DXGI_FORMAT_UNKNOWN };
						return dsf;
					}
				case 4:
					{
						static DXGI_FORMAT dsf[] = { DXGI_FORMAT_UNKNOWN };
						return dsf;
					}
				case 8:
					{
						static const DXGI_FORMAT dsf[] = { DXGI_FORMAT_UNKNOWN };
						return dsf;
					}
					break;
			}
			break;

		case 32:
			switch (stencil)
			{
				case -1:
				case 0:
				{
					static const DXGI_FORMAT dsf[] = { DXGI_FORMAT_UNKNOWN };
					return dsf;
				}
			}
			break;
	}

	return nullptr;
}

// ----------------------------------------------------------------------

bool Direct3d11::install(Gl_install *gl_install)
{
	DEBUG_FATAL(sizeof(PaddedVector) != sizeof(float) * 4, ("PaddedVector size bad"));
	NOT_NULL(gl_install);
	DEBUG_FATAL(ms_installed, ("already installed"));

	ConfigDirect3d11::install();

	ms_performanceTimer = new PerformanceTimer();

	ms_installed = true;
	ms_currentTime = 0.0f;
	ms_alphaFadeOpacityEnabled = false;
	ms_alphaFadeOpacityDirty = true;
	ms_alphaFadeOpacity.r = 0.0f;
	ms_alphaFadeOpacity.g = 0.0f;
	ms_alphaFadeOpacity.b = 0.0f;
	ms_alphaFadeOpacity.a = 0.0f;

	// store the screen dimensions
	ms_width               = gl_install->width;
	ms_height              = gl_install->height;
	ms_windowed            = gl_install->windowed;
	ms_window              = gl_install->window;
	ms_engineOwnsWindow    = gl_install->engineOwnsWindow;
	ms_borderlessWindow    = gl_install->borderlessWindow;
	ms_windowX             = gl_install->windowX;
	ms_windowY             = gl_install->windowY;
	ms_windowedModeChanged = gl_install->windowedModeChanged;

	// setup the api calls
	ms_glApi.remove                            = Direct3d11Namespace::remove;
	ms_glApi.displayModeChanged                = displayModeChanged;

	ms_glApi.getShaderCapability               = getShaderCapability;
	ms_glApi.requiresVertexAndPixelShaders     = requiresVertexAndPixelShaders;
	ms_glApi.getOtherAdapterRects              = getOtherAdapterRects;
	ms_glApi.getVideoMemoryInMegabytes         = getVideoMemoryInMegabytes;
	ms_glApi.isGdiVisible                      = isGdiVisible;
	ms_glApi.wasDeviceReset                    = wasDeviceReset;

	ms_glApi.addDeviceLostCallback             = addDeviceLostCallback;
	ms_glApi.removeDeviceLostCallback          = removeDeviceLostCallback;
	ms_glApi.addDeviceRestoredCallback         = addDeviceRestoredCallback;
	ms_glApi.removeDeviceRestoredCallback      = removeDeviceRestoredCallback;

	ms_glApi.flushResources                    = flushResources;
	ms_glApi.setBrightnessContrastGamma        = setBrightnessContrastGamma;

	ms_glApi.supportsMipmappedCubeMaps         = supportsMipmappedCubeMaps;
	ms_glApi.supportsScissorRect               = supportsScissorRect;
	ms_glApi.supportsHardwareMouseCursor       = supportsHardwareMouseCursor;
	ms_glApi.supportsTwoSidedStencil           = supportsTwoSidedStencil;
	ms_glApi.supportsStreamOffsets             = supportsStreamOffsets;
	ms_glApi.supportsDynamicTextures           = supportsDynamicTextures;

	ms_glApi.resize                            = resize;
	ms_glApi.setWindowedMode                   = setWindowedMode;

	ms_glApi.setFillMode                       = setFillMode;
	ms_glApi.setCullMode                       = setCullMode;

	ms_glApi.setPointSize                      = setPointSize;
	ms_glApi.setPointSizeMax                   = setPointSizeMax;
	ms_glApi.setPointSizeMin                   = setPointSizeMin;
	ms_glApi.setPointScaleEnable               = setPointScaleEnable;
	ms_glApi.setPointScaleFactor               = setPointScaleFactor;
	ms_glApi.setPointSpriteEnable              = setPointSpriteEnable;

	ms_glApi.clearViewport                     = clearViewport;

	ms_glApi.update                            = update;
	ms_glApi.beginScene                        = beginScene;
	ms_glApi.endScene                          = endScene;

	ms_glApi.lockBackBuffer                    = lockBackBuffer;
	ms_glApi.unlockBackBuffer                  = unlockBackBuffer;

	ms_glApi.present                           = present;
	ms_glApi.presentToWindow                   = present;
	ms_glApi.setRenderTarget                   = setRenderTarget;
	ms_glApi.copyRenderTargetToNonRenderTargetTexture = copyRenderTargetToNonRenderTargetTexture;

	ms_glApi.screenShot                        = screenShot;

	ms_glApi.setBadVertexShaderStaticShader    = setBadVertexShaderStaticShader;
	ms_glApi.setStaticShader                   = setStaticShader;

	ms_glApi.createTextureData                 = createTextureData;
	ms_glApi.setMouseCursor                    = setMouseCursor;
	ms_glApi.showMouseCursor                   = showMouseCursor;
	ms_glApi.setViewport                       = setViewport;
	ms_glApi.setScissorRect                    = setScissorRect;
	ms_glApi.setWorldToCameraTransform         = setWorldToCameraTransform;
	ms_glApi.setProjectionMatrix               = setProjectionMatrix;
	ms_glApi.setFog                            = setFog;

	ms_glApi.setObjectToWorldTransformAndScale = setObjectToWorldTransformAndScale;
	ms_glApi.setGlobalTexture                  = setGlobalTexture;
	ms_glApi.releaseAllGlobalTextures          = releaseAllGlobalTextures;
	ms_glApi.setTextureTransform               = setTextureTransform;
	ms_glApi.setVertexShaderUserConstants      = setVertexShaderUserConstants;
	ms_glApi.setPixelShaderUserConstants       = setPixelShaderUserConstants;

	ms_glApi.createShaderImplementationGraphicsData    = createShaderImplementationGraphicsData;
	ms_glApi.createStaticShaderGraphicsData            = createStaticShaderGraphicsData;
	ms_glApi.createVertexShaderData            = createVertexShaderData;
	ms_glApi.createPixelShaderProgramData      = createPixelShaderProgramData;

	ms_glApi.setAlphaFadeOpacity               = setAlphaFadeOpacity;

	ms_glApi.setLights                         = setLights;

	ms_glApi.createStaticVertexBufferData      = createVertexBufferData;
	ms_glApi.createDynamicVertexBufferData     = createVertexBufferData;
	ms_glApi.createVertexBufferVectorData      = createVertexBufferVectorData;
	ms_glApi.createStaticIndexBufferData       = createIndexBufferData;
	ms_glApi.createDynamicIndexBufferData      = createIndexBufferData;

	ms_glApi.getOneToOneUVMapping              = getOneToOneUVMapping;

	ms_glApi.setVertexBuffer                   = setVertexBuffer;
	ms_glApi.setVertexBufferVector             = setVertexBufferVector;
	ms_glApi.setIndexBuffer                    = setIndexBuffer;

	ms_glApi.drawPointList                     = drawPointList;
	ms_glApi.drawLineList                      = drawLineList;
	ms_glApi.drawLineStrip                     = drawLineStrip;
	ms_glApi.drawTriangleList                  = drawTriangleList;
	ms_glApi.drawTriangleStrip                 = drawTriangleStrip;
	ms_glApi.drawTriangleFan                   = drawTriangleFan;
	ms_glApi.drawQuadList                      = drawQuadList;

	ms_glApi.drawIndexedPointList              = drawIndexedPointList;
	ms_glApi.drawIndexedLineList               = drawIndexedLineList;
	ms_glApi.drawIndexedLineStrip              = drawIndexedLineStrip;
	ms_glApi.drawIndexedTriangleList           = drawIndexedTriangleList;
	ms_glApi.drawIndexedTriangleStrip          = drawIndexedTriangleStrip;
	ms_glApi.drawIndexedTriangleFan            = drawIndexedTriangleFan;

	ms_glApi.drawPartialPointList              = drawPointList;
	ms_glApi.drawPartialLineList               = drawLineList;
	ms_glApi.drawPartialLineStrip              = drawLineStrip;
	ms_glApi.drawPartialTriangleList           = drawTriangleList;
	ms_glApi.drawPartialTriangleStrip          = drawTriangleStrip;
	ms_glApi.drawPartialTriangleFan            = drawTriangleFan;

	ms_glApi.drawPartialIndexedPointList       = drawIndexedPointList;
	ms_glApi.drawPartialIndexedLineList        = drawIndexedLineList;
	ms_glApi.drawPartialIndexedLineStrip       = drawIndexedLineStrip;
	ms_glApi.drawPartialIndexedTriangleList    = drawIndexedTriangleList;
	ms_glApi.drawPartialIndexedTriangleStrip   = drawIndexedTriangleStrip;
	ms_glApi.drawPartialIndexedTriangleFan     = drawIndexedTriangleFan;

	ms_glApi.getMaximumVertexBufferStreamCount = getMaximumVertexBufferStreamCount;

	ms_glApi.optimizeIndexBuffer			   = optimizeIndexBuffer;

	ms_glApi.setBloomEnabled = setBloomEnabled;

	ms_glApi.pixSetMarker = pixSetMarker;
	ms_glApi.pixBeginEvent = pixBeginEvent;
	ms_glApi.pixEndEvent = pixEndEvent;

	ms_glApi.writeImage = writeImage;

	ms_glApi.supportsAntialias = supportsAntialias;
	ms_glApi.setAntialiasEnabled = setAntialiasEnabled;

	// create interface factory
	HRESULT hresult = CreateDXGIFactory(__uuidof(IDXGIFactory), reinterpret_cast<void**>(ms_dxgiFactory.GetAddressOf()));
	
	FATAL(FAILED(hresult), ("Failed creating dxgi factory"));
	
	// -----------------------------------

	const bool verboseHardwareLogging = ConfigSharedFoundation::getVerboseHardwareLogging();

	// ---------------------------------------------------------------------------------
	// Query for adapter video memory.
	_queryVideoMemory();

	REPORT_LOG(verboseHardwareLogging, ("Video memory = %i\n", ms_videoMemoryInMegabytes));
	CrashReportInformation::addStaticText("VideoMemory: %i\n", ms_videoMemoryInMegabytes);
	CrashReportInformation::addStaticText("GameResolution: %d %d\n", ms_width, ms_height);

	// ---------------------------------------------------------------------------------
	{
		// figure out which adapter to use
		const int configAdapter = ConfigDirect3d11::getAdapter();
		
		unsigned int adapterNr = configAdapter < 0 ? 0 : configAdapter;
		
		UINT index = 0;
		while (SUCCEEDED(ms_dxgiFactory->EnumAdapters(index, &ms_adapter)))
		{
			ms_adapters.emplace_back(AdapterData(ms_adapter));
			index += 1;
		}

		// use primary adapter output
		hresult = ms_adapters[0].pAdapter->EnumOutputs(0, &ms_adapterOutput);
		FATAL(FAILED(hresult), ("Enum adapter output fail."));
		
		int numberOfAdapters = ms_adapters.size();
		REPORT_LOG(verboseHardwareLogging, ("Using adapter %d of %d:\n", configAdapter, numberOfAdapters));
		CrashReportInformation::addStaticText("VideoAdapter: %d/%d\n", configAdapter, numberOfAdapters);

		// adapter info
		hresult = ms_adapters[0].pAdapter->GetDesc(&ms_adapterIdentifier);
		FATAL(FAILED(hresult), ("get adapter description fail."));

		//REPORT_LOG(verboseHardwareLogging, ("Adapter device: 0x%08x 0x%08x 0x%08x 0x%08x\n", ms_adapterIdentifier.VendorId, ms_adapterIdentifier.DeviceId, ms_adapterIdentifier.SubSysId, ms_adapterIdentifier.Revision));
		//REPORT_LOG(verboseHardwareLogging, ("Adapter driver: %s\n", ms_adapterIdentifier.Description));
	}

	// ---------------------------------------------------------------------------------
	
	unsigned int numberOfSupportedModes = 0;

	// get mode count
	hresult = ms_adapterOutput->GetDisplayModeList(DXGI_FORMAT_B8G8R8A8_UNORM, 0, &numberOfSupportedModes, nullptr);	
	DEBUG_FATAL(numberOfSupportedModes <= 0, ("GetDisplayModeList returned %d", numberOfSupportedModes));

	DXGI_MODE_DESC* supportedModes = new DXGI_MODE_DESC[numberOfSupportedModes];
	ZeroMemory(supportedModes, sizeof(DXGI_MODE_DESC) * numberOfSupportedModes);

	hresult = ms_adapterOutput->GetDisplayModeList(DXGI_FORMAT_R8G8B8A8_UNORM, 0, &numberOfSupportedModes, supportedModes);
	FATAL(FAILED(hresult), ("get adapter display mode list fail."));

	for(unsigned int i = 0; i < numberOfSupportedModes; i++)
	{
		if(supportedModes[i].Width == ms_height)
		{
			if(supportedModes[i].Height == ms_width)
			{
				ms_displayMode = supportedModes[i];
			}
		}
	}

	// ---------------------------------------------------------------------------------
	// figure out color/alpha/depth/stencil buffer formats
	/*const DXGI_FORMAT *depthStencilFormats = getMatchingDepthStencilFormats(gl_install->zBufferBitDepth, gl_install->stencilBufferBitDepth);
	FATAL(!depthStencilFormats, ("invalid depth/stencil format specified"));
	const DXGI_FORMAT *backBufferFormats = getMatchingColorAlphaFormats(gl_install->colorBufferBitDepth, gl_install->alphaBufferBitDepth);
	FATAL(!backBufferFormats, ("invalid color/alpha format specified"));*/
	// ---------------------------------------------------------------------------------

	// Initialize the swap chain description.
	DXGI_SWAP_CHAIN_DESC scd;
	ZeroMemory(&scd, sizeof(scd));

	// Set to a single back buffer.
	scd.BufferCount = 1;

	// Set the width and height of the back buffer.
	scd.BufferDesc.Width = ms_width;
	scd.BufferDesc.Height = ms_height;

	// Set regular 32-bit surface for the back buffer.
	scd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;

	// Set the refresh rate of the back buffer.
	if(ConfigDirect3d11::getAllowTearing())
	{
		scd.BufferDesc.RefreshRate.Numerator = ms_displayMode.RefreshRate.Numerator;
		scd.BufferDesc.RefreshRate.Denominator = ms_displayMode.RefreshRate.Denominator;
	}
	else
	{
		scd.BufferDesc.RefreshRate.Numerator = 0;
		scd.BufferDesc.RefreshRate.Denominator = 1;
	}

	// Set the usage of the back buffer.
	scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;

	// Set the handle for the window to render to.
	scd.OutputWindow = ms_window;

	// Turn multisampling off.
	scd.SampleDesc.Count = 1;
	scd.SampleDesc.Quality = 0;

	// Set to full screen or windowed mode.
	scd.Windowed = ms_windowed;

	// Set the scan line ordering and scaling to unspecified.
	scd.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	scd.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;

	// Discard the back buffer contents after presenting.
	scd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

	// Don't set the advanced flags.
	scd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

	// Set the feature level
	ms_featureLevel = D3D_FEATURE_LEVEL_11_1;

	// Create the swap chain, Direct3D device, and Direct3D device context.
	hresult = D3D11CreateDeviceAndSwapChain(ms_adapters[0].pAdapter, D3D_DRIVER_TYPE_UNKNOWN, nullptr, D3D11_CREATE_DEVICE_DEBUG, &ms_featureLevel, 1, 
					       D3D11_SDK_VERSION, &scd, ms_swapChain.GetAddressOf(), ms_device.GetAddressOf(), nullptr, ms_deviceContext.GetAddressOf());
	FATAL(FAILED(hresult), ("create device + swapchain fail."));

	// Get the pointer to the back buffer.
	hresult = ms_swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(ms_backBuffer.GetAddressOf()));
	FATAL(FAILED(hresult), ("get backbuffer fail."));

	// Create the render target view with the back buffer pointer.
	hresult = ms_device->CreateRenderTargetView(ms_backBuffer.Get(), nullptr, ms_renderTargetView.GetAddressOf());
	FATAL(FAILED(hresult), ("create rendertargetview fail."));

	// Release pointer to the back buffer as we no longer need it.
	/*ms_backBuffer->Release();
	ms_backBuffer = 0;*/

	// Initialize the description of the depth buffer.
	ZeroMemory(&ms_depthBufferDesc, sizeof(ms_depthBufferDesc));

	// Set up the description of the depth buffer.
	ms_depthBufferDesc.Width = ms_width;
	ms_depthBufferDesc.Height = ms_height;
	ms_depthBufferDesc.MipLevels = 1;
	ms_depthBufferDesc.ArraySize = 1;
	ms_depthBufferDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	ms_depthBufferDesc.SampleDesc.Count = 1;
	ms_depthBufferDesc.SampleDesc.Quality = 0;
	ms_depthBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	ms_depthBufferDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	ms_depthBufferDesc.CPUAccessFlags = 0;
	ms_depthBufferDesc.MiscFlags = 0;

	// Create the texture for the depth buffer using the filled out description.
	hresult = ms_device->CreateTexture2D(&ms_depthBufferDesc, nullptr, ms_depthStencilBuffer.GetAddressOf());
	FATAL(FAILED(hresult), ("create depthstencilbuffer fail."));

	// Initialize the description of the stencil state.
	D3D11_DEPTH_STENCIL_DESC dsd;
	ZeroMemory(&dsd, sizeof(dsd));

	// Set up the description of the stencil state.
	dsd.DepthEnable = true;
	dsd.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	dsd.DepthFunc = D3D11_COMPARISON_LESS;

	dsd.StencilEnable = true;
	dsd.StencilReadMask = 0xFF;
	dsd.StencilWriteMask = 0xFF;

	// Stencil operations if pixel is front-facing.
	dsd.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	dsd.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_INCR;
	dsd.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	dsd.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

	// Stencil operations if pixel is back-facing.
	dsd.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	dsd.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_DECR;
	dsd.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	dsd.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

	// Create the depth stencil state.
	hresult = ms_device->CreateDepthStencilState(&dsd, ms_depthStencilState.GetAddressOf());
	FATAL(FAILED(hresult), ("create depthstencilstate fail."));

	// Initailze the depth stencil view.
	CD3D11_DEPTH_STENCIL_VIEW_DESC dsvd;
	ZeroMemory(&dsvd, sizeof(dsvd));

	// Set up the depth stencil view description.
	dsvd.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	dsvd.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	dsvd.Texture2D.MipSlice = 0;

	// Create the depth stencil view.
	hresult = ms_device->CreateDepthStencilView(ms_depthStencilBuffer.Get(), &dsvd, ms_depthStencilView.GetAddressOf());
	FATAL(FAILED(hresult), ("create depthstencilview fail."));

	// Bind the render target view and depth stencil buffer to the output render pipeline.
	ms_deviceContext->OMSetRenderTargets(1, ms_renderTargetView.GetAddressOf(), ms_depthStencilView.Get());
	
	D3D11_VIEWPORT viewport;
	ZeroMemory(&viewport, sizeof(D3D11_VIEWPORT));

	viewport.TopLeftX = 0;
	viewport.TopLeftY = 0;
	viewport.Width = ms_width;
	viewport.Height = ms_height;
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;

	// set the Viewport
	ms_deviceContext->RSSetViewports(1, &viewport);
	
	D3D11_RASTERIZER_DESC rsd;
	ZeroMemory(&rsd, sizeof(D3D11_RASTERIZER_DESC));
	
	rsd.AntialiasedLineEnable = false;
	rsd.CullMode = D3D11_CULL_BACK;
	rsd.DepthBias = 0;
	rsd.DepthBiasClamp = 0.0f;
	rsd.DepthClipEnable = true;
	rsd.FillMode = D3D11_FILL_WIREFRAME;
	rsd.FrontCounterClockwise = false;
	rsd.MultisampleEnable = false;
	rsd.ScissorEnable = false;
	rsd.SlopeScaledDepthBias = 0.0f;

	// Create the rasterizer state from the description we just filled out.
	hresult = ms_device->CreateRasterizerState(&rsd, ms_rasterState.GetAddressOf());
	FATAL(FAILED(hresult), ("create rasterizer state fail."));

	D3D11_SAMPLER_DESC sampDesc;
	ZeroMemory(&sampDesc, sizeof(sampDesc));
	sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	sampDesc.MinLOD = 0;
	sampDesc.MaxLOD = D3D11_FLOAT32_MAX;
	hresult = ms_device->CreateSamplerState(&sampDesc, ms_samplerState.GetAddressOf());
	
	FATAL(FAILED(hresult), ("create sampler state fail."));

	//---------------------------------------------
	// create constant buffers
	D3D11_BUFFER_DESC constantBufferDesc;   // create the constant buffer

	ZeroMemory(&constantBufferDesc, sizeof(constantBufferDesc));

	constantBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	constantBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	constantBufferDesc.ByteWidth = 16;
	constantBufferDesc.CPUAccessFlags = 0;
	constantBufferDesc.MiscFlags = 0;
	constantBufferDesc.StructureByteStride = 0;
	ms_device->CreateBuffer(&constantBufferDesc, nullptr, &ms_constantBufferTime);
	ms_device->CreateBuffer(&constantBufferDesc, nullptr, &ms_constantBufferViewport);
	ms_device->CreateBuffer(&constantBufferDesc, nullptr, &ms_constantBufferFog);
	ms_device->CreateBuffer(&constantBufferDesc, nullptr, &ms_constantBufferUserConstants);

	constantBufferDesc.ByteWidth = sizeof(PaddedVector);
	ms_device->CreateBuffer(&constantBufferDesc, nullptr, &ms_constantBufferCameraPosition);
	
	constantBufferDesc.ByteWidth = sizeof(DirectX::XMFLOAT4X4[2]);
	ms_device->CreateBuffer(&constantBufferDesc, nullptr, &ms_constantBufferObjectWorldCameraPM);

	updateWindowSettings();

	ms_hasDepthBuffer = true;//getDepthBufferBitDepth(ms_depthStencilFormat) != 0;
	ms_hasStencilBuffer = true;//getStencilBufferBitDepth(ms_depthStencilFormat) != 0;

	// install the other subsystems
	Direct3d11_VertexDeclarationMap::install();
	Direct3d11_VertexShaderData::install();
	Direct3d11_StaticShaderData::install();
	Direct3d11_ShaderImplementationData::install();
	Direct3d11_StateCache::install(getMaximumVertexBufferStreamCount());
	Direct3d11_VertexBufferDescriptorMap::install();
	Direct3d11_StaticVertexBufferData::install();
	Direct3d11_DynamicVertexBufferData::install();
	Direct3d11_StaticIndexBufferData::install();
	Direct3d11_DynamicIndexBufferData::install();
	Direct3d11_TextureData::install();
	Direct3d11_RenderTarget::install();
	Direct3d11_LightManager::install();

	setViewport(0, 0, ms_width, ms_height, 0.0, 1.0);

	// initialize the last row of the matrix when using VS only so we don't have to touch it again
	ms_cachedObjectToWorldMatrix._41 = 0.0f;
	ms_cachedObjectToWorldMatrix._42 = 0.0f;
	ms_cachedObjectToWorldMatrix._43 = 0.0f;
	ms_cachedObjectToWorldMatrix._44 = 1.0f;

	ms_cachedWorldToCameraMatrix._41 = 0.0f;
	ms_cachedWorldToCameraMatrix._42 = 0.0f;
	ms_cachedWorldToCameraMatrix._43 = 0.0f;
	ms_cachedWorldToCameraMatrix._44 = 1.0f;

	resizeQuadListIndexBuffer(4 * 1024);

	return true;
}

// ----------------------------------------------------------------------

void Direct3d11Namespace::remove()
{
	DEBUG_FATAL(!ms_installed, ("not installed"));
	ms_installed = false;

	delete ms_performanceTimer;
	ms_performanceTimer = nullptr;

	delete ms_temporaryBuffer;
	ms_temporaryBuffer = nullptr;

	if (ms_quadListIndexBuffer)
	{
		delete ms_quadListIndexBuffer;
		ms_quadListIndexBuffer = nullptr;
	}

	Direct3d11_RenderTarget::remove();
	Direct3d11_TextureData::remove();
	Direct3d11_DynamicIndexBufferData::remove();
	Direct3d11_StaticIndexBufferData::remove();
	Direct3d11_StaticVertexBufferData::remove();
	Direct3d11_DynamicVertexBufferData::remove();
	Direct3d11_VertexBufferDescriptorMap::remove();
	Direct3d11_VertexShaderData::remove();
	Direct3d11_VertexDeclarationMap::remove();
	Direct3d11_StateCache::remove();

	releaseBackBuffer();

	if(ms_swapChain)
	{
		ms_swapChain->SetFullscreenState(false, nullptr);
	}
	
	if(ms_rasterState)
	{
		IGNORE_RETURN(ms_rasterState->Release());
		ms_rasterState = nullptr;
	}

	if(ms_depthStencilView)
	{
		IGNORE_RETURN(ms_depthStencilView->Release());
		ms_depthStencilView = nullptr;
	}

	if(ms_depthStencilState)
	{
		IGNORE_RETURN(ms_depthStencilState->Release());
		ms_depthStencilState = nullptr;
	}

	if(ms_depthStencilBuffer)
	{
		IGNORE_RETURN(ms_depthStencilBuffer->Release());
		ms_depthStencilBuffer = nullptr;
	}

	if(ms_renderTargetView)
	{
		IGNORE_RETURN(ms_renderTargetView->Release());
		ms_renderTargetView = nullptr;
	}

	if(ms_deviceContext)
	{
		IGNORE_RETURN(ms_deviceContext->Release());
		ms_deviceContext = nullptr;
	}

	if(ms_device)
	{
		IGNORE_RETURN(ms_device->Release());
		ms_device = nullptr;
	}

	if(ms_swapChain)
	{
		IGNORE_RETURN(ms_swapChain->Release());
		ms_swapChain = nullptr;
	}

	if (ms_engineOwnsWindow)
		SetWindowPos(ms_window, HWND_BOTTOM, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOCOPYBITS | SWP_HIDEWINDOW);
}

// ----------------------------------------------------------------------------
// Create a DirectDraw interface and query for the video memory
void Direct3d11Namespace::_queryVideoMemory()
{
	IDirectDraw* directDraw = nullptr;
	
	if (SUCCEEDED(DirectDrawCreate (NULL, &directDraw, NULL)) && directDraw)
	{
		DDCAPS caps;
		
		memset (&caps, 0, sizeof (DDCAPS));
		caps.dwSize = sizeof (DDCAPS);
		
		if (SUCCEEDED(directDraw->GetCaps (&caps, nullptr)))
			ms_videoMemoryInMegabytes = caps.dwVidMemTotal / (1024 * 1024);

		directDraw->Release();
		directDraw = nullptr;
	}
}

// ----------------------------------------------------------------------

void Direct3d11Namespace::displayModeChanged()
{
	ms_displayModeChanged = true;
}

// ----------------------------------------------------------------------

bool Direct3d11Namespace::checkDisplayMode()
{
	ms_displayModeChanged = false;
	
	if (ms_windowed && ms_engineOwnsWindow)
	{
		ms_adapterFormat = ms_displayMode.Format;

		if (!ms_borderlessWindow && (static_cast<int>(ms_displayMode.Width) == ms_width || static_cast<int>(ms_displayMode.Height) == ms_height))
		{
			if (ms_adapterFormat != DXGI_FORMAT_B8G8R8A8_UNORM && ms_adapterFormat != DXGI_FORMAT_B8G8R8X8_UNORM)
				Graphics::setLastError("engine", "desktop_same_not_32");
			else
				Graphics::setLastError("engine", "desktop_same");

			return false;
		}

		if (static_cast<int>(ms_displayMode.Width) < ms_width || static_cast<int>(ms_displayMode.Height) < ms_height)
		{
			if (ms_adapterFormat != DXGI_FORMAT_B8G8R8A8_UNORM && ms_adapterFormat != DXGI_FORMAT_B8G8R8X8_UNORM)
				Graphics::setLastError("engine", "desktop_too_small_not_32");
			else
				Graphics::setLastError("engine", "desktop_too_small");

			return false;
		}

		if (ms_adapterFormat != DXGI_FORMAT_B8G8R8A8_UNORM && ms_adapterFormat != DXGI_FORMAT_B8G8R8X8_UNORM)
		{
			Graphics::setLastError("engine", "desktop_not_32bit");
			return false;
		}
	}

	return true;
}

// ----------------------------------------------------------------------

IDXGIAdapter* Direct3d11::getAdapter()
{
	return ms_adapter;
}

// ----------------------------------------------------------------------

bool Direct3d11Namespace::isGdiVisible()
{
	return ms_windowed;
}

// ----------------------------------------------------------------------

bool Direct3d11Namespace::wasDeviceReset()
{
	return ms_deviceReset;
}

// ----------------------------------------------------------------------

void Direct3d11Namespace::flushResources(bool fullReset)
{
	if (fullReset)
	{
		setWindowedMode(ms_windowed, true);
	}
	else
	{
		IGNORE_RETURN(ms_deviceContext->Flush());
	}
}

// ----------------------------------------------------------------------

void Direct3d11Namespace::updateWindowSettings()
{
	if (ms_engineOwnsWindow)
	{
		DWORD const windowStyleWindowed    = WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_BORDER | WS_CLIPCHILDREN | WS_CLIPSIBLINGS;
		DWORD const windowStyleFullscreen  = WS_POPUP | WS_CLIPCHILDREN | WS_CLIPSIBLINGS;

		if (ms_windowed)
		{
			RECT rect;
			rect.left   = 0;
			rect.top    = 0;
			rect.right  = ms_width;
			rect.bottom = ms_height;

			DWORD const windowStyle = ms_borderlessWindow ? windowStyleFullscreen : windowStyleWindowed;
			SetWindowLong(ms_window, GWL_STYLE, windowStyle);

			// adjust it to include the windows crap around the client area
			const BOOL result1 = AdjustWindowRect(&rect, windowStyle, FALSE);
			DEBUG_FATAL(!result1, ("AdjustWindowRect failed"));
			UNREF(result1);

			// get the area of the monitor
			HRESULT hresult = ms_adapters[0].pAdapter->EnumOutputs(0, &ms_adapterOutput);
			FATAL(FAILED(hresult), ("Enum adapter output fail."));

			DXGI_OUTPUT_DESC oDesc;
			ms_adapterOutput->GetDesc(&oDesc);
			
			HMONITOR monitor = oDesc.Monitor;
			MONITORINFO monitorInfo;
			Zero(monitorInfo);
			monitorInfo.cbSize = sizeof(monitorInfo);
			GetMonitorInfo(monitor, &monitorInfo);

			if (ms_borderlessWindow)
			{
				ms_windowX = monitorInfo.rcMonitor.left;
				ms_windowY = monitorInfo.rcMonitor.top;
			}
			else
				if (ms_windowX == INT_MAX || ms_windowY == INT_MAX)
				{
					ms_windowX = monitorInfo.rcMonitor.left + (((monitorInfo.rcMonitor.right - monitorInfo.rcMonitor.left) - ms_width) / 2);
					ms_windowY = monitorInfo.rcMonitor.top  + (((monitorInfo.rcMonitor.bottom - monitorInfo.rcMonitor.top) - ms_height) / 2);
				}

			// reposition and resize the window
			const BOOL result2 = SetWindowPos(ms_window, HWND_NOTOPMOST, ms_windowX, ms_windowY, rect.right - rect.left, rect.bottom - rect.top, SWP_NOCOPYBITS |  SWP_SHOWWINDOW);
			FATAL(!result2, ("SetWindowPos failed"));
		}
		else
		{
			SetWindowLong(ms_window, GWL_STYLE, windowStyleFullscreen);

			HRESULT hresult = ms_adapters[0].pAdapter->EnumOutputs(0, &ms_adapterOutput);
			FATAL(FAILED(hresult), ("Enum adapter output fail."));

			DXGI_OUTPUT_DESC oDesc;
			ms_adapterOutput->GetDesc(&oDesc);
			
			HMONITOR monitor = oDesc.Monitor;
			MONITORINFO monitorInfo;
			Zero(monitorInfo);
			monitorInfo.cbSize = sizeof(monitorInfo);
			GetMonitorInfo(monitor, &monitorInfo);
			RECT const & r = monitorInfo.rcMonitor;

			const BOOL result2 = SetWindowPos(ms_window, HWND_TOPMOST, r.left, r.top, ms_width, ms_height, SWP_NOCOPYBITS |  SWP_SHOWWINDOW);
			FATAL(!result2, ("SetWindowPos failed"));
		}
	}
}

// ----------------------------------------------------------------------

bool Direct3d11::engineOwnsWindow()
{
	return ms_engineOwnsWindow;
}

// ----------------------------------------------------------------------

int Direct3d11::getMaxRenderTargetWidth()
{
	return ms_width;
}

// ----------------------------------------------------------------------

int Direct3d11::getMaxRenderTargetHeight()
{
	return ms_height;
}

// ----------------------------------------------------------------------

bool Direct3d11::supportsPixelShaders()
{
	return true;
}

// ----------------------------------------------------------------------

bool Direct3d11::supportsAntialias()
{
	return ConfigDirect3d11::getAntiAlias() && ms_supportsMultiSample;
}

// ----------------------------------------------------------------------

void Direct3d11::setAntialiasEnabled(bool enabled)
{
	if(supportsAntialias())
	{
		if(ms_antialiasEnabled != enabled)
		{
			ms_antialiasEnabled = enabled;
			setWindowedMode(ms_windowed, true);
		}
		ms_antialiasEnabled = enabled;
		//Direct3d11_StateCache::setRenderState(D3DRS_MULTISAMPLEANTIALIAS, enabled);
	}
}

// ----------------------------------------------------------------------

bool Direct3d11::supportsVertexShaders()
{
	return true;
}

// ----------------------------------------------------------------------

bool Direct3d11Namespace::supportsScissorRect()
{
	//return (ms_deviceCaps.RasterCaps & D3DPRASTERCAPS_SCISSORTEST) != 0;
	return false;
}

// ----------------------------------------------------------------------

bool Direct3d11Namespace::supportsMipmappedCubeMaps()
{
	//return (ms_deviceCaps.CubeTextureFilterCaps & (D3DPTFILTERCAPS_MIPFLINEAR | D3DPTFILTERCAPS_MIPFPOINT)) != 0;
	return false;
}

// ----------------------------------------------------------------------

bool Direct3d11Namespace::supportsHardwareMouseCursor()
{
	//return (ms_deviceCaps.CursorCaps & D3DCURSORCAPS_COLOR) != 0;
	return true;
}

// ----------------------------------------------------------------------

bool Direct3d11::supportsTwoSidedStencil()
{
	//return (ms_deviceCaps.StencilCaps & D3DSTENCILCAPS_TWOSIDED) != 0;
	return false;
}

// ----------------------------------------------------------------------

bool Direct3d11::supportsStreamOffsets()
{
	return ms_supportsStreamOffsets;
}

// ----------------------------------------------------------------------

bool Direct3d11::supportsDynamicTextures()
{
	return ms_supportsDynamicTextures;
}

// ----------------------------------------------------------------------

DWORD Direct3d11::getMaxAnisotropy()
{
	//return ms_deviceCaps.MaxAnisotropy;
	return 2;
}

// ----------------------------------------------------------------------

void Direct3d11Namespace::addDeviceLostCallback(CallbackFunction callbackFunction)
{
	DEBUG_FATAL(std::find(ms_deviceLostCallbacks.begin(), ms_deviceLostCallbacks.end(), callbackFunction) != ms_deviceLostCallbacks.end(), ("Callback function already on list"));
	ms_deviceLostCallbacks.push_back(callbackFunction);
}

// ----------------------------------------------------------------------

void Direct3d11Namespace::removeDeviceLostCallback(CallbackFunction callbackFunction)
{
	CallbackFunctions::iterator const i = std::find(ms_deviceLostCallbacks.begin(), ms_deviceLostCallbacks.end(), callbackFunction);
	DEBUG_FATAL(i == ms_deviceLostCallbacks.end(), ("Callback function not on list"));
	ms_deviceLostCallbacks.erase(i);
}

// ----------------------------------------------------------------------

void Direct3d11Namespace::addDeviceRestoredCallback(CallbackFunction callbackFunction)
{
	DEBUG_FATAL(std::find(ms_deviceRestoredCallbacks.begin(), ms_deviceRestoredCallbacks.end(), callbackFunction) != ms_deviceRestoredCallbacks.end(), ("Callback function already on list"));
	ms_deviceRestoredCallbacks.push_back(callbackFunction);
}

// ----------------------------------------------------------------------

void Direct3d11Namespace::removeDeviceRestoredCallback(CallbackFunction callbackFunction)
{
	CallbackFunctions::iterator const i = std::find(ms_deviceRestoredCallbacks.begin(), ms_deviceRestoredCallbacks.end(), callbackFunction);
	DEBUG_FATAL(i == ms_deviceRestoredCallbacks.end(), ("Callback function not on list"));
	ms_deviceRestoredCallbacks.erase(i);
}

// ----------------------------------------------------------------------

void Direct3d11Namespace::lostDevice()
{
	CallbackFunctions::const_iterator const iEnd = ms_deviceLostCallbacks.end();
	for (CallbackFunctions::const_iterator i = ms_deviceLostCallbacks.begin(); i != iEnd; ++i)
		(*(*i))();

	Direct3d11_DynamicVertexBufferData::lostDevice();
	Direct3d11_DynamicIndexBufferData::lostDevice();
	Direct3d11_StateCache::lostDevice();
	Direct3d11_RenderTarget::lostDevice();
	Direct3d11_ShaderImplementationData::lostDevice();
	ms_transformDirty = true;

	releaseBackBuffer();
}

// ----------------------------------------------------------------------

void Direct3d11Namespace::restoreDevice()
{
	Direct3d11_RenderTarget::restoreDevice();
	Direct3d11_DynamicVertexBufferData::restoreDevice();
	Direct3d11_DynamicIndexBufferData::restoreDevice();
	Direct3d11_StateCache::restoreDevice();
	setViewport(ms_viewportX, ms_viewportY, ms_viewportWidth, ms_viewportHeight, ms_viewportMinimumZ, ms_viewportMaximumZ);

	CallbackFunctions::const_iterator const iEnd = ms_deviceRestoredCallbacks.end();
	for (CallbackFunctions::const_iterator i = ms_deviceRestoredCallbacks.begin(); i != iEnd; ++i)
		(*(*i))();

	ms_deviceReset = true;
}

// ----------------------------------------------------------------------

void Direct3d11Namespace::resize(int newWidth, int newHeight)
{
	/*lostDevice();

	// change the width
	ms_width = newWidth;
	ms_height = newHeight;
	ms_presentParameters.BackBufferWidth  = ms_width;
	ms_presentParameters.BackBufferHeight = ms_height;

	// try to restore the device
	const HRESULT hresult = ms_device->Reset(&ms_presentParameters);
	FATAL(FAILED(hresult), ("Reset failed in resize %d", HRESULT_CODE(hresult)));

	// recreate necessary resources after resetting the device
	restoreDevice();*/
}

// ----------------------------------------------------------------------

void Direct3d11Namespace::setWindowedMode(bool windowed, bool force)
{
	/*if (ms_engineOwnsWindow && (force || ms_windowed != windowed))
	{
		if (ms_windowed)
		{
			RECT rect;
			GetWindowRect(ms_window, &rect);
			ms_windowX = rect.left;
			ms_windowY = rect.top;
		}

		// change the windowed mode
		bool old = ms_windowed;
		ms_windowed = windowed;
		setPresentParameters();
		lostDevice();
		HRESULT hresult = ms_device->Reset(&ms_presentParameters);
		if (FAILED(hresult))
		{
			// couldn't switch.  try to go back
			ms_windowed = old;
			setPresentParameters();
			HRESULT second = ms_device->Reset(&ms_presentParameters);

			FATAL(FAILED(second), ("Reset failed in setWindowedMode %d %d", HRESULT_CODE(hresult), HRESULT_CODE(second)));
		}

		updateWindowSettings();

		//  When going from full screen to windowed mode, a portion of the upper left corner of the screen
		//   with dimensions up to the size of the game's full screen mode dimensions does not always get
		//   updated.  This RedrawWindow call fixes the problem by making all desktop windows draw.
		if (ms_windowed)
			RedrawWindow( NULL, NULL, NULL, RDW_INVALIDATE | RDW_ALLCHILDREN );

		// recreate necessary resources after resetting the device
		restoreDevice();
	}

	if (!checkDisplayMode())
		setWindowedMode(false);

	ms_windowedModeChanged(ms_windowed);*/
}

// ----------------------------------------------------------------------

void Direct3d11Namespace::setWindowedMode(bool windowed)
{
	setWindowedMode(windowed, false);
}

// ----------------------------------------------------------------------

void Direct3d11Namespace::setBrightnessContrastGamma(float brightness, float contrast, float gamma)
{
	/*D3DGAMMARAMP gammaRamp;
	int i = 0;;
	float f = 0.0f;
	float const step = 1.0 / 256.0f;
	float oneOverGamma = 1.0f / gamma;
	for ( ; i < 256; ++i, f += step)
	{
		float result = pow(0.5f + contrast * ((f * brightness) - 0.5f), oneOverGamma);
		WORD w = static_cast<WORD>(clamp(0, static_cast<int>(result * 65535.0f), 65535));
		BYTE b = static_cast<BYTE>(clamp(0, static_cast<int>(result * 255.0f), 255));
		gammaRamp.red[i]   = w;
		gammaRamp.green[i] = w;
		gammaRamp.blue[i]  = w;
		ms_colorCorrectionTable[i] = b;
	}

	ms_device->SetGammaRamp(0, D3DSGR_NO_CALIBRATION, &gammaRamp);*/
}

// ----------------------------------------------------------------------
/**
 * This sets the presentParameters MultiSampleType, SwapEffect, and Fullscreen_PresentationInterval.
 * Antialiasing in windowed mode is disabled.
 */

void Direct3d11Namespace::setPresentParameters()
{
	/*if (ms_windowed)
	{
		ms_presentParameters.Windowed = TRUE;
		ms_presentParameters.SwapEffect =  D3DSWAPEFFECT_COPY;
		ms_presentParameters.FullScreen_RefreshRateInHz = 0;
		ms_presentParameters.MultiSampleType = D3DMULTISAMPLE_NONE;
		ms_presentParameters.MultiSampleQuality = 0;
	}
	else
	{
		ms_presentParameters.Windowed = FALSE;
		ms_presentParameters.SwapEffect = D3DSWAPEFFECT_DISCARD;
		const int refreshRate = ConfigDirect3d11::getFullscreenRefreshRate();
		ms_presentParameters.FullScreen_RefreshRateInHz = refreshRate > 0 ? refreshRate : D3DPRESENT_RATE_DEFAULT;
		ms_presentParameters.MultiSampleType = D3DMULTISAMPLE_NONE;
		ms_presentParameters.MultiSampleQuality = 0;
	}
	if(!ms_windowed && ms_supportsMultiSample && ConfigDirect3d11::getAntiAlias() && ms_antialiasEnabled)
	{
		ms_presentParameters.MultiSampleType = D3DMULTISAMPLE_NONMASKABLE;
		ms_presentParameters.MultiSampleQuality = ms_multiSampleQualityLevels - 1;
		if(ms_presentParameters.MultiSampleQuality > 2)
			ms_presentParameters.MultiSampleQuality = 2;
		ms_presentParameters.SwapEffect = D3DSWAPEFFECT_DISCARD;
	}

	if (ConfigDirect3d11::getAllowTearing())
		ms_presentParameters.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;
	else
		ms_presentParameters.PresentationInterval = D3DPRESENT_INTERVAL_ONE;*/
}

// ----------------------------------------------------------------------

void Direct3d11Namespace::setFillMode(GlFillMode newFillMode)
{
	/*switch (newFillMode)
	{
		case GFM_wire:
			Direct3d11_StateCache::setRenderState(D3DRS_FILLMODE, D3DFILL_WIREFRAME);
			break;

		case GFM_solid:
			Direct3d11_StateCache::setRenderState(D3DRS_FILLMODE, D3DFILL_SOLID);
			break;

		default:
			DEBUG_FATAL(true, ("bad fill mode"));
	}*/
}

// ----------------------------------------------------------------------

void Direct3d11Namespace::setCullMode(GlCullMode newCullMode)
{
	/*switch (newCullMode)
	{
		case GCM_none:
			Direct3d11_StateCache::setRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
			break;

		case GCM_clockwise:
			Direct3d11_StateCache::setRenderState(D3DRS_CULLMODE, D3DCULL_CW);
			break;

		case GCM_counterClockwise:
			Direct3d11_StateCache::setRenderState(D3DRS_CULLMODE, D3DCULL_CCW);
			break;

		default:
			DEBUG_FATAL(true, ("bad cull mode"));
	}*/
}

// ----------------------------------------------------------------------

void Direct3d11Namespace::setPointSize(float size)
{
	//Direct3d11_StateCache::setRenderState(D3DRS_POINTSIZE,size);
}

// ----------------------------------------------------------------------

void Direct3d11Namespace::setPointSizeMin( float min )
{
	//Direct3d11_StateCache::setRenderState(D3DRS_POINTSIZE_MIN,min);
}

// ----------------------------------------------------------------------

void Direct3d11Namespace::setPointSizeMax( float max )
{
	//Direct3d11_StateCache::setRenderState(D3DRS_POINTSIZE_MAX,max);
}

// ----------------------------------------------------------------------

void Direct3d11Namespace::setPointScaleEnable( bool bEnable )
{
	//Direct3d11_StateCache::setRenderState(D3DRS_POINTSCALEENABLE,bEnable);
}

// ----------------------------------------------------------------------

void Direct3d11Namespace::setPointScaleFactor( float A, float B, float C )
{
	/*Direct3d11_StateCache::setRenderState(D3DRS_POINTSCALE_A,A);
	Direct3d11_StateCache::setRenderState(D3DRS_POINTSCALE_B,B);
	Direct3d11_StateCache::setRenderState(D3DRS_POINTSCALE_C,C);*/
}

// ----------------------------------------------------------------------

void Direct3d11Namespace::setPointSpriteEnable( bool bEnable )
{
	//Direct3d11_StateCache::setRenderState(D3DRS_POINTSPRITEENABLE,bEnable);
}

// ----------------------------------------------------------------------

void Direct3d11Namespace::clearViewport(bool clearColor, uint32 colorValue, bool clearDepth, float depthValue, bool clearStencil, uint32 stencilValue)
{
	// d3d will fail the clear call if we try to clear a buffer that's not present
	/*if (!ms_hasDepthBuffer)
		clearDepth = false;
	if (!ms_hasStencilBuffer)
		clearStencil = false;

	const DWORD flags = (clearColor ? D3DCLEAR_TARGET : 0) | (clearDepth ? D3DCLEAR_ZBUFFER : 0) | (clearStencil ? D3DCLEAR_STENCIL : 0);
	const HRESULT hresult = ms_deviceContext->Clear->Clear(0, NULL, flags, colorValue, depthValue, stencilValue);
	FATAL(FAILED(hresult), ("Clear failed %d: %08x %08x %5.2f %08x", HRESULT_CODE(hresult), flags, colorValue, depthValue, stencilValue));*/
}

// ----------------------------------------------------------------------

void Direct3d11Namespace::resizeQuadListIndexBuffer(int numberOfQuads)
{
	if (ms_quadListIndexBufferNumberOfQuads == 0)
	{
		ms_quadListIndexBufferNumberOfQuads = numberOfQuads;
	}
	else
	{
		delete ms_quadListIndexBuffer;
		ms_quadListIndexBuffer = nullptr;

		while (ms_quadListIndexBufferNumberOfQuads < numberOfQuads)
			ms_quadListIndexBufferNumberOfQuads *= 2;
	}

	// create the new index buffer
		
	ms_quadListIndexBuffer = new StaticIndexBuffer(ms_quadListIndexBufferNumberOfQuads * 6);
	
	Index *index = ms_quadListIndexBuffer->begin();
	
	for (int i = 0, base = 0; i < ms_quadListIndexBufferNumberOfQuads; ++i, base += 4)
	{
		*index = static_cast<Index>(base + 0);
		++index;
		*index = static_cast<Index>(base + 1);
		++index;
		*index = static_cast<Index>(base + 2);
		++index;
		*index = static_cast<Index>(base + 0);
		++index;
		*index = static_cast<Index>(base + 2);
		++index;
		*index = static_cast<Index>(base + 3);
		++index;
	}
	ms_quadListIndexBuffer->lock();
	
	ms_quadListIndexBuffer->unlock();
}

// ----------------------------------------------------------------------

void Direct3d11Namespace::update(float elapsedTime)
{
	++ms_frameNumber;
	ms_currentTime += elapsedTime; 

	//const float time[4] = { ms_currentTime, 0.f, 0.f, 0.f };
	//ms_deviceContext->UpdateSubresource(ms_constantBufferTime, 0, nullptr, &time, 0, 0);
	//Direct3d11_StateCache::setVertexShaderConstants(0, ms_constantBufferTime, 1); //VSCR_currentTime
}

// ----------------------------------------------------------------------

void Direct3d11Namespace::beginScene()
{
	if (ms_displayModeChanged && !checkDisplayMode())
		setWindowedMode(false);

	float color[4] = { 0.0f, 0.0f, 0.0f, 1.0f};

	// clear buffers
	ms_deviceContext->ClearRenderTargetView(ms_renderTargetView.Get(), color);
	ms_deviceContext->ClearDepthStencilView(ms_depthStencilView.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

	ms_deviceContext->RSSetState(ms_rasterState.Get());
	ms_deviceContext->OMSetDepthStencilState(ms_depthStencilState.Get(), 0);

	Direct3d11_StaticShaderData::beginFrame();
	Direct3d11_DynamicVertexBufferData::beginFrame();
	Direct3d11_DynamicIndexBufferData::beginFrame();
	Direct3d11_LightManager::beginFrame();
	ms_savedIndexBuffer = nullptr;
}

// ----------------------------------------------------------------------

void Direct3d11Namespace::endScene()
{
	// end the 3d scene
}

// ----------------------------------------------------------------------

bool Direct3d11Namespace::getBackBuffer()
{
	return ms_backBuffer;
}

// ----------------------------------------------------------------------

bool Direct3d11Namespace::lockBackBuffer(Gl_pixelRect &o_pixels, const RECT *i_lockRect)
{
	/*HRESULT hresult;

	if (!ms_backBuffer && !getBackBuffer())
	{
		return false;
	}

	D3DLOCKED_RECT lockedRect;
	hresult = ms_backBuffer->LockRect(&lockedRect, i_lockRect, 0);
	if (FAILED(hresult))
	{
		Graphics::setLastError("engine", "failure_to_lock_backbuffer");
		return false;
	}
	ms_backBufferLocked=true;

	o_pixels.pixels = lockedRect.pBits;
	o_pixels.pitch = lockedRect.Pitch;

	D3DSURFACE_DESC desc;
	hresult = ms_backBuffer->GetDesc(&desc);

	//o_height = desc.Height;

	switch (desc.Format)
	{
	case D3DFMT_A2R10G10B10:
		o_pixels.colorBits=30;
		o_pixels.alphaBits=2;
		break;
	case D3DFMT_A8R8G8B8:
		o_pixels.colorBits=24;
		o_pixels.alphaBits=8;
		break;
	case D3DFMT_A1R5G5B5:
		o_pixels.colorBits=15;
		o_pixels.alphaBits=1;
		break;
	case D3DFMT_R5G6B5:
		o_pixels.colorBits=16;
		o_pixels.alphaBits=0;
		break;
	default:
		FATAL(true, ("Unsupported back buffer pixel format.\n"));
	}
	*/
	return true;
}

// ----------------------------------------------------------------------

bool Direct3d11Namespace::unlockBackBuffer()
{
	if (!ms_backBufferLocked || !ms_backBuffer)
	{
		return false;
	}

	//ms_backBuffer->UnlockRect();
	ms_backBufferLocked=false;
	
	return true;
}

// ----------------------------------------------------------------------

void Direct3d11Namespace::releaseBackBuffer()
{
	if (ms_backBuffer)
	{
		if (ms_backBufferLocked)
		{
			unlockBackBuffer();
		}

		IGNORE_RETURN(ms_backBuffer->Release());
		ms_backBuffer = nullptr;
	}
}

// ----------------------------------------------------------------------

bool Direct3d11Namespace::present(bool windowed, HWND window, int width, int height)
{
	// Present the back buffer to the screen since rendering is complete.
	if(ConfigDirect3d11::getAllowTearing())
	{
		// Lock to screen refresh rate.
		ms_swapChain->Present(1, 0);
	}
	else
	{
		// Present as fast as possible.
		ms_swapChain->Present(0, 0);
	}
	
	/*if (ms_displayModeChanged)
	{
		if (!checkDisplayMode())
			setWindowedMode(false);
		return false;
	}

	// display the new frame (but only do it when we're rendering to the frame buffer rather than a texture)
	HRESULT hresult;
	if (windowed)
	{
		RECT source;
		source.top = 0;
		source.left = 0;
		source.right = width;
		source.bottom = height;

		hresult = ms_device->Present(&source, NULL, window, NULL);
	}
	else
	{
		// flipping requires all the arguments are NULL
		hresult = ms_device->Present(NULL, NULL, NULL, NULL);
	}

	// check if the device was lost for any reason
	if (hresult == D3DERR_DEVICELOST || hresult == D3DERR_DRIVERINTERNALERROR)
	{
		char present[16];
		sprintf(present, "%d", HRESULT_CODE(hresult));

		// check if we can restore the device now
		hresult = ms_device->TestCooperativeLevel();
		if (SUCCEEDED(hresult) || hresult == D3DERR_DEVICENOTRESET)
		{
			char tcl[16];
			sprintf(tcl, "%d", HRESULT_CODE(hresult));

			DEBUG_REPORT_LOG(true, ("Device lost, restoring now\n"));

			lostDevice();

			// try to restore the device
			hresult = ms_device->Reset(&ms_presentParameters);

			// give a better message if we get back INVALIDCALL
			FATAL(hresult == D3DERR_INVALIDCALL, ("Reset failed %d - likely unreleased render target", HRESULT_CODE(hresult)));

			for (int i = 0; hresult == D3DERR_DEVICELOST && i < 60; ++i)
			{
				char present2[16];
				sprintf(present2, "%d", HRESULT_CODE(hresult));
				DEBUG_REPORT_LOG(true, ("Reset failed, trying repeatedly\n"));

				Sleep(500);
				hresult = ms_device->Reset(&ms_presentParameters);
			}

			FATAL_DX_HR("Reset failed after present %s", hresult);

			// recreate necessary resources after resetting the device
			restoreDevice();
			return false;
		}
		else
			if (hresult == D3DERR_DEVICELOST)
			{
				// device is lost, can't restore just yet.  waste some time.
				DEBUG_REPORT_LOG(true, ("Device lost, waiting to restore\n"));
				Sleep(50);
				return false;
			}
			else
			{
				FATAL_DX_HR("TestCooperativeLevel failed %s", hresult);
			}
	}
	else
	{
		FATAL_DX_HR("Present failed %s", hresult);
	}

	ms_deviceReset = false;
	*/
	return true;
}

// ----------------------------------------------------------------------

bool Direct3d11Namespace::present()
{
	return present(ms_windowed, ms_window, ms_width, ms_height);
}

// ----------------------------------------------------------------------

bool Direct3d11Namespace::present(HWND window, int width, int height)
{
	return present(true, window, width, height);
}

// ----------------------------------------------------------------------

void Direct3d11Namespace::setRenderTarget(Texture *texture, CubeFace cubeFace, int mipmapLevel)
{
	if (texture)
		Direct3d11_RenderTarget::setRenderTarget(texture, cubeFace, mipmapLevel);
	else
		Direct3d11_RenderTarget::setRenderTargetToPrimary();
}

// ----------------------------------------------------------------------

bool Direct3d11Namespace::copyRenderTargetToNonRenderTargetTexture()
{
	return Direct3d11_RenderTarget::copyRenderTargetToNonRenderTargetTexture();
}

// ----------------------------------------------------------------------

bool Direct3d11Namespace::applyGammaCorrectionToXRGBSurface( ID3D11Texture2D *surface )
{
	/*if (!surface)
		return false;

	D3DSURFACE_DESC desc;
	surface->GetDesc(&desc);

	// check the format
	if (desc.Format != D3DFMT_A8R8G8B8 && desc.Format != D3DFMT_X8R8G8B8)
		return false;

	D3DLOCKED_RECT lockedRect;

	// lock the surface
	HRESULT hresult = surface->LockRect(&lockedRect, NULL, 0);
	if (FAILED(hresult))
		return false;

	// color correct the bytes
	for( unsigned nLine = 0; nLine != desc.Height; nLine++ )
	{
		// get the start of the line
		PackedArgb * pBuffer = reinterpret_cast<PackedArgb*>(reinterpret_cast<intptr_t>(lockedRect.pBits) + static_cast<intptr_t>(lockedRect.Pitch) * nLine);

		// color correct the line
		PackedArgb * pBufferEol = pBuffer + desc.Width;
		for(;pBuffer != pBufferEol;pBuffer++)
		{
			// color
			pBuffer->setR( ms_colorCorrectionTable[ pBuffer->getR() ] );
			pBuffer->setG( ms_colorCorrectionTable[ pBuffer->getG() ] );
			pBuffer->setB( ms_colorCorrectionTable[ pBuffer->getB() ] );
		}
	}

	// unlock the surface
	hresult = surface->UnlockRect();
	*/
	return true;
}

// ----------------------------------------------------------------------

bool Direct3d11Namespace::screenShot(GlScreenShotFormat format, int quality, const char *fileName)
{
	/*HRESULT hresult;
	IDirect3DSurface9 *surface = NULL;
	POINT clientTopLeft = { 0, 0 };
	RECT monitorCoordinates = { 0, 0, 0, 0 };
	int offset = 0;

	if (ConfigDirect3d11::getScreenShotBackBuffer())
	{
		// I don't feel like handling pixel format conversions, and this is what the game is supposed to be running in anyway.
		if (ms_backBufferFormat != D3DFMT_A8R8G8B8 && ms_backBufferFormat != D3DFMT_X8R8G8B8)
		{
			Graphics::setLastError("engine", "screenshot_failed_wrong_format");
			return false;
		}

		// get access to the back buffer surface
		hresult = ms_device->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &surface);

		// if unable to get access to the back buffer return false
		if (FAILED(hresult)||surface==NULL)
		{
			Graphics::setLastError("engine", "screenshot_failed_unknown");
			return false;
		}
	}
	else
	{
		// find out how big the scratch buffer should be
		D3DDISPLAYMODE displayMode;
		hresult = ms_device->GetDisplayMode(0, &displayMode);
		if (FAILED(hresult))
		{
			Graphics::setLastError("engine", "screenshot_failed_unknown");
			return false;
		}

		// find out where the monitor is anchored
		HMONITOR monitor = ms_direct3d->GetAdapterMonitor(ms_adapter);
		MONITORINFO monitorInfo;
		Zero(monitorInfo);
		monitorInfo.cbSize = sizeof(monitorInfo);
		GetMonitorInfo(monitor, &monitorInfo);
		monitorCoordinates = monitorInfo.rcMonitor;

		// get the window location relative to the monitor
		if (ms_windowed)
			ClientToScreen(ms_window, &clientTopLeft);

		// make sure the entire window is on the screen
		if (ms_windowed)
		{
			if (clientTopLeft.x < static_cast<int>(monitorCoordinates.left) || clientTopLeft.x+ms_width > static_cast<int>(monitorCoordinates.right) || clientTopLeft.y < static_cast<int>(monitorCoordinates.top) || clientTopLeft.y+ms_height > static_cast<int>(monitorCoordinates.bottom))
			{
				Graphics::setLastError("engine", "screenshot_failed_off_desktop");
				return false;
			}

			offset = ((clientTopLeft.x - monitorCoordinates.left) * 4) + ((clientTopLeft.y - monitorCoordinates.top) * displayMode.Width * 4);
		}

		// create a surface for the screen shot
		hresult = ms_device->CreateOffscreenPlainSurface(displayMode.Width, displayMode.Height, D3DFMT_A8R8G8B8, D3DPOOL_SCRATCH, &surface, NULL);
		if (FAILED(hresult) || surface == NULL)
		{
			Graphics::setLastError("engine", "screenshot_failed_unknown");
			return false;
		}

		// copy the front buffer to the surface
		hresult = ms_device->GetFrontBufferData(0, surface);

		// if unable to copy the data return false
		if (FAILED(hresult))
		{
			Graphics::setLastError("engine", "screenshot_failed_unknown");
			surface->Release();
			return false;
		}

		// if not windowed apply gamma correction to the ARGB copy of the front buffer
		if (!ms_windowed)
			applyGammaCorrectionToXRGBSurface( surface );
	}

	switch (format)
	{
		case GSSF_tga:
		{
			// lock the buffer
			D3DLOCKED_RECT lockedRect;
			hresult = surface->LockRect(&lockedRect, NULL, D3DLOCK_READONLY);
			if (FAILED(hresult))
			{
				Graphics::setLastError("engine", "screenshot_failed_unknown");
				surface->Release();
				return false;
			}

			// write it out as a TGA
			char buffer[Os::MAX_PATH_LENGTH];
			sprintf(buffer, "%s.tga", fileName);
			WriteTGA::write(buffer, ms_width, ms_height, reinterpret_cast<const byte *>(lockedRect.pBits) + offset, true, lockedRect.Pitch);

			// unlock the surface
			hresult = surface->UnlockRect();
			FATAL_DX_HR("Unlock failed %s", hresult);
			break;
		}

		case GSSF_bmp:
		{
			char buffer[Os::MAX_PATH_LENGTH];
			sprintf(buffer, "%s.bmp", fileName);

			RECT r;
			r.left   = (clientTopLeft.x - monitorCoordinates.left);
			r.top    = (clientTopLeft.y - monitorCoordinates.top);
			r.right  = r.left + ms_width;
			r.bottom = r.top + ms_height;

			hresult = D3DXSaveSurfaceToFile(buffer, D3DXIFF_BMP, surface, NULL, &r);
			if (FAILED(hresult))
			{
				Graphics::setLastError("engine", "screenshot_failed_write_problem");
				surface->Release();
				return false;
			}
			break;
		}

		case GSSF_jpg:
		{
			// get access to the pixels
			D3DLOCKED_RECT lockedRect;
			hresult = surface->LockRect(&lockedRect, NULL, D3DLOCK_READONLY);
			if (FAILED(hresult))
			{
				Graphics::setLastError("engine", "screenshot_failed_unknown");
				surface->Release();
				return false;
			}
			byte const *start = reinterpret_cast<byte const *>(lockedRect.pBits) + offset;

			// open the output file
			char buffer[Os::MAX_PATH_LENGTH];
			sprintf(buffer, "%s.jpg", fileName);
			FILE *outputFile = fopen(buffer, "wb");
			if (outputFile == NULL)
			{
				Graphics::setLastError("engine", "screenshot_failed_write_problem");
				return false;
			}

			// setup the jpeg compressions
			jpeg_compress_struct cinfo;
			jpeg_error_mgr jerr;
			cinfo.err = jpeg_std_error(&jerr);
			jpeg_create_compress(&cinfo);
			jpeg_stdio_dest(&cinfo, outputFile);
			cinfo.image_width = ms_width;
			cinfo.image_height = ms_height;
			cinfo.input_components = 3;
			cinfo.in_color_space = JCS_RGB;
			jpeg_set_defaults(&cinfo);
			if (quality > 0)
				jpeg_set_quality(&cinfo, clamp(0, quality, 100), TRUE);
			jpeg_start_compress(&cinfo, TRUE);

			// add the pixels to the jpeg image
			byte *scanLine = new byte[ms_width * 3];
			for (int y = 0; y < ms_height; ++y, start += lockedRect.Pitch)
			{
				uint32 const * source = reinterpret_cast<uint32 const *>(start);
				for (int x = 0, b = 0; x < ms_width; ++x)
				{
					uint32 pixel = source[x];
					scanLine[b++] = static_cast<byte>((pixel >> 16) & 0xff);
					scanLine[b++] = static_cast<byte>((pixel >>  8) & 0xff);
					scanLine[b++] = static_cast<byte>((pixel >>  0) & 0xff);
				}

				jpeg_write_scanlines(&cinfo, &scanLine, 1);
			}

			// done compressing the JPEG image, time to clean up
			delete [] scanLine;
			jpeg_finish_compress(&cinfo);
			jpeg_destroy_compress(&cinfo);

			fclose(outputFile);

			// unlock the surface
			hresult = surface->UnlockRect();
			FATAL_DX_HR("Unlock failed %s", hresult);

			break;
		}

		default:
		{
			Graphics::setLastError("engine", "screenshot_failed_unknown_format");
			surface->Release();
			return false;
		}
	}

	surface->Release();*/
	return true;
}

//-----------------------------------------------------------------------

TextureGraphicsData *Direct3d11Namespace::createTextureData(const Texture &texture, const TextureFormat *runtimeFormats, int numberOfRuntimeFormats)
{
	return new Direct3d11_TextureData(texture, runtimeFormats, numberOfRuntimeFormats);
}

// ----------------------------------------------------------------------

void Direct3d11Namespace::setBadVertexShaderStaticShader(const StaticShader *shader)
{
	ms_badVertexShaderStaticShader = shader;
}

// ----------------------------------------------------------------------

void Direct3d11::setStaticShader(const StaticShader &shader, int pass)
{
	/*const Direct3d11_StaticShaderData *d3dShader = static_cast<const Direct3d11_StaticShaderData *>(shader.m_graphicsData);
	IGNORE_RETURN(d3dShader->apply(pass));

	Direct3d11_LightManager::setObeysLightScale(shader.obeysLightScale());*/
}

// ----------------------------------------------------------------------

void Direct3d11::convertTransformToMatrix(const Transform &transform, DirectX::XMFLOAT4X4 & matrix)
{
	matrix._11 = transform.matrix[0][0];
	matrix._12 = transform.matrix[0][1];
	matrix._13 = transform.matrix[0][2];
	matrix._14 = transform.matrix[0][3];

	matrix._21 = transform.matrix[1][0];
	matrix._22 = transform.matrix[1][1];
	matrix._23 = transform.matrix[1][2];
	matrix._24 = transform.matrix[1][3];

	matrix._31 = transform.matrix[2][0];
	matrix._32 = transform.matrix[2][1];
	matrix._33 = transform.matrix[2][2];
	matrix._34 = transform.matrix[2][3];
}

// ----------------------------------------------------------------------

void Direct3d11::convertScaleAndTransformToMatrix(const Vector &scale, const Transform &transform, DirectX::XMFLOAT4X4 & matrix)
{
	matrix._11 = transform.matrix[0][0] * scale.x;
	matrix._12 = transform.matrix[0][1] * scale.x;
	matrix._13 = transform.matrix[0][2] * scale.x;
	matrix._14 = transform.matrix[0][3];

	matrix._21 = transform.matrix[1][0] * scale.y;
	matrix._22 = transform.matrix[1][1] * scale.y;
	matrix._23 = transform.matrix[1][2] * scale.y;
	matrix._24 = transform.matrix[1][3];

	matrix._31 = transform.matrix[2][0] * scale.z;
	matrix._32 = transform.matrix[2][1] * scale.z;
	matrix._33 = transform.matrix[2][2] * scale.z;
	matrix._34 = transform.matrix[2][3];
}

// ----------------------------------------------------------------------

bool Direct3d11Namespace::setMouseCursor(Texture const & mouseCursorTexture, int hotSpotX, int hotSpotY)
{
	/*if (GetForegroundWindow() == ms_window)
	{
		Direct3d11_TextureData const * textureData = safe_cast<Direct3d11_TextureData const *>(mouseCursorTexture.getGraphicsData());
		ID3D11Texture2D * baseTexture = textureData->getBaseTexture();

		ID3D11Texture2D * surface = NULL;
		HRESULT hresult = static_cast<ID3D11Texture2D *>(baseTexture)->GetSurfaceLevel(0, &surface);
		//FATAL_DX_HR("Could not get top surface %s", hresult);

		//hresult = ms_device->SetCursorProperties(hotSpotX, hotSpotY, surface);
		//FATAL_DX_HR("Could not set cursor properties %s", hresult);
		surface->Release();

		return true;
	}
	*/
	return false;
}

// ----------------------------------------------------------------------

bool Direct3d11Namespace::showMouseCursor(bool enabled)
{
	if (GetForegroundWindow() == ms_window)
	{
		ShowCursor(enabled);

		return true;
	}

	return false;
}

// ----------------------------------------------------------------------

void Direct3d11Namespace::setViewport(int x, int y, int viewportWidth, int viewportHeight, float minZ, float maxZ)
{
	// cache these for device lost
	ms_viewportX            = x;
	ms_viewportY            = y;
	ms_viewportWidth        = viewportWidth;
	ms_viewportHeight       = viewportHeight;
	ms_viewportMinimumZ     = minZ;
	ms_viewportMaximumZ     = maxZ;

	// setup the viewport for this camera
	D3D11_VIEWPORT   viewport;
	viewport.TopLeftX      = x;
	viewport.TopLeftY      = y;
	viewport.Width  = viewportWidth;
	viewport.Height = viewportHeight;
	viewport.MinDepth   = minZ;
	viewport.MaxDepth   = maxZ;
	ms_deviceContext->RSSetViewports(1,&viewport);

	// let the vertex shader know this information for 2d stuff
	float xOffset = ((x * 2.0f) / static_cast<float>(viewportWidth));
	float yOffset = ((y * 2.0f) / static_cast<float>(viewportHeight));
	float const viewportData[4] = { 2.0f / static_cast<float>(viewportWidth), -2.0f / static_cast<float>(viewportHeight), -1.0f - xOffset,  1.0f + yOffset };

	ms_deviceContext->UpdateSubresource(ms_constantBufferViewport, 0, nullptr, &viewportData, 0, 0);
	Direct3d11_StateCache::setVertexShaderConstants(VSCR_viewportData, ms_constantBufferViewport, 1);
}

// ----------------------------------------------------------------------

void Direct3d11Namespace::setScissorRect(bool enabled, int x, int y, int width, int height)
{
	//Direct3d11_StateCache::setRenderState(D3DRS_SCISSORTESTENABLE, enabled);
	if (enabled)
	{
		RECT const r = { x, y, x+width, y+height };
		//HRESULT const hresult = ms_device->SetScissorRect(&r);
		//FATAL_DX_HR("SetSissorRect failed %s", hresult);
	}
}

// ----------------------------------------------------------------------

void Direct3d11Namespace::setWorldToCameraTransform(const Transform &transform, const Vector &cameraPosition)
{
	Direct3d11_LightManager::setCameraPosition(cameraPosition);

	Direct3d11::convertTransformToMatrix(transform, ms_cachedWorldToCameraMatrix);

	PaddedVector paddedPosition(cameraPosition);

	ms_deviceContext->UpdateSubresource(ms_constantBufferCameraPosition, 0, nullptr, &paddedPosition, 0, 0);
	Direct3d11_StateCache::setVertexShaderConstants(VSCR_cameraPosition, ms_constantBufferCameraPosition, 1);

	DirectX::XMMATRIX proj = XMLoadFloat4x4( &ms_cachedProjectionMatrix );
	DirectX::XMMATRIX world = XMLoadFloat4x4( &ms_cachedWorldToCameraMatrix );
	
	XMStoreFloat4x4(&ms_cachedWorldToProjectionMatrix, XMMatrixMultiply(proj, world));
}

// ----------------------------------------------------------------------

void Direct3d11Namespace::setProjectionMatrix(const GlMatrix4x4 &projectionMatrix)
{
	ms_cachedProjectionMatrix._11 = projectionMatrix.matrix[0][0];
	ms_cachedProjectionMatrix._12 = projectionMatrix.matrix[0][1];
	ms_cachedProjectionMatrix._13 = projectionMatrix.matrix[0][2];
	ms_cachedProjectionMatrix._14 = projectionMatrix.matrix[0][3];

	ms_cachedProjectionMatrix._21 = projectionMatrix.matrix[1][0];
	ms_cachedProjectionMatrix._22 = projectionMatrix.matrix[1][1];
	ms_cachedProjectionMatrix._23 = projectionMatrix.matrix[1][2];
	ms_cachedProjectionMatrix._24 = projectionMatrix.matrix[1][3];

	ms_cachedProjectionMatrix._31 = projectionMatrix.matrix[2][0];
	ms_cachedProjectionMatrix._32 = projectionMatrix.matrix[2][1];
	ms_cachedProjectionMatrix._33 = projectionMatrix.matrix[2][2];
	ms_cachedProjectionMatrix._34 = projectionMatrix.matrix[2][3];

	ms_cachedProjectionMatrix._41 = projectionMatrix.matrix[3][0];
	ms_cachedProjectionMatrix._42 = projectionMatrix.matrix[3][1];
	ms_cachedProjectionMatrix._43 = projectionMatrix.matrix[3][2];
	ms_cachedProjectionMatrix._44 = projectionMatrix.matrix[3][3];

	DirectX::XMMATRIX proj = XMLoadFloat4x4( &ms_cachedProjectionMatrix );
	DirectX::XMMATRIX world = XMLoadFloat4x4( &ms_cachedWorldToCameraMatrix );
	
	XMStoreFloat4x4(&ms_cachedWorldToProjectionMatrix, XMMatrixMultiply(proj, world));
}

// ----------------------------------------------------------------------

void Direct3d11Namespace::setFog(bool enabled, float density, const PackedArgb &color)
{
	if (enabled)
	{
		ms_fogColor = color.getArgb();

		/*Direct3d11_StateCache::setRenderState(D3DRS_FOGENABLE, TRUE);
		Direct3d11_StateCache::setRenderState(D3DRS_FOGCOLOR, ms_fogColor);
		Direct3d11_StateCache::setRenderState(D3DRS_FOGVERTEXMODE, D3DFOG_EXP2);*/
		
		const float fog[4] = { 0.0f, 0.0f, density, sqr(density) };

		ms_deviceContext->UpdateSubresource(ms_constantBufferFog, 0, nullptr, &fog, 0, 0);
		Direct3d11_StateCache::setVertexShaderConstants(VSCR_fog, ms_constantBufferFog, 1);
	}
	else
	{
		//Direct3d11_StateCache::setRenderState(D3DRS_FOGENABLE, FALSE);
	}
}

// ----------------------------------------------------------------------

void Direct3d11Namespace::setObjectToWorldTransformAndScale(const Transform &transform, const Vector &scale)
{
	ms_transformDirty = true;
	Direct3d11::convertScaleAndTransformToMatrix(scale, transform, ms_cachedObjectToWorldMatrix);

	Direct3d11_LightManager::setObjectToWorldTransform(transform);
}

// ----------------------------------------------------------------------

void Direct3d11Namespace::setGlobalTexture(Tag tag, const Texture &texture)
{
	Direct3d11_TextureData::setGlobalTexture(tag, texture);
}

// ----------------------------------------------------------------------

void Direct3d11Namespace::releaseAllGlobalTextures()
{
	Direct3d11_TextureData::releaseAllGlobalTextures();
}

// ----------------------------------------------------------------------

void Direct3d11Namespace::setTextureTransform(int stage, bool enabled, int dimension, bool projected, const float *transform)
{
	UNREF(stage);
	UNREF(enabled);
	UNREF(dimension);
	UNREF(projected);
	UNREF(transform);
}

// ----------------------------------------------------------------------

void Direct3d11Namespace::setVertexShaderUserConstants(int index, float c0, float c1, float c2, float c3)
{
	UNREF(index);
	UNREF(c0);
	UNREF(c1);
	UNREF(c2);
	UNREF(c3);

	const float constants[4] = { c0, c1, c2, c3 };
	//ms_deviceContext->UpdateSubresource(ms_constantBufferUserConstants, 0, nullptr, &constants, 0, 0);
	//Direct3d11_StateCache::setVertexShaderConstants(VCSR_userConstant0 + index, ms_constantBufferUserConstants, 1);
}

// ----------------------------------------------------------------------

void Direct3d11Namespace::setPixelShaderUserConstants(VectorRgba const * constants, int count)
{
	//Direct3d11_StateCache::setPixelShaderConstants(PSCR_userConstant, constants, count);
}

// ----------------------------------------------------------------------

ShaderImplementationGraphicsData *Direct3d11Namespace::createShaderImplementationGraphicsData(const ShaderImplementation &shaderImplementation)
{
	return new Direct3d11_ShaderImplementationData(shaderImplementation);
}

// ----------------------------------------------------------------------

StaticShaderGraphicsData *Direct3d11Namespace::createStaticShaderGraphicsData(const StaticShader &shader)
{
	return new Direct3d11_StaticShaderData(shader);
}

// ----------------------------------------------------------------------

ShaderImplementationPassVertexShaderGraphicsData *Direct3d11Namespace::createVertexShaderData(ShaderImplementationPassVertexShader const &vertexShader)
{
	return new Direct3d11_VertexShaderData(vertexShader);
}

// ----------------------------------------------------------------------

ShaderImplementationPassPixelShaderProgramGraphicsData *Direct3d11Namespace::createPixelShaderProgramData(ShaderImplementationPassPixelShaderProgram const &pixelShaderProgram)
{
	return new Direct3d11_PixelShaderProgramData(pixelShaderProgram);
}

// ----------------------------------------------------------------------

void Direct3d11::setAlphaBlendEnable(bool alphaBlendEnable)
{
	ms_alphaBlendEnable = alphaBlendEnable;
}

// ----------------------------------------------------------------------

void Direct3d11::setAlphaTestReferenceValue(uint8 alphaTestReferenceValue)
{
	ms_alphaTestReferenceValue = alphaTestReferenceValue;
}

// ----------------------------------------------------------------------

void Direct3d11::setColorWriteEnable(uint8 colorWriteEnable)
{
	ms_colorWriteEnable = colorWriteEnable;
}

// ----------------------------------------------------------------------

void Direct3d11Namespace::setAlphaFadeOpacity(bool enabled, float opacity)
{
	if (ms_alphaFadeOpacityEnabled != enabled)
	{
		ms_alphaFadeOpacityEnabled = enabled;
		ms_alphaFadeOpacity.r = ms_alphaFadeOpacityEnabled ? 1.0f : 0.0f;
		ms_alphaFadeOpacityDirty = true;
	}

	if (opacity != ms_alphaFadeOpacity.a)
	{
		ms_alphaFadeOpacity.a = opacity;
		ms_alphaFadeOpacityDirty = true;
	}
}

// ----------------------------------------------------------------------

void Direct3d11Namespace::noSetAlphaFadeOpacity(bool, float)
{
}

// ----------------------------------------------------------------------

void Direct3d11Namespace::setLights(const stdvector<const Light*>::fwd &lightList)
{
	//Direct3d11_LightManager::setLights(lightList);
}

// ----------------------------------------------------------------------

StaticVertexBufferGraphicsData *Direct3d11Namespace::createVertexBufferData(const StaticVertexBuffer &vertexBuffer)
{
	return new Direct3d11_StaticVertexBufferData(vertexBuffer);
}

// ----------------------------------------------------------------------

DynamicVertexBufferGraphicsData *Direct3d11Namespace::createVertexBufferData(const DynamicVertexBuffer &vertexBuffer)
{
	return new Direct3d11_DynamicVertexBufferData(vertexBuffer);
}

// ----------------------------------------------------------------------

VertexBufferVectorGraphicsData *Direct3d11Namespace::createVertexBufferVectorData(VertexBufferVector const & vertexBufferVector)
{
	return new Direct3d11_VertexBufferVectorData(vertexBufferVector);
}

// ----------------------------------------------------------------------

StaticIndexBufferGraphicsData *Direct3d11Namespace::createIndexBufferData(const StaticIndexBuffer &indexBuffer)
{
	return new Direct3d11_StaticIndexBufferData(indexBuffer);
}

// ----------------------------------------------------------------------

DynamicIndexBufferGraphicsData *Direct3d11Namespace::createIndexBufferData()
{
	return new Direct3d11_DynamicIndexBufferData();
}

// ----------------------------------------------------------------------

void Direct3d11Namespace::setDynamicIndexBufferSize(int numberOfIndices)
{
	Direct3d11_DynamicIndexBufferData::setSize(numberOfIndices);
}

// ----------------------------------------------------------------------

void Direct3d11Namespace::getOneToOneUVMapping(int textureWidth, int textureHeight, float &u0, float &v0, float &u1, float &v1)
{
	u0 = (0.0f + 0.5f) / static_cast<float>(textureWidth);
	v0 = (0.0f + 0.5f) / static_cast<float>(textureHeight);
	u1 = (static_cast<float>(textureWidth - 1) + 0.5f) / static_cast<float>(textureWidth);
	v1 = (static_cast<float>(textureHeight - 1) + 0.5f) / static_cast<float>(textureHeight);
}

// ----------------------------------------------------------------------

void Direct3d11::setVertexBuffer(const HardwareVertexBuffer & vertexBuffer)
{
	ID3D11Buffer *				  vb                = 0;
	DWORD                         byteOffset        = 0;
	DWORD                         vertexSize        = 0;
	ID3D11InputLayout			* vertexDeclaration = 0;

	switch (vertexBuffer.getType())
	{
		case HardwareVertexBuffer::T_static:
			{
				const StaticVertexBuffer *staticVertexBuffer = safe_cast<const StaticVertexBuffer *>(&vertexBuffer);
				const Direct3d11_StaticVertexBufferData *data = safe_cast<Direct3d11_StaticVertexBufferData *>(staticVertexBuffer->m_graphicsData);

				vb  = data->getVertexBuffer();
				vertexSize = data->getVertexSize();
				vertexDeclaration = data->getVertexDeclaration();

				// remember the number of vertices for the draw calls
				ms_sliceNumberOfVertices = staticVertexBuffer->getNumberOfVertices();
				ms_sliceFirstVertex      = 0;
			}
			break;

		case HardwareVertexBuffer::T_dynamic:
			{
				const DynamicVertexBuffer *dynamicVertexBuffer = safe_cast<const DynamicVertexBuffer *>(&vertexBuffer);
				const Direct3d11_DynamicVertexBufferData *data = safe_cast<Direct3d11_DynamicVertexBufferData *>(dynamicVertexBuffer->m_graphicsData);

				vb  = data->getVertexBuffer();
				vertexSize = data->getVertexSize();
				vertexDeclaration = data->getVertexDeclaration();

				// remember the number of vertices for the draw calls
				ms_sliceNumberOfVertices = data->getNumberOfVertices();
				ms_sliceFirstVertex      = data->getOffset();
			}
			break;

		default:
			DEBUG_FATAL(true, ("Unknown VB type"));
	}

	// set the VB declaration
	Direct3d11_StateCache::setVertexDeclaration(vertexDeclaration);

	// set the vertex buffer stream
	Direct3d11_StateCache::setStreamSource(0, vb, byteOffset, vertexSize);
	
	for (int i = 1; i < ms_lastVertexBufferCount; ++i)
		Direct3d11_StateCache::setStreamSource(i, nullptr, 0, 0);
	
	ms_lastVertexBufferCount = 1;
}

// ----------------------------------------------------------------------

void Direct3d11::setVertexBufferVector(const VertexBufferVector & vertexBufferVector)
{
	ms_sliceFirstVertex = 0;

	int stream = 0;
	bool staticBufferFound=false;
	bool dynamicBufferFound=false;
	VertexBufferVector::VertexBufferList const & vertexBufferList = *(vertexBufferVector.m_vertexBufferList);
	VertexBufferVector::VertexBufferList::const_iterator const iEnd = vertexBufferList.end();
	for (VertexBufferVector::VertexBufferList::const_iterator i = vertexBufferList.begin(); i != iEnd; ++i, ++stream)
	{
		ID3D11Buffer  *vb         = nullptr;
		DWORD                    byteOffset = 0;
		DWORD                    vertexSize = 0;

		HardwareVertexBuffer const & vertexBuffer = **i;

		switch (vertexBuffer.getType())
		{
			case HardwareVertexBuffer::T_static:
				{
					staticBufferFound=true;

					const StaticVertexBuffer *staticVertexBuffer = safe_cast<const StaticVertexBuffer *>(&vertexBuffer);
					const Direct3d11_StaticVertexBufferData *data = safe_cast<Direct3d11_StaticVertexBufferData *>(staticVertexBuffer->m_graphicsData);

					vb  = data->getVertexBuffer();
					vertexSize = data->getVertexSize();

					// remember the number of vertices for the draw calls
					ms_sliceNumberOfVertices = staticVertexBuffer->getNumberOfVertices();
					ms_sliceFirstVertex      = 0;
				}
				break;

			case HardwareVertexBuffer::T_dynamic:
				{
					dynamicBufferFound=true;

					const DynamicVertexBuffer *dynamicVertexBuffer = safe_cast<const DynamicVertexBuffer *>(&vertexBuffer);
					const Direct3d11_DynamicVertexBufferData *data = safe_cast<Direct3d11_DynamicVertexBufferData *>(dynamicVertexBuffer->m_graphicsData);

					vb  = data->getVertexBuffer();
					vertexSize = data->getVertexSize();

					// remember the number of vertices for the draw calls
					ms_sliceNumberOfVertices = data->getNumberOfVertices();
					if (ms_supportsStreamOffsets)
					{
						byteOffset = data->getOffset() * vertexSize;
					}
					else
					{
						ms_sliceFirstVertex = data->getOffset();
					}
				}
				break;

			default:
				DEBUG_FATAL(true, ("Unknown VB type"));
		}

		Direct3d11_StateCache::setStreamSource(stream, vb, byteOffset, vertexSize);
	}

	Direct3d11_VertexBufferVectorData *data = safe_cast<Direct3d11_VertexBufferVectorData *>(vertexBufferVector.m_graphicsData);
	
	ms_deviceContext->IASetInputLayout(data->getVertexDeclaration());

	// clear the remaining vertex buffer streams
	{
		for (int i = stream; i < ms_lastVertexBufferCount; ++i)
			Direct3d11_StateCache::setStreamSource(i, 0, 0, 0);
		ms_lastVertexBufferCount = stream;
	}
}

// ----------------------------------------------------------------------

void Direct3d11::setIndexBuffer(const HardwareIndexBuffer &indexBuffer)
{
	// get the index buffer
	if (indexBuffer.getType() == HardwareIndexBuffer::T_static)
	{
		const StaticIndexBuffer *sib = safe_cast<const StaticIndexBuffer *>(&indexBuffer);
		const Direct3d11_StaticIndexBufferData *ib = safe_cast<const Direct3d11_StaticIndexBufferData *>(sib->m_graphicsData);
		NOT_NULL(ib);

		// remember stuff about the indices
		ms_savedIndexBuffer     = ib->getIndexBuffer();
		ms_sliceNumberOfIndices = sib->getNumberOfIndices();
		ms_sliceFirstIndex      = 0;
	}
	else
	{
		const DynamicIndexBuffer *dib = safe_cast<const DynamicIndexBuffer *>(&indexBuffer);
		const Direct3d11_DynamicIndexBufferData *ib = safe_cast<const Direct3d11_DynamicIndexBufferData *>(dib->m_graphicsData);
		NOT_NULL(ib);

		// remember stuff about the indices
		ms_savedIndexBuffer     = Direct3d11_DynamicIndexBufferData::getIndexBuffer();
		ms_sliceNumberOfIndices = ib->getNumberOfIndices();
		ms_sliceFirstIndex      = ib->getOffset();
	}

	Direct3d11_StateCache::setIndexBuffer(ms_savedIndexBuffer);
}

// ----------------------------------------------------------------------

inline bool Direct3d11::drawPrimitive()
{
	Direct3d11_LightManager::selectLights();

	//Direct3d11_StateCache::setRenderState(D3DRS_FOGVERTEXMODE, D3DFOG_NONE);

	if (ms_transformDirty)
	{
		DirectX::XMFLOAT4X4 matrices[2];
		DirectX::XMMATRIX proj = XMLoadFloat4x4( &ms_cachedProjectionMatrix );
		DirectX::XMMATRIX world = XMLoadFloat4x4( &ms_cachedWorldToCameraMatrix );
	
		XMStoreFloat4x4(&matrices[0], XMMatrixMultiply(proj, world));
		
		matrices[1] = ms_cachedObjectToWorldMatrix;

		//ms_deviceContext->UpdateSubresource(ms_constantBufferObjectWorldCameraPM, 0, nullptr, &matrices, 0, 0);
		//Direct3d11_StateCache::setVertexShaderConstants(VSCR_objectWorldCameraProjectionMatrix, ms_constantBufferObjectWorldCameraPM, 8);
		ms_transformDirty = false;
	}

/*	if (ms_alphaFadeOpacityEnabled)
	{
		if (ms_alphaBlendEnable)
			Direct3d11_StateCache::setRenderState(D3DRS_COLORWRITEENABLE, ms_colorWriteEnable);
		else
			Direct3d11_StateCache::setRenderState(D3DRS_COLORWRITEENABLE, ms_colorWriteEnable & ~D3DCOLORWRITEENABLE_ALPHA);
		Direct3d11_StateCache::setRenderState(D3DRS_ALPHABLENDENABLE, true);
		Direct3d11_StateCache::setRenderState(D3DRS_ALPHAREF, static_cast<DWORD>(static_cast<float>(ms_alphaTestReferenceValue) * ms_alphaFadeOpacity.a));
	}
	else
	{
		Direct3d11_StateCache::setRenderState(D3DRS_COLORWRITEENABLE, ms_colorWriteEnable);
		Direct3d11_StateCache::setRenderState(D3DRS_ALPHABLENDENABLE, ms_alphaBlendEnable);
		Direct3d11_StateCache::setRenderState(D3DRS_ALPHAREF, ms_alphaTestReferenceValue);
	}*/

	return true;
}

// ----------------------------------------------------------------------

inline void Direct3d11::drawPrimitive(int startVertex, int primitiveCount)
{
	if (drawPrimitive())
	{
		ms_deviceContext->Draw(primitiveCount, ms_sliceFirstVertex + startVertex);
	}
}

// ----------------------------------------------------------------------

inline void Direct3d11::drawIndexedPrimitive(int baseIndex, int minimumVertexIndex, int numberOfVertices, int startIndex, int primitiveCount, int numberOfIndices)
{
	UNREF(numberOfIndices);

	if (drawPrimitive())
	{
		DEBUG_FATAL(!ms_savedIndexBuffer, ("No saved index buffer"));
		
		ms_deviceContext->DrawIndexed(numberOfVertices, ms_sliceFirstIndex + startIndex,  ms_sliceFirstVertex + baseIndex);
	}
}

// ----------------------------------------------------------------------

void Direct3d11Namespace::drawPointList()
{
	ms_deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);
	Direct3d11::drawPrimitive(0, ms_sliceNumberOfVertices);
}

// ----------------------------------------------------------------------

void Direct3d11Namespace::drawLineList()
{
	ms_deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
	Direct3d11::drawPrimitive(0, ms_sliceNumberOfVertices / 2);
}

// ----------------------------------------------------------------------

void Direct3d11Namespace::drawLineStrip()
{
	ms_deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP);
	Direct3d11::drawPrimitive(0, ms_sliceNumberOfVertices - 1);
}

// ----------------------------------------------------------------------

void Direct3d11Namespace::drawTriangleList()
{
	ms_deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	Direct3d11::drawPrimitive(0, ms_sliceNumberOfVertices / 3);
}

// ----------------------------------------------------------------------

void Direct3d11Namespace::drawTriangleStrip()
{
	ms_deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
	Direct3d11::drawPrimitive(0, ms_sliceNumberOfVertices - 2);
}

// ----------------------------------------------------------------------

void Direct3d11Namespace::drawTriangleFan()
{
	//ms_deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY);
	//Direct3d11::drawPrimitive(D3DPT_TRIANGLEFAN, 0, ms_sliceNumberOfVertices - 2);
}

// ----------------------------------------------------------------------

void Direct3d11Namespace::drawQuadList()
{
	int const numberOfQuads = ms_sliceNumberOfVertices / 4;
	int const numberOfTriangles = numberOfQuads * 2;

	// make sure the index buffer is large enough, and set it
	if (numberOfQuads > ms_quadListIndexBufferNumberOfQuads)
		resizeQuadListIndexBuffer(numberOfQuads);
	Direct3d11::setIndexBuffer(*ms_quadListIndexBuffer);

	ms_deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	Direct3d11::drawIndexedPrimitive(0, 0, ms_sliceNumberOfVertices, 0, numberOfTriangles, numberOfTriangles * 3);
}

// ----------------------------------------------------------------------

void Direct3d11Namespace::drawIndexedPointList()
{
	ms_deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);
	Direct3d11::drawIndexedPrimitive(0, 0, ms_sliceNumberOfVertices, 0, ms_sliceNumberOfIndices, ms_sliceNumberOfIndices);
}

// ----------------------------------------------------------------------

void Direct3d11Namespace::drawIndexedLineList()
{
	ms_deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
	Direct3d11::drawIndexedPrimitive(0, 0, ms_sliceNumberOfVertices, 0, ms_sliceNumberOfIndices / 2, ms_sliceNumberOfIndices);
}

// ----------------------------------------------------------------------

void Direct3d11Namespace::drawIndexedLineStrip()
{
	ms_deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP);
	Direct3d11::drawIndexedPrimitive(0, 0, ms_sliceNumberOfVertices, 0, ms_sliceNumberOfIndices - 1, ms_sliceNumberOfIndices);
}

// ----------------------------------------------------------------------

void Direct3d11Namespace::drawIndexedTriangleList()
{
	ms_deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	Direct3d11::drawIndexedPrimitive(0, 0, ms_sliceNumberOfVertices, 0, ms_sliceNumberOfIndices / 3, ms_sliceNumberOfIndices);
}

// ----------------------------------------------------------------------

void Direct3d11Namespace::drawIndexedTriangleStrip()
{
	ms_deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
	Direct3d11::drawIndexedPrimitive(0, 0, ms_sliceNumberOfVertices, 0, ms_sliceNumberOfIndices - 2, ms_sliceNumberOfIndices);
}

// ----------------------------------------------------------------------

void Direct3d11Namespace::drawIndexedTriangleFan()
{
	//Direct3d11::drawIndexedPrimitive(D3DPT_TRIANGLEFAN, 0, 0, ms_sliceNumberOfVertices, 0, ms_sliceNumberOfIndices - 2, ms_sliceNumberOfIndices);
}

// ----------------------------------------------------------------------

void Direct3d11Namespace::drawPointList(int startVertex, int primitiveCount)
{
	ms_deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);
	Direct3d11::drawPrimitive(startVertex, primitiveCount);
}

// ----------------------------------------------------------------------

void Direct3d11Namespace::drawLineList(int startVertex, int primitiveCount)
{
	ms_deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
	Direct3d11::drawPrimitive(startVertex, primitiveCount);
}

// ----------------------------------------------------------------------

void Direct3d11Namespace::drawLineStrip(int startVertex, int primitiveCount)
{
	ms_deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP);
	Direct3d11::drawPrimitive(startVertex, primitiveCount);
}

// ----------------------------------------------------------------------

void Direct3d11Namespace::drawTriangleList(int startVertex, int primitiveCount)
{
	ms_deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	Direct3d11::drawPrimitive(startVertex, primitiveCount);
}

// ----------------------------------------------------------------------

void Direct3d11Namespace::drawTriangleStrip(int startVertex, int primitiveCount)
{
	ms_deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
	Direct3d11::drawPrimitive(startVertex, primitiveCount);
}

// ----------------------------------------------------------------------

void Direct3d11Namespace::drawTriangleFan(int startVertex, int primitiveCount)
{
	//Direct3d11::drawPrimitive(D3DPT_TRIANGLEFAN, startVertex, primitiveCount);
}

// ----------------------------------------------------------------------

void Direct3d11Namespace::drawIndexedPointList(int baseIndex, int minimumVertexIndex, int numberOfVertices, int startIndex, int primitiveCount)
{
	ms_deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);
	Direct3d11::drawIndexedPrimitive(baseIndex, minimumVertexIndex, numberOfVertices, startIndex, primitiveCount, primitiveCount);
}

// ----------------------------------------------------------------------

void Direct3d11Namespace::drawIndexedLineList(int baseIndex, int minimumVertexIndex, int numberOfVertices, int startIndex, int primitiveCount)
{
	ms_deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
	Direct3d11::drawIndexedPrimitive(baseIndex, minimumVertexIndex, numberOfVertices, startIndex, primitiveCount, primitiveCount * 2);
}

// ----------------------------------------------------------------------

void Direct3d11Namespace::drawIndexedLineStrip(int baseIndex, int minimumVertexIndex, int numberOfVertices, int startIndex, int primitiveCount)
{
	ms_deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP);
	Direct3d11::drawIndexedPrimitive(baseIndex, minimumVertexIndex, numberOfVertices, startIndex, primitiveCount, primitiveCount+1);
}

// ----------------------------------------------------------------------

void Direct3d11Namespace::drawIndexedTriangleList(int baseIndex, int minimumVertexIndex, int numberOfVertices, int startIndex, int primitiveCount)
{
	ms_deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	Direct3d11::drawIndexedPrimitive(baseIndex, minimumVertexIndex, numberOfVertices, startIndex, primitiveCount, primitiveCount * 3);
}

// ----------------------------------------------------------------------

void Direct3d11Namespace::drawIndexedTriangleStrip(int baseIndex, int minimumVertexIndex, int numberOfVertices, int startIndex, int primitiveCount)
{
	ms_deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
	Direct3d11::drawIndexedPrimitive(baseIndex, minimumVertexIndex, numberOfVertices, startIndex, primitiveCount, primitiveCount + 2);
}

// ----------------------------------------------------------------------

void Direct3d11Namespace::drawIndexedTriangleFan(int baseIndex, int minimumVertexIndex, int numberOfVertices, int startIndex, int primitiveCount)
{
	//Direct3d11::drawIndexedPrimitive(D3DPT_TRIANGLEFAN, baseIndex, minimumVertexIndex, numberOfVertices, startIndex, primitiveCount, primitiveCount + 2);
}

// ----------------------------------------------------------------------

int Direct3d11Namespace::getMaximumVertexBufferStreamCount()
{
	/*if (ms_deviceCaps.MaxStreams < 1)
		return 1;

	return static_cast<int>(ms_deviceCaps.MaxStreams);*/
	return 1;
}

// ----------------------------------------------------------------------

void *Direct3d11::getTemporaryBuffer(int size)
{
	if (ms_temporaryBufferSize < size)
	{
		ms_temporaryBufferSize = size;
		delete ms_temporaryBuffer;
		ms_temporaryBuffer = operator new(ms_temporaryBufferSize);
	}

	return ms_temporaryBuffer;
}

VectorRgba const & Direct3d11::getAlphaFadeAndBloomSettings()
{
	return ms_alphaFadeOpacity;
}

// ----------------------------------------------------------------------

void Direct3d11Namespace::getOtherAdapterRects(std::vector<RECT> &otherAdapterRects)
{
	/*otherAdapterRects.clear();

	uint const numberOfAdapters = ms_direct3d->GetAdapterCount();
	for (uint i = 0; i < numberOfAdapters; ++i)
		if (i != ms_adapter)
		{
			// get the area of the monitor
			HMONITOR monitor = ms_direct3d->GetAdapterMonitor(i);
			MONITORINFO monitorInfo;
			Zero(monitorInfo);
			monitorInfo.cbSize = sizeof(monitorInfo);
			GetMonitorInfo(monitor, &monitorInfo);

			// put it on the list
			otherAdapterRects.push_back(monitorInfo.rcMonitor);
		}*/
}

// ----------------------------------------------------------------------

void Direct3d11::startPerformanceTimer()
{
	if (ms_performanceTimer)
		ms_performanceTimer->start();
}

// ----------------------------------------------------------------------

float Direct3d11::stopPerformanceTimer()
{
	if (!ms_performanceTimer)
			return 0.0f;

	ms_performanceTimer->stop();
	return ms_performanceTimer->getElapsedTime();
}

// ----------------------------------------------------------------------

void Direct3d11Namespace::optimizeIndexBuffer(WORD *indices, int numIndices)
{
/*	ID3DXMesh* pD3DXMesh;
	HRESULT hRslt;
	WORD* indexData = NULL;

	if(numIndices == 0 || !indices)
		return;

	DEBUG_FATAL(numIndices % 3 != 0, ("Fatal: can't optimize a buffer that doesn't contain triangle face data"));
	hRslt = D3DXCreateMeshFVF(numIndices / 3,
							  numIndices,
							  D3DXMESH_SYSTEMMEM,
							  D3DFVF_XYZ,
							  ms_device,
							  &pD3DXMesh);

	if(hRslt != D3D_OK)
	{
		WARNING_DEBUG_FATAL(true, ("Could not optimize index buffer"));
		return;
	}

	hRslt = pD3DXMesh->LockIndexBuffer(0, (void**)&indexData);

	if(hRslt != D3D_OK)
	{
		WARNING_DEBUG_FATAL(true, ("Could not optimize index buffer"));
		return;
	}

	memcpy(indexData, indices, sizeof(WORD) * numIndices);

	pD3DXMesh->UnlockIndexBuffer();

	if(hRslt != D3D_OK)
	{
		WARNING_DEBUG_FATAL(true, ("Could not optimize index buffer"));
		return;
	}

	DWORD* adjacencyTable = new DWORD[numIndices];
	pD3DXMesh->GenerateAdjacency(0.0f, adjacencyTable);
	for(int j = 0; j < numIndices; j++)
	{
		adjacencyTable[j] = 0xFFFFFFFF;
	}

	pD3DXMesh->OptimizeInplace(D3DXMESHOPT_IGNOREVERTS | D3DXMESHOPT_VERTEXCACHE,
		adjacencyTable, NULL, NULL, NULL);

	hRslt = pD3DXMesh->LockIndexBuffer(0, (void**)&indexData);

	if(hRslt != D3D_OK)
	{
		WARNING_DEBUG_FATAL(true, ("Could not optimize index buffer"));
		return;
	}

	memcpy(indices, indexData, sizeof(WORD) * numIndices);
	pD3DXMesh->UnlockIndexBuffer();
	delete [] adjacencyTable;
	pD3DXMesh->Release();*/
}

// ----------------------------------------------------------------------

void Direct3d11Namespace::setBloomEnabled(bool enabled)
{
	ms_alphaFadeOpacity.g = enabled ? 1.0f : 0.0f;
	ms_alphaFadeOpacityDirty = true;
}

// ----------------------------------------------------------------------

void Direct3d11Namespace::pixSetMarker(WCHAR const * markerName)
{
}

// ----------------------------------------------------------------------

void Direct3d11Namespace::pixBeginEvent(WCHAR const * eventName)
{
}

// ----------------------------------------------------------------------

void Direct3d11Namespace::pixEndEvent(WCHAR const *)
{
}

// ----------------------------------------------------------------------

int Direct3d11Namespace::nextPowerOfTwo(int x)
{
	x--;
	x |= (x >> 1);
	x |= (x >> 2);
	x |= (x >> 4);
	x |= (x >> 8);
	x |= (x >> 16);
	x++;
	return x;
}

// ----------------------------------------------------------------------

bool Direct3d11Namespace::writeImage(char const * file, int const width, int const height, int const pitch, int const * pixelsARGB, bool const alphaExtend, Gl_imageFormat const imageFormat, Rectangle2d const * subRect)
{
	/*FATAL(file == NULL, ("Invalid file pointer."));
	FATAL(width <= 0 || width > 2048, ("Invalid width %d.", width));
	FATAL(height <= 0  || height > 2048, ("Invalid height %d.", height));
	FATAL(pitch < width, ("Invalid pitch %d.", pitch));
	FATAL(pixelsARGB == NULL, ("Invalid source pixel pointer."));

	if (subRect) 
	{
		FATAL(subRect->getWidth() <= 0, ("Invalid sub-rect width %d.", subRect->getWidth()));
		FATAL(subRect->getHeight() <= 0, ("Invalid sub-rect height %d.", subRect->getHeight()));
	}

	IDirect3DTexture9 * texturePointer = NULL;

	int const textureWidth = static_cast<int>(subRect ? subRect->getWidth() : width);
	int const textureHeight = static_cast<int>(subRect ? subRect->getHeight() : height);

	int const textureWidth2 = nextPowerOfTwo(textureWidth);
	int const textureHeight2 = nextPowerOfTwo(textureHeight);
	
	if (D3D_OK == Direct3d11::getDevice()->CreateTexture(textureWidth2, textureHeight2, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_SYSTEMMEM, &texturePointer, 0))
	{
		D3DLOCKED_RECT lockedRect;
		HRESULT hresult(0);
	
		hresult = texturePointer->LockRect(0, &lockedRect, NULL, D3DLOCK_DISCARD);
		FATAL_DX_HR("LockRect failed %s", hresult);

		int * lockedPixels = reinterpret_cast<int *>(lockedRect.pBits);
		int const sourcePitch = pitch / sizeof(int);
		int const destPitch = lockedRect.Pitch  / sizeof(int);

		int const subLeft = subRect ? static_cast<int>(subRect->x0) : 0;
		int const subTop = subRect ? static_cast<int>(subRect->y0) : 0;
		
		for (int yp = 0; yp < textureHeight; ++yp) 
		{
			int const sourcePixelOffset = subLeft + ((yp + subTop) * sourcePitch);
			int const destPixelOffset = yp * destPitch;
			
			if (alphaExtend) 
			{
				for (int xp = 0; xp < textureWidth; ++xp) 
				{
					unsigned int pixel = static_cast<unsigned int>(pixelsARGB[xp + sourcePixelOffset]);
					unsigned char const alpha = static_cast<unsigned char>(pixel >> 24);
					lockedPixels[xp + destPixelOffset] = D3DCOLOR_ARGB(0x00, alpha, alpha, alpha);
				}
			}
			else
			{
				memcpy(lockedPixels + destPixelOffset, pixelsARGB + sourcePixelOffset, textureWidth * sizeof(int));
			}
		}

		texturePointer->UnlockRect(0);

		D3DXSaveTextureToFile(file, static_cast<D3DXIMAGE_FILEFORMAT>(imageFormat), texturePointer, NULL);

		texturePointer->Release();

		return true;
	}*/

	return false;
}

// ======================================================================
