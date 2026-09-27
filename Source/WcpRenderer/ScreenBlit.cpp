#include "ScreenBlit.h"


ScreenBlit::ScreenBlit()
{

}


ScreenBlit::~ScreenBlit()
{

}


bool ScreenBlit::Init(TreFile *pTreFile)
{
	bool ret = m_shader.LoadFromFile("shaders\\ScreenBlit.fx", pTreFile);
	if(!ret)
	{
		assertMsg(false, "Failed to load screen blit effect");
		return false;
	}

	m_technique		= m_shader.GetTechiqueByName("ScreenBlit");
	m_pass			= m_shader.GetFirstPass(m_technique);
	m_texHndl		= m_shader.GetVariableHandleByName("ScreenImage");

	const char* compileError = cgGetLastListing(CGShader::s_cgContext);
	if(compileError && compileError[0])
	{
		::MessageBox(0, compileError, "ScreenBlit Effect: Shader compile warnings", 0);
		ret = false;
	}

	return true;
}


void ScreenBlit::Draw(FBO *pScreenFBO)
{
 	// Store the original viewport info
	GLint viewport[4];
	glGetIntegerv(GL_VIEWPORT, viewport);
	glPushAttrib(GL_ENABLE_BIT);
	glEnable(GL_TEXTURE_2D);
	glDisable(GL_SCISSOR_TEST);

	glEnable(GL_SCISSOR_TEST);

	cgGLSetTextureParameter(m_texHndl, pScreenFBO->GetTexture(FBT_COLOUR) );
	m_shader.BeginPass(m_pass);
	float left		= -1.0f;
	float right		= 1.0f;
	float top		= 1.0f;
	float bottom	= -1.0f;

	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
	glBegin(GL_TRIANGLE_STRIP);
	glVertex2f(left,	bottom);
	glVertex2f(left,	top);
	glVertex2f(right,	bottom);
	glVertex2f(right,	top);
	glEnd();
	m_shader.EndPass(m_pass);
	glPopAttrib();
}