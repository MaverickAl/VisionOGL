#include "BriefingBloom.h"
#include "Texture.h"

//const float kExposureLevel		= 1.0f;

#if !EDER_TEST_BUILD
static const float kGaussDeviation		= 1.1f;
static const float kGaussMultiplier		= 0.8f;
#else
float kGaussDeviation	= 7.0f;
float kGaussMultiplier	= 1.5f;
#endif

BriefingBloom::BriefingBloom() 
{
}


BriefingBloom::~BriefingBloom() 
{ 
}


bool BriefingBloom::Init(TreFile* pTreFile, int width, int height, float fsaaScale)
{
	bool ret = m_shader.LoadFromFile("shaders\\HDR.fx", pTreFile);
	if(!ret)
	{
		assertMsg(false, "Failed to load bloom effect");
		return false;
	}

	m_width = width*fsaaScale;
	m_height = height*fsaaScale;

	m_scaleTechnique		= m_shader.GetTechiqueByName("BrightPass");
	m_gaussXTechnique		= m_shader.GetTechiqueByName("BlurHorizontal");
	m_gaussYTechnique		= m_shader.GetTechiqueByName("BlurVertical");
	m_screenBlitTechnique	= m_shader.GetTechiqueByName("SceneComposite");
	
	assert(cgValidateTechnique(m_scaleTechnique));
	assert(cgValidateTechnique(m_gaussXTechnique));
	assert(cgValidateTechnique(m_gaussYTechnique));
	assert(cgValidateTechnique(m_screenBlitTechnique));

	m_gaussXPass		= m_shader.GetFirstPass(m_gaussXTechnique);
	m_gaussYPass		= m_shader.GetFirstPass(m_gaussYTechnique);
	m_scalePass			= m_shader.GetFirstPass(m_scaleTechnique);
	m_screenBlitPass	= m_shader.GetFirstPass(m_screenBlitTechnique);

	m_horzUVOffsHndl	=	m_shader.GetVariableHandleByName("horzTapOffsets");
	m_vertUVOffsHndl	=	m_shader.GetVariableHandleByName("vertTapOffsets");
	m_texelWeightHndl	=	m_shader.GetVariableHandleByName("texelWeights");
	m_pixelSizeHndl		=	m_shader.GetVariableHandleByName("pixelSize");
	m_exposureLevelHndl	=	m_shader.GetVariableHandleByName("exposureLevel");
	m_brightPassCutoff	=	m_shader.GetVariableHandleByName("brightPassCutoff");
	m_renderMapHndl		=	m_shader.GetVariableHandleByName("renderMap");
	m_fullResMapHndl	=	m_shader.GetVariableHandleByName("fullResMap");

	const char* compileError = cgGetLastListing(CGShader::s_cgContext);
	if(compileError && compileError[0])
	{
		::MessageBox(0, compileError, "Briefing Bloom: Shader compile warnings", 0);
	}

	m_tempFBO1.Create(true, false, false, m_width/2, m_height/2);
	m_tempFBO2.Create(true, false, false, m_width/2, m_height/2);

	float sampleOffsets[16];

	float fAspect = (float)m_width/(float)m_height;

	CreateTexelWeights(m_height/2.0f, sampleOffsets, m_vTexelWeights);

	for(int i=0; i < 16; i++)
	{
		m_vSampleOffsets[i*2]		= 0.0f;
		m_vSampleOffsets[(i*2)+1]	= sampleOffsets[i];
	}

	CreateTexelWeights(m_width/2.0f, sampleOffsets, m_hTexelWeights);

	for(int i=0; i < 16; i++)
	{
		m_hSampleOffsets[i*2]		= sampleOffsets[i];
		m_hSampleOffsets[(i*2)+1]	= 0.0f;
	}


	return true;
}

void BriefingBloom::DrawScreenQuad()
{
	float left		= -1.0f;
	float right		= 1.0f;
	float top		= 1.0f;
	float bottom	= -1.0f;

	glBegin(GL_TRIANGLE_STRIP);
	glVertex2f(left,	bottom);
	glVertex2f(left,	top);
	glVertex2f(right,	bottom);
	glVertex2f(right,	top);
	glEnd();
}

void BriefingBloom::Draw(FBO* pSourceFBO, float brightPassCutoff, float exposureLevel)
{	
 	// Store the original viewport info
	GLint scissor[4];
	GLint viewport[4];
	glGetIntegerv(GL_SCISSOR_BOX, scissor);
	glGetIntegerv(GL_VIEWPORT, viewport);
	glPushAttrib(GL_ENABLE_BIT);
	glEnable(GL_TEXTURE_2D);
	glDisable(GL_SCISSOR_TEST);

	// TODO: Not really necessary to set all these in draw, just being safe
	cgSetParameter1f(m_brightPassCutoff, brightPassCutoff);
	cgSetParameter1f(m_exposureLevelHndl, exposureLevel);//kExposureLevel);
	cgGLSetParameterArray2f(m_horzUVOffsHndl,	0, 7, m_hSampleOffsets);
	cgGLSetParameterArray2f(m_vertUVOffsHndl,	0, 7, m_vSampleOffsets);
	// Lie on pixel centers in opengl
	float pixelSizeX = -1.0f / ((float)m_width/2.0f);
	float pixelSizeY =  1.0f / ((float)m_height/2.0f);
	cgSetParameter4f(m_pixelSizeHndl, pixelSizeX, pixelSizeY, 1.0f, 1.0f);

	// downsample
	glViewport(0, 0, m_tempFBO1.GetWidth(), m_tempFBO1.GetHeight());
	m_tempFBO1.Bind();
	cgGLSetTextureParameter(m_renderMapHndl, pSourceFBO->GetTexture(FBT_COLOUR) );
	m_shader.BeginPass(m_scalePass);
	DrawScreenQuad();
	m_shader.EndPass(m_scalePass);

	// GaussY filter
	m_tempFBO2.Bind();
	cgGLSetTextureParameter(m_renderMapHndl, m_tempFBO1.GetTexture(FBT_COLOUR) );
	cgGLSetParameterArray4f(m_texelWeightHndl,	0, 7, (float*)m_vTexelWeights);
	m_shader.BeginPass(m_gaussYPass);
	DrawScreenQuad();
	m_shader.EndPass(m_gaussYPass);

	// GaussX filter
	m_tempFBO1.Bind();
	cgGLSetTextureParameter(m_renderMapHndl, m_tempFBO2.GetTexture(FBT_COLOUR) );
	cgGLSetParameterArray4f(m_texelWeightHndl,	0, 7, (float*)m_hTexelWeights);
	m_shader.BeginPass(m_gaussXPass);
	DrawScreenQuad();
	m_shader.EndPass(m_gaussXPass);

	// Dump out to main backbuffer
	// Reset original clip (we don't reset viewport so as not to mess up the co-ordiantes)
	glViewport(0, 0, pSourceFBO->GetWidth(), pSourceFBO->GetHeight());
	glScissor(scissor[0], scissor[1], scissor[2], scissor[3]);
	glEnable(GL_SCISSOR_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

	m_tempFBO1.Detach();	// Reset back to default backbuffer
	cgGLSetTextureParameter(m_renderMapHndl, m_tempFBO1.GetTexture(FBT_COLOUR) );
	cgGLSetTextureParameter(m_fullResMapHndl, pSourceFBO->GetTexture(FBT_COLOUR) );
	m_shader.BeginPass(m_screenBlitPass);
	DrawScreenQuad();
	m_shader.EndPass(m_screenBlitPass);
	glPopAttrib();

}


void BriefingBloom::CreateTexelWeights(int rtSize, float* texCoordOffsets, Vector4* texelWeights)
{
	float tu = 1.0f / (float)rtSize; 

	// Fill the center texel
	float weight = kGaussMultiplier * GaussianDistribution( 0, 0, kGaussDeviation );
	texelWeights[0] = Vector4( weight, weight, weight, 1.0f );
	texCoordOffsets[0] = 0.0f;

	// Fill the first half
	for( int i=1; i < 8; i++ )
	{
		// Get the Gaussian intensity for this offset
		weight = kGaussMultiplier * GaussianDistribution( (float)i, 0, kGaussDeviation );
		texCoordOffsets[i] = i * tu;
		texelWeights[i] = Vector4( weight, weight, weight, 1.0f );
	}

	// Mirror to the second half
	for( int i=8; i < 15; i++ )
	{
		texelWeights[i]		= texelWeights[i-7];
		texCoordOffsets[i]	= -texCoordOffsets[i-7];
	}
}