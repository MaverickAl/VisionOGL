#ifndef _WCPOGL_TEXTUREMGR
#define _WCPOGL_TEXTUREMGR
#include "WcpOglUtility.h"
#include "Materials.h"
#include "Texture.h"
#include "TreFile.h"

class TextureMgr
{
public:
	static void Init();
	static TextureMgr* Inst();
	static void Cleanup();

	TextureMgr();
	~TextureMgr();

	void PreLoadAllTexturesInDirectory(const char* szDirName);
	void PreloadAllTexturesInTre(TreFile* treFile);
	Texture* GetTexture(TreFile* treFile, const char* szName);

private:

	map<std::string, Texture*> m_textures;
};



#endif