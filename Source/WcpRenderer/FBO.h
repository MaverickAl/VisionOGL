#pragma once
#include "WcpOglUtility.h"

enum FBTextures
{
	FBT_COLOUR,
	FBT_DEPTH,
	FBT_STENCIL,
	FBT_NONE
};

class FBO
{
public:
	FBO();
	~FBO();

	bool Create(bool colour, bool floatingPoint, bool depth, int width, int height, int samples = 1, int depthBits = 24);
	void Bind();
	void Detach();
	void BlitMS();
	GLuint GetTexture(FBTextures textureType);
	bool IsAttached() { return m_isAttached; }
	int GetWidth() { return m_width; }
	int GetHeight() { return m_height; }
	bool GetUseRect() { return m_useRect; }

private:
	GLuint	m_frameBuffer;
	GLuint	m_copyFBO;	// For copying multisampled render target to
	GLuint	m_copyTexture;
	GLuint	m_renderBuffers[FBT_NONE];
	GLuint	m_textures[FBT_NONE];
	bool	m_isAttached;
	bool	m_isBufferDirty;
	bool	m_useRect;
	int		m_width;
	int		m_height;
	int		m_samples;
	bool	m_bInitCalled;
};