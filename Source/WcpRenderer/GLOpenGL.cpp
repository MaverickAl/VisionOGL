#include "GLOpenGL.h"
#include "GLExt.h"
#include "Materials.h"
#include "BitmapDataGL.h"
#include "MaterialGL.h"
#include "TextureMgr.h"
#include "camera.h"
#include "LightGL.h"
#include "Light.h"
#include "VariableTable.h"
#include "CommonInputStructs.h"
#include <fstream>
#include <algorithm>
#include <gl/GLU.h>

const int OPENGL_MAX_LIGHTS = 8;
bool g_bRequirePowerOfTwo = false;
bool g_bForceAutoSpec = false;
bool g_bObeyScreenPolyAlpha = true;
int g_iWindowOffset = 0;
float g_fFogDensityScale = 0.5f;
float g_screenPolyUVInsetTexels = 0.25f;

const int MAX_LINES = 50000;

#if STANDOFF_BUILD
const char* g_szName = "standoff";
#else
const char* g_szName = "WCPOGL";
#endif

#define DEPTH_TEST_LINES 0

GLOpenGL::GLOpenGL(void):
m_isRunning(false),
m_textured(false),
m_pCamera(NULL),
m_vStackHeight(0),
m_renderState(RS_NONE),
m_fsaaSamples(0),
m_starFieldThisFrame(false),
m_isGameScene(false),
m_useStarbox(false),
m_shaderMaterials(false),
m_isFullscreen(false),
m_multiSampleSupported(false),
m_useHDR(false),
m_navThisFrameHack(false)
{
#if !STANDOFF_BUILD
	m_glInfo = "OpenGL [v1.4.1]";
#else
	m_glInfo = "OpenGL Standoff [v1.4.1]";
#endif

	m_transVerts.reserve(MAX_TRANS_VERTS);
	m_transPolys.reserve(MAX_TRANS_POLYS);
	m_screenVerts.reserve(MAX_SCREEN_VERTS);
	m_screenPolys.reserve(MAX_SCREEN_POLYS);
	m_points.reserve(MAX_NUM_POINTS);
	m_delayedEffects.useFlash = false;
	TextureMgr::Init();
}

GLOpenGL::~GLOpenGL(void)
{
	TextureMgr::Cleanup();
	wcpogl::SafeDelete(&m_pScreenImage);
	wcpogl::SafeDelete(&m_pDoubleScreenImage);

#if SUPPORT_HI_RES_BGS
	wcpogl::SafeDelete(&m_pMedScreenImage);
	wcpogl::SafeDelete(&m_pHiScreenImage);
#endif
	wcpogl::DeleteStlVector(m_globalLights);
	wcpogl::DeleteStlVector(m_localLights);
	wcpogl::SafeArrayDelete(&m_pLineSortEntries);
	wcpogl::SafeArrayDelete(&m_pLines);

	wcpogl::DeleteStlVector(m_fmvTextures);


	DestroyWindowGL();
}

void GLOpenGL::GetGLDetails(GLDetails* details)
{
	// Only seems to be used by launcher
	details->name = m_glInfo;
	details->val1 = 2;
	details->val2 = 0;
	details->val3 = 200;
}



#if EDER_TEST_BUILD
float kGameBloomCutOff = 1.0f;
float kGameBloomExposure = 10.0f;
extern float kGaussDeviation;
extern float kGaussMultiplier;
#else
const float kGameBloomCutOff = 0.2f;
const float kGameBloomExposure = 10.0f;
#endif

void ClampToScreenDimensions(int& width, int& height)
{
	const int screenWidth = GetSystemMetrics(SM_CXSCREEN);
	const int screenHeight = GetSystemMetrics(SM_CYSCREEN);

	if (width <= 0 || height <= 0)
	{
		width = screenWidth;
		height = screenHeight;
		return;
	}

	width = min(width, screenWidth);
	height = min(height, screenHeight);
}

bool GLOpenGL::LoadInfo(const char* filename, int &width, int &height, SCALE_MODE &scaling, bool &fullscreen, int &depthBits, int &refreshRate)
{
	bool scale = true;
	bool fastScale = false;
	string treName;

	VariableTable table;
	if(!table.LoadFromFile(filename))
		return false;
	bool result = true;
	bool widescreen = false;
	int anisotropy = 4;
	refreshRate = 0;

	bool bForceNonPowerTwo = false;

	result &= table.GetInt("width", &width);
	result &= table.GetInt("height", &height);
	result &= table.GetBool("scaleBackground", scale);
	result &= table.GetBool("fastScaling", fastScale);
	result &= table.GetBool("fullscreen", fullscreen);
	result &= table.GetBool("shaderMaterials", m_shaderMaterials);
	result &= table.GetInt("maxAnisotropy", &anisotropy);
	result &= table.GetInt("FSAASamples", &m_fsaaSamples);
	result &= table.GetBool("starbox", m_useStarbox);
	result &= table.GetString("treFileName", &treName);
	result &= table.GetInt("depthBits", &depthBits);
	result &= table.GetBool("useHDR", m_useHDR);
	result &= table.GetBool("forceATIHDRFix", bForceNonPowerTwo);
	result &= table.GetBool("forceAutoSpec", g_bForceAutoSpec);
	table.GetOptionalBool("obeyScreenPolyAlpha", g_bObeyScreenPolyAlpha, true);
	table.GetOptionalFloat("fogDensityScale", &g_fFogDensityScale, 0.5f);
	table.GetOptionalFloat("screenPolySafetyOffset", &g_screenPolyUVInsetTexels, 0.25f);
	table.GetOptionalInt("refreshRate", &refreshRate);
	DISPLAY_DEVICE dd;
	dd.cb = sizeof(dd);
	bool isIntel = EnumDisplayDevices(nullptr, 0, &dd, 0) && strstr(dd.DeviceString, "Intel") != nullptr;
	bool isAMD = EnumDisplayDevices(nullptr, 0, &dd, 0) && (strstr(dd.DeviceString, "AMD") != nullptr || strstr(dd.DeviceString, "Radeon") != nullptr || strstr(dd.DeviceString, "ATI ") != nullptr);

	table.GetOptionalInt("windowOffset", &g_iWindowOffset, (isIntel || isAMD) ? 1 : 0);

	g_bRequirePowerOfTwo = bForceNonPowerTwo;

	ClampToScreenDimensions(width, height);

#if EDER_TEST_BUILD
	result = result && table.GetFloat("gaussDeviation", &kGaussDeviation);
	result = result && table.GetFloat("gaussMultiplier", &kGaussMultiplier);
	result = result && table.GetFloat("bloomCutOff", &kGameBloomCutOff);
	result = result && table.GetFloat("exposureLevel", &kGameBloomExposure);
#endif

	Texture::SetMaxAnisotropy(anisotropy);

	if(!m_treFile.Init(treName.c_str()))
	{
		::MessageBox(m_hwnd, "FATAL", "Could not open TRE FILE", 0);
	}

	char szExtraTRE[256];
	for (int i = 0; i < 10; i++)
	{
		string exTREName;

		sprintf_s(szExtraTRE, sizeof(szExtraTRE), "treFileNameEX%d", i);
		if (table.GetOptionalString(szExtraTRE, &exTREName))
		{
			if (!m_treFile.Init(exTREName.c_str()))
			{
				::MessageBox(m_hwnd, "FATAL", "Could not open EX TRE FILE", 0);
			}
		}
		else
		{
			break;
		}
	}

	if(scale)
	{
		scaling = fastScale ? SCALE_SPEED : SCALE_QUALITY;
	}
	else
	{
		scaling = SCALE_NONE;
	}
	return result;
}

void GLOpenGL::WMActivateCallback(bool bActivated)
{

}


bool GLOpenGL::Init(InitSettings* settings, InitSupport* support)
{
	m_bitmapMode = BITMAP_UNKNOWN;
	m_sourceWidth = settings->width;
	m_sourceHeight = settings->height;

	//m_bWidescreenHack = true;	// Check the settings

	int width = 1024;
	int height = 768;
	int depthBits = 24;
	int refreshRate = 0;
	m_hdrAAScale = 1.f;
	bool fullScreen;
	m_scaleBackground = SCALE_QUALITY;
#if STANDOFF_BUILD
	bool result = LoadInfo("STAN_OGL.cfg", width, height, m_scaleBackground, fullScreen, depthBits, refreshRate);
#else
	bool result = LoadInfo("GLOpenGL.cfg", width, height, m_scaleBackground, fullScreen, depthBits, refreshRate);
#endif

	InitWindow(settings->window, width, height, depthBits, 0, fullScreen, refreshRate);

	if(m_scaleBackground == SCALE_SPEED)
	{
		glPixelZoom( m_scale, -m_scale );
	}
	else if(m_scaleBackground == SCALE_NONE)
	{
		glPixelZoom( 1.0f, -1.0f );
	}

	width    = width;
	height   = height;

	support->redFromByteShift	= R_FROM_BYTE_SHIFT;
	support->redOffset			= R_COMBINED_OFFSET;
	support->greenFromByteShift	= G_FROM_BYTE_SHIFT;
	support->greenOffset		= G_COMBINED_OFFSET;
	support->blueFromByteShift	= B_FROM_BYTE_SHIFT;
	support->blueOffset			= B_COMBINED_OFFSET;

	support->superDetail		= true;	// Doesn't seem to disable anything, just set the default
	support->colouredLights		= true;
	support->translucency		= true;
	support->fog				= true;
	support->texturedSpace		= true;
	support->lensFlares			= true;
	
	support->vidAccMem			= 512*1024*1024;	// Claim we have 512MB of vid memory, shouldn't be a lie

	g_bRequirePowerOfTwo |= !GLExt::ARB_texture_non_power_of_two;

	m_isRunning = true;

	int error = glGetError();
	assert(error == GL_NO_ERROR);


	glShadeModel(GL_SMOOTH);
	glEnable(GL_DEPTH_TEST);

	glEnable(GL_LIGHTING);
	float globalAmbient[] = {0.0f, 0.0f, 0.0f, 0.0f};
	glLightModelfv(GL_LIGHT_MODEL_AMBIENT, globalAmbient);
	//glClearColor(1.0f, 0.0f, 0.0f, 0.0f);
	// Set default material states
	Colour white( 1.0f, 1.0f, 1.0f );
	Colour black( 0.0f, 0.0f, 0.0f );
	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, white.rgba() );
	glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, white.rgba() );
	glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, black.rgba() );
	glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, black.rgba() );
	glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 0.0f );

	assert(glGetError() == GL_NO_ERROR);

	// Set point size based on window size
	glLineWidth(m_scale*m_hdrAAScale);
	glEnable(GL_POINT_SMOOTH);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glEnable(GL_LINE_SMOOTH);
	glFrontFace(GL_CCW);
	glCullFace(GL_FRONT);
	//glEnable(GL_NORMALIZE);

	m_2dViewport.InitViewport(0, 0, m_windowWidth, m_windowHeight);
	m_2dViewport.ApplyViewport();

	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	glOrtho(0.0f, m_windowWidth, m_windowHeight, 0.0f, -1.0f, 1.0f);
	glGetFloatv(GL_PROJECTION_MATRIX, m_2dViewport.m_proj2D.ColumnMajor());
	glPopMatrix();

	assert(glGetError() == GL_NO_ERROR);

	m_renderMode = PM_ORTHO;
	m_renderState = RS_NONE;
	m_zBufferWrite = false;

	float aspectRatio = (float)m_sourceWidth/(float)m_sourceHeight;
	m_pScreenImage = new ScreenImage(480.0f*aspectRatio, 480.f, g_bRequirePowerOfTwo);
	m_pDoubleScreenImage = new ScreenImage(960.0f * aspectRatio, 960.0f, g_bRequirePowerOfTwo);

#if SUPPORT_HI_RES_BGS
	m_pMedScreenImage = new ScreenImage(600.0f*aspectRatio, 600.f, g_bRequirePowerOfTwo);
	m_pHiScreenImage = new ScreenImage(768.0f*aspectRatio, 768.f, g_bRequirePowerOfTwo);
#endif

	m_currentLines = 0;
	m_pLines = new LineData[MAX_LINES];
	m_pLineSortEntries = new SortEntry[MAX_LINES];

	assert(glGetError() == GL_NO_ERROR);

	CGShader::InitCG();

	assert(glGetError() == GL_NO_ERROR);

	if(m_shaderMaterials)
	{
		if(!m_specularEffect.Init(&m_treFile) || !m_transparentEffect.Init(&m_treFile) )
		{
			m_shaderMaterials = false;
			assertMsg(false, "OpenGL - EnableSpecular Failed");
		}
	}

	if(!m_shaderMaterials)
		m_useHDR = false;

	if(m_useHDR)
	{
		if(m_bloomEffect.Init(&m_treFile, m_windowWidth, m_windowHeight, m_hdrAAScale)
			&& m_briefingBloom.Init(&m_treFile, m_windowWidth, m_windowHeight, m_hdrAAScale)
			&& m_blitEffect.Init(&m_treFile) )
		{
			if( !m_fbo.Create(true, true, true, m_windowWidth, m_windowHeight, m_fsaaSamples, depthBits) )
			{
				m_useHDR = false;
				assertMsg(false, "OpenGL - Create Floating Point FBO Failed");
			}
		}
		else
		{
			assertMsg(false, "OpenGL - InitBloomEffects Failed");
			m_useHDR = false;
		}
	}

	Set2DRendering(true);

	assert(glGetError() == GL_NO_ERROR);


	return m_isRunning;
}


void GLOpenGL::InitWindow(HWND window, int width, int height, int depthBits, int stencilBits, bool fullscreen, int refreshRate)
{
	m_windowHeight	= height;
	m_windowWidth	= width;
	m_isFullscreen = fullscreen;

	// NVidia only supports 32bit on FBOs so we make our default depth buffer 24 bit
	if (depthBits > 24)
		depthBits = 24;
	
	
	float aspect		= (float)width/(float)height;
	float sourceAspect	= (float)m_sourceWidth/float(m_sourceHeight);
	if(m_scaleBackground != SCALE_NONE)
	{
		if(aspect>sourceAspect)
		{
			m_scale	= (float)m_windowHeight / m_sourceHeight;	
			m_2dPosOffset.x() = ((float)m_windowWidth-(m_sourceWidth * m_scale))/2.0f;
			m_2dPosOffset.y() = 0.0f;
		}
		else
		{
			m_scale = (float)m_windowWidth / m_sourceWidth;
			m_2dPosOffset.x() = 0.0f;			
			m_2dPosOffset.y() = ((float)m_windowHeight-(m_sourceHeight * m_scale))/2.0f;
		}
	}
	else
	{
		m_scale = 1.0f;
		m_2dPosOffset.SetUp(0.0f, 0.0f, 0.0f);
	}

	m_2dPosOffsetHDR = m_2dPosOffset*m_hdrAAScale;

	m_hwnd			= window;

	m_hDC = GetDC(window);	// Get the device context for passed window

	// Set up the pixel format for the window
	int pixelFormat;
	
	PIXELFORMATDESCRIPTOR pfd =
	{   
		sizeof(PIXELFORMATDESCRIPTOR),  // size
		1,                          // version
		PFD_SUPPORT_OPENGL |        // OpenGL window
		PFD_DRAW_TO_WINDOW |        // render to window
		PFD_DOUBLEBUFFER |           // support double-buffering
		PFD_SUPPORT_COMPOSITION,	// non exclusive
		PFD_TYPE_RGBA,              // color type
		(BYTE)24,                         // preferred color depth (RGB colour bits, excludes alpha)
		(BYTE)0, (BYTE)0, (BYTE)0, (BYTE)0, (BYTE)0, (BYTE)0,           // color bits (ignored)
		(BYTE)0,                          // no alpha buffer
		(BYTE)0,                          // alpha bits (ignored)
		(BYTE)0,                          // no accumulation buffer
		(BYTE)0, 0, 0, 0,                 // accum bits (ignored)
		(BYTE)depthBits,                  // depth buffer
		(BYTE)stencilBits,                // no stencil buffer
		(BYTE)0,                          // no auxiliary buffers
		(BYTE)PFD_MAIN_PLANE,             // main layer
		(BYTE)0,                          // reserved
		(BYTE)0, (BYTE)0, (BYTE)0,                    // no layer, visible, damage masks
	};

	if (m_fsaaSamples > 1 && InitMultiSampleInfo(depthBits, stencilBits))
	{
		CreateWindowInt(m_multiSampleFormat, m_isFullscreen, pfd, refreshRate);
	}
	else
	{
		pixelFormat = ChoosePixelFormat(m_hDC, &pfd);
		CreateWindowInt(pixelFormat, fullscreen, pfd, refreshRate);
	}

	#if 0
	// Old 1.4 settings
	if (fullscreen)
	{
		SetWindowLong(m_hwnd, GWL_STYLE, WS_POPUP);
		SetWindowLong(m_hwnd, GWL_EXSTYLE, WS_EX_APPWINDOW);

		SetWindowPos(m_hwnd, HWND_NOTOPMOST, 0, 0,
			m_windowWidth,
			m_windowHeight,
			SWP_SHOWWINDOW);
	}
	else
	{
		// Non-fullscreen setup: borderless windowed
		SetWindowLong(m_hwnd, GWL_STYLE, WS_POPUP);             // no border/caption
		SetWindowLong(m_hwnd, GWL_EXSTYLE, WS_EX_APPWINDOW);    // show in taskbar

		SetWindowPos(m_hwnd, HWND_TOP,
			0, 0,
			m_windowWidth, m_windowHeight,
			SWP_SHOWWINDOW | SWP_FRAMECHANGED);
	}
	#elif 0

	// Set up the window
	if (fullscreen)
	{
		SetWindowLong(m_hwnd, GWL_STYLE, 0);

		SetWindowPos(m_hwnd, HWND_NOTOPMOST, 0, 0,
			m_windowWidth + g_iWindowOffset,
			m_windowHeight,
			SWP_NOREDRAW);
	}
	else
	{
		SetWindowLong(m_hwnd, GWL_STYLE, 0);
		SetWindowPos(m_hwnd, HWND_NOTOPMOST, 0, 0, m_windowWidth + g_iWindowOffset,
			m_windowHeight,
			SWP_SHOWWINDOW | SWP_NOMOVE);
	}
#else

	// Set up the window
	if (fullscreen)
	{
		SetWindowLong(m_hwnd, GWL_STYLE, 0);

		SetWindowPos(m_hwnd, HWND_NOTOPMOST, 0, 0,
			m_windowWidth + g_iWindowOffset,
			m_windowHeight,
			SWP_NOREDRAW);
	}
	else
	{
		DWORD style = WS_CLIPSIBLINGS | WS_CLIPCHILDREN;
		RECT rc = { 0, 0, m_windowWidth + g_iWindowOffset, m_windowHeight };
		AdjustWindowRectEx(&rc, style, FALSE, WS_EX_APPWINDOW);

		SetWindowLong(m_hwnd, GWL_STYLE, style);
		SetWindowLong(m_hwnd, GWL_EXSTYLE, WS_EX_APPWINDOW);
		SetWindowPos(m_hwnd, HWND_TOP, 0, 0, rc.right - rc.left,
			rc.bottom - rc.top,
			SWP_SHOWWINDOW | SWP_NOMOVE | SWP_FRAMECHANGED);
	}
#endif


	UpdateWindow(m_hwnd);
	ShowWindow(m_hwnd, SW_SHOWNORMAL);
	SetForegroundWindow(m_hwnd);
}


bool GLOpenGL::InitMultiSampleInfo(int depthBits, int stencilBits)
{
	if (m_fsaaSamples <= 1)
	{
		return false;
	}

	WNDCLASS      wc;
	int           result;

	wc.style = CS_OWNDC;
	wc.lpfnWndProc = DefWindowProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = nullptr;
	wc.hIcon = nullptr;
	wc.hCursor = NULL;
	wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
	wc.lpszMenuName = NULL;
	wc.lpszClassName = "MultiSampleTest";

	result = RegisterClass(&wc);

	// Temporary window
	HWND hwnd = CreateWindowEx(WS_EX_APPWINDOW,
		"MultiSampleTest",
		"Prophecy",
		WS_POPUP,
		0, 0,
		1,
		1,
		HWND_DESKTOP,
		0,
		NULL,
		NULL);

	int error = GetLastError();
	HDC hDC = GetDC(hwnd);
	error = GetLastError();

	int pixelFormat;
	PIXELFORMATDESCRIPTOR pfd =
	{
		sizeof(PIXELFORMATDESCRIPTOR),  // size
		1,                          // version
		PFD_SUPPORT_OPENGL |        // OpenGL window
		PFD_DRAW_TO_WINDOW |        // render to window
		PFD_DOUBLEBUFFER,           // support double-buffering
		PFD_TYPE_RGBA,              // color type
		24,                         // preferred color depth (RGB colour bits, excludes alpha)
		0, 0, 0, 0, 0, 0,           // color bits (ignored)
		0,                          // no alpha buffer
		0,							// alpha bits (ignored)
		0,                          // no accumulation buffer
		0, 0, 0, 0,                 // accum bits (ignored)
		(BYTE)depthBits,			// depth buffer
		(BYTE)stencilBits,          // no stencil buffer
		0,                          // no auxiliary buffers
		PFD_MAIN_PLANE,             // main layer
		0,                          // reserved
		0, 0, 0,                    // no layer, visible, damage masks
	};

	pixelFormat = ChoosePixelFormat(hDC, &pfd);
	if (SetPixelFormat(hDC, pixelFormat, &pfd) == false)
	{
		ReleaseDC(hwnd, hDC);
		DestroyWindow(hwnd);
		return false;
	}
	HGLRC hRC = wglCreateContext(hDC);
	error = GetLastError();
	wglMakeCurrent(hDC, hRC);
	error = GetLastError();

	HGLRC current = wglGetCurrentContext();

	GLExt::Init();


	int		valid;
	UINT	numFormats;
	float	fAttributes[] = { 0,0 };

	int iAttributes[] =
	{
		WGL_DRAW_TO_WINDOW_ARB,GL_TRUE,
		WGL_SUPPORT_OPENGL_ARB,GL_TRUE,
		WGL_DOUBLE_BUFFER_ARB,GL_TRUE,			
		WGL_PIXEL_TYPE_ARB,WGL_TYPE_RGBA_ARB,
		WGL_ACCELERATION_ARB,WGL_FULL_ACCELERATION_ARB,
		WGL_COLOR_BITS_ARB,24,
		WGL_ALPHA_BITS_ARB,0,
		WGL_DEPTH_BITS_ARB,depthBits,
		WGL_STENCIL_BITS_ARB,0,
		WGL_SAMPLE_BUFFERS_ARB,GL_TRUE,
		WGL_SAMPLES_ARB,m_fsaaSamples,
		0,0
	};

	int samplesValueIndex = -1;
	for (int i = 0; iAttributes[i] != 0; i += 2)
	{
		if (iAttributes[i] == WGL_SAMPLES_ARB)
		{
			samplesValueIndex = i + 1;
			break;
		}
	}

	error = GetLastError();
	valid = wglChoosePixelFormatARB(m_hDC, iAttributes, fAttributes, 1, &pixelFormat, &numFormats);
	error = GetLastError();

	// Check if there were any valid pixel formats
	if (valid && numFormats >= 1)
	{
		m_multiSampleSupported = true;
		m_multiSampleFormat = pixelFormat;
	}

	for (int samples = m_fsaaSamples - 1; samples >= 2 && !m_multiSampleSupported; samples--)
	{
		// Try again with fewer samples
		if (samplesValueIndex >= 0)
		{
			iAttributes[samplesValueIndex] = samples;
		}
		valid = wglChoosePixelFormatARB(m_hDC, iAttributes, fAttributes, 1, &pixelFormat, &numFormats);
		if (valid && numFormats >= 1)
		{
			m_multiSampleSupported = true;
			m_multiSampleFormat = pixelFormat;
		}
	}

	wglMakeCurrent(NULL, NULL);
	wglDeleteContext(hRC);
	ReleaseDC(hwnd, hDC);
	DestroyWindow(hwnd);
	UnregisterClass("MultiSampleTest", NULL);

	return  m_multiSampleSupported;
}


void GLOpenGL::DestroyWindowGL()
{
	if (m_hwnd != 0)												
	{	
		if (m_hDC != 0)	
		{
			wglMakeCurrent (0, 0);
			ReleaseDC (m_hwnd, m_hDC);
			m_hDC = 0;
			if (m_hRC != 0)
			{
				wglDeleteContext (m_hRC);
				m_hRC = 0;
			}
		}
	}

	if (m_isFullscreen)
	{
		ChangeDisplaySettings(NULL,0);
		ShowCursor (TRUE);
	}
}


void GLOpenGL::CreateWindowInt(int pixelFormat, bool fullscreen, const PIXELFORMATDESCRIPTOR &pfd, int refreshRate)
{
	SetPixelFormat(m_hDC, pixelFormat, &pfd);

	if(fullscreen)
	{
		DEVMODE		checkMode;
		int			mode = 0;
		bool		foundRes;

		while (mode++)
		{
			if (!EnumDisplaySettings(NULL, mode, &checkMode))
			{
				// We either didn't find a matching resolution or a referesh rate. Take the defaults of refresh and optionally size
				if (!foundRes)
				{
					m_windowWidth = GetSystemMetrics(SM_CXSCREEN);
					m_windowHeight = GetSystemMetrics(SM_CYSCREEN);
				}
				refreshRate = 0;
				break;
			}
			if ((int)checkMode.dmPelsWidth == m_windowWidth	&& (int)checkMode.dmPelsHeight == m_windowHeight && checkMode.dmBitsPerPel == 32)
			{
				foundRes = true;

				if (refreshRate == 0 || checkMode.dmDisplayFrequency == refreshRate) {
					break;
				}
			}
		}


		DEVMODE dm;
		memset(&dm, 0, sizeof(DEVMODE));
		dm.dmSize = sizeof(DEVMODE);
		dm.dmBitsPerPel = 32;
		dm.dmPelsWidth = m_windowWidth;
		dm.dmPelsHeight = m_windowHeight;
		dm.dmDisplayFrequency = refreshRate;
		dm.dmFields = DM_PELSWIDTH | DM_PELSHEIGHT | DM_BITSPERPEL;
		if (refreshRate != 0)
		{
			dm.dmFields |= DM_DISPLAYFREQUENCY;
		}

		if( ChangeDisplaySettings(&dm, CDS_FULLSCREEN) != DISP_CHANGE_SUCCESSFUL )
		{
			// TODO: Error
		}
	}

	m_hRC = wglCreateContext(m_hDC);
	wglMakeCurrent(m_hDC, m_hRC);

	GLExt::Init();

	wglSwapIntervalEXT(0);			// Vsync, 0 disable, 1 enable. We never want to enable as game has its own frame limit

}


void GLOpenGL::SetClearColour(int colour)
{
	Colour clearCol;
	clearCol.Assign565(colour);
	glClearColor(clearCol.rgba()[0], clearCol.rgba()[1], clearCol.rgba()[2], 0.0f);
}


bool GLOpenGL::BeginScene(void)
{
	assert(glGetError() == GL_NO_ERROR); 
	 
	memset(m_scissorBounds, 0, sizeof(float)*4);
	m_navThisFrameHack = false;
	glDisable(GL_SCISSOR_TEST);
	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
	if(m_useHDR)
	{
		m_fbo.Bind();
		glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
		m_fbo.Detach();
	}
	glEnable(GL_SCISSOR_TEST);
	m_modelMatrix.LoadIdentity();
	m_viewMatrix.LoadIdentity();
	m_renderMode = PM_ORTHO;
	//UpdateWorldView();
	Set2DRendering(true);
	m_starFieldThisFrame = false;
	m_isGameScene = false;
	m_bitmapMode = BITMAP_UNKNOWN;
	m_bWidescreenHackThisFrame = false;

	assert(glGetError() == GL_NO_ERROR);


	return true;
}

void GLOpenGL::EndScene(int unknown)
{
	assert(glGetError() == GL_NO_ERROR);

	DrawLines();

	//if(m_isGameScene)
		SplatHDR();

	DrawScreenPolys();
	//SplatHDR();
	SplatCurrentBitmap();
	DrawDelayedEffects();

	assert(glGetError() == GL_NO_ERROR);

	if(!m_screenshotName.empty())
	{
		uint8* rgb = new uint8[m_windowWidth * m_windowHeight*3];
		glReadPixels(0, 0, m_windowWidth, m_windowHeight, GL_RGB, GL_UNSIGNED_BYTE, rgb);
		WriteOutScreenshot(rgb, m_screenshotName.c_str(), m_windowWidth, m_windowHeight);
		delete [] rgb;
		m_screenshotName = "";
	}

	assert(glGetError() == GL_NO_ERROR);

	m_pCamera = NULL;
	SwapBuffers(m_hDC);

	assert(glGetError() == GL_NO_ERROR);

	static int frameCount = 0;
	static bool bLoaded = false;
	if (frameCount > 0 && ! bLoaded)
	{
#if !STANDOFF_BUILD
		TextureMgr::Inst()->PreLoadAllTexturesInDirectory("overridemaps");
		assert(glGetError() == GL_NO_ERROR);
		TextureMgr::Inst()->PreLoadAllTexturesInDirectory("normalmaps");
		TextureMgr::Inst()->PreLoadAllTexturesInDirectory("specmaps");
		TextureMgr::Inst()->PreLoadAllTexturesInDirectory("emissivemaps");
		TextureMgr::Inst()->PreLoadAllTexturesInDirectory("lookupmaps");
		TextureMgr::Inst()->PreLoadAllTexturesInDirectory("shaders");
		TextureMgr::Inst()->PreLoadAllTexturesInDirectory("iridescencemaps");
#endif
		TextureMgr::Inst()->PreloadAllTexturesInTre(&m_treFile);
		
		assert(glGetError() == GL_NO_ERROR);

		if (m_useStarbox)
		{
			m_useStarbox = LoadCustomNebulas();
		}

		assert(glGetError() == GL_NO_ERROR);

		bLoaded = true;
	}

	assert(glGetError() == GL_NO_ERROR);

	frameCount++;
}

void GLOpenGL::SaveScreenshot(char* fileName)
{
	m_screenshotName = fileName;
}


void GLOpenGL::Circle(int centerX, int centerY, int radius, int color, bool solid)
{
	if (radius == 0)
	{
		Point(centerX, centerY, color);
	}
	else
	{
		m_renderState = RS_NONE;
		Set2DRendering(false);
		SetRenderState(RS_LINES_2D);
		glPushMatrix();
		glTranslatef(centerX, centerY, 0.0f);
		SetActiveColour(color);

		if (!solid)
			glBegin(GL_LINE_STRIP);
		else
			glBegin(GL_POLYGON);

		const int noSides = 50;
		const float piOverSides = (float)wcpogl::pi/(float)noSides;
		for (int vertex = 0; vertex < noSides; vertex++)
		{
			float angle = (float)vertex * 2.0f * piOverSides;
			glVertex3f(cosf(angle)*radius, sinf(angle)*radius, 0.0f);
		}

		if (!solid)
			glVertex3f(radius, 0.0, 0.0);

		glEnd();

		glPopMatrix();
	}
}


void GLOpenGL::Point(int x, int y, int color)
{
	m_isGameScene = !m_starFieldThisFrame;

	if (m_useStarbox)
		return;

	if (m_useHDR && m_isGameScene && !m_fbo.IsAttached())
	{
		m_fbo.Bind();
	}

	Set2DRendering(false, PM_2D, false, false);
	SetRenderState(RS_POINTS_2D);

	SetActiveColour(color);
	glBegin(GL_POINTS);
	glVertex2i(x, y);
	glEnd();
}

void GLOpenGL::Line(int x0, int y0, int x1, int y1, int color)
{
	// TODO: Nasty hack for scaled backgrounds
	SplatCurrentBitmap();
	
	Set2DRendering(false, PM_2D_WORLD);
	SetRenderState(RS_LINES_2D);
	SetActiveColour(color);
	glBegin(GL_LINES);
	glVertex2i(x0, y0);
	glVertex2i(x1, y1);
	glEnd();

}

void GLOpenGL::SetActiveColour(unsigned int color)
{
	uint8 r, g, b;
	make888(color, r, g, b);
	glColor4ub(r, g, b, 255);
	//glColor3f(r/255.f, g/255.f, b/255.f);
}

void SetActiveColourHack(unsigned int color, unsigned int minValHack)
{
	uint8 r, g, b;
	make888(color, r, g, b);
	r = max(minValHack, r);
	g = max(minValHack, g);
	b = max(minValHack, b);
	glColor4ub(r, g, b, 255);
	//glColor3f(r/255.f, g/255.f, b/255.f);
}


void GLOpenGL::Rectangle(int x0, int y0, int x1, int y1, int color, bool solid)
{
	if(solid)
	{
		SplatCurrentBitmap();
		DrawScreenPolys();
		DrawLines();
	}
	Set2DRendering(false);
	SetRenderState(RS_RECT);
	SetActiveColour(color);

	solid |= ((x1 - x0 <= 1) || (y1 - y0 <= 1));

	float x0f = (float)x0;
	float y0f = (float)y0;
	float x1f = (float)x1;
	float y1f = (float)y1;

	if (solid)
	{
		// Offset slightly so something will always draw
		float nudge = 0.5f / m_scale;
		x0f -= nudge; y0f -= nudge;
		x1f += nudge; y1f += nudge;
	}

	if (!solid)
	{
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	}

	glBegin(GL_QUADS);
	glVertex2f(x0f, y0f); // bottom-left
	glVertex2f(x1f, y0f); // bottom-right
	glVertex2f(x1f, y1f); // top-right
	glVertex2f(x0f, y1f); // top-left
	glEnd();

	if (!solid)
	{
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	}
}

void GLOpenGL::DrawBitmap(void* pBitmap, int x, int y)
{
	DrawScreenPolys();
	DrawLines();

	const BitmapDataGL* bmogl = (BitmapDataGL*)(pBitmap);

	// Massive hack due to how lines get sent through, 45 line grid, then text, the main polys, then background
	// but sometimes there's no text. We don't want to bloom until we're done.
	if (!m_isGameScene)
		SplatHDR(m_linesThis3DPass != 45 || bmogl->GetWidth() >= 640.f);
	
	if(m_bitmapMode != BITMAP_FULLSCREEN)
	{
		m_bitmapMode = bmogl->GetWidth() >= 130 ? BITMAP_FULLSCREEN : BITMAP_PIECES;
	}

	if(bmogl->ShouldOffset())
	{
		float offset = ((float)m_sourceWidth-640.0f)/2.f;
		SCREEN_2D_DETAIL detail = bmogl->GetTargetDetail();
		switch(detail)
		{
		case S2D_DOUBLE:
			offset *= 1280.f/640.f;
		case S2D_HIGH:
			offset*= 1024.0f/640.0f;
			break;
		case S2D_MED:
			offset*= 800.0f/640.0f;
		case S2D_LOW:
		default:
			break;
		}
		x+= offset;
	}

	if(m_scaleBackground == SCALE_QUALITY)
	{
		if( m_bitmapMode == BITMAP_FULLSCREEN /*&& !m_starFieldThisFrame*/)
		{
			GetScreenImage(bmogl->GetTargetDetail())->UpdateImage(bmogl, x, y);
		}
		else
		{
			DrawDelayedEffects();
			Set2DRendering(true);
			SetRenderState(RS_BITMAP);
			bmogl->DrawSmooth(x, y);
		}
	}
	else
	{
		Set2DRendering(true);
		SetRenderState(RS_PIXELS);
		bmogl->Draw(x, y);
	}
}

void*	GLOpenGL::CreateBitmap(int width, int height, uint8* pData, void* pPointer, Palette* pal)
{
	BitmapDataGL* ogl = new BitmapDataGL(width, height, pData, pal);
	if(m_scaleBackground == SCALE_QUALITY) 
	{
		ogl->GenTex(g_bRequirePowerOfTwo);
	}
	return ogl;
}

void* GLOpenGL::CreateBitmap(int width, int height, uint16* pData, void* pPointer, Palette* pal)
{
	BitmapDataGL* ogl = new BitmapDataGL(width, height, pData, pal);
	if(m_scaleBackground == SCALE_QUALITY)
	{
		ogl->GenTex(g_bRequirePowerOfTwo);
	}
	return ogl;
}

void GLOpenGL::DestroyBitmap(void* bitmapData)
{
	delete (BitmapDataGL*)bitmapData;
}
 
void GLOpenGL::DrawFMVFrame(VideoFrame* frame)
{
	// What the hell is with the empty images?
	if(!(frame->width && frame->height) )
	{
		return;
	} 
	
	SplatCurrentBitmap();

	int width = frame->width;
	int height = frame->height;
	 
	int posX = 0, posY = 0;
	int lineOffset = 0;
	float scale = 1.0f;
	m_renderMode = PM_ORTHO;
	m_renderState = RS_NONE;
	Set2DRendering(true);
	 
	bool bFullScreen = false;
	float fAspectMul = 1.0f;
	if( width >= 300 )
	{
		float fAspect = (float)width / (float)height;
		#if 0
		if (fAspect < 16.f/10.f && m_bHDMovieHack && frame->iNumbits > 16)
		{
			int oldHeight = height;
			fAspectMul = (4.f / 3.f) / fAspect;
			height = (width*fAspectMul) / (16.f/9.f);
			lineOffset = (oldHeight-height)/2;
		}
		#endif
		bFullScreen = true;
		// Ok we're assuming it's full screen video then, also assuming it's wider than
		// our monitor, a lot of assumptions, fix me
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		fAspect = (float)width / (float)height;
		fAspect *= fAspectMul;
		float fScreenAspect = (float)m_windowWidth / (float)m_windowHeight;

		scale = (float)m_windowWidth / width;
		 
		if (fScreenAspect > fAspect)
		{
			float fAdj = (fAspect / fScreenAspect);
			scale *= fAdj;
			posX = (m_windowWidth-((scale * width)))*0.5f;
		}

		posY = ((float)m_windowHeight - (height * scale)) / 2.f;
	}
	else if( !(frame->flags & (VF_DOUBLE_Y | VF_INTERLACE_Y | VF_DOUBLE_X | VF_INTERLACE_X)) )
	{
		posX = frame->posX-1;
		posY = frame->posY-1;
	} 
	else
	{
		// Non full screen video
		posX = frame->posX;
		posY = frame->posY;
		scale = 2.0f;
	}

	// HACK: For some odd reason main movies seem to be aligned to 32bit and ingame to 16bit
	// using what I know about the flags I'm hacking this :(
	bool useShort = !(frame->flags & (VF_DOUBLE_Y | VF_INTERLACE_Y | VF_DOUBLE_X | VF_INTERLACE_X));

	// Fill the texture buffer
	int texWidth = width;
	int texHeight = height;
	if (g_bRequirePowerOfTwo)
	{
		texWidth = wcpogl::FindNextPowerOfTwo(width);
		texHeight = wcpogl::FindNextPowerOfTwo(height);
	}
	int imageSize = texWidth*texHeight*3;
	uint16* srcShort = (uint16*)(frame->pFrontBuffer);
	uint32* src = (uint32*)(frame->pFrontBuffer);


	float texCoordX = (float)(width-1.f)/(float)texWidth;
	float texCoordY = (float)(height-1.f)/(float)texHeight;
	float quadWidth = scale*width;
	float quadHeight = scale*height;

	uint8* dest = new uint8[imageSize];
	uint8*	pDest = dest;
	int		unusedPerLine = 3*(texWidth-width);
	bool	bHDColor = false;
	if (m_bHDMovieHack)
	{
		// DO NOT CHECK THIS UNLESS HACK IS TRUE
		bHDColor = frame->iNumbits > 16;
		if (bHDColor)
		{
			useShort = false;
		}
	}

	if( useShort)
	{
		srcShort += (width*lineOffset);
		for(int y=0; y<height; y++)
		{
			for(int x=0; x<width; x++)
			{
				make888(*(srcShort), *pDest, *(pDest+1), *(pDest+2));
				pDest+=3;
				srcShort++;
			}
			pDest += unusedPerLine;
		}
	}
	else
	{
		src += (width * lineOffset);
		for(int y=0; y<height; y++)
		{
			for(int x=0; x<width; x++)
			{
				if (bHDColor)
				{
					uint8* pSrcRGBA = (uint8*)src;
					pDest[0] = pSrcRGBA[0];
					pDest[1] = pSrcRGBA[1];
					pDest[2] = pSrcRGBA[2];
				}
				else
				{
					make888(*(src), *pDest, *(pDest + 1), *(pDest + 2));
				}
				pDest+=3;
				src++;
			}
			pDest += unusedPerLine;
		}
	}


	if(m_scaleBackground == SCALE_SPEED)
	{
		SetRenderState(RS_PIXELS);
		glPixelZoom(scale*m_scale, -scale*m_scale);
		glRasterPos2i(posX, posY);
		glDrawPixels(texWidth, texHeight, GL_RGB, GL_UNSIGNED_BYTE, dest );
		glPixelZoom(m_scale, -m_scale);
	}
	else
	{
		SetRenderState(RS_BITMAP);
		glPushAttrib(GL_ENABLE_BIT);
		glDisable(GL_SCISSOR_TEST);
		glDisable(GL_BLEND);
		
		Texture* pTex = nullptr;
		for (auto itr : m_fmvTextures)
		{
			// Note this would go wrong if we had more than one video per frame - we don't
			if (itr->GetWidth() == texWidth && itr->GetHeight())
			{
				pTex = itr;
				pTex->UpdateSubRegion(dest, 0, 0, texWidth, texHeight);
				break;
			}
		}

		if (!pTex)
		{
			pTex = new Texture(dest, texWidth, texHeight, 3);
			pTex->SetClamp();
			m_fmvTextures.push_back(pTex);
		}

		// Then draw on screen with adjusted co-ordinates
		pTex->Bind(0);
		glColor3f(1.0f, 1.0f, 1.0f);
		glBegin(GL_TRIANGLE_STRIP);

		if(bFullScreen)
		{
			glTexCoord2f(0.0f,		(2.f/texHeight));		glVertex2f(posX,			posY);
			glTexCoord2f(texCoordX, (2.f/texHeight));		glVertex2f(posX+quadWidth,	posY);
		}
		else
		{
			glTexCoord2f(0.0f,		0.0f);		glVertex2f(posX,			posY);
			glTexCoord2f(texCoordX, 0.0f);		glVertex2f(posX+quadWidth,	posY);
		}
		glTexCoord2f(0.0f,		texCoordY);	glVertex2f(posX,			posY+quadHeight);
		glTexCoord2f(texCoordX, texCoordY);	glVertex2f(posX+quadWidth,	posY+quadHeight);
		glEnd();
		glPopAttrib();
	}

	// Full screen video will have messed up projection mode
	m_renderMode = PM_NONE;
	m_renderState = RS_NONE;

	Set2DRendering(true);

	delete[] dest;
}

void* GLOpenGL::CreateMaterial(TextureMaterial *material)
{
	MaterialGL *matData = new MaterialGL(material, &m_treFile, m_shaderMaterials, m_useHDR);
	return matData;
}

void GLOpenGL::DeleteMaterial(void* mat)
{
	delete (MaterialGL*)mat;
}

void GLOpenGL::SetCamera(Camera* pCamera)
{
	DrawScreenPolys();
	DrawLines();

	m_pCamera = pCamera;
	BuildViewPortData(m_3dViewport);
	m_modelMatrix.LoadIdentity();
	m_viewMatrix.LoadIdentity();
	m_modelViewMatrix.LoadIdentity();
	m_renderMode = PM_ORTHO;

	int x0 = m_pCamera->viewLeft;
	int x1 = m_pCamera->viewRight;
	int y0 = m_pCamera->viewTop;
	int y1 = m_pCamera->viewBottom;

	if (m_bWidescreenHack && x0 == 0 && x1 == 639)
	{
		m_bWidescreenHackActive = true;
		m_bWidescreenHackThisFrame = true;
		int iWidth = (int)(480.f * ((float)m_windowWidth/(float)m_windowHeight));
		int iOffset = iWidth - 640;
		x0 = x0 - iOffset;
		x1 = x1 + iOffset;
	}
	else
	{
		m_bWidescreenHackActive = false;
	}

	int yStart, yEnd = 0;
	if(m_scaleBackground != SCALE_NONE)
	{
		yStart = 479.f - y0;	yEnd = 479.f - y1;	
	}
	else
	{
		yStart = m_windowHeight - y0;	yEnd = m_windowHeight - y1;	
	}
	int width = x1-x0+1;			int height = y1-y0+1;

	m_3dViewport.InitViewport(x0, yEnd, width, height);

	Matrix44 camMtx;
	camMtx.FillFromProph(pCamera->modelMatrix);
	Vector3 up, right, view, pos;
	camMtx.GetAxisI(right);
	camMtx.GetAxisJ(up);
	camMtx.GetAxisK(view);
	camMtx.GetTranslation(pos);
	m_eyePos = pos;
#if ORIGIN_OFFSET
	pos = Vector3(0.0, 0.0, 0.0);
	camMtx.SetTranslation(pos);
#endif
	m_eyeDir = view;
	//Vector3 pos(worldPos.x, worldPos.y, worldPos.z);
	m_viewMatrix.CameraMatrix(right, up, view, pos);
	m_modelViewMatrix = m_modelMatrix * m_viewMatrix;

	UpdateWorldView();
}

void GLOpenGL::SetModelMatrix(ProphMatrix* pModelMtx)
{
	m_modelMatrix.LoadIdentity();
	Set3DRendering();
	m_modelMatrix.FillFromProph(*pModelMtx);
	Vector3 worldPos;
	m_modelMatrix.GetTranslation(worldPos);
	#if ORIGIN_OFFSET
	m_modelMatrix.SetTranslation(worldPos - m_eyePos);
	#endif
	m_shaderMaterials ? ApplyShaderLights(worldPos) : ApplyLocalLights(worldPos);
	m_modelViewMatrix = m_modelMatrix * m_viewMatrix;

	Set3DRendering();
	UpdateWorldView();
}


void GLOpenGL::BuildViewPortData(Viewport &viewport)
{
	float fov = wcpogl::RadToDeg(m_pCamera->fov);
	float aspect = viewport.GetAspect();
	char msg[250];
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	float hWidth =  1.f/m_pCamera->oneOverFrustumWidth;
	float hHeight = 1.f/m_pCamera->oneOverFrustumHeight;
	glFrustum(-hWidth, hWidth, -hHeight, hHeight, m_pCamera->nearDist, m_pCamera->farDist);
	glGetFloatv(GL_PROJECTION_MATRIX, viewport.m_perspective.ColumnMajor());
	glPopMatrix();

	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	glOrtho(-1.0f, 1.0f, -1.0f, 1.0f, 0.0f, m_pCamera->farDist);
	glGetFloatv(GL_PROJECTION_MATRIX, viewport.m_ortho.ColumnMajor());
	glPopMatrix();

	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	glOrtho(0.0f, m_windowWidth, m_windowHeight, 0.0f, -1.0f, m_pCamera->farDist);
	glGetFloatv(GL_PROJECTION_MATRIX, viewport.m_proj2D.ColumnMajor());
	glPopMatrix();
}


void GLOpenGL::SetLights(Light **ppLights, int noLights)
{
	// Clear the hardware lights
	wcpogl::DeleteStlVector(m_globalLights);
	wcpogl::DeleteStlVector(m_localLights);

	m_lightList = ppLights;
	m_lightCount = noLights;

	float globalAmbient[] = {0.0f, 0.0f, 0.0f, 0.0f};
	glLightModelfv(GL_LIGHT_MODEL_AMBIENT, globalAmbient);
	m_totalAmbient.Assign(0.0f, 0.0f, 0.0f, 0.0f);
	// Disable previous frame lights
 
	LightGL* lightGL;
	for(int i=0; i<noLights; i++)
	{
		lightGL = new LightGL;
		lightGL->Init(ppLights[i]);
		
		switch(ppLights[i]->lightType)
		{
		case LT_AMBIENT:
			m_totalAmbient += lightGL->GetAmbient();
			delete lightGL;
			break;
		case LT_DIRECTIONAL:
			m_globalLights.push_back(lightGL);
			break;
		case LT_LASER:
			m_localLights.push_back(lightGL);
			break;
		default:
			// TODO: Determine the other types of lights
			delete lightGL;
		}
	}

	glLightModelfv(GL_LIGHT_MODEL_AMBIENT, m_totalAmbient.rgba());
	// model view must be identity for this to work
	m_modelMatrix.LoadIdentity();
	m_renderMode = PM_ORTHO;
	Set3DRendering();
	glEnable(GL_LIGHTING);
	ApplyGlobalLights();
	//glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE);
	//glEnable(GL_NORMALIZE);

	m_renderState = RS_NONE;
}

void GLOpenGL::ApplyGlobalLights()
{
	if(m_shaderMaterials)
	{
		const LightGL*	dirLights[3];
		int				dirLightCount = 0;

		for(int i=0; i<3 && i<m_globalLights.size(); i++)
		{
			dirLights[dirLightCount] = m_globalLights[i];
			dirLightCount++;
		}

		m_specularEffect.SetGlobalLightInfo(m_totalAmbient, dirLightCount, dirLights);
	}
	else
	{
		glMatrixMode(GL_MODELVIEW);
		glPushMatrix();
		glLoadMatrixf(m_viewMatrix.ColumnMajor());

		for(int i=0; i<OPENGL_MAX_LIGHTS; i++)
		{
			glDisable(GL_LIGHT0+i);
		}

		for(uint i=0; i < m_globalLights.size() && i < OPENGL_MAX_LIGHTS; i++ )
		{
			m_globalLights[i]->SetNumber(i);
			m_globalLights[i]->SetUpLight(m_eyePos);
			m_globalLights[i]->SwitchOn();
		}

		glPopMatrix();
	}
}


struct lightDist
{
	int		index;
	float	distance;
};

bool operator < (const lightDist& left, const lightDist& right)
{
	return (left.distance < right.distance);
} 


void GLOpenGL::ApplyShaderLights(const Vector3 &objectPos)
{
	const LightGL*	posLights[4];
	int				posLightCount = 0;

	const int maxLights = 30;
	int noLights = min(30, m_localLights.size());
	static lightDist	distances[maxLights];
	for(int i=0; i<noLights; i++)
	{
		distances[i].index = i;
		distances[i].distance = objectPos.GetSquaredDistanceFrom(m_localLights[i]->GetPos());
	}
	std::sort(distances, distances+noLights);

	for (int j=0; j<noLights && posLightCount<4; j++)
	{
		if(distances[j].distance > m_localLights[distances[j].index]->GetRangeSquare()*50.0f)
		{
			// Ok we're almost certainly out of range of these lights, don't go setting them needlessly
			// Removed for release, I'm sure a modern GPU can handle it 
		//	break;
		}
		posLights[posLightCount] = m_localLights[distances[j].index];
		posLightCount++;
	}

	
	Vector3 vPosShift(0.0, 0.0, 0.0);
	#if ORIGIN_OFFSET
		vPosShift = vPosShift-m_eyePos;
	#endif

	m_specularEffect.SetLocalLightInfo(posLightCount, posLights, vPosShift);

}

void GLOpenGL::ApplyLocalLights(const Vector3 &objectPos)
{
	SetViewNoWorld();

	int firstLight = m_globalLights.size();
	for(int i=firstLight; i<OPENGL_MAX_LIGHTS; i++)
	{
		glDisable(GL_LIGHT0+i);
	}

	vector<lightDist>	distances;

	lightDist dist;
	for(int i=0; i<m_localLights.size(); i++)
	{
		dist.index = i;
		dist.distance = objectPos.GetSquaredDistanceFrom(m_localLights[i]->GetPos());
		distances.push_back(dist);
	}

	sort(distances.begin(), distances.end());

	int localNo = 0;
	// Enable a maximum of four point lights for now
	for (int i=0; ((i+firstLight)<OPENGL_MAX_LIGHTS && i<distances.size()); i++)
	{
		// TODO: This *should* be handelled by the attenuation, but isn't
	//	if(distances[i].distance < m_localLights[distances[i].index]->GetRangeSquare()*50.0f)
		{
			m_localLights[distances[i].index]->SetNumber(localNo+firstLight);
			m_localLights[distances[i].index]->SetUpLight(m_eyePos);
			m_localLights[distances[i].index]->SwitchOn();
			localNo++;
		}
	}
}


bool GLOpenGL::PushGeometry(Vector3 *verts, int noVerts, Vector3 *norms, int noNorms)
{
	if (m_vStackHeight < MAX_VSTACK_HEIGHT)
	{
		m_vertData = &m_vertexStack[m_vStackHeight];
		m_vertData->verts = verts;
		m_vertData->normals = norms;
		m_vertData->noVerts = noVerts;
		m_vertData->noNormals = noNorms;
		m_vStackHeight++;

		return true;
	}

	return false;
}

void GLOpenGL::PopGeometry(void)
{
	if (m_vStackHeight > 0)
	{
		m_vStackHeight--;
		m_vertData = &m_vertexStack[m_vStackHeight];
	}
}

void GLOpenGL::Point(const Vector3* pPos, int colour)
{
	uint8 r, g, b;
	if(m_points.size()==0)
		m_pointMatrix = m_modelMatrix;

	make888(colour, r, g, b);
	PointVertex in;
	in.alpha = (float)b/255.f;
	in.worldPos = m_modelMatrix.ModelTransform((*pPos));
	m_points.push_back(in);
}

void GLOpenGL::DrawPoints()
{
	if(!m_pCamera || !m_points.size())
		return;

	assert(glGetError() == GL_NO_ERROR);

	m_modelMatrix.LoadIdentity();
	Set3DRendering();
	UpdateWorldView();
	SetRenderState(RS_POINTS);
	glDepthMask(GL_FALSE);
	glBindTexture(GL_TEXTURE_2D, 0);
	glDisable(GL_TEXTURE_2D);
	glBegin(GL_POINTS);
	vector<PointVertex>::const_iterator itr;
	for(itr = m_points.begin(); itr!=m_points.end(); ++itr)
	{
		glColor4f(1.0f, 1.0f, 1.0f, (*itr).alpha);
		glVertex3fv((*itr).worldPos.xyz());
	}
	glEnd();
	m_points.clear();
	glDepthMask(GL_TRUE);

	assert(glGetError() == GL_NO_ERROR);

}


static const float kDistEpsilon = 0.001f; 
static const float kColinearEps = 0.001f;  
static const float kMinPieceLen = 0.001f; 

static bool ClipBehindOverlap(const LineData& back, const LineData& front, std::vector<LineData>& outPieces)
{
	Vector3 backDir = back.end - back.start;
	float backLen = backDir.Magnitude();
	if (backLen < kMinPieceLen) return false;
	backDir *= (1.0f / backLen);

	Vector3 frontDir = front.end - front.start;
	float frontLen = frontDir.Magnitude();
	if (frontLen < kMinPieceLen) return false;
	frontDir *= (1.0f / frontLen);

	if (CrossProduct(backDir, frontDir).Magnitude() > kColinearEps) return false;

	Vector3 toFrontStart = front.start - back.start;
	Vector3 perp = toFrontStart - backDir * DotProduct(toFrontStart, backDir);
	if (perp.Magnitude() > kColinearEps) return false;

	float t0 = DotProduct(toFrontStart, backDir);
	float t1 = t0 + frontLen * DotProduct(frontDir, backDir);
	float tStart = max(0.0f, min(t0, t1));
	float tEnd = min(backLen, max(t0, t1));
	if (tEnd <= tStart) return false;

	if (tStart > kMinPieceLen)
	{
		LineData piece = back;
		piece.end = back.start + backDir * tStart;
		outPieces.push_back(piece);
	}
	if (backLen - tEnd > kMinPieceLen)
	{
		LineData piece = back;
		piece.start = back.start + backDir * tEnd;
		outPieces.push_back(piece);
	}
	return true;
}


void GLOpenGL::DrawLines()
{
	if (m_currentLines <= 0)
	{
		return;
	}

	for (int i = 0; i < m_currentLines; ++i)
	{
		m_pLineSortEntries[i].dist = m_pLines[i].camDist;
		m_pLineSortEntries[i].index = i;
	}

	std::sort(m_pLineSortEntries, m_pLineSortEntries + m_currentLines,
		[](const SortEntry& a, const SortEntry& b)
		{
			float diff = a.dist - b.dist;
			if (diff > kDistEpsilon)  return true;
			if (diff < -kDistEpsilon) return false;
			return a.index < b.index;
		});

	SetViewNoWorld();
	SetRenderState(RS_LINES);

	for (int i = 0; i < m_currentLines; ++i)
	{
		const LineData& itr = m_pLines[m_pLineSortEntries[i].index];

#if 0//!STANDOFF_BUILD
		if (m_useHDR)
		{
			SetActiveColourHack(itr.colour, 20);
		}
		else
#endif
		{
			SetActiveColour(itr.colour);
		}
		glBegin(GL_LINES);
		glVertex3fv(itr.start.xyz());
		glVertex3fv(itr.end.xyz());
		glEnd();
	}

	UpdateWorldView();
	m_linesThis3DPass = m_currentLines;
	m_currentLines = 0;
}

void GLOpenGL::Line(const Vector3* p0, const Vector3* p1, int colour)
{

	if (!m_isGameScene)
	{
		if (m_currentLines >= MAX_LINES)
		{
			DrawLines();
		}

		LineData& line = m_pLines[m_currentLines];
		line.start = m_modelMatrix.ModelTransform(*p0);
		line.end = m_modelMatrix.ModelTransform(*p1);
		line.colour = colour;

		Vector3 MiddlePos = (line.start * 0.5f + line.end * 0.5f);
		MiddlePos -= m_eyePos;
		line.camDist = DotProduct(MiddlePos, m_eyeDir);

		m_currentLines++;
	}
	else
	{
		assert(glGetError() == GL_NO_ERROR);

		SetRenderState(RS_LINES);
#if !STANDOFF_BUILD
		if (m_useHDR)
		{
			SetActiveColourHack(colour, 20);
		}
		else
#endif
		{
			SetActiveColour(colour);
		}
		glBegin(GL_LINES);
		glVertex3fv(p0->xyz());
		glVertex3fv(p1->xyz());
		glEnd();

		assert(glGetError() == GL_NO_ERROR);
	}
}

void GLOpenGL::Point(int geomIndex, int colour)
{
	Point(&m_vertData->verts[geomIndex], colour);
}

void GLOpenGL::Line(int geomIndex0, int geomIndex1, int colour)
{
	assert(glGetError() == GL_NO_ERROR);

	Line(&m_vertData->verts[geomIndex0], &m_vertData->verts[geomIndex1], colour);

	assert(glGetError() == GL_NO_ERROR);

}


void GLOpenGL::Init3DRendering(bool depthClear, int clipXMin, int clipYMin, int clipXMax, int clipYMax)
{
	assert(glGetError() == GL_NO_ERROR);
	// Only use HDR for full screen, otherwise it gets messy
	bool hdr = false;
	if(m_useHDR && ((clipXMax-clipXMin) > (512)) && !m_fbo.IsAttached())
	{
		m_fbo.Bind();
		hdr = true;
	}
	else
	{
		m_hdrAAScale = 1.0f;
	}

	if (m_bWidescreenHackActive)
	{
		int iWidth = (int)(480.f * ((float)m_windowWidth / (float)m_windowHeight));
		int iOffset = iWidth - 640;
		clipXMin = clipXMin - iOffset;
		clipXMax = clipXMax + iOffset;
	}

	// Viewport values are upside down
	int yStart;// = (float)m_sourceHeight - 1.0f - clipYMin;
	int yEnd;// = (float)m_sourceHeight - 1.0f - clipYMax; 
	//int width = clipXMax-clipXMin + 1.f;
	//int height = clipYMax-clipYMin + 1.f;

	if(m_scaleBackground != SCALE_NONE)
	{
		yStart = 479.f - clipYMin;	yEnd = 479.f - clipYMax;	
	}
	else
	{
		yStart = m_windowHeight - clipYMin;	yEnd = m_windowHeight - clipYMax;	
	}
	int width = clipXMax-clipXMin+1;			int height = clipYMax-clipYMin+1;

	m_3dViewport.SetScissor(clipXMin, yEnd, width, height);

	if(width==568)
		m_navThisFrameHack = true;

	if(clipXMin < m_scissorBounds[0])
		m_scissorBounds[0] = clipXMin;

	if(yEnd < m_scissorBounds[1])
		m_scissorBounds[1] = yEnd;

	if(width > m_scissorBounds[2])
		m_scissorBounds[2] = width;

	if(height > m_scissorBounds[3])
		m_scissorBounds[3] = height;


	BuildViewPortData(m_3dViewport);
	// Shouldn't need to do here
	Set3DRendering();
	//SetRenderState(RS_GEOMETRY);
//	UpdateWorldView();
	m_3dViewport.ApplyViewport(m_scale, m_2dPosOffset/m_scale);
	m_3dViewport.ApplyScissor(m_scale, m_2dPosOffset/m_scale);

	
	// States may have changed since the last polygons were drawn, so
	// ensure states are forcibly set on first call
	MaterialGL::ResetActiveMat();
	SpecularEffect::ResetActiveMat();

	if(depthClear)
	{	
		m_zBufferWrite = true;
		glDisable(GL_STENCIL_TEST);
		glClear(GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	}

	if(m_useStarbox && m_isGameScene && !m_starFieldThisFrame)
	{
		UpdateWorldView();
		SetRenderState(RS_SKYBOX);
		m_starFieldThisFrame = true;
		m_nebulas[NEBULA_STARS].Draw();
	}

	glEnable(GL_MULTISAMPLE);
	assert(glGetError() == GL_NO_ERROR);

	m_linesThis3DPass = 0;
}

void GLOpenGL::Done3DRendering(void)
{
	assert(glGetError() == GL_NO_ERROR);
	DrawPoints();
	assert(glGetError() == GL_NO_ERROR);
	DrawTransparentPolys();
	DrawLines();
	if(m_zBufferWrite && !m_useHDR)
	{
		glClear(GL_DEPTH_BUFFER_BIT);
	}
	m_zBufferWrite = false;
	glBindTexture(GL_TEXTURE_2D, 0);
	glDisable(GL_MULTISAMPLE);
	if(m_shaderMaterials)
	{
		m_specularEffect.Deactivate();
	}

	//if(!m_isGameScene)
	//	SplatHDR();

	// Don't call SplatHDR here, called numerous times.

	MaterialGL::ResetActiveMat();
	SpecularEffect::ResetActiveMat();

	m_3dViewport.SetScissor(0, 0, m_sourceWidth, m_sourceHeight);

	//DrawScreenPolys();
	//DrawDelayedEffects();

	assert(glGetError() == GL_NO_ERROR);

}

void GLOpenGL::Polygon(Poly2D* poly, int noPolys)
{
	// No longer draw screen polys directly, they fuck up the rendering
	for(int polyNo=0; polyNo<noPolys; polyNo++)
	{
		const Poly2D* thisPoly = &poly[polyNo];
		AddTransparentPoly(thisPoly);
	}
}


void GLOpenGL::DrawScreenPolys()
{
	if (!m_screenPolys.size())
		return;
	assert(glGetError() == GL_NO_ERROR);
	Set2DRendering(true, PM_2D, true);
	SetRenderState(RS_SCREENPOLY, true);
	TransparentVertex* vertex;
	vector<TransparentPoly>::iterator it;

	float insetFraction = max(0.0f, min(g_screenPolyUVInsetTexels, 0.5f));
	bool  doInset = (insetFraction > 0.0f);

	for (it = m_screenPolys.begin(); it != m_screenPolys.end(); ++it)
	{
		TransparentPoly* thisPoly = &(*it);
		MaterialGL* mat = thisPoly->material;
		mat->ClampTexture();
		if (thisPoly->flags & PL_POLY_ALPHA && g_bObeyScreenPolyAlpha)
			thisPoly->material->EnableMaterial(true, ((float)255 - thisPoly->alpha) / 255.0f);
		else
			thisPoly->material->EnableMaterial();

		if (!doInset)
		{
			// We don't alter the incoming co-ordiantes
			glBegin(GL_TRIANGLE_FAN);
			for (int i = 0; i < thisPoly->noVerts; i++)
			{
				vertex = &m_screenVerts[thisPoly->firstVert + i];
				//glColor4f(vertex->r, vertex->g, vertex->b, vertex->a);	// Don't use this, seems useless, always white
				glTexCoord2f(vertex->s, vertex->t);
				glVertex3fv(vertex->worldPos.xyz());
			}
			glEnd();
			continue;
		}


		// Alternate to avoid texel alignment issues, nudge the co-ordiantes away from the edge
		float minS = FLT_MAX, maxS = -FLT_MAX;
		float minT = FLT_MAX, maxT = -FLT_MAX;
		for (int i = 0; i < thisPoly->noVerts; i++)
		{
			vertex = &m_screenVerts[thisPoly->firstVert + i];
			minS = min(minS, vertex->s);
			maxS = max(maxS, vertex->s);
			minT = min(minT, vertex->t);
			maxT = max(maxT, vertex->t);
		}

		float texelU = 0.0f, texelV = 0.0f;
		int texWidth = 0, texHeight = 0;
		if (mat->GetTexture())	
		{
			texWidth = mat->GetTexture()->GetWidth();
			texHeight = mat->GetTexture()->GetHeight();
			if (texWidth > 0) texelU = 1.0f / (float)texWidth;
			if (texHeight > 0) texelV = 1.0f / (float)texHeight;
		}

		// Clamped so we never invert a poly narrower/shorter than one texel
		float insetS = min(texelU * insetFraction, (maxS - minS) * 0.5f);
		float insetT = min(texelV * insetFraction, (maxT - minT) * 0.5f);

		glBegin(GL_TRIANGLE_FAN);
		for (int i = 0; i < thisPoly->noVerts; i++)
		{
			vertex = &m_screenVerts[thisPoly->firstVert + i];

			float s = vertex->s;
			float t = vertex->t;
			if (insetS > 0.0f)
				s = (s == minS) ? s + insetS : (s == maxS) ? s - insetS : s;
			if (insetT > 0.0f)
				t = (t == minT) ? t + insetT : (t == maxT) ? t - insetT : t;

			glTexCoord2f(s, t);
			glVertex3fv(vertex->worldPos.xyz());
		}
		glEnd();
		//	MaterialGL::ResetActiveMat();
	}
	m_screenPolys.clear();
	m_screenVerts.clear();
	assert(glGetError() == GL_NO_ERROR);
}

#if 0
void GLOpenGL::CalculateBinormalAndTangent(const Poly3D* poly, uint32 uIdx0, uint32 uIdx1, uint32 uIdx2, uint32 uPrimaryIdx, Vector3& binormal, Vector3& tangent)
{
	const Vertex3D* pVI0 = &poly->verts[uIdx0];
	const Vertex3D* pVI1 = &poly->verts[uIdx0];
	const Vertex3D* pVI2 = &poly->verts[uIdx0];

	const Vector3& p0 = m_vertData->verts[uIdx0];
	const Vector3& p1 = m_vertData->verts[uIdx1];
	const Vector3& p2 = m_vertData->verts[uIdx2];

	Vector3 v1(p1.x() - p0.x(), p1.y() - p0.y(), p1.z() - p0.z());
	Vector3 v2(p2.x() - p0.x(), p2.y() - p0.y(), p2.z() - p0.z());

	float s1 = pVI1->s - pVI0->s;
	float s2 = pVI2->s - pVI0->s;
	float t1 = pVI1->t - pVI0->t;
	float t2 = pVI2->t - pVI0->t;

	float r = 1.0f / (s1 * t2 - s2 * t1);

	Vector3 sDir((t2 * v1.x() - t1 * v2.x()) * r,
		(t2 * v1.y() - t1 * v2.y()) * r,
		(t2 * v1.z() - t1 * v2.z()) * r);

	Vector3 tDir((s1 * v2.x() - s2 * v1.x()) * r,
		(s1 * v2.y() - s2 * v1.y()) * r,
		(s1 * v2.z() - s2 * v1.z()) * r);

	const Vector3& normal = poly->flags & PL_FLAT_SHADING ? m_vertData->normals[uPrimaryIdx] : m_vertData->normals[uPrimaryIdx];
	tangent = sDir - normal * DotProduct(normal, sDir);
	tangent.Normalise();

	binormal = tDir - normal * DotProduct(normal, tDir);
	binormal.Normalise();

}
#endif

void GLOpenGL::CalculateBinormalAndTangent(const Poly3D* poly, uint32 uIdx0, uint32 uIdx1, uint32 uIdx2, uint32 uPrimaryIdx, Vector3& binormal, Vector3& tangent)
{
	const Vertex3D* pVI0 = &poly->verts[uIdx0];
	const Vertex3D* pVI1 = &poly->verts[uIdx1];
	const Vertex3D* pVI2 = &poly->verts[uIdx2];

	const Vector3& p0 = m_vertData->verts[uIdx0];
	const Vector3& p1 = m_vertData->verts[uIdx1];
	const Vector3& p2 = m_vertData->verts[uIdx2];

	Vector3 v1(p1.x() - p0.x(), p1.y() - p0.y(), p1.z() - p0.z());
	Vector3 v2(p2.x() - p0.x(), p2.y() - p0.y(), p2.z() - p0.z());

	float s1 = pVI1->s - pVI0->s;
	float s2 = pVI2->s - pVI0->s;
	float t1 = pVI1->t - pVI0->t;
	float t2 = pVI2->t - pVI0->t;

	float r = 1.0f / (s1 * t2 - s2 * t1);

	tangent.SetUp((t2 * v1.x() - t1 * v2.x()) * r, (t2 * v1.y() - t1 * v2.y()) * r,	(t2 * v1.z() - t1 * v2.z()) * r);

	binormal.SetUp((s1 * v2.x() - s2 * v1.x()) * r, (s1 * v2.y() - s2 * v1.y()) * r, (s1 * v2.z() - s2 * v1.z()) * r);

	// The dot product and normalize are faster on the GPU, so we push that work there
}


void GLOpenGL::PolygonStandard(Poly3D* poly, const TextureMaterial* texMat, MaterialGL* mat)
{
	assert(glGetError() == GL_NO_ERROR);

	glEnable(GL_LIGHTING);
	mat->EnableMaterial();

	glBegin(GL_TRIANGLE_FAN);
	for (int i = 0; i < poly->noVerts; i++)
	{
		const Vertex3D*	vertexIndex = &poly->verts[i];

		if ((poly->flags & PL_FLAT_SHADING))
		{
			glNormal3fv((float*)&m_vertData->normals[poly->normId]);
		}
		else
		{
			glNormal3fv((float*)&m_vertData->normals[vertexIndex->normId]);
		}

		glTexCoord2fv(&vertexIndex->s);
		glVertex3fv((float*)&m_vertData->verts[vertexIndex->vertId]);
	}
	glEnd();

	assert(glGetError() == GL_NO_ERROR);

}

void GLOpenGL::PolygonUnlitStandard(Poly3D* poly, const TextureMaterial* texMat, MaterialGL* mat)
{
	assert(glGetError() == GL_NO_ERROR);

	glDisable(GL_LIGHTING);

	float brightness = (poly->flags & PL_POLY_BRIGHTNESS) != 0 ?  poly->brightness : 1.0f;
	mat->EnableMaterial(false, 0.0f, false, brightness);


	glBegin(GL_TRIANGLE_FAN);
	for (int i = 0; i < poly->noVerts; i++)
	{
		const Vertex3D*	vertexIndex = &poly->verts[i];
		glNormal3f(0.0f, 0.0f, 0.0f);
		glTexCoord2fv(&vertexIndex->s);
		glVertex3fv((float*)&m_vertData->verts[vertexIndex->vertId]);
	}
	glEnd();

	assert(glGetError() == GL_NO_ERROR);
}


void GLOpenGL::PolygonSpecular(Poly3D* poly, const TextureMaterial* texMat, MaterialGL* mat)
{
	assert(glGetError() == GL_NO_ERROR);

	m_specularEffect.SetUseLighting(true);
	mat->EnableShaderMaterial();
	m_specularEffect.SetMaterialInfo(mat);
	m_specularEffect.Activate();

	glBegin(GL_TRIANGLE_FAN);
	for (int i = 0; i < poly->noVerts; i++)
	{
		const Vertex3D*	vertexIndex = &poly->verts[i];

		if ((poly->flags & PL_FLAT_SHADING))
		{
			glNormal3fv((float*)&m_vertData->normals[poly->normId]);
		}
		else
		{
			glNormal3fv((float*)&m_vertData->normals[vertexIndex->normId]);
		}

		glTexCoord2fv(&vertexIndex->s);
		glVertex3fv((float*)&m_vertData->verts[vertexIndex->vertId]);
	}
	glEnd();

	assert(glGetError() == GL_NO_ERROR);
}

void GLOpenGL::PolygonBump(Poly3D* poly, const TextureMaterial* texMat, MaterialGL* mat)
{

	assert(glGetError() == GL_NO_ERROR);

	m_specularEffect.SetUseLighting(true);
	mat->EnableShaderMaterial();
	m_specularEffect.SetMaterialInfo(mat);
	m_specularEffect.Activate();

	Vector3 binormal, tangent;
	glBegin(GL_TRIANGLE_FAN);
	for (int i = 0; i < poly->noVerts; i++)
	{
		const Vertex3D*	vertexIndex = &poly->verts[i];

		if ((poly->flags & PL_FLAT_SHADING))
		{
			glNormal3fv((float*)&m_vertData->normals[poly->normId]);
		}
		else
		{
			glNormal3fv((float*)&m_vertData->normals[vertexIndex->normId]);
		}

		if (i <= 2)
		{
			CalculateBinormalAndTangent(poly, 0, 1, 2, i, binormal, tangent);
		}
		else
		{
			CalculateBinormalAndTangent(poly, 0, i - 1, i, i, binormal, tangent);
		}
		glMultiTexCoord3fv(GL_TEXTURE1, binormal.xyz());
		glMultiTexCoord3fv(GL_TEXTURE2, tangent.xyz());
		
		glTexCoord2fv(&vertexIndex->s);
		glVertex3fv((float*)&m_vertData->verts[vertexIndex->vertId]);
	}
	glEnd();

	assert(glGetError() == GL_NO_ERROR);

}

void GLOpenGL::PolygonUnlit(Poly3D* poly, const TextureMaterial* texMat, MaterialGL* mat)
{
	assert(glGetError() == GL_NO_ERROR);

	m_specularEffect.SetUseLighting(false);
	float brightness = (poly->flags & PL_POLY_BRIGHTNESS) != 0 ? poly->brightness : 1.0f;
	mat->EnableShaderMaterial(false, 0.0f, false, brightness);
	m_specularEffect.SetMaterialInfo(mat);
	m_specularEffect.Activate();

	glBegin(GL_TRIANGLE_FAN);
	for (int i = 0; i < poly->noVerts; i++)
	{
		const Vertex3D*	vertexIndex = &poly->verts[i];
		glNormal3f(0.0f, 0.0f, 0.0f);
		glTexCoord2fv(&vertexIndex->s);
		glVertex3fv((float*)&m_vertData->verts[vertexIndex->vertId]);
	}
	glEnd();

	assert(glGetError() == GL_NO_ERROR);

}

void GLOpenGL::Set3DRenderingInt()
{
	assert(glGetError() == GL_NO_ERROR);

	SplatCurrentBitmap();
	assert(glGetError() == GL_NO_ERROR);

	if (m_fbo.IsAttached() && m_useHDR)
	{
		m_3dViewport.ApplyViewport(m_hdrAAScale*m_scale, m_2dPosOffset / m_scale);
		m_3dViewport.ApplyScissor(m_hdrAAScale*m_scale, m_2dPosOffset / m_scale);
	}
	else
	{
		m_3dViewport.ApplyViewport(m_scale, m_2dPosOffset / m_scale);
		m_3dViewport.ApplyScissor(m_scale, m_2dPosOffset / m_scale);
	}
	assert(glGetError() == GL_NO_ERROR);

	glEnable(GL_LIGHTING);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_TEXTURE_2D);
	glDepthFunc(GL_LESS);

	assert(glGetError() == GL_NO_ERROR);

	//UpdateWorldView();
}

void GLOpenGL::Polygon(Poly3D* poly, int noPolys)
{
	const Vector3*		vertex;
	const Vector3*		normal;
	Set3DRendering();
	SetRenderState(RS_GEOMETRY);

	for (int polyNo = 0; polyNo < noPolys; polyNo++)
	{
		const Poly3D* thisPoly = &poly[polyNo];
		const TextureMaterial* texMat = thisPoly->material->GetActiveMat();
		MaterialGL* mat = (MaterialGL*)(texMat->GetMaterialGL());
		bool bHasNormal = mat->HasNormalMap();

		if ((mat->Translucency() || (poly->flags & PL_POLY_ALPHA)) && (m_zBufferWrite || m_shaderMaterials))
		{
			// Store the poly for later processing
			AddTransparentPoly(poly);
		}
		else
		{
			if (!(poly->flags & (PL_NO_LIGHTING| PL_POLY_BRIGHTNESS) ))
			{
				if (!m_shaderMaterials)
				{
					PolygonStandard(poly, texMat, mat);
				}
				if (bHasNormal)
				{
					PolygonBump(poly, texMat, mat);
				}
				else
				{
					PolygonSpecular(poly, texMat, mat);
				}
			}
			else
			{
				if (m_shaderMaterials)
				{
					PolygonUnlit(poly, texMat, mat);
				}
				else
				{
					PolygonUnlitStandard(poly, texMat, mat);
				}
			}
		}
	}
}


void GLOpenGL::AddTransparentPoly(const Poly3D* poly)
{
	TransparentPoly transPoly;
	float sqDist		= 0.f;
	transPoly.flags		= poly->flags;
	transPoly.firstVert	= m_transVerts.size();
	transPoly.noVerts	= poly->noVerts;
	transPoly.material	= (MaterialGL*)(poly->material->GetActiveMat()->GetMaterialGL());
	transPoly.alpha		= poly->alpha;
	Vector3 totalVertPos(0.0f, 0.0f, 0.0f);


	// TODO: Some definite room for optimization here
	TransparentVertex transVert;
	for(int i=0; i<poly->noVerts; i++)
	{
		const Vector3* vertPos = &m_vertData->verts[poly->verts[i].vertId];
		
		// Translate into world space so we don't have to hold the matrix and get the distance to the camera
		transVert.worldPos = m_modelViewMatrix * (*vertPos);
		
		transVert.s = poly->verts[i].s;
		transVert.t = poly->verts[i].t;
		totalVertPos += *vertPos;
		m_transVerts.push_back(transVert);
	}

	Vector3 avPos;
	avPos = totalVertPos / poly->noVerts;

	// TODO: Optimise
	Matrix44 camMtx;
	Vector3 camPos;
	camMtx.FillFromProph(m_pCamera->modelMatrix);
	camMtx.GetTranslation(camPos);
	transPoly.camSqDist = avPos.GetSquaredDistanceFrom(camPos);

	m_transPolys.push_back(transPoly);
}


void GLOpenGL::AddTransparentPoly(const Poly2D* poly)
{
	Vector3			vertPos;
	TransparentPoly		transPoly;
	TransparentVertex	transVert;

	float sqDist		= 0.f;
	transPoly.flags		= poly->flags;
	transPoly.firstVert	= m_screenVerts.size();
	transPoly.noVerts	= poly->numberOfVertices;

	const TextureMaterial* texMat = poly->material->GetActiveMat();
	MaterialGL* mat = (MaterialGL *)(texMat->GetMaterialGL());

	transPoly.material	= mat;
	transPoly.alpha		= poly->alpha;

	for(int i=0; i<poly->numberOfVertices; i++)
	{
		vertPos.SetUp(poly->verts[i].x, poly->verts[i].y, poly->verts[i].z);

		transVert.s = poly->verts[i].s;
		transVert.t = poly->verts[i].t;
		transVert.worldPos = vertPos;

		m_screenVerts.push_back(transVert);
	}

	m_screenPolys.push_back(transPoly);
}


bool operator < (const TransparentPoly& left, const TransparentPoly& right)
{
	return (left.camSqDist > right.camSqDist);
} 


// TODO: Should transparent polys really never use lighting?
void GLOpenGL::DrawTransparentPolys()
{
	if(!m_pCamera || !m_transVerts.size() || !m_transPolys.size())
	{
		return;
	}

	int					vertexIndex = 0;
	TransparentVertex *	vertex		= NULL;
	m_modelMatrix.LoadIdentity();

	assert(glGetError() == GL_NO_ERROR);

	Set3DRendering();

	assert(glGetError() == GL_NO_ERROR);


	// TODO: Transparent geometry states
	SetRenderState(RS_TRANSPARENT_GEOMETRY);

	assert(glGetError() == GL_NO_ERROR);


	//UpdateWorldView();
	if(m_shaderMaterials)
	{
		Matrix44 worldView = /*m_viewMatrix **/ m_3dViewport.m_perspective;
		m_transparentEffect.SetViewInfo(worldView);
	}
	else
	{
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glLoadMatrixf(m_3dViewport.m_perspective.ColumnMajor());

		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
	}
	
	sort(m_transPolys.begin(), m_transPolys.end());
	assert(glGetError() == GL_NO_ERROR);

	vector<TransparentPoly>::iterator polyIt;
	for(polyIt = m_transPolys.begin(); polyIt!=m_transPolys.end(); ++polyIt)
	{
		TransparentPoly* poly = &(*polyIt);
		
		if(m_shaderMaterials)
		{
			m_specularEffect.SetMaterialInfo(poly->material);
			if(poly->flags & PL_POLY_ALPHA)
				poly->material->EnableShaderMaterial(true, ((float)255-poly->alpha)/255.0f);
			else
				poly->material->EnableShaderMaterial();
			m_transparentEffect.Activate(poly->material);
		}			
		else
		{
			if(poly->flags & PL_POLY_ALPHA)
				poly->material->EnableMaterial(true, ((float)255-poly->alpha)/255.0f);
			else
				poly->material->EnableMaterial();
		}

		glBegin(GL_TRIANGLE_FAN);
		for (int vertNo=0; vertNo < poly->noVerts; vertNo++)
		{
			vertexIndex = poly->firstVert + vertNo;
			vertex = &m_transVerts[vertexIndex];

			glTexCoord2f(vertex->s, vertex->t);
			glVertex3fv(vertex->worldPos.xyz());
		}
		glEnd();
	}
	assert(glGetError() == GL_NO_ERROR);

	if(m_shaderMaterials)
	{
		m_transparentEffect.Deactivate();
	}

	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
	assert(glGetError() == GL_NO_ERROR);

	glDepthMask(GL_TRUE);

	m_transVerts.clear();
	m_transPolys.clear();

	int error = glGetError();
	assert(error == GL_NO_ERROR);
}

bool GLOpenGL::Effect(int effectType, void* pEffectData)
{
	switch (effectType)
	{
	case EFFECT_LENS_FLARE:
		{
		#if STANDOFF_BUILD
			if(!m_useHDR)
		#endif
			{
				DrawLensFlareQuad((LensFlareFX*)(pEffectData));
			}
			return true;
		}

	case EFFECT_FOG:
		EnableFog((FogFX*)(pEffectData));
		return true;

	case EFFECT_SCREEN_FLASH:
		{
			//ScreenFlash(reinterpret_cast<ScreenFlashFX *>(data));
			m_delayedEffects.useFlash = true;
			memcpy(&m_delayedEffects.flashFX, pEffectData, sizeof(m_delayedEffects.flashFX));
			return true;
		}

	case EFFECT_TOGGLE:
	{
		ToggleFX* value = (ToggleFX*)(pEffectData);
		return true;
	}

	case EFFECT_HDFMV:
	{
		struct GL_SFX_capabilities
		{
			DWORD dwSize;
			BOOL bHDVideoSupported;
			BOOL bEnable32BitVideo;
			UINT32 uiHDMaxWidth;
			UINT32 uiHDMaxHeight;
		};

		GL_SFX_capabilities* cap = (GL_SFX_capabilities*)(pEffectData);
		if (cap->dwSize >= sizeof(GL_SFX_capabilities))
		{
			cap->bHDVideoSupported = true;
			if (g_bRequirePowerOfTwo)
			{
				cap->uiHDMaxWidth = 1024;
				cap->uiHDMaxHeight = 1024; 
			}
			else
			{
				cap->uiHDMaxWidth = 2560;
				cap->uiHDMaxHeight = 1440;
			}
			cap->bEnable32BitVideo = true;
			m_bHDMovieHack = true;
		}

		return true;
	}
	case EFFECT_UNKNOWN1:
		return true;
	case EFFECT_UNKNOWN2:
		return true;
	case EFFECT_UNKNOWN3:
		return true;
	case EFFECT_UNKNOWN4:
		return true;
	case EFFECT_QUERY1:
	case EFFECT_QUERY2:
	default:
		return true;
	}

	return false;
}




void GLOpenGL::ScreenFlash(ScreenFlashFX* flash)
{
	m_renderState = RS_NONE;
	Set2DRendering(false, PM_ORTHO);
	SetRenderState(RS_SCREENPOLY);
	glEnable(GL_BLEND);
	glBindTexture(GL_TEXTURE_2D, 0);
	glDisable(GL_LIGHTING);
	EnableTransparency(GL_ONE_MINUS_SRC_ALPHA, GL_SRC_ALPHA);
	glDepthMask(GL_FALSE);

	glColor4f(flash->red, flash->green, flash->blue, 0.5f);
	glBegin(GL_TRIANGLE_STRIP);
	glVertex2i(-1, -1);
	glVertex2i(-1, 1);
	glVertex2i(1, -1);
	glVertex2i(1, 1);
	glEnd();
	glDepthMask(GL_TRUE);
	m_renderState = RS_NONE;

}


void GLOpenGL::FillQuad(Vertex2D vertList[4], float left, float right, float bottom, float top, float sMin, float sMax, float tMin, float tMax)
{
	vertList[0].x = left;
	vertList[0].y = bottom;
	vertList[0].s = sMin;
	vertList[0].t = tMin;

	vertList[1].x = right;
	vertList[1].y = bottom;
	vertList[1].s = sMax;
	vertList[1].t = tMin;

	vertList[2].x = right;
	vertList[2].y = top;
	vertList[2].s = sMax;
	vertList[2].t = tMax;

	vertList[3].x = left;
	vertList[3].y = top;
	vertList[3].s = sMin;
	vertList[3].t = tMax;
}


void GLOpenGL::DrawLensFlareQuad(LensFlareFX* lensFlareData)
{
	float x = (float)(lensFlareData->centerx);
	float y = (float)(lensFlareData->centery);

	// Lens flares are split into 4 sections with 4 materials
	float radius = (float)(lensFlareData->radius);

	Vertex2D verts[4];
	const int numVerts = 4;

	uint8 ru, gu, bu;
	make888(lensFlareData->colour, ru, gu, bu);

	for(int i=0; i<numVerts; i++)
	{
		verts[i].red = ru; verts[i].green = gu; verts[i].blue = bu;
		verts[i].alpha = 0.5f;
		verts[i].z = 1; 
	}
	
	TextureMaterial* mat = lensFlareData->mat[min(lensFlareData->materialToUse, 3) ];

	assertIf (mat != NULL)
	{
		MaterialGL *matData = (MaterialGL*)(mat->GetMaterialGL());

		Poly2D sp = {};
		sp.material = mat;
		sp.verts = verts;
		sp.flags = 0;
		sp.numberOfVertices = numVerts;	

		float l = x - radius;
		float r = x + radius;
		float b = y - radius;
		float t = y + radius;
		
		FillQuad(verts, l, x, b, y, 0.0f, 1.0f, 0.0f, 1.0f);
		AddTransparentPoly(&sp);

		FillQuad(verts, x, r, b, y, 1.0f, 0.0f, 0.0f, 1.0f);
		AddTransparentPoly(&sp);

		FillQuad(verts, l, x, y, t, 0.0f, 1.0f, 1.0f, 0.0f);
		AddTransparentPoly(&sp);

		FillQuad(verts, x, r, y, t, 1.0f, 0.0f, 1.0f, 0.0f);
		AddTransparentPoly(&sp);
	}

}



void GLOpenGL::EnableFog(FogFX *fogData)
{
	if(fogData->active)
	{
		float colour[4];
		colour[0] = fogData->red;		colour[1] = fogData->green;
		colour[2] = fogData->blue;		colour[3] = 1.0f;
		glFogfv(GL_FOG_COLOR, colour);
		
		GLint decayType = GL_EXP;
		
		glFogi(GL_FOG_MODE, decayType);	// Set to EXP to use Fog density
		glHint(GL_FOG_HINT, GL_NICEST);

		if (decayType == GL_LINEAR)
		{
			glFogf(GL_FOG_START, 100.0f);
			glFogf(GL_FOG_END, 1000.0f);
		}
		else
		{
			glFogf(GL_FOG_DENSITY, fogData->density * g_fFogDensityScale);
		}

		//glEnable(GL_FOG);
	}
	else
	{
		//glDisable(GL_FOG);
	}
	
	m_activeFog = *fogData;
	m_activeFog.density *= g_fFogDensityScale;

	if (m_shaderMaterials)
	{
		m_transparentEffect.SetFogInfo(m_activeFog);
		m_specularEffect.SetFogInfo(m_activeFog);
	}
}


bool GLOpenGL::LoadCustomNebulas()
{
	return m_nebulas[NEBULA_STARS].InitHardcoded(&m_treFile);
}


void GLOpenGL::WriteOutScreenshot(const uint8* pixels, const char *filename, int width, int height)
{
	BITMAPFILEHEADER	fileHeader;
	BITMAPINFOHEADER	infoHeader;
	uint8				*bgr, *dest;
	const uint8			*src;
	FILE				*pFile;

	pFile = fopen(filename, "wb");
 	assertIf (pFile)
	{
		fileHeader.bfType = 'MB';
		fileHeader.bfSize = sizeof(fileHeader) + sizeof(infoHeader) + (3 * width * height);
		fileHeader.bfReserved1 = 0;
		fileHeader.bfReserved2 = 0;
		fileHeader.bfOffBits = sizeof(fileHeader) + sizeof(infoHeader);
		fwrite(&fileHeader, sizeof(fileHeader), 1, pFile);

		infoHeader.biSize = sizeof(infoHeader);
		infoHeader.biWidth = width;
		infoHeader.biHeight = height;
		infoHeader.biPlanes = 1;
		infoHeader.biBitCount = 24;
		infoHeader.biCompression = BI_RGB;
		infoHeader.biSizeImage = 0;
		infoHeader.biXPelsPerMeter = 1;
		infoHeader.biYPelsPerMeter = 1;
		infoHeader.biClrUsed = 0;
		infoHeader.biClrImportant = 0;
		fwrite(&infoHeader, sizeof(infoHeader), 1, pFile);

		bgr = new uint8[width * 3];
		src = pixels;
		for(int i=0; i<height; i++)
		{
			dest = bgr;
			src = pixels+(width * i * 3);
			for (int j=0; j<width; j++)
			{
				dest[2] = src[0];
				dest[1] = src[1];
				dest[0] = src[2];
				dest += 3;
				src+=3;
			}

			fwrite(bgr, 1, width * 3, pFile);
		}

		fclose(pFile);

		delete [] bgr;
	}
}


void GLOpenGL::SplatHDR(bool bAllowBriefBloom)
{
	if( m_useHDR && m_fbo.IsAttached() )
	{	
		SetRenderState(RS_POSTPROCESS);
		m_fbo.Detach();
		m_fbo.BlitMS();
		// Set scissor and viewport for effect to grab
		Viewport tmpView;
		tmpView.InitViewport(0.0f, 0.0f, m_fbo.GetWidth(), m_fbo.GetHeight());
		tmpView.SetScissor(m_scissorBounds[0], m_scissorBounds[1], m_scissorBounds[2], m_scissorBounds[3]);
		tmpView.ApplyViewport();
		tmpView.ApplyScissor(m_scale, m_2dPosOffset/m_scale);
		bool bViewport = m_3dViewport.GetWidth() == 96;
		
		if((m_isGameScene && m_navThisFrameHack) || bViewport || !bAllowBriefBloom) // Nav map
		{
			m_blitEffect.Draw(&m_fbo);
		}
		else if(m_isGameScene)
		{
			m_bloomEffect.Draw(&m_fbo, kGameBloomCutOff, kGameBloomExposure);
		}
		else
		{
			m_briefingBloom.Draw(&m_fbo, 0.0f, 20.0f);
		}

		// Breaks the OpenGL as in rare instances is called twice
	//	m_fbo.Bind();
	//	glClear(GL_COLOR_BUFFER_BIT);
		m_fbo.Detach();
		
		m_3dViewport.ApplyViewport(m_scale, m_2dPosOffset/m_scale);
		m_renderMode = PM_NONE;
		m_renderState = RS_NONE;
	}
}


void GLOpenGL::SetRenderStateInt(RenderState state)
{
	if(state!=RS_GEOMETRY)
	{
		MaterialGL::ResetActiveMat();
		SpecularEffect::ResetActiveMat();
	}
	switch(state)
	{
	case RS_POINTS:
		glDisable(GL_TEXTURE_2D);
		glEnable(GL_BLEND);	// TODO: Enable and have points fade in/out
		glEnable(GL_POINT_SMOOTH);
		EnableTransparency(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glPointSize(((float)m_windowHeight/480.0f)*1.5f);
		glDisable(GL_LIGHTING);
		glDisable(GL_ALPHA_TEST);
		glEnable(GL_DEPTH_TEST);
		glDepthMask(GL_TRUE);

		if (m_activeFog.active)
		{
			glEnable(GL_FOG);
		}

		if(m_shaderMaterials)
		{
			m_specularEffect.Deactivate();
		}
		break;
	case RS_POINTS_2D:
		glDisable(GL_TEXTURE_2D);
		glDisable(GL_BLEND);	
		//EnableTransparency(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glPointSize(((float) (uint)((m_windowHeight / 480.0f) + 0.9f)) );
		glDisable(GL_POINT_SMOOTH);
		glDisable(GL_LIGHTING);
		glDisable(GL_ALPHA_TEST);
		glDisable(GL_DEPTH_TEST);
		glDepthMask(GL_FALSE);
		glDisable(GL_FOG);

		if (m_shaderMaterials)
		{
			m_specularEffect.Deactivate();
		}
		break;
	case RS_CIRCLE:
		glDisable(GL_TEXTURE_2D);
		DisableTransparency();
		glPointSize(((float)m_windowHeight/480.0f)*2.0f);
		glDisable(GL_LIGHTING);
		glDisable(GL_ALPHA_TEST);
		glEnable(GL_DEPTH_TEST);
		glDepthMask(GL_TRUE);
		glDisable(GL_FOG);

		if(m_shaderMaterials)
		{
			m_specularEffect.Deactivate();
		}
		break;
	case RS_LINES:
		glDisable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, 0);
		EnableTransparency(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glDisable(GL_LIGHTING);
		glLineWidth( max(m_scale*m_hdrAAScale * 0.66f, 1.0f) );
		glDisable(GL_ALPHA_TEST);
		glDepthMask(GL_TRUE);
		glDisable(GL_FOG);
		glDepthFunc(GL_LEQUAL);

		#if DEPTH_TEST_LINES
		glEnable(GL_DEPTH_TEST);
		#else
		glDisable(GL_DEPTH_TEST);
		#endif
		
		if(m_shaderMaterials)
		{
			m_specularEffect.Deactivate();
		}
		break;
	case RS_LINES_2D:
		glDisable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, 0);
		EnableTransparency(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glDisable(GL_LIGHTING);
		glLineWidth(max(m_scale * m_hdrAAScale * 0.66f, 1.0f));
		glDisable(GL_ALPHA_TEST);
		glDepthMask(GL_TRUE);
		glDisable(GL_FOG);

		glDisable(GL_DEPTH_TEST);

		if (m_shaderMaterials)
		{
			m_specularEffect.Deactivate();
		}
		break;
	case RS_RECT:
		glDepthMask(GL_TRUE);
		glDisable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, 0);
		EnableTransparency(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glEnable(GL_BLEND);
		glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
		glDepthMask(GL_FALSE);
		glDisable(GL_LIGHTING);
		glEnable(GL_LINE_SMOOTH);
		glLineWidth(m_scale * m_hdrAAScale);
		glDisable(GL_ALPHA_TEST);
		glDisable(GL_FOG);

		if(m_shaderMaterials)
		{
			m_specularEffect.Deactivate();
		}
		break;
	case RS_BITMAP:
		glEnable(GL_ALPHA_TEST);
		glAlphaFunc(GL_GREATER, 0.2f);
		glDisable(GL_BLEND);
		glDepthMask(GL_TRUE);
		glDisable(GL_LIGHTING);
		glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
		glDisable(GL_DEPTH_TEST);
		glEnable(GL_TEXTURE_2D);
		glDisable(GL_FOG);

		if(m_shaderMaterials)
		{
			m_specularEffect.Deactivate();
		}
		break;
	case RS_PIXELS:
		glEnable(GL_ALPHA_TEST);
		glAlphaFunc(GL_GREATER, 0.2f);
		EnableTransparency(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glDisable(GL_BLEND);
		glDepthMask(GL_FALSE);
		glDisable(GL_DEPTH_TEST);
		glDisable(GL_TEXTURE_2D);
		glDisable(GL_LIGHTING);
		glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
		glDisable(GL_FOG);

		if(m_shaderMaterials)
		{
			m_specularEffect.Deactivate();
		}
		break;
	case RS_GEOMETRY:
		// Handled by the material information
		glEnable(GL_DEPTH_TEST);
		glDisable(GL_ALPHA_TEST);
		glDepthMask(GL_TRUE);
		glDepthFunc(GL_LESS);
		glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
		if (m_activeFog.active && !m_shaderMaterials)
		{
			glEnable(GL_FOG);
		}
		break;
	case RS_TRANSPARENT_GEOMETRY:
		glEnable(GL_DEPTH_TEST);
		glDisable(GL_ALPHA_TEST);
		glDepthFunc(GL_LEQUAL);
		glDepthMask(GL_FALSE);
		glEnable(GL_TEXTURE_2D);
		glDisable(GL_LIGHTING);
		glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
		EnableTransparency(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		if (m_shaderMaterials)
		{
			m_specularEffect.Deactivate();
		}
		else if(m_activeFog.active)
		{
			glEnable(GL_FOG);
		}
		break;
	case RS_SKYBOX:
		DisableTransparency();
		glDisable(GL_DEPTH_TEST);
		glDisable(GL_ALPHA_TEST);
		glDepthMask(GL_FALSE);
		glDepthFunc(GL_LESS);
		glDisable(GL_LIGHTING);
		glEnable(GL_TEXTURE_2D);
		glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
		glDisable(GL_FOG);
		if(m_shaderMaterials)
		{
			m_specularEffect.Deactivate();
		}
		break;
	case RS_SCREENPOLY:
		glEnable(GL_BLEND);
		glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
		glDisable(GL_LIGHTING);
		EnableTransparency(GL_ONE_MINUS_SRC_ALPHA, GL_SRC_ALPHA);
		glEnable(GL_TEXTURE_2D);
		glDisable(GL_ALPHA_TEST);
		glDepthMask(GL_FALSE);
		glDisable(GL_FOG);

		if(m_shaderMaterials)
		{
			m_specularEffect.Deactivate();
		}
		break;
	case RS_POSTPROCESS:
		{
			glDisable(GL_LIGHTING);
			glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
			glDisable(GL_DEPTH_TEST);
			glEnable(GL_TEXTURE_2D);
			glDepthFunc(GL_ALWAYS);
			glDisable(GL_ALPHA_TEST);
			glDisable(GL_BLEND);
			glDepthMask(GL_FALSE);
			glDisable(GL_MULTISAMPLE);
			glDisable(GL_FOG);

		}
		break;
	}
}


void GLOpenGL::Set2DRenderingInt(bool texturing, ProjectionMode projMode)
{
	MaterialGL::ResetActiveMat();
	SpecularEffect::ResetActiveMat();

	if (projMode == PM_2D_WORLD && !m_bWidescreenHackThisFrame)
	{
		projMode = PM_2D;
	}

	if( projMode == PM_2D /*&& m_renderMode != PM_2D*/ )
	{
		float srcAspect = (float)m_sourceWidth / (float)m_sourceHeight;
		float Offset = 0.0f;// (m_sourceWidth * srcAspect - m_windowWidth * (4.3f / 3.f)) * 0.5f;

		glScissor(m_2dPosOffset.x() + Offset, m_2dPosOffset.y(), m_sourceWidth*m_scale, m_sourceHeight*m_scale);
		m_2dViewport.ApplyViewport();
		glMatrixMode(GL_PROJECTION);
		glLoadMatrixf(m_2dViewport.m_proj2D.ColumnMajor());
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		glDepthFunc(GL_ALWAYS);
		glTranslatef(m_2dPosOffset.x() + Offset, m_2dPosOffset.y(), 0.0f);
		glScalef(m_scale, m_scale, 1.0f);
		m_renderMode = PM_2D;
	}
	else if (projMode == PM_2D_WORLD /*&& m_renderMode != PM_2D*/)
	{
		float srcAspect = (float)m_sourceWidth / (float)m_sourceHeight;
		float realAspect = (float)m_windowWidth / (float)m_windowHeight;
		float aspectMul = (realAspect / srcAspect);
		glScissor(0.0f, 0.0f,m_windowWidth, m_windowHeight);
		m_2dViewport.ApplyViewport();
		glMatrixMode(GL_PROJECTION);
		glLoadMatrixf(m_2dViewport.m_proj2D.ColumnMajor());
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		glDepthFunc(GL_ALWAYS);
		glTranslatef(0.0f, 0.0f, 0.0f);
		glScalef(m_scale * aspectMul, m_scale, 1.0f);
		m_renderMode = PM_2D;
	}
	else if( projMode == PM_ORTHO)
	{
		glScissor(0, 0, m_windowWidth, m_windowHeight);
		m_2dViewport.ApplyViewport();
		glDepthFunc(GL_ALWAYS);
		glMatrixMode(GL_PROJECTION);
		glLoadMatrixf(m_2dViewport.m_ortho.ColumnMajor());
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		m_renderMode = PM_ORTHO;
	}

	if( texturing )
	{
		if( !m_textured )
			glEnable(GL_TEXTURE_2D);
	}
	else
	{
		if( m_textured )
			glDisable(GL_TEXTURE_2D);
	}

	m_textured = texturing;

	assert(glGetError() == GL_NO_ERROR);

}

void GLOpenGL::DrawDelayedEffects()
{
	if(m_delayedEffects.useFlash)
	{
		ScreenFlash(&m_delayedEffects.flashFX);
	}

	m_delayedEffects.useFlash = false;
}



void GLOpenGL::SetViewNoWorld()
{

	Matrix44 identityWorld;
	identityWorld.LoadIdentity();

	if (m_shaderMaterials)
	{
		Matrix44 worldViewProj = m_viewMatrix * m_3dViewport.m_perspective;
#if ORIGIN_OFFSET
		Vector3 vEyePos(0.0, 0.0, 0.0);
#else
		Vector3 vEyePos = m_eyePos;
#endif
		m_specularEffect.SetViewInfo(identityWorld, m_viewMatrix, worldViewProj, vEyePos, m_eyeDir);
	}
	//else
	{
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glLoadMatrixf(m_3dViewport.m_perspective.ColumnMajor());

		glMatrixMode(GL_MODELVIEW);
		glLoadMatrixf(m_viewMatrix.ColumnMajor());
		glMultMatrixf(identityWorld.ColumnMajor());
	}
}


void GLOpenGL::UpdateWorldView()
{
	if(m_shaderMaterials)
	{
		Matrix44 worldViewProj = m_modelMatrix * m_viewMatrix * m_3dViewport.m_perspective;
		#if ORIGIN_OFFSET
		Vector3 vEyePos(0.0, 0.0, 0.0);
		#else
		Vector3 vEyePos = m_eyePos;
		#endif
		m_specularEffect.SetViewInfo(m_modelMatrix, m_viewMatrix, worldViewProj, vEyePos, m_eyeDir);
	}
	//else
	{
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glLoadMatrixf(m_3dViewport.m_perspective.ColumnMajor());

		glMatrixMode(GL_MODELVIEW);
		glLoadMatrixf(m_viewMatrix.ColumnMajor());
		glMultMatrixf(m_modelMatrix.ColumnMajor());
	}
	m_renderMode = PM_PERSPECTIVE;
}