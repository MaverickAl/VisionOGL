#pragma once
#include "WcpOglUtility.h"
#include "Vector4.h"
#include "CGShader.h"
#include "FBO.h"

class Texture;

class BriefingBloom
{
public:
	BriefingBloom();
	~BriefingBloom();

	bool Init(TreFile* pTreFile, int width, int height, float fsaaScale);
	void Draw(FBO* pScreenFBO, float brightPassCutoff, float exposureLevel);
private:
	void CreateTexelWeights(int rtSize, float* texCoordOffset, Vector4* texelWeights);
	float GaussianDistribution( float x, float y, float rho );
	void DrawScreenQuad();

	CGShader		m_shader;
	CGtechnique		m_gaussXTechnique;
	CGtechnique		m_gaussYTechnique;
	CGtechnique		m_scaleTechnique;
	CGtechnique		m_screenBlitTechnique;
	CGpass			m_gaussXPass;
	CGpass			m_gaussYPass;
	CGpass			m_scalePass;
	CGpass			m_screenBlitPass;

	int				m_width;
	int				m_height;

	// We don't really need to hang onto these but will do for now
	// for easy confirmation
	Vector4		m_vTexelWeights[16];
	Vector4		m_hTexelWeights[16];
	float			m_vSampleOffsets[32];
	float			m_hSampleOffsets[32];

	CGparameter		m_horzUVOffsHndl;
	CGparameter		m_vertUVOffsHndl;
	CGparameter		m_texelWeightHndl;
	CGparameter		m_pixelSizeHndl;
	CGparameter		m_exposureLevelHndl;
	CGparameter		m_renderMapHndl;
	CGparameter		m_fullResMapHndl;
	CGparameter		m_brightPassCutoff;

	FBO				m_tempFBO1;
	FBO				m_tempFBO2;

};


inline float BriefingBloom::GaussianDistribution( float x, float y, float rho )
{
	// Compute gaussian distribution using standard deviation rho
	float g = 1.0f / sqrtf( 2.0f * wcpogl::pi * rho * rho );
	g *= expf( -(x * x + y * y)/(2 * rho * rho));

	return g;
}