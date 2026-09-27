#ifndef TGA_LOADER_H
#define TGA_LOADER_H

#include <cstdio>

// TGA image type codes, as defined by the public Truevision TGA file format spec.
enum TGATypes
{
	TGA_NODATA        = 0,
	TGA_INDEXED       = 1,
	TGA_RGB           = 2,
	TGA_GRAYSCALE     = 3,
	TGA_INDEXED_RLE   = 9,
	TGA_RGB_RLE       = 10,
	TGA_GRAYSCALE_RLE = 11
};

// Image data formats returned by GetImageFormat()
#define IMAGE_RGB       0
#define IMAGE_RGBA      1
#define IMAGE_LUMINANCE 2

// Image data types
#define IMAGE_DATA_UNSIGNED_BYTE 0

// Origin flags found in the TGA image descriptor byte (bits 4-5)
#define BOTTOM_LEFT  0x00
#define BOTTOM_RIGHT 0x10
#define TOP_LEFT     0x20
#define TOP_RIGHT    0x30

#pragma pack(push, 1)
struct tgaheader_t
{
	unsigned char  idLength;
	unsigned char  colorMapType;
	unsigned char  imageTypeCode;
	unsigned char  colorMapSpec[5];
	unsigned short xOrigin;
	unsigned short yOrigin;
	unsigned short width;
	unsigned short height;
	unsigned char  bpp;
	unsigned char  imageDesc;
};
#pragma pack(pop)

struct rgba_t
{
	unsigned char r, g, b, a;
};

struct rgb_t
{
	unsigned char r, g, b;
};

class TgaLoader
{
public:
	TgaLoader();
	virtual ~TgaLoader();

	// loading and unloading
	bool Load(const char *filename);
	bool Load(FILE* file, size_t uSize);
	void Release();

	// flips image vertically
	bool FlipVertical();

	unsigned short GetWidth() { return m_width; }
	unsigned short GetHeight() { return m_height; }
	unsigned char  GetImageFormat() { return m_imageDataFormat; }

	// converts RGB format to RGBA format and vice versa
	bool ConvertRGBAToRGB();
	bool ConvertRGBToRGBA(unsigned char alphaValue);

	// returns the current image data
	unsigned char *GetImage() { return m_pImageData; }

private:
	bool DecodeUncompressed(FILE* pFile, int channels);
	bool DecodeRLE(FILE* pFile, size_t bytesAvailable, int channels);
	void SwapRedBlue();

	unsigned char  m_colorDepth;
	unsigned char  m_imageDataType;
	unsigned char  m_imageDataFormat;
	unsigned char *m_pImageData;
	unsigned short m_width;
	unsigned short m_height;
	unsigned long  m_imageSize;
};

#endif
