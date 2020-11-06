#ifndef INCLUDED_Direct3d11_ShaderImplementationData_H
#define INCLUDED_Direct3d11_ShaderImplementationData_H

// ======================================================================

#include "clientGraphics/ShaderImplementation.h"

#include "../../../../../../engine/shared/library/sharedFoundation/include/public/sharedFoundation/Production.h"

#include <d3d9.h>
#include <d3d11.h>
#include <vector>

// ======================================================================

class Direct3d11_ShaderImplementationData : public ShaderImplementationGraphicsData
{
public:

	static void install();
	static void lostDevice();

	Direct3d11_ShaderImplementationData(const ShaderImplementation &implementation);
	virtual ~Direct3d11_ShaderImplementationData();

	void     apply(int passNumber) const;

	template <class T> struct State
	{
		T       state;
		DWORD   value;
		State(T s, DWORD v) : state(s), value(v) {}
	};

	typedef State<D3DRENDERSTATETYPE>        RenderState;
	typedef std::vector<RenderState>         RenderStates;

	typedef State<D3DTEXTURESTAGESTATETYPE > TextureStageState;
	typedef std::vector<TextureStageState>   TextureStageStates;

	class Stage
	{
	public:
		void construct(const ShaderImplementation::Pass::PixelShader::TextureSampler &textureSampler);
		void apply(int stageNumber) const;

		TextureStageStates m_textureStageState;
	};
	typedef std::vector<Stage> Stages;

	class Pass
	{
	public:

		Pass();
		~Pass();

		void construct(const ShaderImplementation::Pass &pass);
		void apply() const;

		bool                   m_alphaBlendEnable;
		uint8                  m_colorWriteEnable;
		RenderStates           m_renderState;
		ID3D11PixelShader	   *m_pixelShader;
	};

	typedef std::vector<Pass> Passes;

private:

	typedef std::vector<const bool *> OptionFlags;
	Passes       m_pass;
};

// ======================================================================

#endif
