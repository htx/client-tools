#include "FirstDirect3d11.h"
#include "Direct3d11_StaticShaderData.h"

#include "Direct3d11.h"
#include "Direct3d11_LightManager.h"
#include "Direct3d11_PixelShaderConstantRegisters.h"
#include "Direct3d11_ShaderImplementationData.h"
#include "Direct3d11_StateCache.h"
#include "Direct3d11_TextureData.h"
#include "Direct3d11_VertexShaderConstantRegisters.h"
#include "Direct3d11_VertexShaderData.h"

#include "clientGraphics/Material.h"
#include "clientGraphics/ShaderEffect.h"
#include "clientGraphics/ShaderImplementation.h"
#include "sharedDebug/DataLint.h"
#include "sharedDebug/DebugFlags.h"
#include "sharedFoundation/Os.h"
#include "sharedFoundation/CrcLowerString.h"

#include <d3d11.h>

// ======================================================================

ID3D11Buffer*	Direct3d11_StaticShaderData::Pass::ms_constantBufferMaterial;
ID3D11Buffer*	Direct3d11_StaticShaderData::Pass::ms_constantBufferTextureFactor;
ID3D11Buffer*	Direct3d11_StaticShaderData::Pass::ms_constantBufferTextureScroll;
ID3D11Buffer*	Direct3d11_StaticShaderData::Pass::ms_constantBufferPSMaterialSpecularColor;
ID3D11Buffer*	Direct3d11_StaticShaderData::Pass::ms_constantBufferPSTextureFactor;

// ======================================================================

namespace Direct3d11_StaticShaderDataNamespace
{
	const Tag TAG_A255 = TAG(A,2,5,5);
	const Tag TAG_A128 = TAG(A,1,2,8);
	const Tag TAG_A001 = TAG(A,0,0,1);
	const Tag TAG_A000 = TAG(A,0,0,0);

	static const D3DTEXTUREADDRESS TextureAddress[] =
	{
		D3DTADDRESS_WRAP,                          // TA_wrap
		D3DTADDRESS_MIRROR,                        // TA_mirror
		D3DTADDRESS_CLAMP,                         // TA_clamp
		D3DTADDRESS_BORDER,                        // TA_border
		D3DTADDRESS_MIRRORONCE,                    // TA_mirrorOnce

		D3DTADDRESS_WRAP,                          // TA_invalid
	};

	static const D3DTEXTUREFILTERTYPE TextureFilter[] =
	{
		D3DTEXF_NONE,                              // TF_none
		D3DTEXF_POINT,                             // TF_point
		D3DTEXF_LINEAR,                            // TF_linear
		D3DTEXF_ANISOTROPIC,                       // TF_anisotropic
		D3DTEXF_NONE,                              // TF_flatCubic
		D3DTEXF_NONE,                              // TF_gaussianCubic

		D3DTEXF_LINEAR                             // TF_invalid
	};

	Direct3d11_StaticShaderData const * ms_active;	
	int                                ms_pass;	
	bool                               ms_usesVertexShader;
}

using namespace Direct3d11_StaticShaderDataNamespace;

//======================================================================

#ifdef _DEBUG

static const char *ConvertTagToStaticString(Tag tag)
{
	static char buffer[5];
	ConvertTagToString(tag, buffer);
	return buffer;
}

#endif

//======================================================================

Direct3d11_StaticShaderData::Stage::Stage()
:
	m_placeholder(false),
	m_texture(NULL)
{
}

// ----------------------------------------------------------------------

void Direct3d11_StaticShaderData::Stage::construct(const StaticShader &shader, const ShaderImplementation::Pass::PixelShader::TextureSampler &textureSampler)
{
	if (textureSampler.m_textureTag)
	{
		StaticShaderTemplate::TextureData textureData;
		const bool result = shader.getTextureData(textureSampler.m_textureTag, textureData);
		if (!result)
		{
			textureData.placeholder         = false;
			textureData.addressU            = StaticShaderTemplate::TA_wrap;
			textureData.addressV            = StaticShaderTemplate::TA_wrap;
			textureData.addressW            = StaticShaderTemplate::TA_wrap;
			textureData.mipFilter           = StaticShaderTemplate::TF_linear;
			textureData.minificationFilter  = StaticShaderTemplate::TF_linear;
			textureData.magnificationFilter = StaticShaderTemplate::TF_linear;
			textureData.maxAnisotropy       = 1;
			textureData.texture             = 0;
		}

		if (Direct3d11_TextureData::isGlobalTexture(textureSampler.m_textureTag))
		{
			m_placeholder = false;
			m_texture = Direct3d11_TextureData::getGlobalTexture(textureSampler.m_textureTag);
		}
		else
		{
			m_placeholder = textureData.placeholder;
			if (textureData.texture)
				m_texture = reinterpret_cast<Direct3d11_TextureData const * const *>(textureData.texture->getGraphicsDataAddress());
			else
				m_texture = nullptr;
		}

		const uint count = 7;
		m_samplerStates.reserve(count);
		m_samplerStates.clear();

#define TA(a,b) StaticShaderTemplate::TextureAddress a = textureData.a != StaticShaderTemplate::TA_invalid ? textureData.a : static_cast<StaticShaderTemplate::TextureAddress>(textureSampler.b)

		TA(addressU, m_textureAddressU);
		TA(addressV, m_textureAddressV);
		TA(addressW, m_textureAddressW);

#undef TA

		DEBUG_WARNING(DataLint::isEnabled () && (addressU == StaticShaderTemplate::TA_invalid || addressV == StaticShaderTemplate::TA_invalid || addressW == StaticShaderTemplate::TA_invalid), ("Old shader %s with new effect", shader.getStaticShaderTemplate().getName().getString()));

#define TF(f,g) StaticShaderTemplate::TextureFilter f  = textureData.f != StaticShaderTemplate::TF_invalid ? textureData.f : static_cast<StaticShaderTemplate::TextureFilter>(textureSampler.g)

		TF(mipFilter,           m_textureMipFilter);
		TF(minificationFilter,  m_textureMinificationFilter);
		TF(magnificationFilter, m_textureMagnificationFilter);

#undef TF

	/*	m_samplerStates.push_back(SamplerState(D3DSAMP_MAXANISOTROPY, clamp(static_cast<DWORD>(1), static_cast<DWORD>(textureData.maxAnisotropy), Direct3d11::getMaxAnisotropy())));

#define TSSM(tss, v, m) m_samplerStates.push_back(SamplerState(tss, m[v]))

		TSSM(D3DSAMP_ADDRESSU,   addressU,             TextureAddress);
		TSSM(D3DSAMP_ADDRESSV,   addressV,             TextureAddress);
		TSSM(D3DSAMP_ADDRESSW,   addressW,             TextureAddress);
		TSSM(D3DSAMP_MIPFILTER,  mipFilter,            TextureFilter);
		TSSM(D3DSAMP_MINFILTER,  minificationFilter,   TextureFilter);
		TSSM(D3DSAMP_MAGFILTER,  magnificationFilter,  TextureFilter);*/

#undef TSSM
	}
	else
	{
		m_placeholder = false;
		m_texture = NULL;
	}
}

// ----------------------------------------------------------------------

bool Direct3d11_StaticShaderData::Stage::getTextureSortKey(intptr_t &value) const
{
	if (m_texture)
	{
		value = reinterpret_cast<intptr_t>((*m_texture)->getBaseTexture());
		return true;
	}

	return false;
}

// ----------------------------------------------------------------------

void Direct3d11_StaticShaderData::Stage::apply(int stage) const
{
	if (m_placeholder)
	{
		DEBUG_WARNING(true, ("Trying to render with a place holder texture"));
		const_cast<Stage*>(this)->m_placeholder = false;
	}

	if (m_texture)
	{
		Direct3d11_StateCache::setTexture(stage, *m_texture);

		const SamplerStates::const_iterator end = m_samplerStates.end();
		for (SamplerStates::const_iterator i = m_samplerStates.begin(); i != end; ++i)
		{
			const SamplerState &sampler = *i;
		//	Direct3d11_StateCache::setSamplerState(stage, sampler.state, sampler.value);
		}
	}
	else
		Direct3d11_StateCache::setTexture(stage, NULL);
}

// ======================================================================

void Direct3d11_StaticShaderData::Pass::install()
{
	D3D11_BUFFER_DESC constantBufferDesc;   // create the constant buffer

	ZeroMemory(&constantBufferDesc, sizeof(constantBufferDesc));

	constantBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	constantBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	constantBufferDesc.ByteWidth = sizeof(PaddedMaterial);
	constantBufferDesc.CPUAccessFlags = 0;
	constantBufferDesc.MiscFlags = 0;
	constantBufferDesc.StructureByteStride = 0;
	
	HRESULT hr = Direct3d11::getDevice()->CreateBuffer(&constantBufferDesc, nullptr, &ms_constantBufferMaterial);

	FATAL(FAILED(hr), ("create constant buffer fail."));

	constantBufferDesc.ByteWidth = sizeof(VectorRgba);
	Direct3d11::getDevice()->CreateBuffer(&constantBufferDesc, nullptr, &ms_constantBufferTextureFactor);
	Direct3d11::getDevice()->CreateBuffer(&constantBufferDesc, nullptr, &ms_constantBufferPSTextureFactor);

	constantBufferDesc.ByteWidth = sizeof(float[4]);
	Direct3d11::getDevice()->CreateBuffer(&constantBufferDesc, nullptr, &ms_constantBufferTextureScroll);
}

// ----------------------------------------------------------------------

void Direct3d11_StaticShaderData::Pass::construct(const StaticShader &shader, const ShaderImplementation::Pass &pass)
{
	// pack the requested texture coordinate sets into uint32. Since each texture coordinate set index can only be a value between
	// 0 and 7, we can fit this into 3 bits.  Since D3D9 only supports 8 texture coordinate sets, this requires a total of 24 bits.
	// We process them in reverse order so we can index into the list by right-shifting.
	uint32 textureCoordinateSetKey = 0;
	ShaderImplementation::Pass::VertexShader const & vertexShader = *pass.m_vertexShader;

	if(pass.m_vertexShader == nullptr)
	{
		return;
	}
	
	Direct3d11_VertexShaderData const * vertexShaderData = dynamic_cast<Direct3d11_VertexShaderData const *>(vertexShader.m_graphicsData);
	Direct3d11_VertexShaderData::TextureCoordinateSetTags const * textureCoordinateSetTags = vertexShaderData->getTextureCoordinateSetTags();
	
	if (textureCoordinateSetTags)
	{
		const size_t numberOfTextureCoordinateSetTags = textureCoordinateSetTags->size();		
		DEBUG_FATAL(numberOfTextureCoordinateSetTags > 8, ("too many texture coordinate sets"));
		
		for (size_t i = 0; i < numberOfTextureCoordinateSetTags ; ++i)
		{
			uint8 textureCoordinate = 0;
			if (!shader.getTextureCoordinateSet((*textureCoordinateSetTags)[i], textureCoordinate))
			{
				DEBUG_WARNING(true, ("Missing texture coordinate set tag %s for shader %s, defaulting to 0", ConvertTagToStaticString((*textureCoordinateSetTags)[i]), shader.getShaderTemplate().getName().getString()));
				textureCoordinate = 0;
			}
			
			if (textureCoordinate > 7)
			{
				DEBUG_WARNING(true, ("shader [%s]: texture coordinate out of range 0/%d/7, resetting to 0", shader.getName() ? shader.getName() : "<NULL shader name>", static_cast<int>(textureCoordinate)));
				textureCoordinate = 0;
			}

			textureCoordinateSetKey = textureCoordinateSetKey | (textureCoordinate << (i * 3));
		}
	}

	m_vertexShader = vertexShaderData->getVertexShader(textureCoordinateSetKey);
	
	if (pass.m_materialTag)
	{
	/*	Material material;
		const bool result = shader.getMaterial(pass.m_materialTag, material);
		if (result)
		{
			m_material.material.Ambient.r  = material.getAmbientColor().r;
			m_material.material.Ambient.g  = material.getAmbientColor().g;
			m_material.material.Ambient.b  = material.getAmbientColor().b;
			m_material.material.Ambient.a  = material.getAmbientColor().a;

			m_material.material.Diffuse.r  = material.getDiffuseColor().r;
			m_material.material.Diffuse.g  = material.getDiffuseColor().g;
			m_material.material.Diffuse.b  = material.getDiffuseColor().b;
			m_material.material.Diffuse.a  = material.getDiffuseColor().a;

			m_material.material.Emissive.r = material.getEmissiveColor().r;
			m_material.material.Emissive.g = material.getEmissiveColor().g;
			m_material.material.Emissive.b = material.getEmissiveColor().b;
			m_material.material.Emissive.a = material.getEmissiveColor().a;

			m_material.material.Specular.r = material.getSpecularColor().r;
			m_material.material.Specular.g = material.getSpecularColor().g;
			m_material.material.Specular.b = material.getSpecularColor().b;
			m_material.material.Specular.a = material.getSpecularColor().a;

			m_material.material.Power      = material.getSpecularPower();
			m_material.pad1                = 0.0f;
			m_material.pad2                = 0.0f;
			m_material.pad3                = 0.0f;
		}
		else
		{
			DEBUG_WARNING(true, ("Could not find material tag %s in shader %s", ConvertTagToStaticString(pass.m_materialTag), shader.getName()));
			m_material.material.Ambient.r  = 1.0;
			m_material.material.Ambient.g  = 1.0;
			m_material.material.Ambient.b  = 1.0;
			m_material.material.Ambient.a  = 1.0;

			m_material.material.Diffuse.r  = 1.0;
			m_material.material.Diffuse.g  = 1.0;
			m_material.material.Diffuse.b  = 1.0;
			m_material.material.Diffuse.a  = 1.0;

			m_material.material.Emissive.r = 0.0;
			m_material.material.Emissive.g = 0.0;
			m_material.material.Emissive.b = 0.0;
			m_material.material.Emissive.a = 1.0;

			m_material.material.Specular.r = 0.0;
			m_material.material.Specular.g = 0.0;
			m_material.material.Specular.b = 0.0;
			m_material.material.Specular.a = 1.0;

			m_material.material.Power      = 0.0;
			m_material.pad1                = 0.0f;
			m_material.pad2                = 0.0f;
			m_material.pad3                = 0.0f;
		}*/

		m_materialValid       = true;
	}
	else
		m_materialValid = false;

	m_textureFactorValid = false;
	if (pass.m_textureFactorTag)
	{
		uint32 textureFactor = 0;
		const bool result = shader.getTextureFactor(pass.m_textureFactorTag, textureFactor);
		if (result)
		{
			m_textureFactorValid = true;
			m_textureFactorData[0].r = static_cast<float>((textureFactor >> 16) & 0xff) / 255.0f;
			m_textureFactorData[0].g = static_cast<float>((textureFactor >>  8) & 0xff) / 255.0f;
			m_textureFactorData[0].b = static_cast<float>((textureFactor >>  0) & 0xff) / 255.0f;
			m_textureFactorData[0].a = static_cast<float>((textureFactor >> 24) & 0xff) / 255.0f;
		}
		else
			DEBUG_WARNING(true, ("Could not find texture factor %s in %s", ConvertTagToStaticString(pass.m_textureFactorTag), shader.getShaderTemplate().getName().getString()));
	}
	if (pass.m_textureFactorTag2)
	{
		WARNING(!pass.m_textureFactorTag, ("Shader has textureFactor2 but no textureFactor"));

		uint32 textureFactor2 = 0;
		const bool result = shader.getTextureFactor(pass.m_textureFactorTag2, textureFactor2);
		if (result)
		{
			m_textureFactorValid = true;

			m_textureFactorData[1].r = static_cast<float>((textureFactor2 >> 16) & 0xff) / 255.0f;
			m_textureFactorData[1].g = static_cast<float>((textureFactor2 >>  8) & 0xff) / 255.0f;
			m_textureFactorData[1].b = static_cast<float>((textureFactor2 >>  0) & 0xff) / 255.0f;
			m_textureFactorData[1].a = static_cast<float>((textureFactor2 >> 24) & 0xff) / 255.0f;
		}
		else
			DEBUG_WARNING(true, ("Could not find texture factor %s in %s", ConvertTagToStaticString(pass.m_textureFactorTag), shader.getShaderTemplate().getName().getString()));
	}

	if (pass.m_textureScrollTag)
	{
		StaticShaderTemplate::TextureScroll textureScroll;
		if (shader.getTextureScroll(pass.m_textureScrollTag, textureScroll))
		{
			m_textureScrollValid = true;
			m_textureScroll[0] = textureScroll.u1;
			m_textureScroll[1] = textureScroll.v1;
			m_textureScroll[2] = textureScroll.u2;
			m_textureScroll[3] = textureScroll.v2;
		}
		else
		{
			DEBUG_WARNING(true, ("Could not find texture scroll tag %s for shader %s", ConvertTagToStaticString(pass.m_textureScrollTag), shader.getName()));
			m_textureScrollValid = true;
			m_textureScroll[0] = 0.0f;
			m_textureScroll[1] = 0.0f;
			m_textureScroll[2] = 0.0f;
			m_textureScroll[3] = 0.0f;
		}
	}
	else
	{
		m_textureScrollValid = false;
	}

	if (pass.m_alphaTestEnable)
	{
		m_alphaTestReferenceValueValid = true;
		if (pass.m_alphaTestReferenceValueTag == TAG_A255)
			m_alphaTestReferenceValue = 255;
		else
			if (pass.m_alphaTestReferenceValueTag == TAG_A128)
				m_alphaTestReferenceValue = 128;
			else
				if (pass.m_alphaTestReferenceValueTag == TAG_A001)
					m_alphaTestReferenceValue = 1;
				else
					if (pass.m_alphaTestReferenceValueTag == TAG_A000)
						m_alphaTestReferenceValue = 0;
					else
						if (!shader.getAlphaTestReferenceValue(pass.m_alphaTestReferenceValueTag, m_alphaTestReferenceValue))
						{
							// @todo @e3hack put this back in
							DEBUG_WARNING(true, ("Could not find alpha reference value %s for shader %s, defaulting to 1", ConvertTagToStaticString(pass.m_alphaTestReferenceValueTag), shader.getName()));
							m_alphaTestReferenceValue = 1;
						}
	}
	else
		m_alphaTestReferenceValueValid = false;

	if (pass.m_stencilEnable)
	{
		const bool result = shader.getStencilReferenceValue(pass.m_stencilReferenceValueTag, m_stencilReferenceValue);
		if (result)
			m_stencilReferenceValueValid = true;
		else
			DEBUG_WARNING(true, ("Could not find stencil reference value"));
	}
	else
		m_stencilReferenceValueValid = false;

	m_fullAmbient = shader.containsPrecalculatedVertexLighting();
	m_fogMode = pass.m_fogMode;

	// construct the shared data for each texture texture sample ("stage")
	// not all source texture stages have to exist, but we'll create all the destination "stages" so that the rendering code can be more simple.
	m_stage.resize(pass.m_pixelShader->m_maxTextureSampler + 1);
	ShaderImplementation::Pass::PixelShader::TextureSamplers::const_iterator end = pass.m_pixelShader->m_textureSamplers->end();
	for (ShaderImplementation::Pass::PixelShader::TextureSamplers::const_iterator i = pass.m_pixelShader->m_textureSamplers->begin(); i != end; ++i)
		m_stage[(*i)->m_textureIndex].construct(shader, **i);
}

// ----------------------------------------------------------------------

bool Direct3d11_StaticShaderData::Pass::getTextureSortKey(intptr_t &value) const
{
	for(const auto& i : m_stage)
	{
		if (i.getTextureSortKey(value))
			return true;
	}

	return false;
}

// ----------------------------------------------------------------------

bool Direct3d11_StaticShaderData::Pass::apply() const
{
	Direct3d11_StateCache::setVertexShader(m_vertexShader);

	if (m_materialValid)
	{
		DEBUG_FATAL(sizeof(m_material) != sizeof(float) * 4 * 5, ("PaddedMaterial size is wrong %d/%d", sizeof(m_material), sizeof(float) * 4 * 5));

		Direct3d11::getDeviceContext()->UpdateSubresource(ms_constantBufferMaterial, 0, nullptr, &m_material, 0, 0);
		Direct3d11_StateCache::setVertexShaderConstants(VSCR_material, ms_constantBufferMaterial, 5);

		//Direct3d11::getDeviceContext()->UpdateSubresource(ms_constantBufferPSMaterialSpecularColor, 0, nullptr, &m_material.material.Specular, 0, 0);
		//Direct3d11_StateCache::setPixelShaderConstants(PSCR_materialSpecularColor, ms_constantBufferPSMaterialSpecularColor, 1);
		//Direct3d11_StateCache::setSpecularPower(m_material.material.Power);
	}

	if (m_textureFactorValid)
	{
		Direct3d11::getDeviceContext()->UpdateSubresource(ms_constantBufferTextureFactor, 0, nullptr, &m_textureFactorData, 0, 0);
		Direct3d11_StateCache::setVertexShaderConstants(VSCR_textureFactor, ms_constantBufferTextureFactor, 2);

		Direct3d11::getDeviceContext()->UpdateSubresource(ms_constantBufferPSTextureFactor, 0, nullptr, &m_textureFactorData, 0, 0);
		Direct3d11_StateCache::setPixelShaderConstants(PSCR_textureFactor, ms_constantBufferPSTextureFactor, 2);
	}
	
	if (m_textureScrollValid)
	{
		float scroll[4];
		float const currentTime = Direct3d11::getCurrentTime();
		double junk;
		scroll[0] = static_cast<float>(modf(m_textureScroll[0] * currentTime, &junk));
		scroll[1] = static_cast<float>(modf(m_textureScroll[1] * currentTime, &junk));
		scroll[2] = static_cast<float>(modf(m_textureScroll[2] * currentTime, &junk));
		scroll[3] = static_cast<float>(modf(m_textureScroll[3] * currentTime, &junk));

		Direct3d11::getDeviceContext()->UpdateSubresource(ms_constantBufferTextureScroll, 0, nullptr, scroll, 0, 0);
		Direct3d11_StateCache::setVertexShaderConstants(VSCR_textureScroll, ms_constantBufferTextureScroll, 1);
	}

	if (m_alphaTestReferenceValueValid)
		Direct3d11::setAlphaTestReferenceValue(m_alphaTestReferenceValue);

	/*if (m_stencilReferenceValueValid)
		Direct3d11_StateCache::setRenderState(D3DRS_STENCILREF, m_stencilReferenceValue);

	switch (m_fogMode)
	{
		case ShaderImplementation::Pass::FM_Normal:
 			Direct3d11_StateCache::setRenderState(D3DRS_FOGCOLOR, Direct3d11::getFogColor());
			break;

		case ShaderImplementation::Pass::FM_Black:
 			Direct3d11_StateCache::setRenderState(D3DRS_FOGCOLOR, 0);
			break;

		case ShaderImplementation::Pass::FM_White:
 			Direct3d11_StateCache::setRenderState(D3DRS_FOGCOLOR, static_cast<DWORD>(0xffffffff));
			break;

		default:
			DEBUG_FATAL(true, ("Unknown fog mode"));
	}*/

	int stageNumber = 0;
	Stages::const_iterator end = m_stage.end();
	for (Stages::const_iterator i = m_stage.begin(); i != end; ++i, ++stageNumber)
		i->apply(stageNumber);

	Direct3d11_LightManager::setFullAmbientOn(m_fullAmbient);

	return true;
}

// ======================================================================

void Direct3d11_StaticShaderData::install()
{
	Pass::install();
}

// ----------------------------------------------------------------------

void Direct3d11_StaticShaderData::beginFrame()
{
	ms_active = nullptr;
	ms_pass = -1;
}

// ----------------------------------------------------------------------

Direct3d11_StaticShaderData::Direct3d11_StaticShaderData(const StaticShader &shader)
: StaticShaderGraphicsData(),
	m_implementation(shader.getStaticShaderTemplate().m_effect->m_implementation)
{
	construct(shader);
}

// ----------------------------------------------------------------------

Direct3d11_StaticShaderData::~Direct3d11_StaticShaderData()
{
	if (ms_active == this)
		ms_active = nullptr;
}

// ----------------------------------------------------------------------

void Direct3d11_StaticShaderData::construct(const StaticShader &shader)
{
	m_pass.resize(m_implementation->m_pass->size());
	
	ShaderImplementation::Passes::const_iterator j = m_implementation->m_pass->begin();
	const Passes::iterator end = m_pass.end();
	
	for (Passes::iterator i = m_pass.begin(); i != end; ++i, ++j)
		i->construct(shader, **j);
}

// ----------------------------------------------------------------------

intptr_t Direct3d11_StaticShaderData::getTextureSortKey() const
{
	const Passes::const_iterator end = m_pass.end();
	
	for (Passes::const_iterator i = m_pass.begin(); i != end; ++i)
	{
		intptr_t value = 0;
		
		if (i->getTextureSortKey(value))
			return value;
	}

	return 0;
}

// ----------------------------------------------------------------------

void Direct3d11_StaticShaderData::update(const StaticShader &shader)
{
	construct(shader);

	// if you change the active shader, make sure it gets updated
	if (ms_active == this)
	{
		ms_active = NULL;
		apply(ms_pass);
	}
}

// ----------------------------------------------------------------------

bool Direct3d11_StaticShaderData::apply(int pass) const
{
	if (ms_active != this || ms_pass != pass)
	{
		ms_active = this;
		ms_pass = pass;
		static_cast<const Direct3d11_ShaderImplementationData *>(m_implementation->m_graphicsData)->apply(pass);
		ms_usesVertexShader = m_pass[pass].apply();
	}

	return ms_usesVertexShader;
}

// ======================================================================
