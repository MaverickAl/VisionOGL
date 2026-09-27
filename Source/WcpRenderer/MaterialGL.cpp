/****************************************************************************
//	Filename: MaterialGL.cpp
//	Description: MaterialGL defintion. Does not use standard OpenGL MaterialGL
//  colours as, except for emmissive, these are assumed to be determined by
//	the textures
*****************************************************************************/
#include ".\MaterialGL.h"
#include "Materials.h"
#include "WcpOglFile.h"
#include "TreFile.h"
#include "TextureMgr.h"

const MaterialGL*	MaterialGL::m_pActiveMat = NULL;
const Colour		MaterialGL::m_white(1.0f, 1.0f, 1.0f, 1.0f);
bool				MaterialGL::m_prevAlphaOverride = false;

extern bool g_bForceAutoSpec;


// 0x2D group
const char kRawColourFlag	= (1 << 5);
const char kBloomFlag		= (1 << 6);
const char kAdditiveFlag	= (1 << 7);

// 0x5F or 0x61 group
const char kSpecularFlag	= (1 << 0);
const char kTexOverrideFlag = (1 << 1);
const char kNormalFlag		= (1 << 2);
const char kEmissiveFlag	= (1 << 3);

MaterialGL::MaterialGL(const TextureMaterial* textureMat, TreFile* treFile, bool useShaders, bool useBloom):
m_pTexture(NULL),
m_solid(false),
m_solidWithAlpha(false),
m_transparent(false),
m_transluscent(false),
m_additiveBlend(false),
m_pSpecularMap(NULL),
m_pEmissiveMap(NULL),
m_pNormalMap(NULL),
m_pIriLookupMap(NULL),
m_pIriMaskMap(NULL),
m_pIriNoiseMap(NULL),
m_useShaders(useShaders),
m_shaderStateFunc(NULL),
m_brightness(1.0f),
m_materialMultiplier(1.0f),
m_autoSpecMap(false)
{
	m_pTextureMat = textureMat;
	m_isNebula = false;

	float r = (float)textureMat->red/255.f;
	float g = (float)textureMat->green/255.f;
	float b = (float)textureMat->blue/255.f;
	float a = (float)textureMat->alpha/255.f;
	m_solidColor.Assign(r, g, b, a);
	
	m_alpha = (float)(255-textureMat->alpha)/255.f;

	if( !textureMat->rgbMap && !textureMat->alphaMap )
	{
		// No texture, just coloured
		m_pTexture = NULL;
		m_solid = true;
		m_shaderStateFunc = &SetShaderSolidStates;
		return;
	}

	m_solidWithAlpha	= (textureMat->rgbMap==NULL);
	m_transparent		= textureMat->transparency;
	m_transluscent		= (textureMat->alphaMap!=NULL || textureMat->alpha);
	m_isNebula =		(textureMat->alpha > 0);
	m_autoSpecMap		= g_bForceAutoSpec;

	// Standoff overrides
	const uint8* rawData = textureMat->palette ? textureMat->palette->rawData : NULL;
	if( rawData && rawData[0]==0x2D && useBloom)
	{
		if( rawData[1]&kAdditiveFlag )
		{
			assertIf(m_transluscent || m_transparent)
			{
				m_additiveBlend = true;
			}
		}
		if( rawData[1]&kBloomFlag )
		{
			// Range 0-25.5f multiplier
			m_materialMultiplier = ((float)rawData[2])/10.0f;
		}
	}
	// No trefile implies no shaders
	bool bTextureOverriden = false;
	if( rawData && ( (rawData[0]==0x5F) || (rawData[0]==0x61)) )
	{
		// We know the first item is 5F so we can find out game brightness
		m_brightness = ((float)textureMat->palette->data[0])/(float)rawData[0];
		// * should be a standoff material* 12bits for the id
		uint flags = (rawData[1] >> 4) & 0x0F;
		if((flags & kSpecularFlag) && m_useShaders)
		{
			uint textureNo = ((rawData[1]<<8)&0x00000F00) + rawData[2];
			if(textureNo == 0)
			{
				m_autoSpecMap = true;
			}
			else
			{
				char specName[MAX_PATH];
				sprintf_s(specName, MAX_PATH, "specmaps\\%.8d.tga\0", textureNo);
				m_pSpecularMap = TextureMgr::Inst()->GetTexture(treFile, specName);
			}
		}
		if ((flags & kEmissiveFlag) && m_useShaders)
		{
			uint textureNo = ((rawData[1] << 8) & 0x00000F00) + rawData[2];
			char emissiveName[MAX_PATH];
			sprintf_s(emissiveName, MAX_PATH, "emissivemaps\\%.8d.tga\0", textureNo);
			m_pEmissiveMap = TextureMgr::Inst()->GetTexture(treFile, emissiveName);
		}
		if ((flags & kNormalFlag) && m_useShaders)
		{
			uint textureNo = ((rawData[1] << 8) & 0x00000F00) + rawData[2];
			char normalName[MAX_PATH];
			sprintf_s(normalName, MAX_PATH, "normalmaps\\%.8d.tga\0", textureNo);
			m_pNormalMap = TextureMgr::Inst()->GetTexture(treFile, normalName);
		}
		if( flags & kTexOverrideFlag )
		{
			uint textureNo = ((rawData[1]<<8)&0x00000F00) + rawData[2];
			char overrideName[MAX_PATH];
			sprintf_s(overrideName, MAX_PATH, "overridemaps\\%.8d.tga\0", textureNo);
			m_pTexture = TextureMgr::Inst()->GetTexture(treFile, overrideName);
			if(m_pTexture)
			{
				if(m_pTexture->HasAlpha())
					m_transluscent = true;
				bTextureOverriden = true;
			}
		}
	}
	if (rawData && rawData[0] == 0x61)
	{
		// Iridesence shader
		uint textureNo = ((rawData[1] << 8) & 0x00000F00) + rawData[2];
		char overrideName[MAX_PATH];

		sprintf_s(overrideName, MAX_PATH, "lookupmaps\\%.8d.tga\0", textureNo);
		m_pIriLookupMap = TextureMgr::Inst()->GetTexture(treFile, overrideName);
		if (!m_pIriLookupMap)
		{
			m_pIriLookupMap = TextureMgr::Inst()->GetTexture(treFile, "shaders\\Lookup.tga");
		}
		sprintf_s(overrideName, MAX_PATH, "noisemaps\\%.8d.tga\0", textureNo);
		m_pIriNoiseMap = TextureMgr::Inst()->GetTexture(treFile, overrideName);
		if (!m_pIriNoiseMap)
		{
			m_pIriNoiseMap = TextureMgr::Inst()->GetTexture(treFile, "shaders\\Noise.tga");
		}

		sprintf_s(overrideName, MAX_PATH, "iridescencemaps\\%.8d.tga\0", textureNo);
		m_pIriMaskMap = TextureMgr::Inst()->GetTexture(treFile, overrideName);
		if (!m_pIriMaskMap)
		{
			m_pIriMaskMap = TextureMgr::Inst()->GetTexture(treFile, "shaders\\White.tga");
		}
	}
	if(!bTextureOverriden)
	{
		m_pTexture	= new Texture(textureMat, m_isNebula);
		m_bOwnsTexture = true;
	}

	m_diffuse.Assign(r, g, b, a);
	//m_ambient.Assign(r, g, b, a);

	if(m_solid)
	{
		m_shaderStateFunc = &SetShaderSolidStates;
	}
	else if(m_solidWithAlpha)
	{
		m_shaderStateFunc = &SetShaderSolidAlphaStates;
	}
	else if(m_transparent)
	{
		m_shaderStateFunc = &SetShaderTransparentStates;
	}
	else if(m_transluscent)
	{
		m_shaderStateFunc = &SetShaderTranslucentStates;
	}
	else
	{
		m_shaderStateFunc = &SetShaderStandardStates;
	}
}

MaterialGL::~MaterialGL(void)
{
	if (m_bOwnsTexture && m_pTexture)
	{
		wcpogl::SafeDelete(&m_pTexture);
	}
}


void MaterialGL::EnableMaterialInt(bool useAlpha, float alphaOverride, bool blendTransparency, float brightness) const
{
	Colour tmpCol = m_solidColor * brightness;
	tmpCol.a() = useAlpha ? alphaOverride : m_alpha;

	if(m_isNebula)
	{
		int i=1;
	}

	// TODO: Re-enable
	if(m_solid)
	{
		glBindTexture(GL_TEXTURE_2D, 0);
		glDisable(GL_TEXTURE_2D);
		glDisable(GL_ALPHA_TEST);
		
		if(useAlpha)
		{
			glBlendFunc(GL_ONE_MINUS_SRC_ALPHA, GL_SRC_ALPHA);
			glEnable(GL_BLEND);
		}
		else
		{
			glDisable(GL_BLEND);
		}
		glColor4fv(tmpCol.rgba());
		glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, tmpCol.rgba());
	}
	else if(m_solidWithAlpha)
	{
		//glDisable(GL_TEXTURE_2D);
		glDisable(GL_ALPHA_TEST);
		glEnable(GL_BLEND);
		//tmpCol.a()= - tmpCol.a();
		glColor4fv(tmpCol.rgba());
		//glColor3f(1.0f, 1.0f, 1.0f);
		glBlendFunc(GL_ONE_MINUS_SRC_ALPHA, GL_SRC_ALPHA);
		glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, m_solidColor.rgba());
		m_pTexture->Bind(0);
	}
	else if(m_transparent && blendTransparency)
	{
		glEnable(GL_TEXTURE_2D);
		glEnable(GL_BLEND);
		glDisable(GL_ALPHA_TEST);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, m_white.rgba());
		glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, m_white.rgba());
		glColor4f(1.0f, 1.0f, 1.0f, m_alpha);
		m_pTexture->Bind(0);
	}
	else if(m_transluscent)
	{
		glEnable(GL_TEXTURE_2D);
		glEnable(GL_BLEND);
		glDisable(GL_ALPHA_TEST);
		if(m_additiveBlend)
		{
			glBlendFunc(GL_SRC_ALPHA, GL_ONE);
		}
		else
		{
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		}

		glColor4f(brightness, brightness, brightness, (useAlpha ? alphaOverride : m_alpha) );

		m_pTexture->Bind(0);
	}
	else if(m_transparent)
	{
		glEnable(GL_TEXTURE_2D);
		glEnable(GL_ALPHA_TEST);
		// TODO: This is where space is expected to have transparency

		if(!useAlpha)
		{
			glDisable(GL_BLEND);
			glColor4f(brightness, brightness, brightness, 1.0f);
		}
		else
		{
			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			glColor4f(brightness, brightness, brightness, alphaOverride);
		}
		glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, m_white.rgba());
		glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, m_white.rgba());
		m_pTexture->Bind(0);	
	}
	else
	{
		glEnable(GL_TEXTURE_2D);
		glDisable(GL_ALPHA_TEST);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glDisable(GL_BLEND);
		glColor4f(brightness, brightness, brightness, 1.0f);
		glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, m_white.rgba());
		m_pTexture->Bind(0);
	}
}



void SetShaderSolidStates(const MaterialGL* mat, bool useAlpha, float alphaOverride, bool blendTransparency, float brightness)
{
	glDisable(GL_ALPHA_TEST);	
	if(useAlpha)
	{
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glEnable(GL_BLEND);
	}
	else
	{
		glDisable(GL_BLEND);
	}
	Colour tmpCol = mat->GetColor();
	tmpCol.a() = alphaOverride;
	glColor4fv(tmpCol.rgba());
}

void SetShaderSolidAlphaStates(const MaterialGL* mat, bool useAlpha, float alphaOverride, bool blendTransparency, float brightness)
{
	glDisable(GL_ALPHA_TEST);
	glEnable(GL_BLEND);
	Colour tmpCol = mat->GetColor() * brightness;
	tmpCol.a() = -alphaOverride;
	glColor3fv(tmpCol.rgba());
	glBlendFunc(GL_ONE_MINUS_SRC_ALPHA, GL_SRC_ALPHA);
}


void SetShaderTransparentStates(const MaterialGL* mat, bool useAlpha, float alphaOverride, bool blendTransparency, float brightness)
{
	// Eurgh, messy
	if(blendTransparency)
	{
		glEnable(GL_BLEND);
		glDisable(GL_ALPHA_TEST);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glColor4f(1.0f, 1.0f, 1.0f, mat->m_alpha);
	}
	else if(mat->m_transluscent)
	{
		SetShaderTranslucentStates(mat, useAlpha, alphaOverride, blendTransparency, brightness);
	}
	else
	{
		glEnable(GL_ALPHA_TEST);
		glAlphaFunc(GL_GREATER, 0.2f);
		if(!useAlpha)
		{
			glDisable(GL_BLEND);
			glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
		}
		else
		{
			glEnable(GL_BLEND);
			glColor4f(1.0f, 1.0f, 1.0f, alphaOverride);
		}
	}
}

void SetShaderTranslucentStates(const MaterialGL* mat, bool useAlpha, float alphaOverride, bool blendTransparency, float brightness)
{
	glEnable(GL_BLEND);
	glDisable(GL_ALPHA_TEST);
	if(mat->m_additiveBlend)
	{
		glBlendFunc(GL_SRC_ALPHA, GL_ONE);
	}
	else
	{
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}
	glColor4f(1.0f, 1.0f, 1.0f, (useAlpha ? alphaOverride : mat->m_alpha) );		
}

void SetShaderStandardStates(const MaterialGL* mat, bool useAlpha, float alphaOverride, bool blendTransparency, float brightness)
{
	glDisable(GL_ALPHA_TEST);
	glDisable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
}