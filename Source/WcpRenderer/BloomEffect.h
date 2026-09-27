#pragma once
#include "WcpOglUtility.h"
#include "Vector4.h"
#include "CGShader.h"
#include "FBO.h"

const int kBloomTextures = 3;

class BloomEffect
{
public:
	BloomEffect();
	~BloomEffect();

	bool Init(TreFile* pTreFile, int width, int height, float fsaaScale);
	void Draw(FBO* pMainFBO, float cameraFov, float exposureLevel);
private:
	void DrawScreenQuad();
	void GetSampleOffsetsDownScale4x4( uint width, uint height, float* sampleOffsets );
	void GetSampleOffsetsGaussBlur5x5( uint texWidth,  uint texHeight, float* texCoordOffset,
										float* sampleWeight, float fMultiplier = 1.0f );

	void SceneToSceneScaled(FBO* pMainFBO);
	void SceneScaledToBrightPass();
	void BrightPassToStarSource();
	void RenderBloom();


	CGShader		m_shader;
	CGtechnique		m_bloomTechnique;
	CGtechnique		m_downScale4x4Technique;
	CGtechnique		m_downScale2x2Technique;
	CGtechnique		m_gaussBlur5x5Technique;
	CGtechnique		m_brightPassFilterTechnique;
	CGtechnique		m_finalScenePassTechnique;

	CGpass		m_bloomPass;
	CGpass		m_downScale4x4Pass;
	CGpass		m_downScale2x2Pass;
	CGpass		m_gaussBlur5x5Pass;
	CGpass		m_brightPassFilterPass;
	CGpass		m_finalScenePass;


	int				m_width;
	int				m_height;

	CGparameter		m_avSampleOffsetsHndl;
	CGparameter		m_avSampleWeightsHndl;
	CGparameter		m_middleGreyHndl;
	CGparameter		m_bloomScaleHndl;
	CGparameter		m_texHndl[2];

	FBO				m_texSceneScaled;			// Scaled copy of the HDR scene
	FBO				m_texBrightPass;			// Bright-pass filtered copy of the scene
	FBO				m_texBloomSource;			// Bloom effect source texture

	FBO				m_texBloom[kBloomTextures];     // Blooming effect working textures
};
