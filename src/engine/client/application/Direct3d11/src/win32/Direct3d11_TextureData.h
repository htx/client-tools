#ifndef INCLUDED_Direct3d11_TextureData_H
#define INCLUDED_Direct3d11_TextureData_H

// ======================================================================

class MemoryBlockManager;

#include "../../../../../../engine/shared/library/sharedFoundation/include/public/sharedFoundation/Tag.h"
#include "clientGraphics/Texture.h"
#include <d3d11.h>

// ======================================================================

class Direct3d11_TextureData: public TextureGraphicsData
{
public:

	static void *operator new(size_t size);
	static void  operator delete(void *pointer);

	static void install(void);
	static void remove(void);

	static void releaseAllGlobalTextures();

	static DXGI_FORMAT getD3dFormat(TextureFormat textureFormat);

	static bool                                 isGlobalTexture(Tag textureTag);
	static Direct3d11_TextureData const * const *getGlobalTexture(Tag textureTag);
	static void                                 setGlobalTexture(Tag textureTag, const Texture &engineTexture);

	static ID3D11Texture2D						*create2dTexture(int width, int height, int mipmapLevelCount, TextureFormat textureFormat);

	Direct3d11_TextureData(const Texture &newEngineTexture, const TextureFormat *runtimeFormats, int numberOfRuntimeFormats);
	virtual ~Direct3d11_TextureData(void);

	virtual TextureFormat   getNativeFormat() const;
	virtual void            lock(LockData &lockData);
	virtual void            unlock(LockData &lockData);

	virtual void            copyFrom(int surfaceLevel, TextureGraphicsData const & rhs, int srcX, int srcY, int srcWidth, int srcHeight, int dstX, int dstY, int dstWidth, int dstHeight);

	ID3D11Texture2D				*getBaseTexture() const;
	TextureFormat				getTextureFormat() const;
	ID3D11ShaderResourceView	*getSrv() const;
	
private:

	/// disabled
	Direct3d11_TextureData(void);
	Direct3d11_TextureData(const Direct3d11_TextureData&);
	Direct3d11_TextureData &operator =(const Direct3d11_TextureData&);

	struct PixelFormatInfo
	{
		bool           isSupported;
		DXGI_FORMAT    pixelFormat;
	};

	struct GlobalTextureInfo
	{
		Texture const *                        engineTexture;
		Direct3d11_TextureData const *          d3dTexture;
	};

	typedef stdmap<Tag, GlobalTextureInfo>::fwd  GlobalTextureList;

	static PixelFormatInfo                   ms_pixelFormatInfoArray[TF_Count];
	static MemoryBlockManager				*ms_memoryBlockManager;
	static GlobalTextureList                *ms_globalTextureList;

	const Texture                           &m_engineTexture;
	ID3D11Texture2D							*m_d3dTexture;
	ID3D11Texture3D							*m_d3dVolumeTexture;
	TextureFormat                            m_destFormat;
	D3D11_MAPPED_SUBRESOURCE				 m_mappedResource;
	D3D11_SHADER_RESOURCE_VIEW_DESC			 m_srvDesc;
	ID3D11ShaderResourceView				*m_srv;
};

// ======================================================================

inline ID3D11Texture2D *Direct3d11_TextureData::getBaseTexture() const
{
	return m_d3dTexture;
}

// ----------------------------------------------------------------------

inline TextureFormat Direct3d11_TextureData::getTextureFormat() const
{
	return m_destFormat;
}

// ----------------------------------------------------------------------

inline ID3D11ShaderResourceView *Direct3d11_TextureData::getSrv() const
{
	return m_srv;
}

// ======================================================================

#endif
