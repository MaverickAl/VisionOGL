#include "SpecularEffect.h"
#include "MaterialGL.h"

const MaterialGL*	SpecularEffect::m_activeMaterial = NULL;

SpecularEffect::SpecularEffect()
{
	m_activeMaterial = NULL;
	m_isActive = false;
	m_statesChanged = true;
	m_lit = false;
	m_solid = false;
	m_noDirLights = 0;
	m_ambientTechnique = NULL;
	m_useSpec = false;
	m_useAutoSpec = false;
	m_useIrridesence = false;
	m_activePass = nullptr;

	m_noDirLights = 0;
}

SpecularEffect::~SpecularEffect()
{

}


bool SpecularEffect::Init(TreFile* treFile)
{
	bool ret = false;

	ret = m_shader.LoadFromFile("shaders\\specularARB.fx", treFile);

	m_specularDiffuse1DirTechnique = m_shader.GetTechiqueByName("Diffuse1Dir");
	m_autoSpec1DirTechnique = m_shader.GetTechiqueByName("AutoSpec1Dir");
	m_specMap1DirTechnique = m_shader.GetTechiqueByName("SpecMap1Dir");
	m_specularDiffuse2DirTechnique = m_shader.GetTechiqueByName("Diffuse2Dir");
	m_autoSpec2DirTechnique = m_shader.GetTechiqueByName("AutoSpec2Dir");
	m_specMap2DirTechnique = m_shader.GetTechiqueByName("SpecMap2Dir");
	m_specularDiffuse3DirTechnique = m_shader.GetTechiqueByName("Diffuse3Dir");
	m_autoSpec3DirTechnique = m_shader.GetTechiqueByName("AutoSpec3Dir");
	m_specMap3DirTechnique = m_shader.GetTechiqueByName("SpecMap3Dir");
	m_unlitTechnique = m_shader.GetTechiqueByName("UnlitTexture");
	m_solidUnlitTechnique = m_shader.GetTechiqueByName("UnlitNoTexture");
	assert(cgValidateTechnique(m_specularDiffuse1DirTechnique));
	assert(cgValidateTechnique(m_autoSpec1DirTechnique));
	assert(cgValidateTechnique(m_specMap1DirTechnique));
	assert(cgValidateTechnique(m_specularDiffuse2DirTechnique));
	assert(cgValidateTechnique(m_autoSpec2DirTechnique));
	assert(cgValidateTechnique(m_specMap2DirTechnique));
	m_specularDiffuse1DirPass	= m_shader.GetFirstPass(m_specularDiffuse1DirTechnique);
	m_autoSpec1DirPass			= m_shader.GetFirstPass(m_autoSpec1DirTechnique);
	m_specMap1DirPass			= m_shader.GetFirstPass(m_specMap1DirTechnique);
	m_specularDiffuse2DirPass	= m_shader.GetFirstPass(m_specularDiffuse2DirTechnique);
	m_autoSpec2DirPass			= m_shader.GetFirstPass(m_autoSpec2DirTechnique);
	m_specMap2DirPass			= m_shader.GetFirstPass(m_specMap2DirTechnique);
	m_specularDiffuse3DirPass	= m_shader.GetFirstPass(m_specularDiffuse3DirTechnique);
	m_autoSpec3DirPass			= m_shader.GetFirstPass(m_autoSpec3DirTechnique);
	m_specMap3DirPass			= m_shader.GetFirstPass(m_specMap3DirTechnique);
	m_unlitPass					= m_shader.GetFirstPass(m_unlitTechnique);
	m_solidUnlitPass			= m_shader.GetFirstPass(m_solidUnlitTechnique);

	m_lightColourHndls[0]		= m_shader.GetVariableHandleByName("fvLightColour0");
	m_lightColourHndls[1]		= m_shader.GetVariableHandleByName("fvLightColour1");
	m_lightColourHndls[2]		= m_shader.GetVariableHandleByName("fvLightColour2");

	m_lightDirsHndls[0]			= m_shader.GetVariableHandleByName("fvLightDirection0");
	m_lightDirsHndls[1]			= m_shader.GetVariableHandleByName("fvLightDirection1");
	m_lightDirsHndls[2]			= m_shader.GetVariableHandleByName("fvLightDirection2");

	m_lightPosHndls[0]			= m_shader.GetVariableHandleByName("fvLightPosition0");
	m_lightPosHndls[1]			= m_shader.GetVariableHandleByName("fvLightPosition1");
	m_lightPosHndls[2]			= m_shader.GetVariableHandleByName("fvLightPosition2");
	m_lightPosHndls[3]			= m_shader.GetVariableHandleByName("fvLightPosition3");

	m_lightAttenHndls[0]		= m_shader.GetVariableHandleByName("fvLightAttenuation0");
	m_lightAttenHndls[1]		= m_shader.GetVariableHandleByName("fvLightAttenuation1");
	m_lightAttenHndls[2]		= m_shader.GetVariableHandleByName("fvLightAttenuation2");
	m_lightAttenHndls[3]		= m_shader.GetVariableHandleByName("fvLightAttenuation3");

	m_posLightColoursHndls[0]	= m_shader.GetVariableHandleByName("fvPosLightColour0");
	m_posLightColoursHndls[1]	= m_shader.GetVariableHandleByName("fvPosLightColour1");
	m_posLightColoursHndls[2]	= m_shader.GetVariableHandleByName("fvPosLightColour2");
	m_posLightColoursHndls[3]	= m_shader.GetVariableHandleByName("fvPosLightColour3");


	m_ambientTechnique = m_shader.GetTechiqueByName("Specular0Dir");
	m_ambientPass		= m_shader.GetFirstPass(m_ambientTechnique);
	assert(cgValidateTechnique(m_ambientTechnique));

	// Vertex shader
	m_eyePosHndl			= m_shader.GetVariableHandleBySemantic("EyePosition");
	m_eyeDirHndl			= m_shader.GetVariableHandleBySemantic("EyeDirection");
	m_worldHndl				= m_shader.GetVariableHandleBySemantic("World");
	m_viewHndl				= m_shader.GetVariableHandleBySemantic("View");
	m_worldViewProjHndl		= m_shader.GetVariableHandleBySemantic("WorldViewProjection");


	// Pixel shader
	// Directional light info 
	m_ambientHndl		= m_shader.GetVariableHandleByName("fvAmbient");
	
	// Material info
	m_diffuseMapHndl		= m_shader.GetVariableHandleByName("DiffuseMap");
	m_specularMapHndl		= m_shader.GetVariableHandleByName("SpecularMap");
	m_emissiveMapHndl		= m_shader.GetVariableHandleByName("EmissiveMap");
	m_normalMapHandle		= m_shader.GetVariableHandleByName("NormalMap");
	m_noiseMapHandle		= m_shader.GetVariableHandleByName("NoiseMap");
	m_iridesenceMapHandle	= m_shader.GetVariableHandleByName("IridesenceMap");
	m_iridesenceMaskHandle = m_shader.GetVariableHandleByName("IridesenceMask");
	m_useIridesenceMapHndl = m_shader.GetVariableHandleByName("bIridescent");
	m_useSpecMapHndl		= m_shader.GetVariableHandleByName("bHasSpecMap");
	m_useEmissiveMapHndl	= m_shader.GetVariableHandleByName("bHasEmissiveMap");
	m_useNormalMapHndl		= m_shader.GetVariableHandleByName("bHasNormalMap");
	m_autoSpecMapHndl		= m_shader.GetVariableHandleByName("bAutoSpecMap");
	m_brightnessHndl		= m_shader.GetVariableHandleByName("fGameBrightness");
	m_materialMultiplierHndl= m_shader.GetVariableHandleByName("fMaterialBrightness");
	m_texturedHndl			= m_shader.GetVariableHandleByName("bTextured");
	m_solidColourHndl		= m_shader.GetVariableHandleByName("fvSolidColour");

	// Fog variables
	m_fogEnabledHndl			= m_shader.GetVariableHandleByName("bFogEnabled");
	m_fogDensityHndl			= m_shader.GetVariableHandleByName("fFogDensity");
	m_fogColorHndl				= m_shader.GetVariableHandleByName("fvFogColor");

	const char* compileError = cgGetLastListing(CGShader::s_cgContext);
	if(compileError && compileError[0])
	{
		::MessageBox(0, compileError, "Specular Effect: Shader compile warnings", 0);
	}

	m_useThisPass = m_ambientPass;

	return ret;
}


void SpecularEffect::SetViewInfo(Matrix44 &worldMat, Matrix44 &viewMat, Matrix44 &worldViewProjMat, Vector3 &eyePos, Vector3& eyeDir)
{
	cgSetMatrixParameterfr(m_worldViewProjHndl, worldViewProjMat.ColumnMajor());
	cgSetMatrixParameterfr(m_viewHndl, viewMat.ColumnMajor());
	cgSetMatrixParameterfr(m_worldHndl, worldMat.ColumnMajor());
	cgSetParameter3fv(m_eyePosHndl, eyePos.xyz());
	if(m_eyeDirHndl)
	{
		cgSetParameter3fv(m_eyeDirHndl, eyeDir.xyz());
	}
	m_statesChanged = true;
}


CGpass SpecularEffect::SelectActivePass()
{
	if(!m_lit)
	{
		if(m_solid)
		{
			return m_solidUnlitPass;
		}
		else
		{
			return m_unlitPass;
		}
	}
	else if(m_useSpec && m_useAutoSpec)
	{
		switch(m_noDirLights)
		{
		case 0: return m_ambientPass;
		case 1: return m_autoSpec1DirPass;
		case 2: return m_autoSpec2DirPass;
		case 3:
		default:
			return m_autoSpec3DirPass;
		}
	}
	else if(m_useSpec)
	{
		switch(m_noDirLights)
		{
		case 0: return m_ambientPass;
		case 1: return m_specMap1DirPass;
		case 2: return m_specMap2DirPass;
		case 3:
		default:
			return m_specMap3DirPass;
		}
	}
	else
	{
		switch(m_noDirLights)
		{
		case 0: return m_ambientPass;
		case 1: return m_specularDiffuse1DirPass;
		case 2: return m_specularDiffuse2DirPass;
		case 3:
		default:
			return m_specularDiffuse3DirPass;
		}
	}


	return NULL;
}

void SpecularEffect::SetGlobalLightInfo(const Colour &ambient, const int noDirLights, const LightGL** dirLights)
{
	cgSetParameter3fv(m_ambientHndl, ambient.rgba());

	if(m_noDirLights!=noDirLights)	// Value gets set to zero per frame, so we can safely make this optimisation
	{
		float position[3];
		float colour[4];
		for(int i=0; i<noDirLights && i<3; i++)
		{
			const LightGL* light = dirLights[i];
			memcpy(position, light->m_position.xyzw(), 3*sizeof(float));
			memcpy(colour, light->m_diffuse.rgba(), 3*sizeof(float));
			colour[3] = max(max(light->m_diffuse.r(), light->m_diffuse.g()), light->m_diffuse.b());
			
			cgSetParameter3fv(m_lightDirsHndls[i], position);
			cgSetParameter4fv(m_lightColourHndls[i], colour);
		}

		m_noDirLights = noDirLights;
	}
	m_statesChanged = true;
}

void SpecularEffect::SetLocalLightInfo(const int noPosLights, const LightGL** posLights, Vector3 vPosShift)
{
	static int activePosLights = -1;

	float position[3];
	float colour[3];
	float attenuation[4];

	for(int i=0; i<noPosLights && i<4; i++)
	{
		const LightGL* light = posLights[i];
		memcpy(position, light->m_position.xyzw(), 3*sizeof(float));
		memcpy(colour, light->m_diffuse.rgba(), 3*sizeof(float));

		attenuation[0] = light->m_constantAttenuation;
		attenuation[1] = light->m_linearAttenuation;
		attenuation[2] = light->m_quadraticAttenuation;
		attenuation[3] = light->m_cutoffDist;	

		position[0] += vPosShift.x();
		position[1] += vPosShift.y();
		position[2] += vPosShift.z();

		cgSetParameter3fv(m_lightPosHndls[i], position);
		cgSetParameter3fv(m_posLightColoursHndls[i], colour);
		cgSetParameter4fv(m_lightAttenHndls[i], attenuation);
	}

	if( !((activePosLights==0) && (noPosLights==0)) )
	{
		for(int i=noPosLights; i<4; i++)
		{
			memset(position, 0, 3*sizeof(float));
			memset(colour, 0, 3*sizeof(float));

			attenuation[0] = 100.0f;
			attenuation[1] = 100.0f;
			attenuation[2] = 100.0f;
			attenuation[3] = 0.0f;	

			cgSetParameter3fv(m_lightPosHndls[i], position);
			cgSetParameter3fv(m_posLightColoursHndls[i], colour);
			cgSetParameter4fv(m_lightAttenHndls[i], attenuation);
		}
	}

	// If we are using 0 lights and 0 are active then we don't need to set this poor parameter
	// all other cases the light info is almost certainly dirty
	if( !((activePosLights==0) && (noPosLights==0)) )
	{
		m_statesChanged = true;
	}

	activePosLights = noPosLights;
}


void SpecularEffect::SetFogInfo(const FogFX& fog)
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

void SpecularEffect::SetMaterialInfoInt(const MaterialGL* mat)
{
	m_solid = (mat->m_solid || mat->m_solidWithAlpha);
	cgSetParameter1f(m_materialMultiplierHndl, mat->m_materialMultiplier);

	if(mat->m_pTexture)
	{
		cgGLSetTextureParameter(m_diffuseMapHndl, mat->m_pTexture->GetData());
		cgSetParameter1i(m_texturedHndl, true);
	}
	else
	{
		cgSetParameter1i(m_texturedHndl, false);
		cgSetParameter4fv(m_solidColourHndl, mat->m_solidColor.rgba());;
	}
	if (mat->m_pEmissiveMap)
	{
		cgSetParameter1i(m_useEmissiveMapHndl, true);
		cgGLSetTextureParameter(m_emissiveMapHndl, mat->m_pEmissiveMap->GetData());
	}
	else
	{
		cgSetParameter1i(m_useEmissiveMapHndl, false);
	}
	if (mat->m_pIriLookupMap)
	{
		cgGLSetTextureParameter(m_iridesenceMapHandle, mat->m_pIriLookupMap->GetData());
		cgGLSetTextureParameter(m_iridesenceMaskHandle, mat->m_pIriMaskMap ->GetData());
		cgGLSetTextureParameter(m_noiseMapHandle, mat->m_pIriNoiseMap->GetData());
		cgSetParameter1i(m_useIridesenceMapHndl, true);
		m_useIrridesence = true;
	}
	else if(m_useIridesenceMapHndl)
	{
		cgSetParameter1i(m_useIridesenceMapHndl, false);
		m_useIrridesence = false;
	}
	if (mat->m_pNormalMap)
	{
		cgSetParameter1i(m_useNormalMapHndl, true);
		cgGLSetTextureParameter(m_normalMapHandle, mat->m_pNormalMap->GetData());
	}
	else if(m_useNormalMapHndl)
	{
		cgSetParameter1i(m_useNormalMapHndl, false);
	}
	if(mat->m_pSpecularMap)
	{
		cgSetParameter1i(m_useSpecMapHndl, true);
		cgSetParameter1f(m_brightnessHndl, mat->m_brightness);
		cgSetParameter1i(m_autoSpecMapHndl, false);
		cgGLSetTextureParameter(m_specularMapHndl, mat->m_pSpecularMap->GetData());
		m_useSpec = true;
		m_useAutoSpec = false;
	}
	else if(mat->m_autoSpecMap)
	{
		cgSetParameter1i(m_useSpecMapHndl, true);
		cgSetParameter1i(m_autoSpecMapHndl, true);
		m_useSpec = true;
		m_useAutoSpec = true;
		cgSetParameter1f(m_brightnessHndl, mat->m_brightness);
	}
	else
	{
		cgSetParameter1i(m_useSpecMapHndl, false);
		m_useSpec = false;
	}

	m_statesChanged = true;
}