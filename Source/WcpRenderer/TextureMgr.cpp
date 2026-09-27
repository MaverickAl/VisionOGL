#include "TextureMgr.h"
#include "WcpOglFile.h"


TextureMgr* g_textureMgr = nullptr;

void TextureMgr::Init()
{
	assert(g_textureMgr == nullptr);
	g_textureMgr = new TextureMgr();
}

TextureMgr* TextureMgr::Inst()
{
	return g_textureMgr;
}


void TextureMgr::Cleanup()
{
	delete g_textureMgr;
	g_textureMgr = nullptr;
}

TextureMgr::TextureMgr()
{


}

TextureMgr::~TextureMgr()
{
	for (auto& itr : m_textures)
	{
		delete itr.second;
	}
}

void TextureMgr::PreLoadAllTexturesInDirectory(const char* szDirName)
{
	WIN32_FIND_DATA findFileData;
	HANDLE hFind;
	std::string fullPath = szDirName;
	fullPath += "/*";
	hFind = FindFirstFile(fullPath.c_str(), &findFileData);
	if (hFind == INVALID_HANDLE_VALUE)
	{
		return;
	}
	size_t entryCount = 0;

	do
	{
		if (findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			continue;
		}

		std::string texPath = szDirName;
		texPath += "\\";
		texPath += findFileData.cFileName;
		Texture* pTexture = new Texture;
		if (pTexture->LoadFromTga(texPath.c_str()))
		{
			m_textures[texPath.c_str()] = pTexture;
		}
		else
		{
			delete pTexture;
		}

		entryCount++;
	} while (FindNextFile(hFind, &findFileData) != 0);

	FindClose(hFind);
}



void TextureMgr::PreloadAllTexturesInTre(TreFile* pTreFile)
{
	std::string fileName;
	for (uint32 i = 0; i < pTreFile->GetNumFiles(); i++)
	{
		FILE* pFile = nullptr;
		DWORD fileSize;
		pTreFile->GetFile(i, &pFile, fileSize, fileName);
		auto val = m_textures.find(fileName);
		if (val != m_textures.end())
		{
			// We already got it from a loose file
			continue;
		}
		if (fileName.size() >= 3 && strcmp(fileName.c_str() + (fileName.size() - 3), "tga") == 0)
		{
			if (pFile)
			{
				Texture* pTexture = new Texture;
				if (pTexture->LoadFromTga(pFile, fileSize))
				{
					m_textures[fileName.c_str()] = pTexture;
				}
				else
				{
					delete pTexture;
				}
			}
		}
	}
}

Texture* TextureMgr::GetTexture(TreFile* treFile, const char* szName)
{
	Texture* pTexture = nullptr;
	auto val =  m_textures.find(szName);
	if (val != m_textures.end())
	{
		pTexture = val->second;
	}
	if (!pTexture)
	{
		if (wcpogl::IsFileReadable(szName))
		{
			pTexture = new Texture;
			pTexture->LoadFromTga(szName);
		}
		else
		{
			DWORD fileSize;
			FILE* file = nullptr;
			treFile->GetFile(szName, &file, fileSize);
			if (file)
			{
				pTexture = new Texture;
				pTexture->LoadFromTga(file, fileSize);
			}
		}

		if (pTexture)
		{
			m_textures[szName] = pTexture;
		}
	}

	return pTexture;
}

