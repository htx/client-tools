#ifndef INCLUDED_Direct3d11_H
#define INCLUDED_Direct3d11_H

// ======================================================================

struct Gl_api;
struct Gl_install;
class  HardwareIndexBuffer;
class  HardwareVertexBuffer;
class  StaticShader;
class  Transform;
class  Vector;
class  VertexBufferVector;

#include <d3d11.h>
#include <dxgi.h>
#include <DirectXMath.h>

#include "../../../../../../engine/shared/library/sharedFoundation/include/public/sharedFoundation/Tag.h"
#include "clientGraphics/Texture.def"

class VectorRgba;

#define FATAL_DX_HR(a,b)       FATAL(FAILED(b), (a, DXGetErrorString9(b)))

class Direct3d11
{
public:

	static IDXGIFactory*		getDXGIFactory();
	static IDXGIAdapter*        getAdapter();
	static IDXGIOutput*			getAdapterOutput();
	static DXGI_FORMAT          getAdapterFormat();
	static int                  getShaderCapability();
	static int                  getVideoMemoryInMegabytes();
	static ID3D11Device *		getDevice();
	static ID3D11DeviceContext* getDeviceContext();
	static DXGI_FORMAT          getDepthStencilFormat();
//	static D3DCUBEMAP_FACES     getD3dCubeFace(CubeFace cubeFace);

	static bool                 engineOwnsWindow();
	static int                  getMaxRenderTargetWidth();
	static int                  getMaxRenderTargetHeight();

	static bool                 supportsPixelShaders();
	static bool                 supportsVertexShaders();
	static bool                 supportsTwoSidedStencil();
	static bool                 supportsStreamOffsets();
	static bool                 supportsDynamicTextures();
	static bool                 supportsAntialias();
	static DWORD                getMaxAnisotropy();

	static float                getCurrentTime();
	static int                  getFrameNumber();
	static DWORD                getFogColor();

	static void                 setAlphaBlendEnable(bool alphaBlendEnable);
	static void                 setAlphaTestReferenceValue(uint8 alphaTestReferenceValue);
	static void                 setColorWriteEnable(uint8 colorWriteEnable);

#ifdef _DEBUG
	static void                 clearStaticShader();
#endif

	static void *               getTemporaryBuffer(int size);

	static VectorRgba const &   getAlphaFadeAndBloomSettings();

	// the following functions are only here because they need friend access as Direct3d9
	static bool                 install(Gl_install * gl_install);
	static void                 setStaticShader(StaticShader const & shader, int pass);
	static void                 setVertexBuffer(HardwareVertexBuffer const & vertexBuffer);
	static void                 setVertexBufferVector(VertexBufferVector const & vertexBufferVector);
	static void                 setIndexBuffer(HardwareIndexBuffer const & indexBuffer);
	static void                 convertTransformToMatrix(Transform const & transform, DirectX::XMFLOAT4X4 & matrix);
	static void                 convertScaleAndTransformToMatrix(Vector const & scale, Transform const & transform, DirectX::XMFLOAT4X4 & matrix);
	static bool                 drawPrimitive();
	static void                 drawPrimitive(int startVertex, int primitiveCount);
	static void                 drawIndexedPrimitive(int baseIndex, int minimumVertexIndex, int numberOfVertices, int startIndex, int primitiveCount, int numberOfIndices);

	static void                 startPerformanceTimer();
	static float                stopPerformanceTimer();

	static void                 setAntialiasEnabled(bool enabled);
};

// ======================================================================

#endif
