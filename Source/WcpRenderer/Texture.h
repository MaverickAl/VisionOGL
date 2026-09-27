#ifndef _WCPOGL_TEXTUREGL
#define _WCPOGL_TEXTUREGL
#include "WcpOglUtility.h"
#include "Materials.h"
#include "GLExt.h"
#include "TgaLoader.h"


class Texture
{
public:
	Texture(const TextureMaterial* textureMat, bool bNebula = false);
	Texture(unsigned char* imageData, int width, int height, int colormode);
	Texture();
	~Texture(void);
	bool LoadFromTga(const char* filename);
	bool LoadFromTga(FILE* file, size_t uSize, bool useMipMaps = true);
	bool LoadFromMemory(unsigned char* imageData, int width, int height, int colorMode, bool useMipMaps = false);
	void SetRepeat();
	void SetClamp();
	void Bind( GLuint textureUnit ) const;
	void UpdateSubRegion(unsigned char* imageData, int x, int y, int width, int height);
	GLuint GetData() const { return m_texture; }
	bool IsValid() { return m_isValid; }
	bool HasAlpha() { return (m_colorMode == 1 || m_colorMode == 4); }
	static void SetMaxAnisotropy(int val) { s_maxAnisotropy = val; }
	unsigned short GetWidth() const { return m_width; }
	unsigned short GetHeight() const { return m_height; }

private:
	void SetColourRGBA(int imageIndex, const uint8* color, uint8 alpha);
	void SetColourRGB(int imageIndex, const uint8* color);
	u_long GetImageIndex(int sourceIndex, int imageWidth, int texWidth);
	void BuildTextureData(int colourMode, unsigned char* data, bool mipMaps = true);


	unsigned char*	m_pImageData;
	unsigned short	m_width;
	unsigned short	m_height;
	unsigned long	m_imageSize;
	bool			m_isValid;
	int				m_colorMode;
	static int		s_maxAnisotropy;

	GLuint			m_texture;
};


inline void Texture::SetColourRGB(int imageIndex, const uint8* color)
{
	memcpy(&m_pImageData[3*imageIndex], color, 3);
}

inline void Texture::SetColourRGBA(int imageIndex, const uint8* color, uint8 alpha)
{
	memcpy(&m_pImageData[4*imageIndex], color, 3);
	m_pImageData[4*imageIndex+3] = alpha;
}



#endif