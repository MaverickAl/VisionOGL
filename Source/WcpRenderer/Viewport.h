#ifndef _WCPOGL_Viewport_OGL
#define _WCPOGL_Viewport_OGL
#include "WcpOglUtility.h"
#include "GLExt.h"
#include "RenderingConstants.h"
#include "Matrix44.h"

class Viewport
{
public:
	Viewport(void): m_isSet(false), m_isScissorSet(false) { }

	Viewport(GLint left, GLint bottom, GLint width, GLint height)
	{
		InitViewport(left, bottom, width, height);
	}

	~Viewport();

	inline void
	InitViewport(
		GLint left, GLint bottom, GLint width, GLint height);

	inline void		SetFov(float fov) { m_fov = fov; }

	inline void		SetScissor(int left, int bottom, int width, int height)
	{
		m_scissorBottom = bottom;
		m_scissorLeft = left;
		m_scissorWidth = width;
		m_scissorHeight = height;
		m_isScissorSet = true;
	}

	inline void		ApplyViewport(float scale = 1.0f, Vector3 &offset = Vector3(0.0f, 0.0f, 0.0f) );

	inline void		ApplyScissor(float scale = 1.0f, Vector3 &offset = Vector3(0.0f, 0.0f, 0.0f) );

	inline float	GetAspect();

	inline GLint	GetWidth() { return m_width; }


	// Projection variables, public because we want the renderer to keep track of them
	Matrix44	m_perspective;
	Matrix44	m_ortho;
	Matrix44	m_proj2D;

private:

	float		m_aspect;
	float		m_fov;

	// Viewport variables
	GLint		m_bottom;
	GLint		m_left;
	GLint		m_width;
	GLint		m_height;

	// Scissor variables
	GLint		m_scissorBottom;
	GLint		m_scissorLeft;
	GLint		m_scissorWidth;
	GLint		m_scissorHeight;

	bool		m_isSet;
	bool		m_isScissorSet;
	
};


inline void
Viewport::InitViewport(
	GLint left, GLint bottom, GLint width, GLint height)
{
	m_bottom = bottom;
	m_left = left;
	m_width = width;
	m_height = height;
	
	m_aspect = ((float)(width)) / (height);

	m_isSet = true;
	if (!m_isScissorSet)
	{
		SetScissor(left, bottom, width, height);
	}
}

inline float
Viewport::GetAspect()
{
	return m_aspect;
}

inline void
Viewport::ApplyViewport(float scale, Vector3 &offset)
{
	assert(glGetError() == GL_NO_ERROR);
	glViewport((float)(m_left+offset.x())*scale, (float)(m_bottom+offset.y())*scale, (float)(m_width*scale), (m_height*scale));
	assert(glGetError() == GL_NO_ERROR);

}

inline void
Viewport::ApplyScissor(float scale, Vector3 &offset)
{
	assert(glGetError() == GL_NO_ERROR);
	glScissor((m_scissorLeft+offset.x())*scale, (m_scissorBottom+offset.y())*scale, (m_scissorWidth*scale), (m_scissorHeight*scale));
	assert(glGetError() == GL_NO_ERROR);
}

#endif