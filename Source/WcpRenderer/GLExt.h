#pragma once
// Lightweight OpenGL/WGL function and capability loader.
// Call GLExt::Init() once, after a GL context has been created and made
// current, before using any of the functions or flags below.

#include <Windows.h>
#include <gl/GL.h>

// ---------------------------------------------------------------------------
// Enum/constant values not declared by Windows' <gl/gl.h> (GL 1.1 only).
// ---------------------------------------------------------------------------

#ifndef GL_CLAMP_TO_EDGE
#define GL_CLAMP_TO_EDGE                               0x812F
#endif
#ifndef GL_TEXTURE0
#define GL_TEXTURE0                                    0x84C0
#endif
#ifndef GL_TEXTURE1
#define GL_TEXTURE1                                    0x84C1
#endif
#ifndef GL_TEXTURE2
#define GL_TEXTURE2                                    0x84C2
#endif
#ifndef GL_MULTISAMPLE
#define GL_MULTISAMPLE                                 0x809D
#endif
#ifndef GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT
#define GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT              0x84FF
#endif
#ifndef GL_TEXTURE_MAX_ANISOTROPY_EXT
#define GL_TEXTURE_MAX_ANISOTROPY_EXT                  0x84FE
#endif
#ifndef GL_DEPTH_COMPONENT16
#define GL_DEPTH_COMPONENT16                           0x81A5
#endif
#ifndef GL_DEPTH_COMPONENT24
#define GL_DEPTH_COMPONENT24                           0x81A6
#endif
#ifndef GL_DEPTH_COMPONENT32
#define GL_DEPTH_COMPONENT32                           0x81A7
#endif
#ifndef GL_RGBA16F
#define GL_RGBA16F                                     0x881A
#endif

#ifndef GL_FRAMEBUFFER
#define GL_FRAMEBUFFER                                 0x8D40
#endif
#ifndef GL_FRAMEBUFFER_EXT
#define GL_FRAMEBUFFER_EXT                             GL_FRAMEBUFFER
#endif
#ifndef GL_READ_FRAMEBUFFER_EXT
#define GL_READ_FRAMEBUFFER_EXT                        0x8CA8
#endif
#ifndef GL_DRAW_FRAMEBUFFER_EXT
#define GL_DRAW_FRAMEBUFFER_EXT                        0x8CA9
#endif
#ifndef GL_RENDERBUFFER
#define GL_RENDERBUFFER                                0x8D41
#endif
#ifndef GL_RENDERBUFFER_EXT
#define GL_RENDERBUFFER_EXT                            GL_RENDERBUFFER
#endif
#ifndef GL_COLOR_ATTACHMENT0
#define GL_COLOR_ATTACHMENT0                           0x8CE0
#endif
#ifndef GL_DEPTH_ATTACHMENT
#define GL_DEPTH_ATTACHMENT                            0x8D00
#endif
#ifndef GL_FRAMEBUFFER_COMPLETE
#define GL_FRAMEBUFFER_COMPLETE                        0x8CD5
#endif
#ifndef GL_FRAMEBUFFER_UNSUPPORTED
#define GL_FRAMEBUFFER_UNSUPPORTED                     0x8CDD
#endif
#ifndef GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT
#define GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT           0x8CD6
#endif
#ifndef GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT
#define GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT   0x8CD7
#endif
#ifndef GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS_EXT
#define GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS_EXT       0x8CD9
#endif
#ifndef GL_FRAMEBUFFER_INCOMPLETE_FORMATS_EXT
#define GL_FRAMEBUFFER_INCOMPLETE_FORMATS_EXT          0x8CDA
#endif
#ifndef GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER
#define GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER          0x8CDB
#endif
#ifndef GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER
#define GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER          0x8CDC
#endif
#ifndef GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE
#define GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE          0x8D56
#endif

#ifndef GL_ARRAY_BUFFER
#define GL_ARRAY_BUFFER                                0x8892
#endif
#ifndef GL_ELEMENT_ARRAY_BUFFER
#define GL_ELEMENT_ARRAY_BUFFER                        0x8893
#endif
#ifndef GL_STATIC_DRAW
#define GL_STATIC_DRAW                                 0x88E4
#endif
#ifndef GL_DYNAMIC_DRAW
#define GL_DYNAMIC_DRAW                                0x88E8
#endif

#ifndef WGL_DRAW_TO_WINDOW_ARB
#define WGL_DRAW_TO_WINDOW_ARB                         0x2001
#endif
#ifndef WGL_SUPPORT_OPENGL_ARB
#define WGL_SUPPORT_OPENGL_ARB                         0x2010
#endif
#ifndef WGL_DOUBLE_BUFFER_ARB
#define WGL_DOUBLE_BUFFER_ARB                          0x2011
#endif
#ifndef WGL_PIXEL_TYPE_ARB
#define WGL_PIXEL_TYPE_ARB                             0x2013
#endif
#ifndef WGL_TYPE_RGBA_ARB
#define WGL_TYPE_RGBA_ARB                              0x202B
#endif
#ifndef WGL_ACCELERATION_ARB
#define WGL_ACCELERATION_ARB                           0x2003
#endif
#ifndef WGL_FULL_ACCELERATION_ARB
#define WGL_FULL_ACCELERATION_ARB                      0x2027
#endif
#ifndef WGL_COLOR_BITS_ARB
#define WGL_COLOR_BITS_ARB                             0x2014
#endif
#ifndef WGL_ALPHA_BITS_ARB
#define WGL_ALPHA_BITS_ARB                             0x201B
#endif
#ifndef WGL_DEPTH_BITS_ARB
#define WGL_DEPTH_BITS_ARB                             0x2022
#endif
#ifndef WGL_STENCIL_BITS_ARB
#define WGL_STENCIL_BITS_ARB                           0x2023
#endif
#ifndef WGL_SAMPLE_BUFFERS_ARB
#define WGL_SAMPLE_BUFFERS_ARB                         0x2041
#endif
#ifndef WGL_SAMPLES_ARB
#define WGL_SAMPLES_ARB                                0x2042
#endif
#ifndef WGL_CONTEXT_MAJOR_VERSION_ARB
#define WGL_CONTEXT_MAJOR_VERSION_ARB                  0x2091
#endif
#ifndef WGL_CONTEXT_MINOR_VERSION_ARB
#define WGL_CONTEXT_MINOR_VERSION_ARB                  0x2092
#endif
#ifndef WGL_CONTEXT_PROFILE_MASK_ARB
#define WGL_CONTEXT_PROFILE_MASK_ARB                   0x9126
#endif
#ifndef WGL_CONTEXT_CORE_PROFILE_BIT_ARB
#define WGL_CONTEXT_CORE_PROFILE_BIT_ARB               0x00000001
#endif
#ifndef WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB
#define WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB      0x00000002
#endif

#ifndef WGL_SWAP_METHOD_ARB
#define WGL_SWAP_METHOD_ARB      0x2007
#define WGL_SWAP_EXCHANGE_ARB    0x2028
#define WGL_SWAP_COPY_ARB        0x2029
#define WGL_SWAP_UNDEFINED_ARB   0x202A
#endif


// ---------------------------------------------------------------------------
// Function pointer typedefs
// ---------------------------------------------------------------------------

typedef void        (APIENTRY *PFNGLACTIVETEXTUREPROC)(GLenum texture);
typedef void        (APIENTRY *PFNGLMULTITEXCOORD3FVPROC)(GLenum target, const GLfloat* v);
typedef void        (APIENTRY *PFNGLGENERATEMIPMAPPROC)(GLenum target);

typedef void        (APIENTRY *PFNGLGENFRAMEBUFFERSPROC)(GLsizei n, GLuint* framebuffers);
typedef void        (APIENTRY *PFNGLDELETEFRAMEBUFFERSPROC)(GLsizei n, const GLuint* framebuffers);
typedef void        (APIENTRY *PFNGLBINDFRAMEBUFFERPROC)(GLenum target, GLuint framebuffer);
typedef GLboolean   (APIENTRY *PFNGLISFRAMEBUFFERPROC)(GLuint framebuffer);
typedef GLenum      (APIENTRY *PFNGLCHECKFRAMEBUFFERSTATUSPROC)(GLenum target);
typedef void        (APIENTRY *PFNGLFRAMEBUFFERTEXTURE2DPROC)(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level);
typedef void        (APIENTRY *PFNGLFRAMEBUFFERRENDERBUFFERPROC)(GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer);
typedef void        (APIENTRY *PFNGLBLITFRAMEBUFFERPROC)(GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter);

typedef void        (APIENTRY *PFNGLGENRENDERBUFFERSPROC)(GLsizei n, GLuint* renderbuffers);
typedef void        (APIENTRY *PFNGLDELETERENDERBUFFERSPROC)(GLsizei n, const GLuint* renderbuffers);
typedef void        (APIENTRY *PFNGLBINDRENDERBUFFERPROC)(GLenum target, GLuint renderbuffer);
typedef GLboolean   (APIENTRY *PFNGLISRENDERBUFFERPROC)(GLuint renderbuffer);
typedef void        (APIENTRY *PFNGLRENDERBUFFERSTORAGEPROC)(GLenum target, GLenum internalformat, GLsizei width, GLsizei height);
typedef void        (APIENTRY *PFNGLRENDERBUFFERSTORAGEMULTISAMPLEPROC)(GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height);

// Vertex buffer objects - core since GL 1.5, but absent from Windows' gl.h
// and near-universally needed by anything built on top of this loader.
typedef void        (APIENTRY *PFNGLGENBUFFERSPROC)(GLsizei n, GLuint* buffers);
typedef void        (APIENTRY *PFNGLDELETEBUFFERSPROC)(GLsizei n, const GLuint* buffers);
typedef void        (APIENTRY *PFNGLBINDBUFFERPROC)(GLenum target, GLuint buffer);
typedef void        (APIENTRY *PFNGLBUFFERDATAPROC)(GLenum target, ptrdiff_t size, const void* data, GLenum usage);
typedef void        (APIENTRY *PFNGLBUFFERSUBDATAPROC)(GLenum target, ptrdiff_t offset, ptrdiff_t size, const void* data);

typedef BOOL        (WINAPI *PFNWGLSWAPINTERVALEXTPROC)(int interval);
typedef BOOL        (WINAPI *PFNWGLCHOOSEPIXELFORMATARBPROC)(HDC hdc, const int* piAttribIList, const FLOAT* pfAttribFList, UINT nMaxFormats, int* piFormats, UINT* nNumFormats);
typedef BOOL        (WINAPI *PFNWGLGETPIXELFORMATATTRIBIVARBPROC)(HDC hdc, int iPixelFormat, int iLayerPlane, UINT nAttributes, const int* piAttributes, int* piValues);
typedef const char* (WINAPI *PFNWGLGETEXTENSIONSSTRINGARBPROC)(HDC hdc);
typedef HGLRC       (WINAPI *PFNWGLCREATECONTEXTATTRIBSARBPROC)(HDC hdc, HGLRC hShareContext, const int* attribList);

// ---------------------------------------------------------------------------
// Loaded function pointers
// ---------------------------------------------------------------------------

extern PFNGLACTIVETEXTUREPROC                      glActiveTexture;
extern PFNGLMULTITEXCOORD3FVPROC                   glMultiTexCoord3fv;
extern PFNGLGENERATEMIPMAPPROC                     glGenerateMipmap;

extern PFNGLGENFRAMEBUFFERSPROC                    glGenFramebuffersEXT;
extern PFNGLDELETEFRAMEBUFFERSPROC                 glDeleteFramebuffers;
extern PFNGLBINDFRAMEBUFFERPROC                    glBindFramebufferEXT;
extern PFNGLISFRAMEBUFFERPROC                      glIsFramebufferEXT;
extern PFNGLCHECKFRAMEBUFFERSTATUSPROC             glCheckFramebufferStatusEXT;
extern PFNGLFRAMEBUFFERTEXTURE2DPROC               glFramebufferTexture2DEXT;
extern PFNGLFRAMEBUFFERRENDERBUFFERPROC            glFramebufferRenderbufferEXT;
extern PFNGLBLITFRAMEBUFFERPROC                    glBlitFramebufferEXT;

extern PFNGLGENRENDERBUFFERSPROC                   glGenRenderbuffersEXT;
extern PFNGLDELETERENDERBUFFERSPROC                glDeleteRenderbuffers;
extern PFNGLBINDRENDERBUFFERPROC                   glBindRenderbufferEXT;
extern PFNGLISRENDERBUFFERPROC                     glIsRenderbufferEXT;
extern PFNGLRENDERBUFFERSTORAGEPROC                glRenderbufferStorageEXT;
extern PFNGLRENDERBUFFERSTORAGEMULTISAMPLEPROC     glRenderbufferStorageMultisampleEXT;

extern PFNGLGENBUFFERSPROC                         glGenBuffers;
extern PFNGLDELETEBUFFERSPROC                      glDeleteBuffers;
extern PFNGLBINDBUFFERPROC                         glBindBuffer;
extern PFNGLBUFFERDATAPROC                         glBufferData;
extern PFNGLBUFFERSUBDATAPROC                      glBufferSubData;

extern PFNWGLSWAPINTERVALEXTPROC                   wglSwapIntervalEXT;
extern PFNWGLCHOOSEPIXELFORMATARBPROC              wglChoosePixelFormatARB;
extern PFNWGLGETPIXELFORMATATTRIBIVARBPROC         wglGetPixelFormatAttribivARB;
extern PFNWGLGETEXTENSIONSSTRINGARBPROC            wglGetExtensionsStringARB;
extern PFNWGLCREATECONTEXTATTRIBSARBPROC           wglCreateContextAttribsARB;

// ---------------------------------------------------------------------------
// Capability / version flags
// ---------------------------------------------------------------------------

namespace GLExt
{
	extern bool Version_1_2;
	extern bool Version_2_0;

	extern bool ARB_texture_non_power_of_two;
	extern bool EXT_framebuffer_object;
	extern bool EXT_framebuffer_multisample;
	extern bool EXT_framebuffer_blit;

	// Call once, after wglMakeCurrent has succeeded on a valid GL context.
	// Loads every function pointer and sets every flag above. A missing
	// individual extension just leaves its flag false / pointers null
	// rather than failing the whole call.
	bool Init();
}
