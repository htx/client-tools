#ifndef INCLUDED_Direct3d11_StateCache_H
#define INCLUDED_Direct3d11_StateCache_H

// ======================================================================

class Direct3d11_TextureData;

#include "Direct3d11.h"
#include "clientGraphics/Graphics.def"

// ======================================================================

class Direct3d11_StateCache
{
public:

	static void install(int maxStreamCount);
	static void remove();

	static void forceBlendState(D3D11_BLEND_DESC blendDesc);
	static void setBlendState(D3D11_BLEND_DESC blendDesc);

	static void forceSamplerState(D3D11_SAMPLER_DESC samplerDesc);
	static void setSamplerState(D3D11_SAMPLER_DESC samplerDesc);

	static void forceRasterizerState(D3D11_RASTERIZER_DESC rasterDesc);
	static void setRasterizerState(D3D11_RASTERIZER_DESC rasterDesc);

	static void forceDepthStencilState(D3D11_DEPTH_STENCIL_DESC dsDesc);
	static void setDepthStencilState(D3D11_DEPTH_STENCIL_DESC dsDesc);

	static void forceIndexBuffer(ID3D11Buffer *indexBuffer);
	static void setIndexBuffer(ID3D11Buffer *indexBuffer);

	static void forceStreamSource(int streamIndex, ID3D11Buffer *stream, UINT byteOffset, UINT stride);
	static void setStreamSource(int streamIndex, ID3D11Buffer *stream, UINT byteOffset, UINT stride);

	static void forceVertexDeclaration(ID3D11InputLayout *vertexDeclaration);
	static void setVertexDeclaration(ID3D11InputLayout *vertexDeclaration);

	static void setTexture(int stage, Direct3d11_TextureData const *texture);
	static void destroyTexture(Direct3d11_TextureData const *texture);

	static void lostDevice();
	static void restoreDevice();

	static void forceVertexShader(ID3D11VertexShader *vertexShader);
	static void setVertexShader(ID3D11VertexShader *vertexShader);

	static void forcePixelShader(ID3D11PixelShader *pixelShader);
	static void setPixelShader(ID3D11PixelShader *pixelShader);

	static void setVertexShaderConstants(unsigned int slot, ID3D11Buffer* buffer, unsigned int numberOfConstants);
	static void setPixelShaderConstants(unsigned int slot, ID3D11Buffer* buffer, unsigned int numberOfConstants);

	static void resetTextureCoordinateIndices();

	static void  setSpecularPower(float power);
	static float getSpecularPower();

private:

	enum
	{
		cms_stages   = 8,
		cms_samplers = 16
	};

	struct StreamData
	{
		ID3D11Buffer		   *m_stream;
		UINT                    m_offset;
		UINT                    m_stride;
	};

	typedef DWORD                        RenderStateCache[210];
	typedef DWORD                        TextureStagesCache[cms_stages][33];
	typedef DWORD                        SamplerCache[cms_samplers][14];
	typedef Direct3d11_TextureData const *TextureCache[cms_samplers];

	static void setConstants();
	static void createConstantBuffers();

	static ID3D11Device *				  ms_device;
	static RenderStateCache               ms_renderStateCache;
	static SamplerCache                   ms_samplerStateCache;
	static TextureStagesCache             ms_textureStagesStateCache;
	static TextureCache                   ms_textureCache;
	static ID3D11InputLayout *			  ms_vertexDeclaration;
	static StreamData *                   ms_streamDataCache;
	static int                            ms_maxStreamCount;

	static ID3D11Buffer *				  ms_indexBuffer;
	static UINT                           ms_indexBufferOffset;
	
	static float                          ms_specularPower;

	static ID3D11VertexShader *			  ms_vertexShader;
	static ID3D11PixelShader  *           ms_pixelShader;
	static ID3D11Buffer*				  ms_constantBufferUnitX;
	static ID3D11Buffer*				  ms_constantBufferUnitY;
	static ID3D11Buffer*				  ms_constantBufferUnitZ;
	static ID3D11Buffer*				  ms_constantBufferC95;
};

// ======================================================================

#define STATE_CACHE_ALWAYS_FORCE 0

// ======================================================================

inline void Direct3d11_StateCache::forceBlendState(D3D11_BLEND_DESC blendDesc)
{
	//ms_renderStateCache[state] = value;
	//ms_device->SetRenderState(state, value);
}

// ----------------------------------------------------------------------

inline void Direct3d11_StateCache::setBlendState(D3D11_BLEND_DESC blendDesc)
{
#if !STATE_CACHE_ALWAYS_FORCE
	//if (ms_renderStateCache[state] != value)
#endif
		forceBlendState(blendDesc);
}

// ----------------------------------------------------------------------

inline void Direct3d11_StateCache::forceSamplerState(D3D11_SAMPLER_DESC samplerDesc)
{
	//ms_samplerStateCache[sampler][state] = value;
	//ms_device->SetSamplerState(static_cast<DWORD>(sampler), state, value);
}

// ----------------------------------------------------------------------

inline void Direct3d11_StateCache::setSamplerState(D3D11_SAMPLER_DESC samplerDesc)
{
#if !STATE_CACHE_ALWAYS_FORCE
	//if (ms_samplerStateCache[sampler][state] != value)
#endif
		forceSamplerState(samplerDesc);
}

// ----------------------------------------------------------------------

inline void Direct3d11_StateCache::forceRasterizerState(D3D11_RASTERIZER_DESC rasterDesc)
{
	//ms_textureStagesStateCache[stage][state] = value;
}

// ----------------------------------------------------------------------

inline void Direct3d11_StateCache::setRasterizerState(D3D11_RASTERIZER_DESC rasterDesc)
{
#if !STATE_CACHE_ALWAYS_FORCE
	//if (ms_textureStagesStateCache[stage][state] != value)
#endif
		forceRasterizerState(rasterDesc);
}

// ----------------------------------------------------------------------

inline void Direct3d11_StateCache::forceDepthStencilState(D3D11_DEPTH_STENCIL_DESC dsDesc)
{
	//ms_textureStagesStateCache[stage][state] = value;
}

// ----------------------------------------------------------------------

inline void Direct3d11_StateCache::setDepthStencilState(D3D11_DEPTH_STENCIL_DESC dsDesc)
{
#if !STATE_CACHE_ALWAYS_FORCE
	//if (ms_textureStagesStateCache[stage][state] != value)
#endif
		forceDepthStencilState(dsDesc);
}


// ----------------------------------------------------------------------

inline void Direct3d11_StateCache::forceIndexBuffer(ID3D11Buffer *indexBuffer)
{
	ms_indexBuffer = indexBuffer;
	Direct3d11::getDeviceContext()->IASetIndexBuffer(ms_indexBuffer, DXGI_FORMAT_R16_UINT, ms_indexBufferOffset);
}

// ----------------------------------------------------------------------

inline void Direct3d11_StateCache::setIndexBuffer(ID3D11Buffer *indexBuffer)
{
#if !STATE_CACHE_ALWAYS_FORCE
	if (ms_indexBuffer != indexBuffer)
#endif
		forceIndexBuffer(indexBuffer);
}

// ----------------------------------------------------------------------

inline void Direct3d11_StateCache::forceStreamSource(int streamIndex, ID3D11Buffer *stream, UINT byteOffset, UINT stride)
{
	VALIDATE_RANGE_INCLUSIVE_EXCLUSIVE(0, streamIndex, ms_maxStreamCount);

	//-- save cache settings
	ms_streamDataCache[streamIndex].m_stream = stream;
	ms_streamDataCache[streamIndex].m_offset = byteOffset;
	ms_streamDataCache[streamIndex].m_stride = stride;

	//-- set stream
	Direct3d11::getDeviceContext()->IASetVertexBuffers(static_cast<UINT>(streamIndex), 1, &stream, &stride, &byteOffset);
}

// ----------------------------------------------------------------------

inline void Direct3d11_StateCache::setStreamSource(int streamIndex, ID3D11Buffer *stream, UINT byteOffset, UINT stride)
{
	VALIDATE_RANGE_INCLUSIVE_EXCLUSIVE(0, streamIndex, ms_maxStreamCount);

	//-- check cache
#if !STATE_CACHE_ALWAYS_FORCE
	if (  (stream != ms_streamDataCache[streamIndex].m_stream) 
		|| (ms_streamDataCache[streamIndex].m_offset != byteOffset)
		|| (ms_streamDataCache[streamIndex].m_stride != stride)
		)
#endif
		forceStreamSource(streamIndex, stream, byteOffset, stride);
}

// ----------------------------------------------------------------------

inline void Direct3d11_StateCache::forceVertexDeclaration(ID3D11InputLayout *vertexDeclaration)
{
	Direct3d11::getDeviceContext()->IASetInputLayout(vertexDeclaration);
}

// ----------------------------------------------------------------------

inline void Direct3d11_StateCache::setVertexDeclaration(ID3D11InputLayout *vertexDeclaration)
{
#if !STATE_CACHE_ALWAYS_FORCE
	if (ms_vertexDeclaration != vertexDeclaration)
#endif
		forceVertexDeclaration(vertexDeclaration);
}

// ----------------------------------------------------------------------

inline void Direct3d11_StateCache::forceVertexShader(ID3D11VertexShader *vertexShader)
{
	ms_vertexShader = vertexShader;
	Direct3d11::getDeviceContext()->VSSetShader(ms_vertexShader, nullptr, 0);
}

// ----------------------------------------------------------------------

inline void Direct3d11_StateCache::setVertexShader(ID3D11VertexShader *vertexShader)
{
/*#if !STATE_CACHE_ALWAYS_FORCE
	if (vertexShader != ms_vertexShader)
#endif*/
		forceVertexShader(vertexShader);
}

// ----------------------------------------------------------------------

inline void Direct3d11_StateCache::forcePixelShader(ID3D11PixelShader *pixelShader)
{
	ms_pixelShader = pixelShader;
	Direct3d11::getDeviceContext()->PSSetShader(ms_pixelShader, nullptr, 0);
}

// ----------------------------------------------------------------------

inline void Direct3d11_StateCache::setPixelShader(ID3D11PixelShader *pixelShader)
{
#if !STATE_CACHE_ALWAYS_FORCE
	if (pixelShader != ms_pixelShader)
#endif
		forcePixelShader(pixelShader);
}

// ----------------------------------------------------------------------

inline void Direct3d11_StateCache::setSpecularPower(float power)
{
	ms_specularPower = power;
}

// ----------------------------------------------------------------------

inline float Direct3d11_StateCache::getSpecularPower()
{
	return ms_specularPower;
}

// ======================================================================

#endif