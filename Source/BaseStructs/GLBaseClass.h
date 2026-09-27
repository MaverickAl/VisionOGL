#pragma once
#include "CommonInputStructs.h"

class Camera;
class Light;


struct GLDetails
{
	const char* name;
	int			val1;
	int			val2;
	int			val3;
};

class GLBaseClass
{
public:
	GLBaseClass() {}
	virtual ~GLBaseClass() {}

	virtual void		GetGLDetails(GLDetails* details) = 0;
	virtual bool		Function02(int unused) { return true; }
	virtual void		Function03() { }
	virtual bool		Init(InitSettings* settings, InitSupport* support) = 0;
	virtual void		WMActivateCallback(bool bActivated) {}
	virtual void		SetClearColour(int colour) = 0;
	virtual void		Function07(int) { }

	// We don't have palette resources
	virtual void*		PaletteCreation(const Palette* pal) { return NULL; }
	virtual void		PaletteCleanup(void* pData) {}
	virtual void*		PaletteCreation2(const Palette* pal) { return NULL; }
	virtual void		PaletteCleanup2(void* pData) {}

	virtual bool		BeginScene() = 0;
	virtual void		EndScene(int unknown) = 0;
	virtual void		SaveScreenshot(char *fileName) = 0;
	virtual bool		Function15() { return true; }
	virtual void		Point(int x, int y, int colour) = 0;
	virtual void		Line(int x0, int y0, int x1, int y1, int colour) = 0;
	virtual void		Rectangle(int x0, int y0, int x1, int y1, int colour, bool solid) = 0;
	virtual void		Circle(int x, int y, int radius, int colour, bool solid) = 0;
	virtual void		DrawBitmap(void* pBitmap, int x, int y) = 0;
	virtual void*		CreateBitmap(int width, int height, uint8* pData, void* pPointer, Palette* pal) = 0;
	virtual void*		CreateBitmap(int width, int height, uint16* pData, void* pPointer, Palette* pal) = 0;
	virtual void		DestroyBitmap(void* bitmapData) = 0;
	virtual void		DrawFMVFrame(VideoFrame* videoFrame) = 0;
	virtual void*		CreateMaterial(TextureMaterial* material) = 0;
	virtual void		DeleteMaterial(void *mat) = 0;
	virtual void		SetCamera(Camera* pCamera) = 0;
	virtual void		SetModelMatrix(ProphMatrix* pModelMtx) = 0;
	virtual void		SetLights(Light **ppLights, int noLights) = 0;
	virtual bool		PushGeometry(Vector3* pVerts, int noVerts, Vector3* pNorms, int noNorms) = 0;
	virtual void		PopGeometry() = 0;
	virtual void		Point(const Vector3* pPos, int colour) = 0;
	virtual void		Line(const Vector3* p0, const Vector3* p1, int colour) = 0;
	// Assumed from line
	virtual void		Point(int geomIndex, int colour) = 0;
	virtual void		Line(int geomIndex0, int geomIndex1, int colour) = 0;
	virtual void		Init3DRendering(bool depthClear, int clipXMin, int clipYMin, int clipXMax, int clipYMax) = 0;
	virtual void		Done3DRendering() = 0;
	virtual void		Polygon(Poly2D* poly, int noPolys) = 0;
	virtual void		Polygon(Poly3D* poly, int noPolys) = 0;
	virtual bool		Effect(int effectType, void *pEffectData) = 0;
};

