#include "WcpOglUtility.h"
#include "Matrix44.h"
#include "Vector3.h"
#include "Texture.h"
#include "LightGL.h"
#include "CGShader.h"

class MaterialGL;
class TreFile;

class SpecularEffect
{
public:
	SpecularEffect();
	~SpecularEffect();
	
	bool Init(TreFile* treFile);
	void Activate();
	void Deactivate();
	void SetViewInfo(Matrix44 &worldMat, Matrix44 &viewMat, Matrix44 &worldViewProjMat, Vector3 &eyePos, Vector3& eyeDir);
	void SetFogInfo(const FogFX& fog);

	void SetGlobalLightInfo(const Colour &ambient, const int noDirLights, const LightGL** dirLights);
	void SetLocalLightInfo(const int noPosLights, const LightGL** posLights, Vector3 vPosShift);
	inline void SetUseLighting(bool useLighting)
	{
		if (useLighting != m_lit)
		{
			m_statesChanged = true;
		}
		m_lit = useLighting;
	}
	void SetMaterialInfo(const MaterialGL* mat);
	static void ResetActiveMat() { m_activeMaterial = NULL; }
	bool GetUseIrridesence() const { return m_useIrridesence; }

private:
	void SetMaterialInfoInt(const MaterialGL* mat);
	CGpass SelectActivePass();

	bool						m_isActive;
	bool						m_statesChanged;
	bool						m_lit;
	bool						m_solid;
	bool						m_useIrridesence;
	bool						m_useSpec;
	bool						m_useAutoSpec;
	int							m_noDirLights;

	// Parameter handles
	CGShader					m_shader;
	CGparameter					m_worldHndl;
	CGparameter					m_viewHndl;
	CGparameter					m_worldViewProjHndl;
	CGparameter					m_eyePosHndl;
	CGparameter					m_eyeDirHndl;
	CGparameter					m_diffuseMapHndl;
	CGparameter					m_specularMapHndl;
	CGparameter					m_emissiveMapHndl;
	CGparameter					m_normalMapHandle;
	CGparameter					m_noiseMapHandle;
	CGparameter					m_iridesenceMapHandle;
	CGparameter					m_iridesenceMaskHandle;
	CGparameter					m_useSpecMapHndl;
	CGparameter					m_useEmissiveMapHndl;
	CGparameter					m_useNormalMapHndl;
	CGparameter					m_useIridesenceMapHndl;
	CGparameter					m_autoSpecMapHndl;
	CGparameter					m_ambientHndl;
	CGparameter					m_brightnessHndl;
	CGparameter					m_fogEnabledHndl;
	CGparameter					m_fogDensityHndl;
	CGparameter					m_fogColorHndl;

	CGparameter					m_materialMultiplierHndl;

	// Techniques
	CGtechnique					m_ambientTechnique;
	CGpass						m_ambientPass;
	CGpass						m_useThisPass;
	CGpass						m_activePass;

	// ARB Techniques
	CGtechnique					m_specularDiffuse1DirTechnique;
	CGtechnique					m_autoSpec1DirTechnique;
	CGtechnique					m_specMap1DirTechnique;
	CGtechnique					m_specularDiffuse2DirTechnique;
	CGtechnique					m_autoSpec2DirTechnique;
	CGtechnique					m_specMap2DirTechnique;
	CGtechnique					m_specularDiffuse3DirTechnique;
	CGtechnique					m_autoSpec3DirTechnique;
	CGtechnique					m_specMap3DirTechnique;
	CGtechnique					m_unlitTechnique;
	CGpass						m_specularDiffuse1DirPass;
	CGpass						m_autoSpec1DirPass;
	CGpass						m_specMap1DirPass;
	CGpass						m_specularDiffuse2DirPass;
	CGpass						m_autoSpec2DirPass;
	CGpass						m_specMap2DirPass;
	CGpass						m_specularDiffuse3DirPass;
	CGpass						m_autoSpec3DirPass;
	CGpass						m_specMap3DirPass;
	CGpass						m_unlitPass;

	// Solid (was not used in standoff)
	CGtechnique					m_solidUnlitTechnique;
	CGpass						m_solidUnlitPass;

	CGparameter					m_lightColourHndls[3];
	CGparameter					m_lightDirsHndls[3];

	CGparameter					m_lightPosHndls[4];
	CGparameter					m_lightAttenHndls[4];
	CGparameter					m_posLightColoursHndls[4];

	CGparameter					m_texturedHndl;
	CGparameter					m_solidColourHndl;

	static const MaterialGL*	m_activeMaterial;
};

inline void SpecularEffect::Activate()
{
	if(!m_isActive || m_statesChanged==true)
	{
		m_activePass = SelectActivePass();
		m_shader.BeginPass(m_activePass);
	}

	m_isActive = true;
	m_statesChanged = false;
}

inline void SpecularEffect::Deactivate()
{
	if(m_isActive)
	{	
		m_shader.EndPass(m_activePass);
	}

	m_isActive = false;
}

inline void SpecularEffect::SetMaterialInfo(const MaterialGL* mat)
{
	if(m_activeMaterial==mat)
		return;

	m_activeMaterial = mat;
	SetMaterialInfoInt(mat);
}