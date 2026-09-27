#pragma once
#include "WcpOglUtility.h"
#include "GLBaseClass.h"
#include "Viewport.h"
#include "Matrix44.h"
#include "Vector3.h"
#include "ScreenImage.h"
#include "camera.h"
#include "MaterialGL.h"
#include "Skybox.h"
#include "TreFile.h"
#include "CGShader.h"
#include "FBO.h"
#include "SpecularEffect.h"
#include "TransparentEffect.h"
#include "BloomEffect.h"
#include "BriefingBloom.h"
#include "ScreenBlit.h"
#include <stack>

#define STANDOFF_BUILD 0
#define ORIGIN_OFFSET  1 
#define SUPPORT_HI_RES_BGS (0 || STANDOFF_BUILD)

const int MAX_VSTACK_HEIGHT = 40;

const int MAX_TRANS_VERTS	= 10000;
const int MAX_TRANS_POLYS	= 3000;
const int MAX_SCREEN_POLYS	= 100;
const int MAX_SCREEN_VERTS	= 400;
const int MAX_NUM_POINTS	= 2000;
 

enum NEBULA_TYPES
{
	NEBULA_GREEN,
	NEBULA_STARS,
	NEBULA_NONE
};

enum SCALE_MODE
{
	SCALE_QUALITY,
	SCALE_SPEED,
	SCALE_NONE
};

enum BITMAP_MODE
{
	BITMAP_FULLSCREEN,
	BITMAP_PIECES,
	BITMAP_UNKNOWN,
};


struct DelayedEffects
{
	ScreenFlashFX	flashFX;

	bool			useFlash;
};

struct TransparentVertex
{
	Vector3	worldPos;
	//Vector3	worldNormal; // TODOs
	float		s;
	float		t;
};
 
struct PointVertex
{
	Vector3	worldPos;
	float		alpha;
};

struct LineData
{
	Vector3	start;
	Vector3	end;
	float	camDist;
	int		colour;
};

struct TransparentPoly
{
	MaterialGL*	material;
	float		camSqDist;
	int			flags;
	int			firstVert;
	int			noVerts;
	uint8		alpha;
};

class LightGL;

// Maybe it should support 888?
enum
{
	R_COMBINED_OFFSET = 11,
	G_COMBINED_OFFSET = 5,
	B_COMBINED_OFFSET = 0,

	R_FROM_BYTE_SHIFT  = 3,
	G_FROM_BYTE_SHIFT  = 2,
	B_FROM_BYTE_SHIFT  = 3,
};

class GLOpenGL : public GLBaseClass
{
public:
	GLOpenGL(void);
	virtual ~GLOpenGL(void);

	virtual void	GetGLDetails(GLDetails* details);
	virtual bool	Init(InitSettings* settings, InitSupport* support);
	virtual void	WMActivateCallback(bool bActivated);
	virtual void	SetClearColour(int colour);

	virtual bool	BeginScene(void);
	virtual void	EndScene(int unknown);
	virtual void	SaveScreenshot(char* fileName);
	virtual void	Point(int x, int y, int colour);
	virtual void	Line(int x0, int y0, int x1, int y1, int colour);
	virtual void	Rectangle(int x0, int y0, int x1, int y1, int colour, bool solid);
	virtual void	Circle(int centreX, int centreY, int radius, int colour, bool solid);

	virtual void	DrawBitmap(void* pBitmap, int x, int y);
	virtual void*	CreateBitmap(int width, int height, uint8* pData, void* pPointer, Palette* pal);
	virtual void*	CreateBitmap(int width, int height, uint16* pData, void* pPointer, Palette* pal);
	virtual void	DestroyBitmap(void* bitmapData);
	virtual void	DrawFMVFrame(VideoFrame* frame);
	virtual void*	CreateMaterial(TextureMaterial* material);
	virtual void	DeleteMaterial(void*  mat);
	virtual void	SetCamera(Camera* pCamera);
	virtual void	SetModelMatrix(ProphMatrix* pModelMtx);
	virtual void	SetLights(Light** ppLights, int noLights);
	virtual bool	PushGeometry(Vector3* verts, int noVerts, Vector3* norms, int noNorms);
	virtual void	PopGeometry(void);
	virtual void	Point(const Vector3* pPos, int colour) ;
	virtual void	Line(const Vector3* p0, const Vector3* p1, int colour);
	virtual void	Point(int geomIndex, int colour);
	virtual void	Line(int geomIndex0, int geomIndex1, int colour);
	virtual void	Init3DRendering(bool depthClear, int clipXMin, int clipYMin, int clipXMax, int clipYMax);
	virtual void	Done3DRendering(void);
	virtual void	Polygon(Poly2D* poly, int noPolys);
	virtual void	Polygon(Poly3D* poly, int noPolys);


	virtual bool	Effect(int effectType, void* pEffectData);

private:
	enum RenderState
	{
		RS_POINTS,
		RS_POINTS_2D,
		RS_LINES,
		RS_LINES_2D,
		RS_GEOMETRY,
		RS_TRANSPARENT_GEOMETRY,
		RS_BITMAP,
		RS_PIXELS,
		RS_SCREENPOLY,
		RS_SKYBOX,
		RS_RECT,
		RS_CIRCLE,
		RS_POSTPROCESS,
		RS_NONE
	};

	struct VertexData
	{
		const Vector3*	verts;
		const Vector3*	normals;
		int				noVerts;
		int				noNormals;
	};

	bool			LoadInfo(const char* filename, int &width, int &height, SCALE_MODE &scaleBackground, bool &fullscreen, int &depthBits, int& refreshRate);
	void			InitWindow(HWND window, int width, int height, int depthBits, int stencilBits, bool fullscreen, int refreshRate);
	void			CreateWindowInt(int pixelFormat, bool fullscreen, const PIXELFORMATDESCRIPTOR &pfd, int refreshRate);
	void			DestroyWindowGL();

	// Rendering states, try to set as infrequently as possible!
	void			Set2DRendering(bool textured, ProjectionMode projMode = PM_2D, bool bForce = false, bool bSplat = true);
	void			Set2DRenderingInt(bool textured, ProjectionMode projMode);
	// TODO: Separate view ports for 2D and 3D
	void			Set3DRendering();
	void			Set3DRenderingInt();

	void			SetViewNoWorld();
	void			UpdateWorldView();
	void			SetRenderState(RenderState state, bool bForce = false);
	void			SetRenderStateInt(RenderState state);
	void			EnableTransparency(GLenum source, GLenum dest);
	void			DisableTransparency();

	void			BuildViewPortData(Viewport &viewport);

	// Special effects
	void			DrawLensFlareQuad(LensFlareFX* lensFlareData);
	void			EnableFog(FogFX *fogData);
	void			ScreenFlash(ScreenFlashFX* flash);

	// Batched, ordered rendering of 3D transparent polygons
	void			AddTransparentPoly(const Poly3D* poly);
	void			AddTransparentPoly(const Poly2D* poly);
	void			FillQuad(Vertex2D vertList[4], float left, float right, float top, float bottom, float sMin, float sMax, float tMin, float tMax);
	void			DrawTransparentPolys();
	void			DrawScreenPolys();
	void			DrawPoints();
	void			DrawLines();
	bool			InitMultiSampleInfo(int depthBits, int stencilBits);

	void			SetActiveColour(unsigned int color);
	void			ApplyGlobalLights();
	void			ApplyLocalLights(const Vector3& worldPos);
	void			ApplyShaderLights(const Vector3 &worldPos);
	// Slow, try to avoid calling unnecessarily
	void			SplatCurrentBitmap();
	void			SplatHDR(bool bAllowBriefBloom = true);

	void			DrawDelayedEffects();
	bool			LoadCustomNebulas();
	bool			ShouldDrawScreenImage();
	void			WriteOutScreenshot(const uint8 *pixels, const char *filename, int width, int height);
	ScreenImage*	GetScreenImage(SCREEN_2D_DETAIL detailLevel);
	void			CalculateBinormalAndTangent(const Poly3D* poly, uint32 uIdx0, uint32 uIdx1, uint32 uIdx2, uint32 uPrimaryIdx, Vector3& binormal, Vector3& tangent);

	void	PolygonStandard(Poly3D *poly, const TextureMaterial* texMat, MaterialGL* mat);
	void	PolygonUnlitStandard(Poly3D* poly, const TextureMaterial* texMat, MaterialGL* mat);
	void	PolygonSpecular(Poly3D *poly, const TextureMaterial* texMat, MaterialGL* mat);
	void	PolygonBump(Poly3D *poly, const TextureMaterial* texMat, MaterialGL* mat);
	void	PolygonUnlit(Poly3D *poly, const TextureMaterial* texMat, MaterialGL* mat);

	HWND						m_hwnd = 0;
	int							m_windowWidth = 0;
	int							m_windowHeight = 0;
	float						m_scale = 1.0f;
	float						m_hdrAAScale = 1.f;
	Vector3						m_2dPosOffset;
	Vector3						m_2dPosOffsetHDR;
	HDC							m_hDC = 0;			// The device context
	HGLRC						m_hRC = 0;			// The rendering context
	bool						m_isRunning;
	int							m_fsaaSamples;
	int							m_sourceWidth = 0;
	int							m_sourceHeight = 0;
	float						m_scissorBounds[4];
	bool						m_navThisFrameHack;

	SCREEN_2D_DETAIL			m_screenDetailThisFrame;
	ScreenImage*				m_pScreenImage = nullptr;	// Used for rendering 2D bitmaps to, flushed on 3D calls/ end of scene
	ScreenImage*				m_pMedScreenImage = nullptr;
	ScreenImage*				m_pHiScreenImage = nullptr;
	ScreenImage*				m_pDoubleScreenImage = nullptr;
	TreFile						m_treFile;

	const char*					m_glInfo;

	// Matrices (view matrix not currently used)
	Matrix44					m_viewMatrix;
	Matrix44					m_modelMatrix;
	Matrix44					m_modelViewMatrix;
	Matrix44					m_pointMatrix;
	Vector3						m_eyePos;
	Vector3						m_eyeDir;

	SpecularEffect				m_specularEffect;
	TransparentEffect			m_transparentEffect;
	BriefingBloom				m_briefingBloom;
	BloomEffect					m_bloomEffect;
	ScreenBlit					m_blitEffect;
	
	// Lighting
	Light**						m_lightList;
	int							m_lightCount = 0;
	Colour						m_totalAmbient;
	vector<LightGL*>			m_globalLights;
	vector<LightGL*>			m_localLights;

	// Vertex data
	int							m_vStackHeight;
	VertexData					m_vertexStack[MAX_VSTACK_HEIGHT];
	VertexData*					m_vertData;
	
	// Options
	SCALE_MODE					m_scaleBackground;
	BITMAP_MODE					m_bitmapMode;
	bool						m_starFieldThisFrame;
	bool						m_isGameScene;
	bool						m_useStarbox;
	bool						m_shaderMaterials;
	bool						m_useHDR; 
	bool						m_isFullscreen;
	bool						m_bHDMovieHack = false;
	bool						m_bWidescreenHack = false;
	bool						m_bWidescreenHackActive = false;
	bool						m_bWidescreenHackThisFrame = false;

	// View info   
	ProjectionMode				m_renderMode;
	RenderState					m_renderState;
	bool						m_textured;
	bool						m_zBufferWrite = false;
	const Camera*				m_pCamera;
	Viewport					m_2dViewport;
	Viewport					m_3dViewport;	// Use delayed transparency
	FBO							m_fbo;

	// Transparency
	vector<TransparentVertex>	m_transVerts;
	vector<TransparentPoly>		m_transPolys;
	vector<TransparentVertex>	m_screenVerts;
	vector<TransparentPoly>		m_screenPolys;
	vector<PointVertex>			m_points;
	vector<Texture*>			m_fmvTextures;

	// With the MUP the number of lines is too high for stl
	struct SortEntry
	{
		float dist;
		int   index;
	};

	LineData*					m_pLines = nullptr;
	SortEntry*					m_pLineSortEntries = nullptr;
	int							m_currentLines = 0;

	DelayedEffects				m_delayedEffects;

	bool						m_multiSampleSupported;
	int							m_multiSampleFormat = 0;
	int							m_linesThis3DPass = 0;

	Skybox						m_nebulas[NEBULA_NONE];

	std::string					m_screenshotName;

	FogFX						m_activeFog;
};


inline ScreenImage* GLOpenGL::GetScreenImage(SCREEN_2D_DETAIL detailLevel)
{
	switch(detailLevel)
	{
#if SUPPORT_HI_RES_BGS
	case S2D_LOW:
	default:
		return m_pScreenImage;
	case S2D_MED:
		return m_pMedScreenImage;
	case S2D_HIGH:
		return m_pHiScreenImage;
#endif
	case S2D_DOUBLE:
		return m_pDoubleScreenImage;
	default:
		return m_pScreenImage;
	}
}

inline bool GLOpenGL::ShouldDrawScreenImage()
{
#if SUPPORT_HI_RES_BGS
	return (m_pScreenImage->ShouldDraw() || m_pMedScreenImage->ShouldDraw() || m_pHiScreenImage->ShouldDraw());
#else
	return m_pScreenImage->ShouldDraw() || m_pDoubleScreenImage->ShouldDraw();
#endif
}

inline void GLOpenGL::SplatCurrentBitmap()
{
	if(m_scaleBackground != SCALE_NONE && ShouldDrawScreenImage())
	{
		m_renderMode = PM_ORTHO;
		Set2DRendering(true);
		m_renderState = RS_NONE;
		SetRenderState(RS_BITMAP);
		// Working on assumption higher will always be in the background
#if SUPPORT_HI_RES_BGS
		m_pHiScreenImage->Draw(m_sourceWidth, m_sourceHeight);
		m_pMedScreenImage->Draw(m_sourceWidth, m_sourceHeight);
#endif
		m_pDoubleScreenImage->Draw(m_sourceWidth, m_sourceHeight);
		m_pScreenImage->Draw(m_sourceWidth, m_sourceHeight);
	}
}


inline void GLOpenGL::Set3DRendering()
{
	if (m_renderMode!=PM_PERSPECTIVE)
	{
		Set3DRenderingInt();
	}

	m_renderMode = PM_PERSPECTIVE;
}


inline void GLOpenGL::EnableTransparency(GLenum source, GLenum dest)
{
	glEnable(GL_BLEND);
	glBlendFunc(source, dest);
}


inline void GLOpenGL::DisableTransparency()
{
	glDisable(GL_BLEND);
}

inline void make888(uint32 color565, uint8 &r, uint8 &g, uint8 &b)
{
	r = (uint8)(((color565 >> R_COMBINED_OFFSET) << R_FROM_BYTE_SHIFT) & 0xff);
	g = (uint8)(((color565 >> G_COMBINED_OFFSET) << G_FROM_BYTE_SHIFT) & 0xff);
	b = (uint8)(((color565 >> B_COMBINED_OFFSET) << B_FROM_BYTE_SHIFT) & 0xff);
}


inline void GLOpenGL::Set2DRendering(bool texturing, ProjectionMode projMode, bool bForce, bool bSplat)
{
	if ((m_renderMode == projMode && texturing == m_textured) && !bForce)
		return;

	if(m_isGameScene && bSplat)
		SplatHDR();

	Set2DRenderingInt(texturing, projMode);

	m_textured = texturing;
}

inline void GLOpenGL::SetRenderState(RenderState state, bool bForce)
{
	if(m_renderState==state && !bForce)
	{
		return;
	}

	SetRenderStateInt(state);
	
	m_renderState = state;
}