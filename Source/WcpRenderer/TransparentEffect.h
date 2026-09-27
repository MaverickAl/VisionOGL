#pragma once
#include "WcpOglUtility.h"
#include "Matrix44.h"
#include "Vector3.h"
#include "Texture.h"
#include "CGShader.h"

class MaterialGL;
class TreFile;

class TransparentEffect
{
public:
	TransparentEffect();
	~TransparentEffect();

	bool Init(TreFile* treFile);
	void Activate(const MaterialGL* pMat);
	void Deactivate();
	void SetViewInfo(Matrix44 &viewProjMata);
	void SetFogInfo(const FogFX& fog);
	static void ResetActiveMat() { m_activeMaterial = NULL; }

private:
	CGShader					m_shader;

	CGtechnique					m_unlitTechnique;
	CGtechnique					m_solidTechnique;
	CGpass						m_unlitPass;
	CGpass						m_solidPass;

	CGparameter					m_diffuseMapHndl;
	CGparameter					m_viewProjHndl;

	CGparameter					m_materialMultiplierHndl;
	 
	CGparameter					m_fogEnabledHndl;
	CGparameter					m_fogDensityHndl;
	CGparameter					m_fogColorHndl;

	CGpass						m_useThisPass;
	CGpass						m_activePass;

	bool						m_active;

	static const MaterialGL*	m_activeMaterial;
};
	

inline void TransparentEffect::Deactivate()
{
	m_shader.EndPass(m_activePass);
	m_active = false;
}
