// ======================================================================
// Direct3d9_TextureData.cpp
//
// Portions Copyright 1998 Bootprint Entertainment
// Portions copyright 2000-2002 Sony Online Entertainment
// All Rights Reserved.
// ======================================================================

#include "FirstDirect3d11.h"
#include "Direct3d11_TextureData.h"

#include "Direct3d11.h"
#include "Direct3d11_StateCache.h"
#include "sharedFoundation/ExitChain.h"
#include "sharedFoundation/MemoryBlockManager.h"
#include "sharedFoundation/Os.h"
#include "clientGraphics/TextureFormatInfo.h"

#include <d3d11.h>
#include <map>

// ======================================================================

const Tag TAG_ENVM = TAG(E,N,V,M);

// ======================================================================

Direct3d11_TextureData::PixelFormatInfo     Direct3d11_TextureData::ms_pixelFormatInfoArray[TF_Count];
MemoryBlockManager                        *Direct3d11_TextureData::ms_memoryBlockManager;
Direct3d11_TextureData::GlobalTextureList  *Direct3d11_TextureData::ms_globalTextureList;

static const DXGI_FORMAT translationTable[] =
{
	DXGI_FORMAT_B8G8R8A8_UNORM, // TF_ARGB_8888,
	DXGI_FORMAT_B4G4R4A4_UNORM, // TF_ARGB_4444,
	DXGI_FORMAT_B5G5R5A1_UNORM, // TF_ARGB_1555,
	DXGI_FORMAT_B8G8R8X8_UNORM, // TF_XRGB_8888,
	DXGI_FORMAT_UNKNOWN,		// TF_RGB_888, n/a
	DXGI_FORMAT_UNKNOWN,		// TF_RGB_565, n/a
	DXGI_FORMAT_UNKNOWN,		// TF_RGB_555, n/a
	DXGI_FORMAT_BC1_UNORM,		// TF_DXT1,
	DXGI_FORMAT_BC1_UNORM,		// TF_DXT2,
	DXGI_FORMAT_BC2_UNORM,		// TF_DXT3,
	DXGI_FORMAT_BC2_UNORM,		// TF_DXT4,
	DXGI_FORMAT_BC3_UNORM,		// TF_DXT5,
	DXGI_FORMAT_A8_UNORM,		// TF_A_8,
	DXGI_FORMAT_R8_UNORM,		// TF_L_8, ?
	DXGI_FORMAT_R8G8_UNORM,		// TF_P_8, ?
	DXGI_FORMAT_R16G16B16A16_FLOAT,	// TF_ABGR_16F,
	DXGI_FORMAT_R32G32B32A32_FLOAT,	// TF_ABGR_32F,
	DXGI_FORMAT_UNKNOWN,		// TF_Count,
	DXGI_FORMAT_UNKNOWN			// TF_Native
};

// ======================================================================
/**
 * install the Direct3d texture data support.
 *
 * This method performs essential initialization for this api's texture
 * system.  It will map Gl-defined texture formats to physical
 * texture formats available for the selected device in addition to
 * performing other required initialization.
 */

void Direct3d11_TextureData::install(void)
{
	DEBUG_FATAL(ms_memoryBlockManager, ("Direct3d11_TextureData already installed"));
	ms_memoryBlockManager = new MemoryBlockManager("Direct3d11_TextureData::memoryBlockManager", true, sizeof(Direct3d11_TextureData), 0, 0, 0);

	ID3D11Device *direct3d = Direct3d11::getDevice();
	for (int i = 0; i < TF_Count; ++i)
	{
		bool supported = true;
		DXGI_FORMAT native = translationTable[i];
		/*if (native != DXGI_FORMAT_UNKNOWN)
		{
			const HRESULT hresult = direct3d->CheckDeviceFormat(Direct3d9::getAdapter(),	Direct3d9::getDeviceType(), Direct3d9::getAdapterFormat(), 0, D3DRTYPE_TEXTURE, native);
			if (hresult == D3D_OK)
				supported = true;
			else
				if (hresult == D3DERR_NOTAVAILABLE)
					supported = false;
				else
				{
					FATAL_DX_HR("CheckDeviceFormat failed %s", hresult);
				}
		}*/

		TextureFormatInfo::setSupported(static_cast<TextureFormat>(i), supported);
	}

	ms_globalTextureList = new GlobalTextureList;
}

// ----------------------------------------------------------------------
/**
 * Remove the Direct3d 6 texture data support.
 */

void Direct3d11_TextureData::remove(void)
{
	if (!ExitChain::isFataling())
	{
		if (ms_memoryBlockManager)
		{
			DEBUG_FATAL(!ms_memoryBlockManager, ("Direct3d11_TextureData not installed"));
			delete ms_memoryBlockManager;
			ms_memoryBlockManager = 0;
		}

		if (ms_globalTextureList)
		{
			// free all the textures
			while (!ms_globalTextureList->empty())
			{
				ms_globalTextureList->begin()->second.engineTexture->release();
				ms_globalTextureList->erase(ms_globalTextureList->begin());
			}

			DEBUG_FATAL(!ms_globalTextureList->empty(), ("global textures are still allocated in the graphics dll"));
			delete ms_globalTextureList;
			ms_globalTextureList = 0;
		}
	}
}

// ----------------------------------------------------------------------

void Direct3d11_TextureData::releaseAllGlobalTextures(void)
{
	while (!ms_globalTextureList->empty())
	{
		ms_globalTextureList->begin()->second.engineTexture->release();
		ms_globalTextureList->erase(ms_globalTextureList->begin());
	}
}

// ----------------------------------------------------------------------

void *Direct3d11_TextureData::operator new(size_t size)
{
	UNREF(size);
	NOT_NULL(ms_memoryBlockManager);
	DEBUG_FATAL(size != sizeof (Direct3d11_TextureData), ("bad size"));
	DEBUG_FATAL(size != static_cast<size_t> (ms_memoryBlockManager->getElementSize()), ("installed with bad size"));

	return ms_memoryBlockManager->allocate();
}

// ----------------------------------------------------------------------

void Direct3d11_TextureData::operator delete(void *pointer)
{
	NOT_NULL(ms_memoryBlockManager);
	ms_memoryBlockManager->free(pointer);
}

// ----------------------------------------------------------------------

Direct3d11_TextureData const * const *Direct3d11_TextureData::getGlobalTexture(Tag tag)
{
	DEBUG_FATAL(!ms_globalTextureList, ("Not installed"));
	DEBUG_FATAL(!isGlobalTexture(tag), ("Tag is not correct for a global texture"));
	GlobalTextureList::const_iterator i = ms_globalTextureList->find(tag);
	DEBUG_FATAL(i == ms_globalTextureList->end(), ("Could not find requested global texture"));
	return & i->second.d3dTexture;
}

// ----------------------------------------------------------------------

bool Direct3d11_TextureData::isGlobalTexture(Tag tag)
{
	return (tag == TAG_ENVM) || ((tag >> 24) & 0xff) == '_';
}

// ----------------------------------------------------------------------

void Direct3d11_TextureData::setGlobalTexture(Tag tag, const Texture &texture)
{
	DEBUG_FATAL(!isGlobalTexture(tag), ("Tag is not correct for a global texture"));

	GlobalTextureList::iterator i = ms_globalTextureList->find(tag);
	if (i != ms_globalTextureList->end())
	{
		if (i->second.engineTexture != &texture)
		{
			texture.fetch();
			i->second.engineTexture->release();
			i->second.engineTexture = &texture;
			i->second.d3dTexture = static_cast<Direct3d11_TextureData const *>(texture.getGraphicsData());
		}
	}
	else
	{
		GlobalTextureInfo gti;
		gti.engineTexture = &texture;
		gti.d3dTexture = static_cast<Direct3d11_TextureData const *>(texture.getGraphicsData());
		const bool inserted = ms_globalTextureList->insert(GlobalTextureList::value_type(tag, gti)).second;
		UNREF(inserted);
		DEBUG_FATAL(!inserted, ("item was already present in map"));
		texture.fetch();
	}
}

// ----------------------------------------------------------------------

DXGI_FORMAT Direct3d11_TextureData::getD3dFormat(TextureFormat textureFormat)
{
	return translationTable[textureFormat];
}

// ----------------------------------------------------------------------

Direct3d11_TextureData::Direct3d11_TextureData(const Texture &newEngineTexture, const TextureFormat *runtimeFormats, int numberOfRuntimeFormats)
:	TextureGraphicsData(),
	m_engineTexture(newEngineTexture),
	m_d3dTexture(nullptr),
	m_destFormat(TF_RGB_555)
{
	DEBUG_FATAL(!Os::isMainThread(), ("Creating texture from alternate thread"));

	D3D11_TEXTURE2D_DESC desc;
    memset(&desc, 0, sizeof(desc));
	
	DWORD   resourceUsage = 0;

	{
		const bool isDynamic      = newEngineTexture.isDynamic();
		const bool isRenderTarget = newEngineTexture.isRenderTarget();
		
		if (isRenderTarget)
		{
			DEBUG_FATAL(newEngineTexture.getWidth() > Direct3d11::getMaxRenderTargetWidth() || newEngineTexture.getHeight() > Direct3d11::getMaxRenderTargetHeight(), ("Cannot create a render target larger than the primary surface (%d,%d) vs (%d,%d)", newEngineTexture.getWidth(), newEngineTexture.getHeight(), Direct3d11::getMaxRenderTargetWidth(), Direct3d11::getMaxRenderTargetHeight()));
			desc.Usage = D3D11_USAGE_DEFAULT;
			desc.BindFlags = D3D11_BIND_RENDER_TARGET;
			resourceUsage = 1; //D3DUSAGE_RENDERTARGET
		}
		else if (isDynamic)
		{
			desc.Usage = D3D11_USAGE_DYNAMIC;
			desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
			resourceUsage = 2; //D3DUSAGE_DYNAMIC;
		}
	}

	unsigned int resourceType = 0; //D3DRTYPE_TEXTURE;
	desc.Width = m_engineTexture.getWidth();
	desc.Height = m_engineTexture.getHeight();
	desc.MipLevels = 1;//m_engineTexture.getMipmapLevelCount();
	desc.ArraySize = 1;
	desc.SampleDesc.Count = 1;
	desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	desc.MiscFlags = 0;
	desc.Usage = D3D11_USAGE_DYNAMIC;
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
		
	if (newEngineTexture.isCubeMap())
		resourceType = 1; //D3DRTYPE_CUBETEXTURE;
	else if (newEngineTexture.isVolumeMap())
		resourceType = 2; //D3DRTYPE_VOLUMETEXTURE;

	for (int i = 0; !m_d3dTexture && i < numberOfRuntimeFormats; ++i)
	{
		TextureFormatInfo const & textureInfo = TextureFormatInfo::getInfo(runtimeFormats[i]);
		if (textureInfo.supported)
		{
			m_destFormat = runtimeFormats[i];
			desc.Format = translationTable[m_destFormat];

			switch (resourceType)
			{
				case 0: //D3DRTYPE_TEXTURE;
				{
					ID3D11Texture2D *texture = nullptr; 
					const HRESULT hresult = Direct3d11::getDevice()->CreateTexture2D(&desc, nullptr, &texture);
					FATAL(FAILED(hresult), ("create texture fail."));
					
					m_d3dTexture = texture;

					DEBUG_FATAL(!m_d3dTexture, ("CreateTexture returned a NULL texture"));
				}
				break;

				case 1: //D3DRTYPE_CUBETEXTURE:
				{
					DEBUG_FATAL(m_engineTexture.getWidth() != m_engineTexture.getHeight(), ("Cube textures must be square"));
					
					//desc.MiscFlags |= D3D11_RESOURCE_MISC_TEXTURECUBE;
					
					ID3D11Texture2D *texture = nullptr;
					HRESULT hresult = Direct3d11::getDevice()->CreateTexture2D(&desc, nullptr, &texture);
					FATAL(FAILED(hresult), ("create cube texture fail."));
					
					m_d3dTexture = texture;
					DEBUG_FATAL(!m_d3dTexture, ("CreateCubeTexture returned a NULL texture"));
				}
				break;

				case 2: //D3DRTYPE_VOLUMETEXTURE:
				{
					D3D11_TEXTURE3D_DESC desc3D;
					desc3D.Usage = desc.Usage;
					desc3D.Width = m_engineTexture.getWidth();
					desc3D.Height = m_engineTexture.getHeight();
					desc3D.Depth = m_engineTexture.getDepth();
					desc3D.MipLevels = desc.ArraySize = m_engineTexture.getMipmapLevelCount();
					desc3D.Format = translationTable[m_destFormat];
					desc3D.BindFlags = D3D11_BIND_SHADER_RESOURCE;
					desc3D.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
					desc3D.MiscFlags = 0;
					
					ID3D11Texture3D *texture = nullptr;
					const HRESULT hresult = Direct3d11::getDevice()->CreateTexture3D(&desc3D, nullptr, &texture);
					
					m_d3dVolumeTexture = texture;

					DEBUG_FATAL(!m_d3dTexture, ("CreateVolumeTexture returned a NULL texture"));
				}
				break;

				default: DEBUG_FATAL(true, ("unknown texture type")); break;
			}
		}
	}

	if (!m_d3dTexture)
	{
		char buffer[1024] = "";
		for (int i = 0; i < numberOfRuntimeFormats; ++i)
		{
			strcat(buffer, TextureFormatInfo::getInfo(runtimeFormats[i]).name);
			strcat(buffer, " ");
		}
		DEBUG_FATAL(true, ("failed to support any of the texture's listed formats: %s\n", buffer));
	}

	m_srvDesc.Format = desc.Format;
	m_srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	m_srvDesc.Texture2D.MostDetailedMip = 0;
	m_srvDesc.Texture2D.MipLevels = -1;

	HRESULT hResult = Direct3d11::getDevice()->CreateShaderResourceView(m_d3dTexture, &m_srvDesc, &m_srv);

	FATAL(FAILED(hResult), ("create srv fail."));
}

// ----------------------------------------------------------------------

Direct3d11_TextureData::~Direct3d11_TextureData(void)
{
	if (m_d3dTexture)
	{
		Direct3d11_StateCache::destroyTexture(this);
		IGNORE_RETURN(m_d3dTexture->Release());
		m_d3dTexture = nullptr;
	}
}

// ----------------------------------------------------------------------

TextureFormat Direct3d11_TextureData::getNativeFormat() const
{
	return m_destFormat;
}

// ----------------------------------------------------------------------

void Direct3d11_TextureData::lock(LockData &lockData)
{
	DEBUG_FATAL(lockData.getWidth() == 0, ("Locking 0 width area"));
	DEBUG_FATAL(lockData.getHeight() == 0, ("Locking 0 height area"));
	DEBUG_FATAL(lockData.getDepth() == 0, ("Locking 0 depth area"));

	D3D11_MAP mapType = D3D11_MAP_READ_WRITE;
		
	DWORD flags = 0;
	if (lockData.isReadOnly()) 
	{
		//mapType = D3D11_MAP_READ;
	}

	if (lockData.getFormat() == TF_Native)
		lockData.m_format = m_destFormat;

	HRESULT hresult;

	// handle locking in native format
	if (lockData.getFormat() == m_destFormat)
	{
		if (m_engineTexture.isVolumeMap())
		{
			D3D11_BOX lockedBox;

			D3D11_BOX box;
			box.left   = lockData.getX();
			box.top    = lockData.getY();
			box.right  = lockData.getX() + lockData.getWidth();
			box.bottom = lockData.getY() + lockData.getHeight();
			box.front  = lockData.getZ();
			box.back   = lockData.getZ() + lockData.getDepth();

			const bool wholeTexture = (
				   box.left   ==0 
				&& box.top    ==0 
				&& box.front  ==0 
				&& box.right  ==unsigned(m_engineTexture.getWidth())
				&& box.bottom ==unsigned(m_engineTexture.getHeight())
				&& box.back   ==unsigned(m_engineTexture.getDepth())
			);

			D3D11_BOX *pBox = (wholeTexture) ? static_cast<D3D11_BOX*>(nullptr) : &box;

			hresult = Direct3d11::getDeviceContext()->Map(m_d3dVolumeTexture, 0, mapType, 0, &m_mappedResource);
		//	hresult = static_cast<ID3D10Texture3D*>(m_d3dTexture)->LockBox(lockData.getLevel(), &lockedBox, pBox, flags);

			// let the user know where and how to write
			lockData.m_pixelData =  m_mappedResource.pData;
			lockData.m_pitch = m_mappedResource.RowPitch;
			lockData.m_slicePitch = m_mappedResource.DepthPitch;
		}
		else
		{
			//D3DLOCKED_RECT lockedRect;

			if (lockData.shouldDiscardContents())
			{
				//mapType = D3D11_MAP_WRITE_DISCARD;
			}

			RECT r, *pr=&r;
			r.left   = lockData.getX();
			r.top    = lockData.getY();
			r.right  = lockData.getX() + lockData.getWidth();
			r.bottom = lockData.getY() + lockData.getHeight();

			if (  r.left==0 
				&& r.top==0 
				&& r.right==m_engineTexture.getWidth() 
				&& r.bottom==m_engineTexture.getHeight()
				)
			{
				pr=nullptr;
			}

			if (m_engineTexture.isCubeMap())
			{ 
				hresult = Direct3d11::getDeviceContext()->Map(m_d3dTexture, 0, D3D11_MAP_WRITE_DISCARD, 0, &m_mappedResource);

				LPTSTR errorText = NULL;

				FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM|FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_IGNORE_INSERTS, NULL, hresult, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&errorText, 0, NULL); 
				FATAL(FAILED(hresult), ("map cubemap fail. %s", errorText));
				//hresult = static_cast<ID3D11Texture2D*>(m_d3dTexture)->LockRect(Direct3d9::getD3dCubeFace(lockData.m_cubeFace), lockData.getLevel(), &lockedRect, pr, flags);
			}
			else
			{
				hresult = Direct3d11::getDeviceContext()->Map(m_d3dTexture, 0, D3D11_MAP_WRITE_DISCARD, 0, &m_mappedResource);
				FATAL(FAILED(hresult), ("map texture fail."));
				//hresult = static_cast<ID3D11Texture2D*>(m_d3dTexture)->LockRect(lockData.getLevel(), &lockedRect, pr, flags);
			}
			
			// let the user know where and how to write
			lockData.m_pixelData =  m_mappedResource.pData;
			lockData.m_pitch = m_mappedResource.RowPitch;
			lockData.m_slicePitch = m_mappedResource.DepthPitch * lockData.getHeight();
		}

		lockData.m_reserved = nullptr;
	}
	else
	{
	/*	FATAL(m_engineTexture.isVolumeMap(), ("Volume map not supported for format conversion (yet)"));

		// the user wants to access the data in a non-native format.  This will not be cheap.

		// DXT textures have a minimum width and height of 4
		int width = lockData.getWidth();
		int height = lockData.getHeight();
		if (lockData.getFormat() == TF_DXT1 || lockData.getFormat() == TF_DXT2 || lockData.getFormat() == TF_DXT3 || lockData.getFormat() == TF_DXT4 || lockData.getFormat() == TF_DXT5)
		{
			if (width < 4)
				width = 4;
			if (height < 4)
				height = 4;
		}

		// create a temporary surface of the format desired by the user
		IDirect3DSurface9 * plainSurface = 0;
		hresult = Direct3d9::getDevice()->CreateOffscreenPlainSurface(width, height, translationTable[lockData.getFormat()], D3DPOOL_SCRATCH, &plainSurface, NULL);

		NOT_NULL(plainSurface);

		// record the temporary surface into the lock data
		lockData.m_reserved = plainSurface;

		// copy the original pixel bits back to the temporary surface if the contents aren't being discarded
		if (!lockData.shouldDiscardContents())
		{
			// get the d3d surface containing the texture bits
			IDirect3DSurface9 * surface = 0;
			if (m_engineTexture.isCubeMap())
			{
				hresult = static_cast<IDirect3DCubeTexture9*>(m_d3dTexture)->GetCubeMapSurface(Direct3d9::getD3dCubeFace(lockData.m_cubeFace), lockData.getLevel(), &surface);
				FATAL_DX_HR("GetCubeMapSurface failed %s", hresult);
			}
			else
			{
				hresult = static_cast<IDirect3DTexture9*>(m_d3dTexture)->GetSurfaceLevel(lockData.getLevel(), &surface);
				FATAL_DX_HR("GetSurfaceLevel failed %s", hresult);
			}

			// we only want to copy the locked data back
			RECT source;
			source.left   = lockData.getX();
			source.top    = lockData.getY();
			source.right  = lockData.getX() + lockData.getWidth();
			source.bottom = lockData.getY() + lockData.getHeight();

			// copy and convert the texture bits
			hresult = D3DXLoadSurfaceFromSurface(plainSurface, NULL, NULL, surface, NULL, &source, D3DX_FILTER_NONE, 0);
			FATAL_DX_HR("D3DXLoadSurfaceFromSurface failed %s", hresult);

			// release the d3d surface
			surface->Release();
		}

		// lock the temporary surface for the user to write to
		D3DLOCKED_RECT lockedRect;
		RECT r;
		r.left   = 0;
		r.top    = 0;
		r.right  = width;
		r.bottom = height;
		hresult = plainSurface->LockRect(&lockedRect, &r, flags);
		FATAL_DX_HR("LockRect failed %s", hresult);

		// let the user know where and how to write
		lockData.m_pixelData = lockedRect.pBits;
		lockData.m_pitch = lockedRect.Pitch;*/
	}
}

// ----------------------------------------------------------------------

void Direct3d11_TextureData::unlock(LockData &lockData)
{
	if (lockData.m_reserved)
	{
		/*// recover the source surface from the lock data and unlock it
		IDirect3DSurface9 * plainSurface = reinterpret_cast<IDirect3DSurface9 *>(lockData.m_reserved);
		HRESULT hresult = plainSurface->UnlockRect();
		FATAL_DX_HR("UnlockRect failed %s", hresult);

		// get the texture surface that we want to update
		IDirect3DSurface9 * surface = 0;
		if (m_engineTexture.isCubeMap())
		{
			hresult = static_cast<IDirect3DCubeTexture9*>(m_d3dTexture)->GetCubeMapSurface(Direct3d9::getD3dCubeFace(lockData.m_cubeFace), lockData.getLevel(), &surface);
			FATAL_DX_HR("GetCubeMapSurface failed %s", hresult);
		}
		else
		{
			hresult = static_cast<IDirect3DTexture9*>(m_d3dTexture)->GetSurfaceLevel(lockData.getLevel(), &surface);
			FATAL_DX_HR("GetSurfaceLevel failed %s", hresult);
		}

		// copy and convert the texture bits
		RECT dest;
		dest.left   = lockData.getX();
		dest.top    = lockData.getY();
		dest.right  = lockData.getX() + lockData.getWidth();
		dest.bottom = lockData.getY() + lockData.getHeight();
		hresult = D3DXLoadSurfaceFromSurface(surface, NULL, &dest, plainSurface, NULL, NULL, D3DX_FILTER_NONE, 0);
		FATAL_DX_HR("D3DXLoadSurfaceFromSurface failed %s", hresult);

		// free the resources
		surface->Release();
		plainSurface->Release();*/

		Direct3d11::getDeviceContext()->Unmap(m_d3dTexture, 0);
	}
	else
	{
		// all we have to do is unlock
		/*if (m_engineTexture.isCubeMap())
		{
			HRESULT result = static_cast<IDirect3DCubeTexture9*>(m_d3dTexture)->UnlockRect(Direct3d9::getD3dCubeFace(lockData.m_cubeFace), lockData.getLevel());
			FATAL_DX_HR("failed to unlock rect %s", result);
		}
		else if (m_engineTexture.isVolumeMap())
		{
			HRESULT result = static_cast<IDirect3DVolumeTexture9*>(m_d3dTexture)->UnlockBox(lockData.getLevel());
			FATAL_DX_HR("failed to unlock box %s", result);
		}
		else
		{
			// unlock the surface
			HRESULT result = static_cast<IDirect3DTexture9*>(m_d3dTexture)->UnlockRect(lockData.getLevel());
			FATAL_DX_HR("failed to unlock rect %s", result);
		}*/
		Direct3d11::getDeviceContext()->Unmap(m_d3dTexture, 0);
	}

	// clear out the lock data
	lockData.m_pitch      = 0;
	lockData.m_slicePitch = 0;
	lockData.m_pixelData  = nullptr;
	lockData.m_reserved   = nullptr;
}

//----------------------------------------------------------------------

void  Direct3d11_TextureData::copyFrom(int surfaceLevel, TextureGraphicsData const & rhs, int srcX, int srcY, int srcWidth, int srcHeight, int dstX, int dstY, int dstWidth, int dstHeight)
{
	/*Direct3d22_TextureData const * const rhs_d3d = safe_cast<Direct3d11_TextureData const *>(&rhs);

	FATAL(this == rhs_d3d, ("Direct3d9_TextureData::copyFrom() src & dst may not be the same texture"));

	RECT const rectDst = { dstX, dstY, dstX + dstWidth, dstY + dstHeight };
	RECT const rectSrc = { srcX, srcY, srcX + srcWidth, srcY + srcHeight };

	// get the texture surface that we want to update
	IDirect3DSurface9 * surfaceSrc = 0;
	IDirect3DSurface9 * surfaceDst = 0;

	if (m_engineTexture.isCubeMap())
	{
		FATAL(true, ("Can't copy cube maps (yet)"));
	}
	else if (m_engineTexture.isVolumeMap())
	{
		FATAL(true, ("Can't copy volume maps (yet)"));
	}
	else
	{
		HRESULT const hresult = static_cast<IDirect3DTexture9*>(m_d3dTexture)->GetSurfaceLevel(surfaceLevel, &surfaceDst);
	}

	if (rhs_d3d->m_engineTexture.isCubeMap())
	{
		FATAL(true, ("Direct3d9_TextureData::copyFrom() Can't copy cube maps (yet)"));
	}
	else if (rhs_d3d->m_engineTexture.isVolumeMap())
	{
		FATAL(true, ("Can't copy volume maps (yet)"));
	}
	else
	{
		HRESULT const hresult = static_cast<IDirect3DTexture9*>(rhs_d3d->m_d3dTexture)->GetSurfaceLevel(surfaceLevel, &surfaceSrc);
	}

	HRESULT const hresult = D3DXLoadSurfaceFromSurface(surfaceDst, NULL, &rectDst, surfaceSrc, NULL, &rectSrc, D3DX_FILTER_NONE, 0);

	surfaceDst->Release();
	surfaceSrc->Release();*/
}

// ----------------------------------------------------------------------

ID3D11Texture2D* create2dTexture(int width, int height, int mipmapLevelCount, TextureFormat textureFormat)
{
	ID3D11Device* device = Direct3d11::getDevice();
	ID3D11Texture2D* newTexture = nullptr;
	
	if(!device)
	{
		FATAL(true, ("Create2DTexture() : Tried to create a texture with an invalid device.\n"));
	}

	D3D11_TEXTURE2D_DESC desc;
    memset(&desc, 0, sizeof(desc));

	desc.Width = width;
	desc.Height = height;
	desc.MipLevels = desc.ArraySize = mipmapLevelCount;
	desc.Format = translationTable[textureFormat];
	desc.SampleDesc.Count = 1;
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	desc.MiscFlags = 0;
	desc.Usage = D3D11_USAGE_DEFAULT;
	
	HRESULT result = device->CreateTexture2D(&desc, nullptr, &newTexture);

	if(!newTexture || FAILED(result))
	{
		FATAL(true, ("Create2DTexture(): Failed to create a texture. D3D Error Code: [%i] \n", result));
	}

	return newTexture;
}

// ======================================================================
