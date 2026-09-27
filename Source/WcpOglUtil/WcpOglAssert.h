#pragma once
#include <intrin.h>
#include <assert.h>
#include <string>
#include <windows.h>


#ifdef _DEBUG	

#define assertIf( condition )		\
	assert( condition );			\
	if( condition )					

#else

#define assertIf( condition )		\
	if( condition )					

#endif


inline void assertMsg(bool condition, const std::string &msg)
{
#ifdef _DEBUG
	if(! condition )
	{
		ShowCursor( true );
		fprintf(stderr, "%s", msg.c_str());
		__debugbreak();

	}
#else
	(void)0;
#endif
}
