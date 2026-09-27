#include "WcpOglFile.h"

void wcpogl::ReadCommentBlock( std::istream &is )
{
	std::string attribute;
	do 
	{
		is >> attribute;
	}
	while( attribute != "*/" );
}



bool wcpogl::IsFileReadable( const std::string &fileName )
{
	FILE *pFile;
	fopen_s(&pFile, fileName.c_str(), "rb");
	if( !pFile )
	{
		return false;
	}
	else
	{
		fclose(pFile);
		return true;
	}
}



void wcpogl::AppendDirectory( std::string &fileName, const char* directory )
{
	uint fullPathLen = (uint)fileName.size() + strlen(directory) + 2;
	char* fullPath = new char[fullPathLen];
	sprintf_s(fullPath, fullPathLen, "%s/%s", directory, fileName.c_str() );
	fileName = fullPath;
	delete[] fullPath;
}
