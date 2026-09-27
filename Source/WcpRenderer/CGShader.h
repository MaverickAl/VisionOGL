#pragma once

#include "WcpOglUtility.h"
#include "GLExt.h"
#include <cg/cg.h>
#include <cg/cgGL.h>

class TreFile;

class CGShader
{
public:
	CGShader();
	~CGShader();

	CGpass	GetFirstPass(CGtechnique technique);
	CGpass	GetNextPass(CGpass prevPass);
	void BeginPass(CGpass pass);
	void EndPass(CGpass pass);

	bool LoadFromFile(const char* fileName, TreFile* treFile);
	CGtechnique GetTechiqueByName(const char* techniqueName);
	CGparameter GetVariableHandleBySemantic(const char* semantic);
	CGparameter GetVariableHandleByName(const char* name);

	static void	InitCG();
	static void ShutdownCG();

	static	CGcontext	s_cgContext;
private:
	CGeffect			m_effect;
};


inline CGpass CGShader::GetFirstPass(CGtechnique technique)
{
	return cgGetFirstPass(technique);
}

inline CGpass CGShader::GetNextPass(CGpass prevPass)
{
	return cgGetNextPass(prevPass);
}

inline void CGShader::BeginPass(CGpass pass)
{
	cgSetPassState(pass);
}

inline void CGShader::EndPass(CGpass pass)
{
	cgResetPassState(pass);
}

inline CGparameter CGShader::GetVariableHandleBySemantic(const char* semantic)
{
	return cgGetEffectParameterBySemantic(m_effect, semantic);
}

inline CGparameter CGShader::GetVariableHandleByName(const char* name)
{
	return cgGetNamedEffectParameter(m_effect, name);
}