#pragma once
#include "WcpOglUtility.h"
#include "Materials.h"


// 
// BEGIN OpenGL Specific overrides
//

// A hack using the unused palette entry for bitmaps to allow for high resolution bitmaps
// 1024768 = 1024x768
// 800600 = 800x600
// 0x004f4654 = Apply offset only
// The assumption is the co-ordinates are center relative
enum SCREEN_2D_DETAIL
{
	S2D_LOW,
	S2D_MED,
	S2D_HIGH,
	S2D_DOUBLE
};
// 
// END OpenGL Specific overrides
//


struct ProphMatrix
{
	float data[12];
};

struct InitSettings
{
	int		width;
	int		height;
	int		depth;
	uint8	unknown[16];

	HWND	window;
};

struct InitSupport
{
	// Effects supported
	bool	superDetail;
	bool	colouredLights;
	bool	translucency;
	bool	fog;
	bool	texturedSpace;
	bool	lensFlares;

	// Unknowns
	uint8   unknown[4];

	int		vidAccMem;

	// Communicating to the game how we want colors encoded
	uint8	redFromByteShift;
	uint8	redOffset;
	uint8	greenFromByteShift;
	uint8	greenOffset;
	uint8	blueFromByteShift;
	uint8	blueOffset;
};


enum PolygonLighting
{
	PL_NO_LIGHTING	= (1<<1),	
	PL_FLAT_SHADING	= (1<<3),	
	PL_POLY_ALPHA   = (1<<5),
	PL_POLY_BRIGHTNESS	= (1<<7)
};

// Basic vertex used by quads specifying their location in screen space with a Z
struct Vertex2D
{
	float	x = 0.f;
	float	y = 0.f;
	float	z = 1.f;	
	float	recipZ = 1.f;  

	float	s = 0.f;
	float	t = 0.f;

	float	red = 0.f;
	float	green = 0.f;
	float	blue = 0.f;
	float	alpha = 0.f;
};

struct Vertex3D
{
	int		vertId; // Reference into the positions passed into PushGeometry
	int		normId; // Reference into the normals passed into PushGeometry
	float	s;
	float	t;
	float	alwaysZero;
};

struct Poly2D
{
	TextureMaterial*	material = nullptr;
	Vertex2D*			verts = nullptr;
	int					numberOfVertices = 0;
	int					flags = 0;	// Can have PL_POLY_ALPHA
	int					alpha = 0;	// Used in case of PL_POLY_ALPHA
};

struct Poly3D
{
	TextureMaterial*	material;
	Vertex3D*			verts;
	int					noVerts;
	int					normId;
	int					flags;
	int					alpha;			// Usually junk unless PL_POLY_ALPHA is set
	float				brightness;		// Usually junk unless PL_POLY_BRIGHTNESS is set	
};


enum EFFECT_TYPE
{
	EFFECT_UNKNOWN1,
	EFFECT_LENS_FLARE,
	EFFECT_FOG,
	EFFECT_UNKNOWN2,	
	EFFECT_SCREEN_FLASH,
	EFFECT_UNKNOWN3,
	EFFECT_UNKNOWN4,
	EFFECT_QUERY1,		// No data
	EFFECT_TOGGLE,		// One bool
	EFFECT_QUERY2,		// No data
	EFFECT_HDFMV,			// Custom effect type to support Grims WCP Unlimited HD movies
	// Return true on unknowns (queries?)
};


struct LensFlareFX
{
	int					centerx;
	int					centery;
	int					radius;
	int					colour;
	int					materialToUse;
	TextureMaterial*	mat[4];	
};

struct FogFX
{
	bool	active = false;
	float	density = 0.0f;
	int		red = 0;
	int		green = 0;
	int		blue = 0;
};

struct ScreenFlashFX
{
	int	red;
	int	green;
	int	blue;
	int	alpha;
};


struct ToggleFX
{
	bool  toggle;
};



// Movie info

enum VideoFlags
{
	VF_DOUBLE_Y		= (1<<5),
	VF_INTERLACE_Y	= (1<<6),
	VF_DOUBLE_X		= (1<<7),
	VF_INTERLACE_X	= (1<<8)
};


struct VideoFrame
{
	int brightness;	
	int height;	
	int width;		
	unsigned char* pBackBuffer;	
	unsigned char* pFrontBuffer;	
	int quality;	
	int YUV;	
	int skipMotion;		
	int flags;	
	int posX;	
	int posY;	
	int iRBits;	
	int iGBits;		
	int iBBits;		
	int iROffset;	
	int iGOffset;	
	int iBOffset;		
	int iBitmask;	
	int iNumbits;	
};

enum LightType
{
	LT_AMBIENT = 0,
	LT_DIRECTIONAL,
	LT_LASER = 5,	
};
