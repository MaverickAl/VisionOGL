#include "FBO.h"
#include "WcpOglMath.h"
#include "GLExt.h"

extern bool g_bRequirePowerOfTwo;


FBO::FBO()
{
	m_isAttached = false;
	m_isBufferDirty = true;
	m_copyFBO = 0;
	m_frameBuffer = 0;
	m_bInitCalled = false;
}

FBO::~FBO()
{
	if (!m_bInitCalled)
	{
		return;
	}
	if (m_frameBuffer)
	{
		glDeleteFramebuffers(1, &m_frameBuffer);
	}
	if(m_copyFBO)
	{
		glDeleteFramebuffers(1, &m_copyFBO);
	}
	glDeleteRenderbuffers(FBT_NONE, m_renderBuffers);
	glDeleteTextures(FBT_NONE, m_textures);
	if(m_samples>1)
		glDeleteTextures(1, &m_copyTexture);
}

bool FBO::Create(bool colour, bool floatingPoint, bool depth, int width, int height, int samples, int depthBits)
{
	// Currently assume we are rendering offscreen (the purpose of the render buffer)
	// Currently no stencil support (despite the texture/rendef buffer)
	assert(glGetError()==GL_NO_ERROR);
	glGetError();	// Remove errors in release

	if(!GLExt::EXT_framebuffer_object)
	{
		assertMsg(false, "Creating FBO - No render target support");
		return false;
	}

	if(!GLExt::Version_2_0)
	{
		assertMsg(false, "Creating FBO - OpenGL 2.0 support not available, results will be unpredictable, please update your drivers");
	}

	if(samples>1 && ((!GLExt::EXT_framebuffer_multisample) || (!GLExt::EXT_framebuffer_blit)) )
	{
		assert(false);
		assertMsg(false, "Creating FBO - No multisample floating point support");
		samples = 1;
	}

	if(g_bRequirePowerOfTwo)
	{
		//assertMsg(false, "Creating FBO - Non power of two textures not avaialble, may be slow");
		// This is going to be slow
		width	=	wcpogl::FindNextPowerOfTwo(width);
		height	=	wcpogl::FindNextPowerOfTwo(height);
		// 8 samples will be insanely slow
		samples = wcpogl::clampi(samples, 1, 16);
	}

	m_samples = samples;
	m_width = width;
	m_height = height;

	int glError = glGetError();
	glGenFramebuffersEXT(1, &m_frameBuffer);
	glError = glGetError();
	glGenRenderbuffersEXT(FBT_NONE, m_renderBuffers);
	glError = glGetError();
	glGenTextures(FBT_NONE, m_textures);
	glError = glGetError();
	glBindFramebufferEXT(GL_FRAMEBUFFER, m_frameBuffer);
	glError = glGetError();
	glEnable(GL_TEXTURE_2D);
	glError = glGetError();

	if(colour)
	{
		// TODO: Are assuming only rendering to texture, no renderbuffer
		glBindTexture(GL_TEXTURE_2D, m_textures[FBT_COLOUR]);

		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

		int internalFormat = floatingPoint ? GL_RGBA16F : GL_RGBA8;
		if(floatingPoint)
		{
			glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, GL_RGBA, GL_FLOAT, NULL);
		}
		else
		{
			glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
		}
		glFramebufferTexture2DEXT(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_textures[FBT_COLOUR], 0);
		assert(glGetError()==GL_NO_ERROR);
		if(m_samples>1)
		{
			glBindRenderbufferEXT(GL_RENDERBUFFER, m_renderBuffers[FBT_COLOUR]);
			glRenderbufferStorageMultisampleEXT(GL_RENDERBUFFER, m_samples, internalFormat, width, height);
			glFramebufferRenderbufferEXT(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, m_renderBuffers[FBT_COLOUR]);

			if (floatingPoint)
			{
				GLenum msStatus = glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
				if (msStatus != GL_FRAMEBUFFER_COMPLETE)
				{
					internalFormat = GL_RGBA8;
					glRenderbufferStorageMultisampleEXT(GL_RENDERBUFFER, m_samples, internalFormat, width, height);
					glFramebufferRenderbufferEXT(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, m_renderBuffers[FBT_COLOUR]);
				}
			}
		}
		
		assert(glGetError()==GL_NO_ERROR);
		

	}
	if(depth)
	{
		glBindRenderbufferEXT(GL_RENDERBUFFER, m_renderBuffers[FBT_DEPTH]);
		GLuint depthFormat = GL_DEPTH_COMPONENT24;
		if (depthBits == 16)
		{
			depthFormat = GL_DEPTH_COMPONENT16;
		}
		else if (depthBits == 32)
		{
			depthFormat = GL_DEPTH_COMPONENT32;
		}
		if(m_samples>1)
		{
			glRenderbufferStorageMultisampleEXT(GL_RENDERBUFFER, samples, depthFormat, width, height);
		}
		else
		{
			glRenderbufferStorageEXT(GL_RENDERBUFFER, depthFormat, width, height);
		}
		glFramebufferRenderbufferEXT(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, m_renderBuffers[FBT_DEPTH]);
		assert(glIsRenderbufferEXT(m_renderBuffers[FBT_DEPTH])==GL_TRUE);
		assert(glGetError()==GL_NO_ERROR);
	}

	assert(glIsFramebufferEXT(m_frameBuffer)==GL_TRUE);
	assert(glGetError()==GL_NO_ERROR);

	if(m_samples>1)
	{
		glGenFramebuffersEXT(1, &m_copyFBO);
		glGenTextures(1, &m_copyTexture);
		glBindFramebufferEXT(GL_FRAMEBUFFER, m_copyFBO);
		glBindTexture(GL_TEXTURE_2D , m_copyTexture);
		glTexParameteri(GL_TEXTURE_2D ,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D ,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D , GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D , GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

		if(floatingPoint)
			glTexImage2D(GL_TEXTURE_2D , 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, NULL);
		else
			glTexImage2D(GL_TEXTURE_2D , 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
		
		glFramebufferTexture2DEXT(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D , m_copyTexture, 0);
		assert(glIsFramebufferEXT(m_copyFBO)==GL_TRUE);
		assert(glGetError()==GL_NO_ERROR);
	}

	glDisable(GL_TEXTURE_2D);

	// Unbind, just for safety
	glBindRenderbufferEXT(GL_RENDERBUFFER, 0);
	glBindFramebufferEXT(GL_FRAMEBUFFER, 0);


	GLenum fboStatus = glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
	if(fboStatus != GL_FRAMEBUFFER_COMPLETE)
	{
		switch(fboStatus)
		{
		 case GL_FRAMEBUFFER_UNSUPPORTED:
			 assertMsg(false, "Creating FBO - framebuffer configuration unsupported");
			break;
		 case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
			 assertMsg(false, "Creating FBO - framebuffer incomplete (incomplete attachment)");
            break;
		 case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
			 assertMsg(false, "Creating FBO - framebuffer incomplete (missing attachment)" );
			break;
		 case GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS_EXT:
			 assertMsg(false, "Creating FBO - framebuffer incomplete (dimension mismatch)" );
			break;
		 case GL_FRAMEBUFFER_INCOMPLETE_FORMATS_EXT:
			 assertMsg(false, "Creating FBO - framebuffer incomplete (format mismatch)");
			break;
		 case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER:
			 assertMsg(false, "Creating FBO - framebuffer incomplete (draw buffer(s) have no attachment)");
			break;
		 case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER:
			 assertMsg(false, "Creating FBO - framebuffer incomplete (read buffer has no attachment)");
			break;
		 case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE:
			 assertMsg(false, "Creating FBO - framebuffer incomplete (multisample counts don't match)");
			break;
		 default:
			 assertMsg(false, "Creating FBO - unknown framebuffer error");
		}
	}

	m_bInitCalled = true;

	GLenum error = glGetError();
	if(error!=GL_NO_ERROR )
	{
		// Just incase something went wrong on those pesky ATI's
		char errorMsg[256];
		sprintf(errorMsg, "Failed: Enum Error: %i", error);
		assertMsg(false, errorMsg);
		return false;
	}

	return true;
}

void FBO::Bind()
{
	glBindFramebufferEXT(GL_FRAMEBUFFER, m_frameBuffer);
	m_isAttached = true;
}

void FBO::Detach()
{
	glBindFramebufferEXT(GL_FRAMEBUFFER, 0);
	m_isAttached = false;
	m_isBufferDirty = true;
}

void FBO::BlitMS()
{
	if(m_samples>1)
	{
		glBindFramebufferEXT(GL_READ_FRAMEBUFFER_EXT, m_frameBuffer);
		glBindFramebufferEXT(GL_DRAW_FRAMEBUFFER_EXT, m_copyFBO);
		// Did both at once before, intel not a fan?
		glBlitFramebufferEXT(0, 0, m_width, m_height,
			0, 0, m_width, m_height,
			GL_COLOR_BUFFER_BIT,
			GL_NEAREST);
		glBlitFramebufferEXT(0, 0, m_width, m_height,
			0, 0, m_width, m_height,
			GL_DEPTH_BUFFER_BIT,
			GL_NEAREST);
	}
	glBindFramebufferEXT(GL_READ_FRAMEBUFFER_EXT, 0);
	glBindFramebufferEXT(GL_DRAW_FRAMEBUFFER_EXT, 0);
	m_isAttached = false;

}

GLuint FBO::GetTexture(FBTextures textureType)
{
	if(textureType == FBT_COLOUR && m_samples>1)
	{
		// Can't access render buffer texture directly, blit it
		return m_copyTexture;
	}
	else
	{
		return m_textures[textureType];
	}
}