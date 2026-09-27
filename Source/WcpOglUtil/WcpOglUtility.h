#ifndef _WCPOGL_UTILITY
#define	_WCPOGL_UTILITY
#include "WcpOglPreComp.h"
#include "WcpOglAssert.h"
#include <Windows.h>
#include "WcpOglMath.h"
#include "Colour.h"
#include <vector>
#include <list>
#include <map>

#define EDER_TEST_BUILD 0

typedef std::vector<int>		intVector;
typedef std::vector<float>		floatVector;


// ProphecyDLL specific
inline uint GetPaletteTag(const uint8* palette)
{
	// RGB = 24bit uint
	uint out =	((palette[0] << 16) & 0x00FF0000);
	out +=		((palette[1] << 8 ) & 0x0000FF00);
	out +=		palette[2];

	return out;
}


namespace wcpogl
{
	template<class ObjectType>
	void SafeDelete( ObjectType** deletionObject )
	{
		if( deletionObject!= NULL )
		{
			delete *deletionObject;
			*deletionObject = NULL;
		}
	}

	template<class ObjectType>
	void SafeArrayDelete( ObjectType** deletionArray )
	{
		if( *deletionArray != NULL )
		{
			delete[] *deletionArray;
			*deletionArray = NULL;
		}
	}

	template<class ObjectType>
	void SafeRelease( ObjectType** deletionObj )
	{
		if( *deletionObj )
		{
			(*deletionObj)->Release();
			*deletionObj = NULL;
		}
	}

	template<class VectorContents>
	void DeleteStlVector(std::vector<VectorContents*> &lDeletionVector)
	{
		for(uint i=0; i < lDeletionVector.size(); i++ )
		{
			delete lDeletionVector[i];
			lDeletionVector[i] = 0;
		}
		lDeletionVector.clear();
	}

	template<class ListContents>
	void DeleteStlList(std::list<ListContents*> &deletionList)
	{
		while (deletionList.size() > 0)
		{
			wcpogl::SafeDelete( deletionList.front() );
			deletionList.pop_front();
		} 
	}

	template<class MapContents, class MapIdType>
	inline bool StlMapHasKey( const std::map<MapIdType, MapContents> &searchMap, const MapIdType &key  )
	{
		return( !( searchMap.find(key ) == searchMap.end() ) );
	}

	template<class MapContents, class MapIdType>
	inline bool StlMapHasKey( const std::map<MapIdType, MapContents> &searchMap, MapIdType &key  )
	{
		return( !( searchMap.find(key) == searchMap.end() ) );
	}

	// We return a pointer to the contents as we have no idea what the size of the stored object might be
	template<class MapContents, class MapIdType>
	inline bool GetMapItemByKey( const std::map<MapIdType, MapContents> &searchMap, const MapIdType &key, MapContents* out  )
	{
		std::map<MapIdType, MapContents>::const_iterator itemPos = searchMap.find(key);
		if( itemPos != searchMap.end() )
		{
			*out = (*itemPos).second;
			return true;
		}
		return false;
	}

	// We return a pointer to the contents as we have no idea what the size of the stored object might be
	template<class MapContents, class MapIdType>
	inline bool GetMapValPointerByKey( const std::map<MapIdType, MapContents*> &searchMap, const MapIdType &key, const MapContents** out  )
	{
		std::map<MapIdType, MapContents*>::const_iterator itemPos = searchMap.find(key);
		if( itemPos != searchMap.end() )
		{
			*out = (*itemPos).second;
			return true;
		}
		return false;
	}


	template<class MapContents, class MapIdType>
	void DeleteStlMap( std::map<MapIdType, MapContents*> &lDeletionMap )
	{
		map<MapIdType, MapContents*>::iterator itr;
		if( lDeletionMap.empty() )
		{
			return;
		}

		for( itr = lDeletionMap.begin(); itr != lDeletionMap.end(); ++itr )
		{
			delete (*itr).second;
			(*itr).second = 0;
		}
		lDeletionMap.clear();
	}
	


}

#endif