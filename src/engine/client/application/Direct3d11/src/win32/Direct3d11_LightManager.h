#ifndef INCLUDED_Direct3d11_LightManager_H
#define INCLUDED_Direct3d11_LightManager_H

// ======================================================================

#include <d3d11.h>

#include "Direct3d11_VertexShaderUtilities.h"
#include "sharedMath/VectorRgba.h"

class Light;
class Transform;
class Vector;
class VectorArgb;

// ======================================================================

class Direct3d11_LightManager
{
	struct SortLightsDistance;
	friend struct SortLightsDistance;

public:

	typedef stdvector<const Light *>::fwd LightList;

	static void install();

	static void beginFrame();

	static void setLights(const LightList &lightList);
	static void setObeysLightScale(bool obeysLightScale);
	static void setObjectToWorldTransform(const Transform &objectToWorldTransform);
	static void setCameraPosition(const Vector &objectPosition);
	static void updateLightDirection();
	static void setUsingVertexShaderProgram(bool usingVertexShaderProgram);

	static void selectLights();

	static void setFullAmbientOn(bool on);

private:

	// disabled
	Direct3d11_LightManager(void);
	Direct3d11_LightManager(const Direct3d11_LightManager &);
	Direct3d11_LightManager &operator =(const Direct3d11_LightManager &);

public:

	enum
	{
		ParallelSpecularCount           = 1,
		ParallelCount                   = 2,
		PointSpecularCount              = 1,
		PointCount                      = 4,
		FixedFunctionPipelineLightCount = 8
	};

	struct SelectedLights
	{
		bool          dirty;
		bool          obeysLightScale;
		VectorRgba    ambient;
		const Light  *parallelSpecular[ParallelSpecularCount];
		const Light  *parallel[ParallelCount];
		const Light  *pointSpecular[PointSpecularCount];
		const Light  *point[PointCount];

		bool operator !=(SelectedLights const &rhs);
		SelectedLights &operator =(SelectedLights const &rhs);
	};

	struct VectorAttenuation
	{
		float k0;
		float k1;
		float k2;
		float k3;
	};

	struct ParallelSpecularData
	{
		VectorXyzw        direction;
		VectorRgba        diffuseColor;
		VectorRgba        specularColor;
	};

	struct ParallelData
	{
		VectorXyzw        direction;
		VectorRgba        diffuseColor;
	};

	struct PointSpecularData
	{
		VectorXyzw        position;
		VectorRgba        diffuseColor;
		VectorAttenuation attenuation;
		VectorRgba        specularColor;
	};

	struct PointData
	{
		VectorXyzw        position;
		VectorRgba        diffuseColor;
		VectorAttenuation attenuation;
	};

	struct Dot3Data
	{
		VectorXyzw        localCameraPosition;
		VectorXyzw        localDirection;
		VectorRgba        diffuseColor;
		VectorRgba        specularColor;
	};

	struct LightData
	{
		VectorRgba           ambient;
		ParallelSpecularData parallelSpecular[ParallelSpecularCount];
		ParallelData         parallel[ParallelCount];
		PointSpecularData    pointSpecular[PointSpecularCount];
		PointData            point[PointCount];
		Dot3Data             dot3;
	};

	struct HemisphericLightData
	{
		VectorRgba backColor;
		VectorRgba tangentColor;
		VectorRgba tangentMinusBackColor;
		VectorRgba tangentMinusDiffuseColor;
	};

	struct ExtendedLightData
	{
		HemisphericLightData   parallelSpecular[1];
	};

	struct PixelDot3Data
	{
		VectorXyzw localDirection;
		VectorRgba diffuseColor;
		VectorRgba specularColor;
		VectorRgba tangentMinusDiffuseColor;
		VectorRgba tangentMinusBackColor;
	};

private:

	static void applyLights_vertexShader();
	static void applyLights_vertexShader_dot3();

	static void _vsps_setExtendedLightData(
		HemisphericLightData &extendedParallelSpecular, 
		const Light *light, 
		const VectorArgb &diffuseColor, 
		float bloomEnabled
		);

	static bool              ms_dirty;
	static bool              ms_usingVertexShaderProgram;
	static bool              ms_useOnlyfullAmbient;
	static bool              ms_setLightsOncePerFrame;
	static bool              ms_newFrame;
	static bool              ms_obeysLightScale;
	static Transform         ms_objectToWorldTransform;
	static Vector            ms_objectPosition;
	static Vector            ms_cameraPosition;
	static LightList         ms_lightList;
	static VectorRgba        ms_fullAmbient;
	static SelectedLights    ms_currentLights;
	static SelectedLights    ms_lastLights;
	static const Light      *ms_fixedFunctionPipelineLight[FixedFunctionPipelineLightCount];
	static const VectorRgba  ms_noDot3Light[3];
	
	static ID3D11Buffer*	 ms_constantBufferLightData;
	static ID3D11Buffer*	 ms_constantBufferExtendedLightData;
	static ID3D11Buffer*	 ms_constantBufferVertexDot3Register;
	static ID3D11Buffer*	 ms_constantBufferTangentMinusDiffuseColor;
	static ID3D11Buffer*	 ms_constantBufferVertexAlphaFadeBloomUpdate;

	static ID3D11Buffer*	 ms_constantBufferDot3LightDirection;
	static ID3D11Buffer*	 ms_constantBufferPixelAlphaFadeBloomUpdate;
};

// ======================================================================

inline void Direct3d11_LightManager::setFullAmbientOn(bool on)
{
	if (on)
	{
		ms_fullAmbient.r = 1.0f;
		ms_fullAmbient.g = 1.0f;
		ms_fullAmbient.b = 1.0f;
		ms_fullAmbient.a = 0.0f;
	}
	else
	{
		ms_fullAmbient.r = 0.0f;
		ms_fullAmbient.g = 0.0f;
		ms_fullAmbient.b = 0.0f;
		ms_fullAmbient.a = 0.0f;
	}
}

// ======================================================================

#endif
