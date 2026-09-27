#include "ScreenImage.h"
#include "WcpOglUtility.h"

ScreenImage::ScreenImage(int sourceWidth, int sourceHeight, bool bForcePowTwo)
{
	m_width = sourceWidth;
	m_height = sourceHeight;
	m_isDirty = false;
	if (bForcePowTwo)
	{
		m_imageWidth = wcpogl::FindNextPowerOfTwo(m_width);
		m_imageHeight = wcpogl::FindNextPowerOfTwo(m_height);
	}
	else
	{
		m_imageWidth = m_width;
		m_imageHeight = m_height;
	}

	int textureSize = m_imageWidth * m_imageHeight * 4;
	m_pImageData = new unsigned char[textureSize];
	memset(m_pImageData, 0, sizeof(unsigned char) * textureSize);
	m_pTexture = new Texture(m_pImageData, m_imageWidth, m_imageHeight, 4);
	delete[] m_pImageData;

	// Our true image 
	int imageSize = m_width * m_height * 4;
	m_pImageData = new unsigned char[imageSize];
	memset(m_pImageData, 0, sizeof(unsigned char) * imageSize);
}

ScreenImage::~ScreenImage(void)
{
	if(m_pImageData)
		delete[] m_pImageData;

	if(m_pTexture)
		delete m_pTexture;
}

void ScreenImage::UpdateImage(const BitmapDataGL* bitmap, int x, int y)
{
	const unsigned char* rgba = bitmap->GetData();
	const bool* visibility = bitmap->GetVisibility();
	const int				srcStride = bitmap->GetWidth(); // full source row width, never changes
	int sourceWidth = bitmap->GetWidth();
	int sourceHeight = bitmap->GetHeight();

	if (y < 0)
	{
		rgba += srcStride * 4 * (-y);
		visibility += srcStride * (-y);
		sourceHeight += y;
		y = 0;
	}

	if (x < 0)
	{
		rgba += 4 * (-x);
		visibility += (-x);
		sourceWidth += x;
		x = 0;
	}

	// Nothing left to draw, or starting position is already off the image
	if (sourceHeight <= 0 || sourceWidth <= 0 || x >= m_width || y >= m_height)
	{
		m_isDirty = true;
		return;
	}

	// Clip bottom/right edges against the destination image
	if (y + sourceHeight > m_height)
	{
		sourceHeight = m_height - y;
	}
	if (x + sourceWidth > m_width)
	{
		sourceWidth = m_width - x;
	}

	unsigned char* pImageDest = &m_pImageData[4 * (x + y * m_width)];

	for (int srcY = 0; srcY < sourceHeight; srcY++)
	{
		const unsigned char* pSrcRow = rgba + (srcY * srcStride * 4);
		const bool* pVisRow = visibility + (srcY * srcStride);

		for (int srcX = 0; srcX < sourceWidth; srcX++)
		{
			if (pVisRow[srcX])
			{
				memcpy(pImageDest, pSrcRow + srcX * 4, 4);
			}
			pImageDest += 4;
		}
		// advance dest to the start of the next destination row
		pImageDest += (m_width - sourceWidth) * 4;
	}

	m_isDirty = true;
}


void ScreenImage::Draw(float width, float height)
{
	if(!m_isDirty)
		return;

	float texCoordX = (float)m_width/(float)m_imageWidth;
	float texCoordY = (float)m_height/(float)m_imageHeight;

	glPushMatrix();
	
	m_pTexture->UpdateSubRegion(m_pImageData, 0, 0, m_width, m_height);
	m_pTexture->SetClamp();
	m_pTexture->Bind(0);
	glColor3f(1.0f, 1.0f, 1.0f);
	glBegin(GL_TRIANGLE_STRIP);
	glTexCoord2f(0.0f, 0.0f);
	glVertex2f(0.0f, 0.0f);
	glTexCoord2f(texCoordX, 0.0f);
	glVertex2f(width, 0.0f);
	glTexCoord2f(0.0f, texCoordY);
	glVertex2f(0.0f,height);
	glTexCoord2f(texCoordX, texCoordY);
	glVertex2f(width, height);
	glEnd();

	glPopMatrix();

	// We won't be drawing a second time, clear it up for reuse
	int imageSize = m_width * m_height * 4;
	memset(m_pImageData, 0, imageSize);

	m_isDirty = false;
}