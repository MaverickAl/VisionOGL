#pragma once
#include "WcpOglUtility.h"
#include <vector>
#include <string>

struct TREMainHeader
{
	DWORD	noFiles;
	DWORD	unknown1;
	DWORD	unknown2;
	DWORD	unknown3;
};

struct TRESubHeader
{
	DWORD	CRC;
	DWORD	uncompressedSize;
	DWORD	fileStartPos;
	DWORD	compressedSize;
	char	fileName[256];
};

class TreFile
{
public:
	TreFile(void);
	~TreFile(void);
	bool Init(const char* fileName);

	bool GetFile(const char* fileName, FILE** fileOut, DWORD& fileSize) const;
	uint32 GetNumFiles() const;
	bool GetFile(uint32 uFileIndex, FILE** fileOut, DWORD& fileSize, std::string& fileNameOut) const;

private:
	struct Archive
	{
		std::string				path;
		TREMainHeader			header;
		std::vector<TRESubHeader>	fileHeaders;
		FILE* pFile;
	};

	std::vector<Archive>	m_archives;
};
