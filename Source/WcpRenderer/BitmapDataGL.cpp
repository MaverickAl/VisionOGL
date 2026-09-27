#include "WcpOglUtility.h"
#include "Texture.h"
#include "WcpOglMath.h"
#include "BitmapDataGL.h"
#include "Texture.h"


BitmapDataGL::BitmapDataGL(int width, int height, const uint8* pData, const Palette *pal)
{
	CreateData(width, height, pData, pal);
}


BitmapDataGL::BitmapDataGL(int width, int height, const uint16* pData, const Palette *pal)
{
	CreateData(width, height, pData, pal);
} 


void BitmapDataGL::SetColourRGBA(int imageIndex, const uint8* color, bool visible)
{
	m_rgba[4*imageIndex]	= color[0];
	m_rgba[4*imageIndex+1]	= color[1];
	m_rgba[4*imageIndex+2]	= color[2];
	m_rgba[4*imageIndex+3]	= visible ? 255 : 0;
}

BitmapDataGL::~BitmapDataGL(void)
{
	wcpogl::SafeArrayDelete(&m_rgba);
	wcpogl::SafeArrayDelete(&m_visible);
	wcpogl::SafeDelete(&m_pImmTex);
}

 
void BitmapDataGL::GenTex(bool bForcePowTwo)
{
	// TODO: Premake the image data
	// Fil the texture buffer 
	if(bForcePowTwo)
	{
		m_texWidth = wcpogl::FindNextPowerOfTwo(m_width);
		m_texHeight = wcpogl::FindNextPowerOfTwo(m_height);
	}
	else
	{
		m_texWidth = m_width;
		m_texHeight = m_height;
	}

	int imageSize = m_texWidth*m_texHeight*4;

	unsigned char* dest = new unsigned char[imageSize];
	memset(dest, 0, sizeof(unsigned char)*imageSize);
	int destIndex = 0;
	int srcIndex = 0;
	for(int i=0; i<m_height; i++)
	{
		memcpy(&dest[destIndex], &m_rgba[srcIndex], m_width*4);
		destIndex += m_texWidth*4;
		srcIndex += m_width*4;
	}

	// Next create the texture 
	m_pImmTex = new Texture(dest, m_texWidth, m_texHeight, 4);	
	m_pImmTex->SetClamp();

	delete[] dest;
}

void BitmapDataGL::DrawSmooth(int x, int y) const
{
	float texCoordX = (float)m_width/(float)m_texWidth;
	float texCoordY = (float)m_height/(float)m_texHeight;

	// Then draw on screen with adjusted co-ordinates
	m_pImmTex->Bind(0);
	//glDisable(GL_TEXTURE_2D);
	glColor3f(1.0f, 1.0f, 1.0f);
	glBegin(GL_TRIANGLE_STRIP);
	glTexCoord2f(0.0f,		0.0f);		glVertex2f(x,			y);
	glTexCoord2f(texCoordX, 0.0f);		glVertex2f(x+m_width,	y);
	glTexCoord2f(0.0f,		texCoordY);	glVertex2f(x,			y+m_height);
	glTexCoord2f(texCoordX, texCoordY);	glVertex2f(x+m_width,	y+m_height);
	glEnd();
}
