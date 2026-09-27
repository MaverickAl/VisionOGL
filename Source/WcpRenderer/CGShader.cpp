#include "CGShader.h"
#include "TreFile.h"
#include "WcpOglFile.h"

CGcontext CGShader::s_cgContext;

CGShader::CGShader()
{
	m_effect = NULL;
}

CGShader::~CGShader()
{
	if(m_effect)
	{
		cgDestroyEffect(m_effect);
	}
}


CGtechnique CGShader::GetTechiqueByName(const char* techniqueName)
{
	return cgGetNamedTechnique(m_effect, techniqueName);
}

bool CGShader::LoadFromFile(const char* fileName, TreFile* treFile)
{
	if(wcpogl::IsFileReadable(fileName))
	{
		m_effect = cgCreateEffectFromFile(s_cgContext, fileName, NULL);
	}
	else
	{
		FILE* file = NULL;
		DWORD fileSize;
		if( treFile->GetFile(fileName, &file, fileSize) )
		{
			char* shader = new char[fileSize+1];
			fread(shader, 1, fileSize, file);
			shader[fileSize] = '\0';	// Just to be safe
			m_effect = cgCreateEffect(s_cgContext, shader, NULL);
			delete[] shader;
		}
	}

	const char* compileError = cgGetLastListing(s_cgContext);
	if(!m_effect)
	{
		return false;
	}

	return true;
}

void __cdecl cgErrorCallback()
{
	CGerror cgError = cgGetError();
	const char* errorString = cgGetErrorString( cgError );
	//assert(false);
	//::MessageBox(0, errorString, "cgCallBack: Shader compile warnings", 0);
}


void CGShader::InitCG()
{
	s_cgContext = cgCreateContext();
#ifdef _DEBUG
	cgSetErrorCallback( cgErrorCallback );
	cgGLSetDebugMode( CG_TRUE );
#else
	cgGLSetDebugMode( CG_FALSE );
#endif
	cgSetParameterSettingMode(s_cgContext, CG_DEFERRED_PARAMETER_SETTING);
	cgGLRegisterStates(s_cgContext);
	cgGLSetManageTextureParameters(s_cgContext, CG_TRUE);

}

void CGShader::ShutdownCG()
{
	cgDestroyContext(s_cgContext);
}