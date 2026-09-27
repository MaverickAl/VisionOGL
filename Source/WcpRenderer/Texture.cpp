#include "Texture.h"
#include "Materials.h"
#include <gl/glu.h>
#include "WcpOglMath.h"


int Texture::s_maxAnisotropy = 4;

Texture::Texture(const TextureMaterial* textureMat, bool bIsNebula):
m_pImageData(NULL),
m_isValid(false),
m_texture(0)
{
	const uint8*	rgb		= textureMat->rgbMap;
	const uint8*	alpha	= textureMat->alphaMap;
	const uint8*	pal		= (rgb ? textureMat->palette->data : NULL);
	m_width					= textureMat->width;
	m_height				= textureMat->height;

	int m_colorMode;
	if(rgb)
	{
		if( alpha || textureMat->transparency )
		{
			m_colorMode = 4;
		}
		else
		{
			m_colorMode = 3;
		}
	}
	else
	{
		m_colorMode = 1;	// Alpha only
	}

	m_imageSize = m_colorMode * m_width * m_height;
	m_pImageData = new unsigned char[m_imageSize];

	int palIndex = 0;
	if( m_colorMode == 4 )
	{
		uint8 aCol = textureMat->alpha;
		uint8 black[3] = { 0, 0, 0 };
		if( alpha )
		{
			for( int i=0; i < m_width * m_height; i++ )
			{
				palIndex = rgb[i];
				// Has a proper alpha map
				if( palIndex == 0 && textureMat->transparency )
				{
					SetColourRGBA(i, black, 0);
				}
				else
				{
					SetColourRGBA(i, &pal[rgb[i]*3], 255-alpha[i]);
				}
			}
		}
		else if( textureMat->transparency )
		{
			for( int i=0; i < m_width * m_height; i++ )
			{
				palIndex = rgb[i];
				if( palIndex == 0 )
				{
					SetColourRGBA(i, black, 0);
				}
				else
				{
					SetColourRGBA(i, &pal[palIndex*3], 255-aCol);
				}
			}
		}
	}
	else if( m_colorMode == 3 )
	{
		// Because the texture has to wrap the first entry should always be black space, but there's a nebula which is too bright
		uint8 offset[3] = {0, 0, 0};
		bool bDoAdjustment = false;
		if (bIsNebula)
		{
			palIndex = rgb[0];
			memcpy(offset, &pal[palIndex * 3], 3);

			// I *really* don't like this hack and worry about false positives so check if its a uniform grey
			// FIXME: Much better to use adjusted data
			bDoAdjustment = (offset[0] == offset[1]) && (offset[1] == offset[2]) && offset[0] > 0;
			for (int c = 0; c < 3; c++)
			{
				if (offset[c] > 40)
				{
					bDoAdjustment = false;
				}
			}
		}
		for( int i=0; i < m_width * m_height; i++ )
		{
			palIndex = rgb[i];
			SetColourRGB(i, &pal[palIndex*3]);
		}
		if (bDoAdjustment && offset[0] != 0)
		{
			uint8 maxValue = 255;
			uint8 minValue = offset[0];
			
			uint8 sourceRange = maxValue - minValue;
			for (int i = 0; i < m_width * m_height; i++)
			{
				for(int c = 0; c<3; c++)
				{
					float normalised = (m_pImageData[(3 * i) + c] - minValue) / (float)(sourceRange);
					//uint8 adjusted = max(0, m_pImageData[(3 * i)+c] - offset[c]);
					uint8 adjusted = normalised * maxValue;
					m_pImageData[(3 * i) + c] = adjusted;
				}
			}
		}
	}
	else
	{
		// Straight copy of the alpha map
		memcpy(m_pImageData, alpha, sizeof(unsigned char)*m_height*m_width);
	}

	BuildTextureData(m_colorMode, m_pImageData);
	// We can safely delete the image data now
	delete[] m_pImageData;
	m_pImageData = NULL;
}


Texture::Texture(unsigned char* imageData, int width, int height, int colormode) :
m_pImageData(NULL),
m_isValid(false),
m_texture(0)
{
	m_width = width;
	m_height = height;

	BuildTextureData(colormode, imageData, false);
}

Texture::Texture():
m_pImageData(NULL),
m_isValid(false),
m_colorMode(0),
m_texture(0)
{

}

bool Texture::LoadFromTga(const char* filename)
{
	TgaLoader tga;

	if(!tga.Load(filename))
		return false;

	int colorMode = tga.GetImageFormat()==IMAGE_RGBA ? 4 : 3;
	m_width = tga.GetWidth();
	m_height = tga.GetHeight();
	BuildTextureData(colorMode, tga.GetImage(), true);

	return true;
}

bool Texture::LoadFromTga(FILE* file, size_t uSize, bool useMipMaps)
{
	TgaLoader tga;

	if(!tga.Load(file, uSize))
		return false;

	int colorMode = tga.GetImageFormat()==IMAGE_RGBA ? 4 : 3;
	m_width = tga.GetWidth();
	m_height = tga.GetHeight();
	BuildTextureData(colorMode, tga.GetImage(), useMipMaps);

	return true;
}

bool Texture::LoadFromMemory(unsigned char* imageData, int width, int height, int colorMode, bool useMipMaps)
{
	if (!imageData || width <= 0 || height <= 0)
		return false;

	if (m_texture)
	{
		glDeleteTextures(1, &m_texture);
		m_texture = 0;
	}

	m_width  = (unsigned short)width;
	m_height = (unsigned short)height;

	BuildTextureData(colorMode, imageData, useMipMaps);

	m_isValid = true;
	return true;
}

void Texture::UpdateSubRegion(unsigned char* imageData, int x, int y, int width, int height)
{
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, m_texture);
	if (m_colorMode == 4)
	{
		glTexSubImage2D(GL_TEXTURE_2D, 0, x, y, width, height, GL_RGBA, GL_UNSIGNED_BYTE, imageData);
	}
	else if (m_colorMode == 3)
	{
		glTexSubImage2D(GL_TEXTURE_2D, 0, x, y, width, height, GL_RGB, GL_UNSIGNED_BYTE, imageData);
	}
	else
	{
		assert(false);
	}
}

void Texture::BuildTextureData(int colormode, unsigned char* data, bool mipMaps)
{
	assert(glGetError() == GL_NO_ERROR);
	m_colorMode = colormode;
	glGenTextures(1, &m_texture);
	glBindTexture(GL_TEXTURE_2D, m_texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	GLint minFilter = mipMaps ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR;

	assert(glGetError() == GL_NO_ERROR);


	if(GLExt::Version_1_2 && mipMaps)
	{
		GLfloat largest_supported_anisotropy;
		glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &largest_supported_anisotropy);
		assert(glGetError() == GL_NO_ERROR);
		largest_supported_anisotropy = largest_supported_anisotropy > s_maxAnisotropy ? (GLfloat)s_maxAnisotropy : largest_supported_anisotropy;
		if(largest_supported_anisotropy > 0.0f)
		{
			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, largest_supported_anisotropy);
			assert(glGetError() == GL_NO_ERROR);
		}
	}

	assert(glGetError() == GL_NO_ERROR);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minFilter);


	if(mipMaps)
	{
		/*
		switch(m_colorMode)
		{
		case 4:
			gluBuild2DMipmaps(GL_TEXTURE_2D, GL_RGBA, m_width,
				m_height, GL_RGBA, GL_UNSIGNED_BYTE, data);
			break;
		case 3:
			gluBuild2DMipmaps(GL_TEXTURE_2D, GL_RGB, m_width,
				m_height, GL_RGB, GL_UNSIGNED_BYTE, data);
			break;
		case 1:
			gluBuild2DMipmaps(GL_TEXTURE_2D, GL_ALPHA, m_width,

				m_height, GL_ALPHA, GL_UNSIGNED_BYTE, data);
			break;
		default:
			break;
		}*/
	}
	//else
	{
		switch(m_colorMode)
		{
		case 4:
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, m_width,
				m_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
			break;
		case 3:
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, m_width,
				m_height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
			break;
		case 1:
			glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, m_width,
				m_height, 0, GL_ALPHA, GL_UNSIGNED_BYTE, data);
			break;
		default:
			break;
		}
	}

	assert(glGetError() == GL_NO_ERROR);


	if (mipMaps)
	{
		glGenerateMipmap(GL_TEXTURE_2D);
	}

	assert(glGetError() == GL_NO_ERROR);

}


u_long Texture::GetImageIndex(int sourceIndex, int imageWidth, int texWidth)
{
	u_long lineNo = sourceIndex/imageWidth;
	u_long extraPixels = (u_long)(texWidth-imageWidth)*lineNo;

	return sourceIndex + (extraPixels);
}


void Texture::SetClamp()
{
	Bind(0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
}


void Texture::SetRepeat()
{
	Bind(0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
}


Texture::~Texture()
{
	if(m_texture)
	{
		glDeleteTextures(1, &m_texture);
	}
	//delete[] m_pImageData;
}


void Texture::Bind( GLuint textureUnit ) const
{
	// Set the active texture unit and enable
	glActiveTexture( GL_TEXTURE0 + textureUnit );
	glEnable( GL_TEXTURE_2D );
	// Bind the texture
	glBindTexture(GL_TEXTURE_2D, m_texture);
}