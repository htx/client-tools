#ifndef INCLUDED_Direct3d11_StaticShaderData_H
#define INCLUDED_Direct3d11_StaticShaderData_H

// ======================================================================

class Material;
class Direct3d11_TextureData;

#include "Direct3d11_VertexShaderUtilities.h"
#include "clientGraphics/ShaderImplementation.h"
#include "clientGraphics/StaticShader.h"
#include "../../../../../../engine/shared/library/sharedFoundation/include/public/sharedFoundation/Production.h"
#include "sharedMath/VectorRgba.h"

#include <vector>
#include <d3d11.h>

// ======================================================================

class Direct3d11_StaticShaderData : public StaticShaderGraphicsData
{
public:

	static void install();
	static void beginFrame();

	explicit Direct3d11_StaticShaderData(const StaticShader &shader);
	virtual ~Direct3d11_StaticShaderData();

	void								update(const StaticShader &shader) override;
	intptr_t							getTextureSortKey() const override;

	bool                                isValid() const;
	bool                                apply(int pass) const;

private:

	class Stage
	{
	private:

		struct SamplerState
		{
		//	D3DSAMPLERSTATETYPE            state;
			DWORD                          value;
		//	SamplerState(D3DSAMPLERSTATETYPE s, DWORD v) : state(s), value(v) {}
		};

		typedef std::vector<SamplerState>   SamplerStates;
		bool                                 m_placeholder;
		Direct3d11_TextureData const * const *m_texture;
		SamplerStates                        m_samplerStates;

	public:
		Stage();

		void construct(const StaticShader &shader, const ShaderImplementation::Pass::PixelShader::TextureSampler &textureSampler);
		bool getTextureSortKey(intptr_t &value) const;
		void apply(int stage) const;
	};

	typedef std::vector<Stage> Stages;

	class Pass
	{
	public:

		// this struct exists so that we can use a pointer to it to set vertex shader constants
		_declspec(align(16))
		struct PaddedMaterial
		{
			//D3DMATERIAL11                      material;
			float                             pad1;
			float                             pad2;
			float                             pad3;
		};

	private:
		bool                                            m_materialValid;
		bool                                            m_textureFactorValid;
		bool                                            m_alphaTestReferenceValueValid;
		bool                                            m_stencilReferenceValueValid;
		bool                                            m_fullAmbient;
		bool                                            m_textureScrollValid;

		ID3D11VertexShader *							m_vertexShader;
		PaddedMaterial                                  m_material;
		VectorRgba                                      m_textureFactorData[2];

		float                                           m_textureScroll[4];
		uint8                                           m_alphaTestReferenceValue;
		uint32                                          m_stencilReferenceValue;
		ShaderImplementation::Pass::FogMode             m_fogMode;
		Stages                                          m_stage;

		static ID3D11Buffer*							ms_constantBufferMaterial;
		static ID3D11Buffer*							ms_constantBufferTextureFactor;
		static ID3D11Buffer*							ms_constantBufferTextureScroll;

		static ID3D11Buffer*							ms_constantBufferPSMaterialSpecularColor;
		static ID3D11Buffer*							ms_constantBufferPSTextureFactor;

	public:

		static void install();

		void construct(const StaticShader &shader, const ShaderImplementation::Pass &pass);
		bool getTextureSortKey(intptr_t &value) const;
		bool apply() const;
	};

	typedef std::vector<Pass> Passes;

private:

	/// Disabled.
	Direct3d11_StaticShaderData();
	Direct3d11_StaticShaderData(const Direct3d11_StaticShaderData &);
	Direct3d11_StaticShaderData &operator =(const Direct3d11_StaticShaderData &);

	void construct(const StaticShader &shader);
	bool validate();

	const ShaderImplementation *m_implementation;
	Passes                      m_pass;
};

// ======================================================================

inline bool Direct3d11_StaticShaderData::isValid() const
{
	return m_implementation != nullptr;
}

// ======================================================================

#endif