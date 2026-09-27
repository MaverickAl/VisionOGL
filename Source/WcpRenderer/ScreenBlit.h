#include "WcpOglUtility.h"
#include "Vector4.h"
#include "CGShader.h"
#include "FBO.h"

class Texture;

class ScreenBlit
{
public:
	ScreenBlit();
	~ScreenBlit();

	bool Init(TreFile* pTreFile);
	void Draw(FBO* pScreenFBO);

private:
	CGShader		m_shader;

	CGtechnique		m_technique;
	CGpass			m_pass;
	CGparameter		m_texHndl;
};