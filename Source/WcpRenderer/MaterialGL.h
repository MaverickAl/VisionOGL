#pragma once
#include "WcpOglUtility.h"
#include "Colour.h"
#include "Texture.h"
#include "RenderingConstants.h"
#include "Materials.h"
#include "TransparentEffect.h"
#include <string>
#include <map>
using namespace std;

class GLSLProgram;
class SpecularEffect;
class TreFile;
class MaterialGL;

typedef void (*shaderStateFunc)(const MaterialGL*, bool useAlpha, float alphaOverride, bool blendTransparency, float brightness);  

void SetShaderSolidStates(const MaterialGL* mat, bool useAlpha, float alphaOverride, bool blendTransparency, float brightness);
void SetShaderSolidAlphaStates(const MaterialGL* mat, bool useAlpha, float alphaOverride, bool blendTransparency, float brightness);
void SetShaderTransparentStates(const MaterialGL* mat, bool useAlpha, float alphaOverride, bool blendTransparency, float brightness);
void SetShaderTranslucentStates(const MaterialGL* mat, bool useAlpha, float alphaOverride, bool blendTransparency, float brightness);
void SetShaderStandardStates(const MaterialGL* mat, bool useAlpha, float alphaOverride, bool blendTransparency, float brightness);

class MaterialGL
{
	friend istream& operator >>(istream &is,MaterialGL &m);
	friend SpecularEffect;
	friend TransparentEffect;
	friend void SetShaderSolidStates(const MaterialGL*, bool, float, bool, float);
	friend void SetShaderSolidAlphaStates(const MaterialGL*, bool, float, bool, float);
	friend void SetShaderTransparentStates(const MaterialGL*, bool, float, bool, float);
	friend void SetShaderTranslucentStates(const MaterialGL*, bool, float, bool, float);
	friend void SetShaderStandardStates(const MaterialGL*, bool, float, bool, float);
public:
	MaterialGL(const TextureMaterial* textureMat, TreFile* treFile, bool useShaders, bool bHDR);
	virtual ~MaterialGL(void);
	// Only used by emissive objects
	void ClampTexture() { if(m_pTexture) { m_pTexture->SetClamp(); } }

	bool IsNebula() { return m_isNebula; }
	bool HasOverride() const { return !m_bOwnsTexture; }

	bool SolidWithAlpha() { return m_solidWithAlpha; }
	
	void EnableMaterial(bool useAlpha = false, float alphaOverride = 0.0f, bool blendTransparency = false, float brightness = 1.0f) const;
	void EnableShaderMaterial(bool useAlpha = false, float alphaOverride = 0.0f, bool blendTransparency = false, float brightness = 1.0f) const;
	static void ResetActiveMat() { m_pActiveMat = NULL; m_prevAlphaOverride = false; }

	Texture* GetTexture() { return m_pTexture; }

	bool Translucency() const
	{
		return (m_transluscent || m_solidWithAlpha);
	}

	bool HasNormalMap() const
	{
		return m_pNormalMap != nullptr;
	}

	const Colour& GetColor() const { return m_solidColor; }

private:
	void EnableMaterialInt(bool useAlpha, float alphaOverride, bool blendTransparency, float brightness) const;

	shaderStateFunc				m_shaderStateFunc;

	Colour						m_diffuse;
	//Colour						m_ambient;
	Colour						m_solidColor;
	float						m_alpha;
	float						m_brightness;
	float						m_materialMultiplier;
	bool						m_autoSpecMap;

	// TODO: Enumerate + switch
	bool						m_transluscent;		// Texture + alpha or alpha map with 8 bit range of values
	bool						m_transparent;		// Texture + alpha or alpha map representing on/off
	bool						m_solid;			// Colour, No texture
	bool						m_solidWithAlpha;	// Colour + Alpha map
	bool						m_additiveBlend;	// Standoff hack for additive transparency
	bool						m_isNebula;
	bool						m_useShaders;
	bool						m_bOwnsTexture = false;
	
	// A map of the textures
	Texture*					m_pTexture = nullptr;
	Texture*					m_pSpecularMap = nullptr;
	Texture*					m_pEmissiveMap = nullptr;
	Texture*					m_pNormalMap = nullptr;
	Texture*					m_pIriLookupMap = nullptr;
	Texture*					m_pIriMaskMap = nullptr;
	Texture*					m_pIriNoiseMap = nullptr;
	const TextureMaterial*		m_pTextureMat = nullptr;
	
	static const MaterialGL*	m_pActiveMat;
	static bool					m_prevAlphaOverride;
	static const Colour			m_white;
};


inline void MaterialGL::EnableMaterial(bool useAlpha, float alphaOverride, bool blendTransparency, float brightness) const
{
	if(m_pActiveMat==this && !useAlpha && !m_prevAlphaOverride)
		return;

	EnableMaterialInt(useAlpha, alphaOverride, blendTransparency, brightness);

	m_pActiveMat = this;
	m_prevAlphaOverride = alphaOverride;
}


inline void MaterialGL::EnableShaderMaterial(bool useAlpha, float alphaOverride, bool blendTransparency, float brightness) const
{
	if(m_pActiveMat==this && !useAlpha && !m_prevAlphaOverride)
		return;

	if(m_isNebula)
		int i=1;

	m_shaderStateFunc(this, useAlpha, alphaOverride, blendTransparency, brightness);
	//EnableMaterialInt(useAlpha, alphaOverride, blendTransparency);

	m_pActiveMat = this;
	m_prevAlphaOverride = alphaOverride;
}