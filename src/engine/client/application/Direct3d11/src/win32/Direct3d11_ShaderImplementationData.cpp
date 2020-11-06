//
// Direct3d9_ShaderImplementationData.cpp
// copyright (c) 2001 Sony Online Entertainment
//
// ======================================================================

#include "FirstDirect3d11.h"
#include "Direct3d11_ShaderImplementationData.h"

#include "Direct3d11.h"
#include "Direct3d11_StateCache.h"
#include "Direct3d11_PixelShaderProgramData.h"
#include "Direct3d11_VertexShaderData.h"

#include "clientGraphics/GraphicsOptionTags.h"
#include "sharedDebug/DebugFlags.h"
#include "sharedFoundation/ExitChain.h"

#include <algorithm>

// ======================================================================

namespace Direct3d11_ShaderImplementationDataNamespace
{
	const D3DSHADEMODE ShadeMode[] =
	{
		D3DSHADE_FLAT,                             // SM_Flat
		D3DSHADE_GOURAUD                           // SM_Gouraud
	};

	const D3DCMPFUNC Compare[] =
	{
		D3DCMP_NEVER,                              // C_Never
		D3DCMP_LESS,                               // C_Less
		D3DCMP_EQUAL,                              // C_Equal
		D3DCMP_LESSEQUAL,                          // C_LessOrEqual
		D3DCMP_GREATER,                            // C_Greater
		D3DCMP_NOTEQUAL,                           // C_GreaterOrEqual
		D3DCMP_GREATEREQUAL,                       // C_NotEqual
		D3DCMP_ALWAYS                              // C_Always
	};

	const D3DBLEND Blend[] =
	{
		D3DBLEND_ZERO,                             // B_Zero
		D3DBLEND_ONE,                              // B_One
		D3DBLEND_SRCCOLOR,                         // B_SourceColor
		D3DBLEND_INVSRCCOLOR,                      // B_InverseSourceColor
		D3DBLEND_SRCALPHA,                         // B_SourceAlpha
		D3DBLEND_INVSRCALPHA,                      // B_InverseSourceAlpha
		D3DBLEND_DESTALPHA,                        // B_DestinationAlpha
		D3DBLEND_INVDESTALPHA,                     // B_InverseDestinationAlpha
		D3DBLEND_DESTCOLOR,                        // B_DestinationColor
		D3DBLEND_INVDESTCOLOR,                     // B_InverseDestinationColor
		D3DBLEND_SRCALPHASAT                       // B_SourceAlphaSaturate
	};

	const D3DBLENDOP BlendOperation[] =
	{
		D3DBLENDOP_ADD,                            // BO_Add
		D3DBLENDOP_SUBTRACT,                       // BO_Subtract
		D3DBLENDOP_REVSUBTRACT,                    // BO_ReverseSubtract
		D3DBLENDOP_MIN,                            // BO_Min
		D3DBLENDOP_MAX                             // BO_Max
	};

	const D3DMATERIALCOLORSOURCE MaterialSource[] =
	{
		D3DMCS_MATERIAL,                           // MS_Material
		D3DMCS_COLOR1,                             // MS_VertexColor1
		D3DMCS_COLOR2                              // MS_VertexColor2
	};

	const D3DSTENCILOP StencilOperation[] =
	{
		D3DSTENCILOP_KEEP,                         // SO_Keep
		D3DSTENCILOP_ZERO,                         // SO_Zero
		D3DSTENCILOP_REPLACE,                      // SO_Replace
		D3DSTENCILOP_INCRSAT,                      // SO_IncrementSaturate
		D3DSTENCILOP_DECRSAT,                      // SO_DecrementSaturate
		D3DSTENCILOP_INVERT,                       // SO_Invert
		D3DSTENCILOP_INCR,                         // SO_IncrementWrap
		D3DSTENCILOP_DECR                          // SO_DecrementWrap
	};

	const D3DTEXTUREOP TextureOperation[] =
	{
		D3DTOP_DISABLE,                            // TO_disable
		D3DTOP_SELECTARG1,                         // TO_selectArg1
		D3DTOP_SELECTARG2,                         // TO_selectArg2
		D3DTOP_MODULATE,                           // TO_modulate
		D3DTOP_MODULATE2X,                         // TO_modulate2x
		D3DTOP_MODULATE4X,                         // TO_modulate4x
		D3DTOP_ADD,                                // TO_add
		D3DTOP_ADDSIGNED,                          // TO_addSigned
		D3DTOP_ADDSIGNED2X,                        // TO_addSigned2x
		D3DTOP_SUBTRACT,                           // TO_subtract
		D3DTOP_ADDSMOOTH,                          // TO_addSmooth
		D3DTOP_BLENDDIFFUSEALPHA,                  // TO_blendDiffuseAlpha
		D3DTOP_BLENDTEXTUREALPHA,                  // TO_blendTextureAlpha
		D3DTOP_BLENDFACTORALPHA,                   // TO_blendFactorAlpha
		D3DTOP_BLENDTEXTUREALPHAPM,                // TO_blendTextureAlphapm
		D3DTOP_BLENDCURRENTALPHA,                  // TO_blendCurrentAlpha
		D3DTOP_PREMODULATE,                        // TO_premodulate
		D3DTOP_MODULATEALPHA_ADDCOLOR,             // TO_modulateAlpha_addColor
		D3DTOP_MODULATECOLOR_ADDALPHA,             // TO_modulateColor_addAlpha
		D3DTOP_MODULATEINVALPHA_ADDCOLOR,          // TO_modulateInvalpha_addColor
		D3DTOP_MODULATEINVCOLOR_ADDALPHA,          // TO_modulateInvcolor_addAlpha
		D3DTOP_BUMPENVMAP,                         // TO_bumpEnvMap
		D3DTOP_BUMPENVMAPLUMINANCE,                // TO_bumpEnvMapLuminance
		D3DTOP_DOTPRODUCT3,                        // TO_dotProduct3
		D3DTOP_MULTIPLYADD,                        // TO_multiplyAdd
		D3DTOP_LERP                                // TO_lerp
	};

	const DWORD TextureArgument[] =
	{
		D3DTA_CURRENT,                             // TA_current
		D3DTA_DIFFUSE,                             // TA_diffuse
		D3DTA_SPECULAR,                            // TA_specular
		D3DTA_TEMP,                                // TA_temp
		D3DTA_TEXTURE,                             // TA_texture
		D3DTA_TFACTOR                              // TA_textureFactor
	};

#ifdef _DEBUG
	bool                                       ms_disableShaderImplementationCaching;
#endif
	Direct3d11_ShaderImplementationData const * ms_activeImplementation;
	int                                        ms_activePass = -1;
}
using namespace Direct3d11_ShaderImplementationDataNamespace;

// ======================================================================

void Direct3d11_ShaderImplementationData::Stage::construct(const ShaderImplementation::Pass::PixelShader::TextureSampler &textureSampler)
{
	UNREF(textureSampler);
}

// ----------------------------------------------------------------------

void Direct3d11_ShaderImplementationData::Stage::apply(int stageNumber) const
{
	const TextureStageStates::const_iterator end = m_textureStageState.end();
	for (TextureStageStates::const_iterator i = m_textureStageState.begin(); i != end; ++i)
	{
		const TextureStageState &tss = *i;
		//Direct3d11_StateCache::setTextureStageState(stageNumber, tss.state, tss.value);
	}
}

// ======================================================================

Direct3d11_ShaderImplementationData::Pass::Pass()
:
	m_alphaBlendEnable(false),
	m_colorWriteEnable(0),
	m_renderState(),
	m_pixelShader(nullptr)
{
}

// ----------------------------------------------------------------------

Direct3d11_ShaderImplementationData::Pass::~Pass()
{
	m_pixelShader = nullptr;
}

// ----------------------------------------------------------------------

void Direct3d11_ShaderImplementationData::Pass::construct(const ShaderImplementation::Pass &pass)
{
	m_alphaBlendEnable = pass.m_alphaBlendEnable;
	const int count = 17 + (Direct3d11::supportsTwoSidedStencil() ? 5 : 0);
	m_renderState.reserve(count);

#define RSM(rs, v, m) m_renderState.push_back(RenderState(rs, m[pass.v]))
#define RSB(rs, v)    m_renderState.push_back(RenderState(rs, pass.v ? TRUE : FALSE))
#define RSV(rs, v)    m_renderState.push_back(RenderState(rs, pass.v))

	RSM(D3DRS_SHADEMODE,              m_shadeMode,                   ShadeMode);

	RSB(D3DRS_DITHERENABLE,           m_ditherEnable);

	RSB(D3DRS_ZENABLE,                m_zEnable);
	RSB(D3DRS_ZWRITEENABLE,           m_zWrite);
	RSM(D3DRS_ZFUNC,                  m_zCompare,                    Compare);

	RSM(D3DRS_BLENDOP,                m_alphaBlendOperation,         BlendOperation);
	RSM(D3DRS_SRCBLEND,               m_alphaBlendSource,            Blend);
	RSM(D3DRS_DESTBLEND,              m_alphaBlendDestination,       Blend);

	RSB(D3DRS_ALPHATESTENABLE,        m_alphaTestEnable);
	RSM(D3DRS_ALPHAFUNC,              m_alphaTestFunction,           Compare);

	if (pass.m_writeEnable & BINARY1(1000))
		m_colorWriteEnable |= D3DCOLORWRITEENABLE_ALPHA;
	if (pass.m_writeEnable & BINARY1(0100))
		m_colorWriteEnable |= D3DCOLORWRITEENABLE_RED;
	if (pass.m_writeEnable & BINARY1(0010))
		m_colorWriteEnable |= D3DCOLORWRITEENABLE_GREEN;
	if (pass.m_writeEnable & BINARY1(0001))
		m_colorWriteEnable |= D3DCOLORWRITEENABLE_BLUE;

	RSB(D3DRS_STENCILENABLE,          m_stencilEnable);
	RSM(D3DRS_STENCILFUNC,            m_stencilCompareFunction,      Compare);
	RSM(D3DRS_STENCILFAIL,            m_stencilFailOperation,        StencilOperation);
	RSM(D3DRS_STENCILZFAIL,           m_stencilZFailOperation,       StencilOperation);
	RSM(D3DRS_STENCILPASS,            m_stencilPassOperation,        StencilOperation);

	if (Direct3d11::supportsTwoSidedStencil())
	{
		RSB(D3DRS_TWOSIDEDSTENCILMODE,  m_stencilTwoSidedMode);
		RSM(D3DRS_CCW_STENCILFUNC,      m_stencilCounterClockwiseCompareFunction,      Compare);
		RSM(D3DRS_CCW_STENCILFAIL,      m_stencilCounterClockwiseFailOperation,        StencilOperation);
		RSM(D3DRS_CCW_STENCILZFAIL,     m_stencilCounterClockwiseZFailOperation,       StencilOperation);
		RSM(D3DRS_CCW_STENCILPASS,      m_stencilCounterClockwisePassOperation,        StencilOperation);
	}

	RSV(D3DRS_STENCILWRITEMASK,       m_stencilWriteMask);
	RSV(D3DRS_STENCILMASK,            m_stencilMask);

	DEBUG_FATAL(static_cast<int>(m_renderState.size()) != count, ("Reserved the wrong amount %d/%d", count, m_renderState.size()));

#undef RSM
#undef RSB
#undef RSV

	//m_pixelShader = safe_cast<Direct3d11_PixelShaderProgramData const *>(pass.m_pixelShader->m_program->m_graphicsData)->getPixelShader();
}

// ----------------------------------------------------------------------

void Direct3d11_ShaderImplementationData::Pass::apply() const
{
	Direct3d11::setAlphaBlendEnable(m_alphaBlendEnable);
	Direct3d11::setColorWriteEnable(m_colorWriteEnable);

	{
		const RenderStates::const_iterator end = m_renderState.end();
		for (RenderStates::const_iterator i = m_renderState.begin(); i != end; ++i)
		{
			const RenderState &rs = *i;
	//		Direct3d11_StateCache::setRenderState(rs.state, rs.value);
		}
	}
}

// ======================================================================

void Direct3d11_ShaderImplementationData::install()
{
}

// ----------------------------------------------------------------------

void Direct3d11_ShaderImplementationData::lostDevice()
{
	ms_activeImplementation = NULL;
	ms_activePass = -1;
}

// ----------------------------------------------------------------------

Direct3d11_ShaderImplementationData::Direct3d11_ShaderImplementationData(const ShaderImplementation &implementation)
: ShaderImplementationGraphicsData(),
	m_pass(implementation.m_pass->size())
{
	// construct all the per-pass data
	ShaderImplementation::Passes::const_iterator j = implementation.m_pass->begin();
	const Passes::iterator end  = m_pass.end();
	for (Passes::iterator i = m_pass.begin(); i != end; ++i, ++j)
		i->construct(**j);
}

// ----------------------------------------------------------------------

Direct3d11_ShaderImplementationData::~Direct3d11_ShaderImplementationData()
{
	if (ms_activeImplementation == this)
		ms_activeImplementation = NULL;
}

// ----------------------------------------------------------------------

void Direct3d11_ShaderImplementationData::apply(int passNumber) const
{
	if (ms_activeImplementation != this || ms_activePass != passNumber)
	{
		ms_activeImplementation = this;
		ms_activePass = passNumber;

		DEBUG_FATAL(passNumber < 0 || passNumber >= static_cast<int>(m_pass.size()), ("Invalid pass %d/%d", passNumber, m_pass.size()));
		m_pass[passNumber].apply();
	}
}

// ======================================================================
