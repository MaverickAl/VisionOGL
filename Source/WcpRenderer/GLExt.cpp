#include "GLExt.h"
#include <cstdio>
#include <cstring>

PFNGLACTIVETEXTUREPROC                      glActiveTexture = NULL;
PFNGLMULTITEXCOORD3FVPROC                   glMultiTexCoord3fv = NULL;
PFNGLGENERATEMIPMAPPROC                     glGenerateMipmap = NULL;

PFNGLGENFRAMEBUFFERSPROC                    glGenFramebuffersEXT = NULL;
PFNGLDELETEFRAMEBUFFERSPROC                 glDeleteFramebuffers = NULL;
PFNGLBINDFRAMEBUFFERPROC                    glBindFramebufferEXT = NULL;
PFNGLISFRAMEBUFFERPROC                      glIsFramebufferEXT = NULL;
PFNGLCHECKFRAMEBUFFERSTATUSPROC             glCheckFramebufferStatusEXT = NULL;
PFNGLFRAMEBUFFERTEXTURE2DPROC               glFramebufferTexture2DEXT = NULL;
PFNGLFRAMEBUFFERRENDERBUFFERPROC            glFramebufferRenderbufferEXT = NULL;
PFNGLBLITFRAMEBUFFERPROC                    glBlitFramebufferEXT = NULL;

PFNGLGENRENDERBUFFERSPROC                   glGenRenderbuffersEXT = NULL;
PFNGLDELETERENDERBUFFERSPROC                glDeleteRenderbuffers = NULL;
PFNGLBINDRENDERBUFFERPROC                   glBindRenderbufferEXT = NULL;
PFNGLISRENDERBUFFERPROC                     glIsRenderbufferEXT = NULL;
PFNGLRENDERBUFFERSTORAGEPROC                glRenderbufferStorageEXT = NULL;
PFNGLRENDERBUFFERSTORAGEMULTISAMPLEPROC     glRenderbufferStorageMultisampleEXT = NULL;

PFNGLGENBUFFERSPROC                         glGenBuffers = NULL;
PFNGLDELETEBUFFERSPROC                      glDeleteBuffers = NULL;
PFNGLBINDBUFFERPROC                         glBindBuffer = NULL;
PFNGLBUFFERDATAPROC                         glBufferData = NULL;
PFNGLBUFFERSUBDATAPROC                      glBufferSubData = NULL;

PFNWGLSWAPINTERVALEXTPROC                   wglSwapIntervalEXT = NULL;
PFNWGLCHOOSEPIXELFORMATARBPROC              wglChoosePixelFormatARB = NULL;
PFNWGLGETPIXELFORMATATTRIBIVARBPROC         wglGetPixelFormatAttribivARB = NULL;
PFNWGLGETEXTENSIONSSTRINGARBPROC            wglGetExtensionsStringARB = NULL;
PFNWGLCREATECONTEXTATTRIBSARBPROC           wglCreateContextAttribsARB = NULL;

namespace GLExt
{
	bool Version_1_2 = false;
	bool Version_2_0 = false;
	bool ARB_texture_non_power_of_two = false;
	bool EXT_framebuffer_object = false;
	bool EXT_framebuffer_multisample = false;
	bool EXT_framebuffer_blit = false;
}

namespace
{
	// Whole-token search of a space-separated extension string, so
	// "GL_EXT_framebuffer_object" doesn't false-match a longer extension
	// name that merely starts with the same text.
	bool HasExtension(const char* extensions, const char* name)
	{
		if (!extensions || !name)
			return false;

		size_t nameLen = strlen(name);
		const char* start = extensions;
		for (;;)
		{
			const char* where = strstr(start, name);
			if (!where)
				return false;

			const char* terminator = where + nameLen;
			bool leftOk  = (where == extensions) || (*(where - 1) == ' ');
			bool rightOk = (*terminator == ' ' || *terminator == '\0');
			if (leftOk && rightOk)
				return true;

			start = terminator;
		}
	}

	template <class T>
	bool LoadProc(T& outFunc, const char* name)
	{
		outFunc = reinterpret_cast<T>(wglGetProcAddress(name));
		return outFunc != NULL;
	}
}

bool GLExt::Init()
{
	Version_1_2 = false;
	Version_2_0 = false;
	const char* versionStr = reinterpret_cast<const char*>(glGetString(GL_VERSION));
	int major = 0, minor = 0;
	if (versionStr && sscanf(versionStr, "%d.%d", &major, &minor) == 2)
	{
		if (major > 1 || (major == 1 && minor >= 2)) Version_1_2 = true;
		if (major >= 2)                              Version_2_0 = true;
	}

	const char* extensions = reinterpret_cast<const char*>(glGetString(GL_EXTENSIONS));

	ARB_texture_non_power_of_two = HasExtension(extensions, "GL_ARB_texture_non_power_of_two");
	EXT_framebuffer_object       = HasExtension(extensions, "GL_EXT_framebuffer_object");
	EXT_framebuffer_multisample  = HasExtension(extensions, "GL_EXT_framebuffer_multisample");
	EXT_framebuffer_blit         = HasExtension(extensions, "GL_EXT_framebuffer_blit");

	LoadProc(glActiveTexture,    "glActiveTexture");
	LoadProc(glMultiTexCoord3fv, "glMultiTexCoord3fv");

	// Promoted to core as glGenerateMipmap in GL 3.0; try core name first,
	// fall back to the EXT_framebuffer_object entry point on older drivers.
	if (!LoadProc(glGenerateMipmap, "glGenerateMipmap"))
		LoadProc(glGenerateMipmap, "glGenerateMipmapEXT");

	LoadProc(glGenBuffers,    "glGenBuffers");
	LoadProc(glDeleteBuffers, "glDeleteBuffers");
	LoadProc(glBindBuffer,    "glBindBuffer");
	LoadProc(glBufferData,    "glBufferData");
	LoadProc(glBufferSubData, "glBufferSubData");

	if (EXT_framebuffer_object)
	{
		LoadProc(glGenFramebuffersEXT,         "glGenFramebuffersEXT");
		LoadProc(glBindFramebufferEXT,         "glBindFramebufferEXT");
		LoadProc(glIsFramebufferEXT,           "glIsFramebufferEXT");
		LoadProc(glCheckFramebufferStatusEXT,  "glCheckFramebufferStatusEXT");
		LoadProc(glFramebufferTexture2DEXT,    "glFramebufferTexture2DEXT");
		LoadProc(glFramebufferRenderbufferEXT, "glFramebufferRenderbufferEXT");

		LoadProc(glGenRenderbuffersEXT,   "glGenRenderbuffersEXT");
		LoadProc(glBindRenderbufferEXT,   "glBindRenderbufferEXT");
		LoadProc(glIsRenderbufferEXT,     "glIsRenderbufferEXT");
		LoadProc(glRenderbufferStorageEXT,"glRenderbufferStorageEXT");

		if (!LoadProc(glDeleteFramebuffers, "glDeleteFramebuffers"))
			LoadProc(glDeleteFramebuffers, "glDeleteFramebuffersEXT");
		if (!LoadProc(glDeleteRenderbuffers, "glDeleteRenderbuffers"))
			LoadProc(glDeleteRenderbuffers, "glDeleteRenderbuffersEXT");

		// A driver can advertise the extension string but fail to resolve
		// one of its entry points - don't leave the flag true if that
		// happens, since every FBO call site branches on it.
		if (!glGenFramebuffersEXT || !glBindFramebufferEXT || !glIsFramebufferEXT ||
			!glCheckFramebufferStatusEXT || !glFramebufferTexture2DEXT ||
			!glFramebufferRenderbufferEXT || !glGenRenderbuffersEXT ||
			!glBindRenderbufferEXT || !glIsRenderbufferEXT ||
			!glRenderbufferStorageEXT || !glDeleteFramebuffers || !glDeleteRenderbuffers)
		{
			EXT_framebuffer_object = false;
		}
	}

	if (EXT_framebuffer_multisample)
	{
		LoadProc(glRenderbufferStorageMultisampleEXT, "glRenderbufferStorageMultisampleEXT");
		if (!glRenderbufferStorageMultisampleEXT)
			EXT_framebuffer_multisample = false;
	}

	if (EXT_framebuffer_blit)
	{
		LoadProc(glBlitFramebufferEXT, "glBlitFramebufferEXT");
		if (!glBlitFramebufferEXT)
			EXT_framebuffer_blit = false;
	}

	LoadProc(wglSwapIntervalEXT,          "wglSwapIntervalEXT");
	LoadProc(wglChoosePixelFormatARB,     "wglChoosePixelFormatARB");
	LoadProc(wglGetPixelFormatAttribivARB,"wglGetPixelFormatAttribivARB");
	LoadProc(wglGetExtensionsStringARB,   "wglGetExtensionsStringARB");
	LoadProc(wglCreateContextAttribsARB,  "wglCreateContextAttribsARB");

	return true;
}
