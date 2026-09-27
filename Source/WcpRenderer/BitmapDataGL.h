#pragma once
#include "WcpOglUtility.h"
#include "Materials.h"
#include "CommonInputStructs.h"

class Texture;

class BitmapDataGL
{
public:
	BitmapDataGL(int width, int height, const uint8* pData, const Palette *pal);
	BitmapDataGL(int width, int height, const uint16* pData, const Palette *pal);
	virtual ~BitmapDataGL(void);

	void SetColourRGBA(int imageIndex, const uint8* color, bool visible);

	void GenTex(bool bForcePowTwo);

	void Draw(int x, int y) const;
	void DrawSmooth(int x, int y) const;

	const unsigned char*	GetData() const { return m_rgba; }
	const bool*				GetVisibility() const { return m_visible; }
	int						GetWidth() const { return m_width; }
	int						GetHeight() const { return m_height; }
	SCREEN_2D_DETAIL		GetTargetDetail() const { return m_targetDetail; }
	bool					ShouldOffset() const { return m_useOffset; }

private:
	template <class RleType> 
	void CreateData(int width, int height, const RleType* rle, const Palette *pal);

	unsigned char*		m_rgba = nullptr;
	bool*				m_visible = nullptr;
	int					m_height;
	int					m_width;
	int					m_texWidth;
	int					m_texHeight;
	bool				m_useOffset;
	Texture*			m_pImmTex;
	SCREEN_2D_DETAIL	m_targetDetail;
};


inline void BitmapDataGL::Draw(int x, int y) const
{
	glRasterPos2i(x, y);
	glDrawPixels(m_width, m_height, GL_RGBA, GL_UNSIGNED_BYTE, m_rgba );
}


template <class RleType> 
void BitmapDataGL::CreateData(int width, int height, const RleType* rle, const Palette *pal)
{
	m_useOffset = false;
	m_pImmTex = NULL;
	const uint8* colourLookUp = pal->data;

	uint tag = GetPaletteTag(pal->rawData);

	switch(tag)
	{
	case 1280960:
		m_targetDetail = S2D_DOUBLE;
		m_useOffset = true;
		break;
#if SUPPORT_HI_RES_BGS
	case 1024768:
		m_targetDetail = S2D_HIGH;
		m_useOffset = true;
		break;
	case 800600:
		m_targetDetail = S2D_MED;
		m_useOffset = true;
		break;
#endif
	case 0x004f4654:
		m_useOffset = true;
		// Fall through to standard low res
	default:
		m_targetDetail = S2D_LOW;
	}

	m_width = width;
	m_height = height;

	int colourMode = 4;	// RGBA
	m_rgba = new unsigned char[m_width* m_height * colourMode];
	m_visible = new bool[m_width*m_height];

	uint32 rleIndex = 0;
	uint32 bitmapIndex = 0;
	const uint32 totalPixels = (uint32)(width * height);

	while(bitmapIndex < totalPixels)
	{
		uint32 func = rle[rleIndex++];
		uint32 size = rle[rleIndex++];

		uint32 remaining = totalPixels - bitmapIndex;
		if(size > remaining)
		{
			size = remaining;
		}

		switch(func)
		{
		case 0:
			{
				for(int i=0; i<size; i++)
				{
					uint32 palIndex = rle[rleIndex++];
					m_visible[bitmapIndex] = true;
					SetColourRGBA(bitmapIndex++, &colourLookUp[palIndex*3], true);
				}
			}
			break;
		case 1:
			{
				u_long palIndex = rle[rleIndex++];
				const uint8* color = &colourLookUp[palIndex*3];
				for (int i=0; i<size; i++)
				{
					m_visible[bitmapIndex] = true;
					SetColourRGBA(bitmapIndex++, color, true);
				}
			}
			break;
		default:
			{
				uint8 col[3]; col[0] = 0; col[1] = 0; col[2] = 0;
				for (int i=0; i<size; i++)
				{
					m_visible[bitmapIndex] = false;
					SetColourRGBA(bitmapIndex++, &col[0], false);
				}
				break;
			}
		}
	}
}