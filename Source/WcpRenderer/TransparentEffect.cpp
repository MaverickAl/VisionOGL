#include "TransparentEffect.h"
#include "MaterialGL.h"


const MaterialGL*	TransparentEffect::m_activeMaterial = NULL;

TransparentEffect::TransparentEffect()
{
	m_activeMaterial = NULL;
	m_active = false;
}

TransparentEffect::~TransparentEffect()
{

}

bool TransparentEffect::Init(TreFile* treFile)
{
	bool ret = m_shader.LoadFromFile("shaders\\Transparency.fx", treFile);

	m_solidTechnique = m_shader.GetTechiqueByName("UnlitNoTexture");
	m_unlitTechnique = m_shader.GetTechiqueByName("UnlitTexture");
	assert(cgValidateTechnique(m_solidTechnique));
	assert(cgValidateTechnique(m_unlitTechnique));

	m_unlitPass			= m_shader.GetFirstPass(m_unlitTechnique);
	m_solidPass			= m_shader.GetFirstPass(m_solidTechnique);
	
	m_viewProjHndl		= m_shader.GetVariableHandleBySemantic("ViewProjection");

	m_diffuseMapHndl		= m_shader.GetVariableHandleByName("DiffuseMap");
	m_materialMultiplierHndl= m_shader.GetVariableHandleByName("fMaterialBrightness");

	// Fog variables
	m_fogEnabledHndl = m_shader.GetVariableHandleByName("bFogEnabled");
	m_fogDensityHndl = m_shader.GetVariableHandleByName("fFogDensity");
	m_fogColorHndl= m_shader.GetVariableHandleByName("fvFogColor");
	
	const char* compileError = cgGetLastListing(CGShader::s_cgContext);
	if(compileError && compileError[0])
	{
		::MessageBox(0, compileError, "Transparency Effect: Shader compile warnings", 0);
		ret = false;
	}

	return ret;
}


void TransparentEffect::SetViewInfo(Matrix44 &viewProjMat)
{
	cgSetMatrixParameterfr(m_viewProjHndl, viewProjMat.ColumnMajor());
}

void TransparentEffect::SetFogInfo(const FogFX& fog)
{
	if (m_fogEnabledHndl)
	{
		cgSetParameter1i(m_fogEnabledHndl, fog.active);
	}
	if (m_fogDensityHndl)
	{
		cgSetParameter1f(m_fogDensityHndl, fog.density);
	}
	if (m_fogColorHndl)
	{
		cgSetParameter3f(m_fogColorHndl, fog.red, fog.green, fog.blue);
	}
}


void TransparentEffect::Activate(const MaterialGL* mat)
{
	if(mat!=m_activeMaterial || !m_active)
	{
		cgSetParameter1f(m_materialMultiplierHndl, mat->m_materialMultiplier);
		if(mat->m_pTexture)
		{
			m_activePass = m_unlitPass;
			cgGLSetTextureParameter(m_diffuseMapHndl, mat->m_pTexture->GetData());
		}
		else
		{
			m_activePass = m_solidPass;
		}

		m_shader.BeginPass(m_activePass);
		m_active = true;
	}
	m_activeMaterial = mat;

}