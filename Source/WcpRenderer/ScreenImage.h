#pragma once
#include "Texture.h"
#include "BitmapDataGL.h"

class ScreenImage
{
public:
	ScreenImage(int sourceWidth, int sourceHeight, bool bForcePowTwo);
	~ScreenImage(void);

	void UpdateImage(const BitmapDataGL* bitmap, int x, int y);
	void Draw(float width, float height);
	bool ShouldDraw() { return m_isDirty; }
	float GetAspect() const { return (float)m_width/(float)m_height; }
private:
	void SetColourRGBA(int imageIndex, const unsigned char* color, uint8 alpha);

	unsigned char*	m_pImageData = nullptr;
	
	Texture*		m_pTexture = nullptr;
	int				m_width;
	int				m_height;
	int				m_imageWidth;
	int				m_imageHeight;
	bool			m_isDirty;
};


inline void ScreenImage::SetColourRGBA(int imageIndex, const unsigned char* color, uint8 alpha)
{
	// Entirely possible index is out of range (e.g. cursor is drawn off screen
	// discard such entrie
	if(imageIndex < 0 || imageIndex >= (m_width*m_height))
	{
		return;
	}
	m_pImageData[4*imageIndex] = color[0];
	m_pImageData[4*imageIndex+1] = color[1];
	m_pImageData[4*imageIndex+2] = color[2];
	m_pImageData[4*imageIndex+3] = alpha;

}