#include <cstring>
#include <algorithm>
#include "TgaLoader.h"

#if defined(_MSC_VER)
	#define TGA_FOPEN(fp, name, mode) fopen_s(&fp, name, mode)
#else
	#define TGA_FOPEN(fp, name, mode) (fp = fopen(name, mode))
#endif

TgaLoader::TgaLoader()
	: m_colorDepth(0)
	, m_imageDataType(IMAGE_DATA_UNSIGNED_BYTE)
	, m_imageDataFormat(IMAGE_RGB)
	, m_pImageData(NULL)
	, m_width(0)
	, m_height(0)
	, m_imageSize(0)
{
}

TgaLoader::~TgaLoader()
{
	Release();
}

void TgaLoader::Release()
{
	delete[] m_pImageData;
	m_pImageData = NULL;
	m_imageSize = 0;
}

void TgaLoader::SwapRedBlue()
{
	if (!m_pImageData)
		return;

	const int pixelCount = m_width * m_height;

	if (m_colorDepth == 32)
	{
		rgba_t* p = reinterpret_cast<rgba_t*>(m_pImageData);
		for (int i = 0; i < pixelCount; ++i)
			std::swap(p[i].r, p[i].b);
	}
	else if (m_colorDepth == 24)
	{
		rgb_t* p = reinterpret_cast<rgb_t*>(m_pImageData);
		for (int i = 0; i < pixelCount; ++i)
			std::swap(p[i].r, p[i].b);
	}
}

bool TgaLoader::Load(const char *filename)
{
	FILE *pFile = NULL;
	TGA_FOPEN(pFile, filename, "rb");

	if (!pFile)
		return false;

	fseek(pFile, 0, SEEK_END);
	size_t uSize = (size_t)ftell(pFile);
	fseek(pFile, 0, SEEK_SET);

	bool result = Load(pFile, uSize);

	fclose(pFile);

	return result;
}


bool TgaLoader::DecodeUncompressed(FILE* pFile, int channels)
{
	if (fread(m_pImageData, 1, m_imageSize, pFile) != m_imageSize)
		return false;

	unsigned char* p = m_pImageData;
	unsigned char* end = m_pImageData + m_imageSize;

	if (channels >= 3)
	{
		for (; p < end; p += channels)
			std::swap(p[0], p[2]); // file order is B,G,R[,A] -> R,G,B[,A]
	}

	return true;
}


bool TgaLoader::DecodeRLE(FILE* pFile, size_t bytesAvailable, int channels)
{
	if (bytesAvailable == 0)
		return false;

	unsigned char* raw = new unsigned char[bytesAvailable];
	if (fread(raw, 1, bytesAvailable, pFile) != bytesAvailable)
	{
		delete[] raw;
		return false;
	}

	unsigned char* src    = raw;
	unsigned char* srcEnd = raw + bytesAvailable;
	unsigned char* dest   = m_pImageData;
	unsigned char* destEnd = m_pImageData + m_imageSize;

	while (dest < destEnd && src < srcEnd)
	{
		unsigned char packet = *src++;
		bool isRun = (packet & 0x80) != 0;
		int  count = (packet & 0x7F) + 1;
		size_t runBytes = (size_t)count * channels;

		if (runBytes > (size_t)(destEnd - dest))
			runBytes = (size_t)(destEnd - dest);

		if (isRun)
		{
			if (src + channels > srcEnd)
				break;

			unsigned char pixel[4];
			pixel[0] = src[2]; // R (file stores B,G,R[,A])
			pixel[1] = src[1]; // G
			pixel[2] = src[0]; // B
			if (channels == 4)
				pixel[3] = src[3]; // A
			src += channels;

			// write the first pixel, then double it outward to fill the run
			size_t written = std::min((size_t)channels, runBytes);
			memcpy(dest, pixel, written);
			while (written < runBytes)
			{
				size_t chunk = std::min(written, runBytes - written);
				memcpy(dest + written, dest, chunk);
				written += chunk;
			}
			dest += runBytes;
		}
		else
		{
			size_t srcBytes = (size_t)count * channels;
			if (src + srcBytes > srcEnd)
				srcBytes = (size_t)(srcEnd - src);

			size_t pixelsAvailable = srcBytes / channels;
			for (size_t i = 0; i < pixelsAvailable && dest < destEnd; ++i)
			{
				dest[0] = src[2];
				dest[1] = src[1];
				dest[2] = src[0];
				if (channels == 4)
					dest[3] = src[3];

				dest += channels;
				src  += channels;
			}
		}
	}

	delete[] raw;
	return true;
}

bool TgaLoader::Load(FILE* pFile, size_t fileSize)
{
	Release(); 

	tgaheader_t tgaHeader;
	size_t uStartPos = (size_t)ftell(pFile);

	if (fread(&tgaHeader, 1, sizeof(tgaheader_t), pFile) != sizeof(tgaheader_t))
		return false;

	// only RGB / grayscale, compressed or not, with no colormap
	bool typeOk = (tgaHeader.imageTypeCode == TGA_RGB) ||
	              (tgaHeader.imageTypeCode == TGA_GRAYSCALE) ||
	              (tgaHeader.imageTypeCode == TGA_RGB_RLE) ||
	              (tgaHeader.imageTypeCode == TGA_GRAYSCALE_RLE);

	if (!typeOk || tgaHeader.colorMapType != 0)
		return false;

	m_width  = tgaHeader.width;
	m_height = tgaHeader.height;

	bool isGrayscale = (tgaHeader.imageTypeCode == TGA_GRAYSCALE) ||
	                    (tgaHeader.imageTypeCode == TGA_GRAYSCALE_RLE);

	int channels = tgaHeader.bpp / 8;

	// non-grayscale images below 24bpp aren't supported
	if (!isGrayscale && channels < 3)
		return false;

	if (isGrayscale)
		channels = 1;

	if (m_width == 0 || m_height == 0)
		return false;

	m_imageSize = (unsigned long)m_width * m_height * channels;
	m_pImageData = new unsigned char[m_imageSize];

	// skip past the image ID field, if present (fseek args are offset, origin)
	if (tgaHeader.idLength > 0)
		fseek(pFile, tgaHeader.idLength, SEEK_CUR);

	bool ok;
	bool isRLE = (tgaHeader.imageTypeCode == TGA_RGB_RLE) ||
	             (tgaHeader.imageTypeCode == TGA_GRAYSCALE_RLE);

	if (isGrayscale)
	{
		// single channel, no byte-order swap needed either way
		ok = (fread(m_pImageData, 1, m_imageSize, pFile) == m_imageSize);
	}
	else if (!isRLE)
	{
		ok = DecodeUncompressed(pFile, channels);
	}
	else
	{
		size_t uCurrPos = (size_t)ftell(pFile);
		size_t uRemaining = (fileSize > (uCurrPos - uStartPos))
			? fileSize - (uCurrPos - uStartPos)
			: 0;
		ok = DecodeRLE(pFile, uRemaining, channels);
	}

	if (!ok)
	{
		Release();
		return false;
	}

	if (isGrayscale)
	{
		m_imageDataFormat = IMAGE_LUMINANCE;
		m_imageDataType   = IMAGE_DATA_UNSIGNED_BYTE;
		m_colorDepth      = 8;
	}
	else if (channels == 3)
	{
		m_imageDataFormat = IMAGE_RGB;
		m_imageDataType   = IMAGE_DATA_UNSIGNED_BYTE;
		m_colorDepth      = 24;
	}
	else
	{
		m_imageDataFormat = IMAGE_RGBA;
		m_imageDataType   = IMAGE_DATA_UNSIGNED_BYTE;
		m_colorDepth      = 32;
	}

	// normalize so the first pixel is always the upper-left corner
	if ((tgaHeader.imageDesc & TOP_LEFT) == 0)
		FlipVertical();

	return (m_pImageData != NULL);
}

bool TgaLoader::FlipVertical()
{
	if (!m_pImageData)
		return false;

	int bytesPerPixel = m_colorDepth / 8;
	if (bytesPerPixel != 1 && bytesPerPixel != 3 && bytesPerPixel != 4)
		return false;

	int lineWidth = m_width * bytesPerPixel;
	unsigned char* tmpLine = new unsigned char[lineWidth];

	unsigned char* top    = m_pImageData;
	unsigned char* bottom = m_pImageData + (size_t)lineWidth * (m_height - 1);

	for (int i = 0; i < m_height / 2; ++i)
	{
		memcpy(tmpLine, top, lineWidth);
		memcpy(top, bottom, lineWidth);
		memcpy(bottom, tmpLine, lineWidth);

		top    += lineWidth;
		bottom -= lineWidth;
	}

	delete[] tmpLine;
	return true;
}

bool TgaLoader::ConvertRGBToRGBA(unsigned char alphaValue)
{
	if (m_colorDepth != 24 || m_imageDataFormat != IMAGE_RGB)
		return false;

	int pixelCount = m_width * m_height;
	rgba_t* newImage = new rgba_t[pixelCount];

	rgb_t* src = reinterpret_cast<rgb_t*>(m_pImageData);
	for (int i = 0; i < pixelCount; ++i)
	{
		newImage[i].r = src[i].r;
		newImage[i].g = src[i].g;
		newImage[i].b = src[i].b;
		newImage[i].a = alphaValue;
	}

	delete[] m_pImageData;
	m_pImageData = reinterpret_cast<unsigned char*>(newImage);

	m_colorDepth       = 32;
	m_imageDataType    = IMAGE_DATA_UNSIGNED_BYTE;
	m_imageDataFormat  = IMAGE_RGBA;

	return true;
}

bool TgaLoader::ConvertRGBAToRGB()
{
	if (m_colorDepth != 32 || m_imageDataFormat != IMAGE_RGBA)
		return false;

	int pixelCount = m_width * m_height;
	rgb_t* newImage = new rgb_t[pixelCount];

	rgba_t* src = reinterpret_cast<rgba_t*>(m_pImageData);
	for (int i = 0; i < pixelCount; ++i)
	{
		newImage[i].r = src[i].r;
		newImage[i].g = src[i].g;
		newImage[i].b = src[i].b;
	}

	delete[] m_pImageData;
	m_pImageData = reinterpret_cast<unsigned char*>(newImage);

	m_colorDepth       = 24;
	m_imageDataType    = IMAGE_DATA_UNSIGNED_BYTE;
	m_imageDataFormat  = IMAGE_RGB;

	return true;
}
