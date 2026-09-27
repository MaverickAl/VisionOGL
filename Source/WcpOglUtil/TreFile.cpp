#include "TreFile.h"
#include <stdio.h>
#include <algorithm>

TreFile::TreFile(void)
{
}

TreFile::~TreFile(void)
{
	for (size_t i = 0; i < m_archives.size(); ++i)
	{
		if (m_archives[i].pFile)
		{
			fclose(m_archives[i].pFile);
		}
	}
}

bool TreFile::Init(const char* fileName)
{
	for (size_t i = 0; i < m_archives.size(); ++i)
	{
		if (m_archives[i].path == fileName)
		{
			return true;
		}
	}

	FILE* pFile = NULL;
	fopen_s(&pFile, fileName, "rb");

	if (!pFile)
	{
		return false;
	}

	Archive archive;
	archive.path = fileName;
	archive.pFile = pFile;

	const DWORD kMaxPlausibleFileCount = 1000000; 

	if (fread(&archive.header, sizeof(TREMainHeader), 1, archive.pFile) != 1)
	{
		fclose(pFile);
		return false;
	}

	if (archive.header.noFiles > kMaxPlausibleFileCount)
	{
		fclose(pFile);
		return false;
	}

	archive.fileHeaders.resize(archive.header.noFiles);
	if (archive.header.noFiles > 0)
	{
		size_t numRead = fread(&archive.fileHeaders[0], sizeof(TRESubHeader), archive.header.noFiles, archive.pFile);
		if (numRead != archive.header.noFiles)
		{
			fclose(pFile);
			return false;
		}

		// Terminate filename for safety
		for (size_t i = 0; i < archive.fileHeaders.size(); ++i)
		{
			archive.fileHeaders[i].fileName[255] = '\0';
		}
	}

	m_archives.push_back(archive);

	return true;
}

bool TreFile::GetFile(const char* fileName, FILE** fileOut, DWORD& fileSize) const
{
	for (int a = (int)m_archives.size() - 1; a >= 0; --a)
	{
		const Archive& archive = m_archives[a];

		for (DWORD i = 0; i < archive.header.noFiles; ++i)
		{
			if (0 == strcmp(archive.fileHeaders[i].fileName, fileName))
			{
				fseek(archive.pFile, archive.fileHeaders[i].fileStartPos, SEEK_SET);
				*fileOut = archive.pFile;
				fileSize = archive.fileHeaders[i].uncompressedSize;
				return true;
			}
		}
	}
	return false;
}

uint32 TreFile::GetNumFiles() const
{
	uint32 total = 0;
	for (size_t a = 0; a < m_archives.size(); ++a)
	{
		total += m_archives[a].header.noFiles;
	}
	return total;
}

bool TreFile::GetFile(uint32 uFileIndex, FILE** fileOut, DWORD& fileSize, std::string& fileNameOut) const
{
	for (size_t a = 0; a < m_archives.size(); ++a)
	{
		const Archive& archive = m_archives[a];

		if (uFileIndex < archive.header.noFiles)
		{
			fseek(archive.pFile, archive.fileHeaders[uFileIndex].fileStartPos, SEEK_SET);
			*fileOut = archive.pFile;
			fileSize = archive.fileHeaders[uFileIndex].uncompressedSize;
			fileNameOut = archive.fileHeaders[uFileIndex].fileName;
			return true;
		}

		uFileIndex -= archive.header.noFiles;
	}
	return false;
}

