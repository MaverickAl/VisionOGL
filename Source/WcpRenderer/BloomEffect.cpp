#include "BloomEffect.h"

const float kKeyValue	= 0.008f;
const int	kMaxSamples	= 16;
const float kGlareLuminance = 1.0f;
const float kBloomLuminance = 1.0f;


BloomEffect::BloomEffect()
{

}


BloomEffect::~BloomEffect()
{

}


bool BloomEffect::Init(TreFile* pTreFile, int width, int height, float fsaaScale)
{
	bool ret = m_shader.LoadFromFile("shaders\\HDR2.fx", pTreFile);
	if(!ret)
	{
		assertMsg(false, "Failed to load bloom effect");
		return false;
	}

	m_width = width*fsaaScale;
	m_height = height*fsaaScale;


	m_bloomTechnique				= m_shader.GetTechiqueByName("Bloom");
	m_downScale4x4Technique			= m_shader.GetTechiqueByName("Downsample4x4");
	m_downScale2x2Technique			= m_shader.GetTechiqueByName("Downsample2x2");
	m_gaussBlur5x5Technique			= m_shader.GetTechiqueByName("GaussianBlur5x5");
	m_brightPassFilterTechnique		= m_shader.GetTechiqueByName("BrightPass");
	m_finalScenePassTechnique		= m_shader.GetTechiqueByName("SceneComposite");
	
	assert(cgValidateTechnique(m_bloomTechnique));

	m_bloomPass					= m_shader.GetFirstPass(m_bloomTechnique);
	m_downScale4x4Pass			= m_shader.GetFirstPass(m_downScale4x4Technique);
	m_downScale2x2Pass			= m_shader.GetFirstPass(m_downScale2x2Technique);
	m_gaussBlur5x5Pass			= m_shader.GetFirstPass(m_gaussBlur5x5Technique);
	m_brightPassFilterPass		= m_shader.GetFirstPass(m_brightPassFilterTechnique);
	m_finalScenePass			= m_shader.GetFirstPass(m_finalScenePassTechnique);

	m_avSampleOffsetsHndl	=	m_shader.GetVariableHandleByName("g_sampleOffsets");
	m_avSampleWeightsHndl	=	m_shader.GetVariableHandleByName("g_sampleWeights");
	m_middleGreyHndl		=	m_shader.GetVariableHandleByName("g_middleGray");
	m_bloomScaleHndl		=	m_shader.GetVariableHandleByName("g_bloomScale");

	char name[256];
	for(int i=0; i<8; i++)
	{
		sprintf(name, "s%i\0", i);
		m_texHndl[i] =	m_shader.GetVariableHandleByName(name);
	}

	const char* compileError = cgGetLastListing(CGShader::s_cgContext);
	if(compileError && compileError[0])
	{
		::MessageBox(0, compileError, "BloomEffect: Shader compile warnings", 0);
	}

	m_texSceneScaled.Create(true, true, false, m_width/4, m_height/4, 1);
	m_texBrightPass.Create(true, true, false, m_width/4, m_height/4, 1);	
	m_texBloomSource.Create(true, false, false, m_width/8, m_height/8, 1);	


	for(int i=0; i<kBloomTextures; i++)
	{
		m_texBloom[i].Create(true, false, false, m_width/8, m_height/8, 1);
	}

	return true;
}

void BloomEffect::DrawScreenQuad()
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

void BloomEffect::Draw(FBO* pMainFBO, float cameraFov, float exposureLevel)
{
	GLint scissor[4];
	GLint viewport[4];
	glGetIntegerv(GL_SCISSOR_BOX, scissor);
	glGetIntegerv(GL_VIEWPORT, viewport);
	glPushAttrib(GL_ENABLE_BIT);
	glEnable(GL_TEXTURE_2D);
	glDisable(GL_SCISSOR_TEST);

	cgSetParameter1f( m_bloomScaleHndl, kBloomLuminance );

	SceneToSceneScaled(pMainFBO);
	SceneScaledToBrightPass();

	BrightPassToStarSource();

	RenderBloom();

	cgSetParameter1f( m_middleGreyHndl, kKeyValue );

	pMainFBO->Detach();	// Return to main LDR fbo
	glViewport(0, 0,pMainFBO->GetWidth(), pMainFBO->GetHeight());
	cgGLSetTextureParameter(m_texHndl[0], pMainFBO->GetTexture(FBT_COLOUR) );
	cgGLSetTextureParameter(m_texHndl[1], m_texBloom[0].GetTexture(FBT_COLOUR) );

	glScissor(scissor[0], scissor[1], scissor[2], scissor[3]);
	glEnable(GL_SCISSOR_TEST);
	m_shader.BeginPass(m_finalScenePass);
	DrawScreenQuad();
	m_shader.EndPass(m_finalScenePass);

	glPopAttrib();
	assert(glGetError()==GL_NO_ERROR);
}


void BloomEffect::GetSampleOffsetsDownScale4x4( uint width, uint height, float* sampleOffsets )
{
    float tU = 1.0f / width;
    float tV = 1.0f / height;

    // Sample from the 16 surrounding points.
    int index = 0;
    for( int y = 0; y < 4; y++ )
    {
        for( int x = 0; x < 4; x++ )
        {
			// Center the 4x4 box-filter kernel symmetrically on the boundary between
			// the middle two source texels: offsets {-1.5,-0.5,0.5,1.5} * texel size.
			// (Previously (x - 2.0f), which shifted the whole kernel by half a source
			// texel)
            sampleOffsets[index*2  ] = ( x - 1.5f ) * tU;
            sampleOffsets[index*2+1] = ( y - 1.5f ) * tV;

            index++;
        }
    }
}



void BloomEffect::SceneToSceneScaled(FBO* pMainFBO)
{
    float avSampleOffsets[kMaxSamples*2];

	// Get the sample offsets used within the pixel shader
    GetSampleOffsetsDownScale4x4( pMainFBO->GetWidth(), pMainFBO->GetHeight(), avSampleOffsets );
    cgGLSetParameterArray2f( m_avSampleOffsetsHndl, 0, kMaxSamples, avSampleOffsets );

    m_texSceneScaled.Bind();
	glViewport(0, 0, m_texSceneScaled.GetWidth(), m_texSceneScaled.GetHeight());
	cgGLSetTextureParameter(m_texHndl[0], pMainFBO->GetTexture(FBT_COLOUR) );

	// Create a 1/4 scale copy of the HDR texture.
    m_shader.BeginPass( m_downScale4x4Pass );
	DrawScreenQuad();
	m_shader.EndPass( m_downScale4x4Pass );
}





void BloomEffect::SceneScaledToBrightPass()
{
	cgGLSetTextureParameter(m_texHndl[0], m_texSceneScaled.GetTexture(FBT_COLOUR) );

	m_texBrightPass.Bind();
	glViewport(0, 0, m_texBrightPass.GetWidth(), m_texBrightPass.GetHeight());
	m_shader.BeginPass( m_brightPassFilterPass );
	// Draw a fullscreen quad to sample the RT
	DrawScreenQuad();
	m_shader.EndPass(m_brightPassFilterPass);
}


void BloomEffect::BrightPassToStarSource()
{
    float sampleOffsets[kMaxSamples*2];
    float sampleWeights[kMaxSamples*4];

	GetSampleOffsetsGaussBlur5x5( m_texBrightPass.GetWidth(), m_texBrightPass.GetWidth(), sampleOffsets, sampleWeights );
    cgGLSetParameterArray2f( m_avSampleOffsetsHndl, 0, kMaxSamples, sampleOffsets);
    cgGLSetParameterArray4f( m_avSampleWeightsHndl, 0, kMaxSamples, sampleWeights);

    m_texBloomSource.Bind();
	glViewport(0, 0, m_texBloomSource.GetWidth(), m_texBloomSource.GetHeight());
    cgGLSetTextureParameter(m_texHndl[0], m_texBrightPass.GetTexture(FBT_COLOUR) );
	m_shader.BeginPass( m_gaussBlur5x5Pass );
	DrawScreenQuad();
	m_shader.EndPass( m_gaussBlur5x5Pass );
}


float GaussianDistribution( float x, float y, float rho )
{
	float g = 1.0f / sqrtf( 2.0f * wcpogl::pi * rho * rho );
    g *= expf( -( x * x + y * y ) / ( 2 * rho * rho ) );

    return g;
}


void BloomEffect::GetSampleOffsetsGaussBlur5x5( uint texWidth,  uint texHeight, float* texCoordOffset,
										float* sampleWeight, float fMultiplier )
{
    float tu = 1.0f / ( float )texWidth;
    float tv = 1.0f / ( float )texHeight;

    float totalWeight = 0.0f;
    int index = 0;
    for( int x = -2; x <= 2; x++ )
    {
        for( int y = -2; y <= 2; y++ )
        {
            // Exclude pixels with a block distance greater than 2 give a 5x5 kernel using only 13
            // sample points 
			if( abs( x ) + abs( y ) > 2 )
                continue;

            // Get the unscaled Gaussian intensity for this offset
            texCoordOffset[index*2]		= x * tu;
			texCoordOffset[index*2+1]	= y * tv;
            sampleWeight[index*4] = sampleWeight[index*4+1] = sampleWeight[index*4+2]
			= sampleWeight[index*4+3] = GaussianDistribution( ( float )x, ( float )y, 1.0f );
            totalWeight += sampleWeight[index*4];

            index++;
        }
    }

    // Divide the current weight by the total weight of all the samples; Gaussian
    // blur kernels add to 1.0f to ensure that the intensity of the image isn't
    // changed when the blur occurs. An optional multiplier variable is used to
    // add or remove image intensity during the blur.
    for( int i = 0; i < index*4; i++ )
    {
        sampleWeight[i] /= totalWeight;
        sampleWeight[i] *= fMultiplier;
    }
}


void GetSampleOffsetsBloom( uint texSize,
                                float texCoordOffset[15],
                                float* colorWeight,
                                float fDeviation,
                                float fMultiplier )
{
    int i = 0;
    float tu = 1.0f / ( float )texSize;

    // Fill the center texel
    float weight = fMultiplier * GaussianDistribution( 0, 0, fDeviation );
    colorWeight[0] = weight;
	colorWeight[1] = weight;
	colorWeight[2] = weight;
	colorWeight[3] = 1.0f;

    texCoordOffset[0] = 0.0f;

    // Fill the first half
    for( i = 1; i < 8; i++ )
    {
        // Gaussian intensity for this offset
        weight = fMultiplier * GaussianDistribution( ( float )i, 0, fDeviation );
        texCoordOffset[i] = i * tu;

        colorWeight[i*4  ] = weight;
		colorWeight[i*4+1] = weight;
		colorWeight[i*4+2] = weight;
		colorWeight[i*4+3] = 1.0f;
    }

    // Mirror to the second half
    for( i = 8; i < 15; i++ )
    {
        memcpy(&colorWeight[i*4], &colorWeight[(i-7)*4], sizeof(float)*4);
        texCoordOffset[i] = -texCoordOffset[i - 7];
    }
}


void BloomEffect::RenderBloom()
{
	float sampleOffsets[kMaxSamples*2];
	float sampleOffsets1D[kMaxSamples];
    float sampleWeights[kMaxSamples*4];

	// Fixme, need a clear
	//m_texBloomSource.Clear();


    if( kGlareLuminance <= 0.0f ||
        kBloomLuminance <= 0.0f )
    {
        return;
	}

	GetSampleOffsetsGaussBlur5x5( m_texBloomSource.GetWidth(), m_texBloomSource.GetHeight(), sampleOffsets, sampleWeights, 1.0f );

	cgGLSetParameterArray2f( m_avSampleOffsetsHndl, 0, kMaxSamples, sampleOffsets);
    cgGLSetParameterArray4f( m_avSampleWeightsHndl, 0, kMaxSamples, sampleWeights);


    m_texBloom[2].Bind();
	glViewport(0, 0, m_texBloom[2].GetWidth(), m_texBloom[2].GetHeight());
	cgGLSetTextureParameter(m_texHndl[0], m_texBloomSource.GetTexture(FBT_COLOUR) );

	m_shader.BeginPass(m_gaussBlur5x5Pass);
	DrawScreenQuad();
	m_shader.EndPass(m_gaussBlur5x5Pass);

    GetSampleOffsetsBloom( m_texBloom[2].GetWidth(), sampleOffsets1D, sampleWeights, 3.0f, 2.0f );
    for( int i = 0; i < kMaxSamples; i++ )
    {
		sampleOffsets[i*2]	= sampleOffsets1D[i];
		sampleOffsets[i*2+1]= 0.0f;
    }

	cgGLSetParameterArray2f( m_avSampleOffsetsHndl, 0, kMaxSamples, sampleOffsets);
    cgGLSetParameterArray4f( m_avSampleWeightsHndl, 0, kMaxSamples, sampleWeights);

    m_texBloom[1].Bind();
	glViewport(0, 0, m_texBloom[1].GetWidth(), m_texBloom[1].GetHeight());
    cgGLSetTextureParameter(m_texHndl[0], m_texBloom[2].GetTexture(FBT_COLOUR) );

	m_shader.BeginPass( m_bloomPass );
	DrawScreenQuad();
	m_shader.EndPass( m_bloomPass );

	GetSampleOffsetsBloom( m_texBloom[1].GetHeight(), sampleOffsets1D, sampleWeights, 3.0f, 2.0f );
    for( int i = 0; i < kMaxSamples; i++ )
    {
		sampleOffsets[i*2]		= 0.0f;
		sampleOffsets[i*2+1]	= sampleOffsets1D[i];
    }

	cgGLSetParameterArray2f( m_avSampleOffsetsHndl, 0, kMaxSamples, sampleOffsets);
    cgGLSetParameterArray4f( m_avSampleWeightsHndl, 0, kMaxSamples, sampleWeights);

	m_texBloom[0].Bind();
	glViewport(0, 0, m_texBloom[0].GetWidth(), m_texBloom[0].GetHeight());
	cgGLSetTextureParameter(m_texHndl[0], m_texBloom[1].GetTexture(FBT_COLOUR) );

	m_shader.BeginPass( m_bloomPass );
	DrawScreenQuad();
	m_shader.EndPass( m_bloomPass );
}
