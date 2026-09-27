#pragma once
#include "WcpOglPreComp.h"
#include "Vector3.h"

namespace wcpogl
{

	const float pi = 3.14159265f;

	inline float RadToDeg( float angle )
	{
		return ( ( 180.0f/pi ) * angle );	
	}

	inline int FindNextPowerOfTwo(int value)
	{
		int powerOfTwo = 1;
		for(;;)
		{
			if(powerOfTwo>=value)
			{
				break;
			}
			powerOfTwo*=2;
		}
		return powerOfTwo;
	}

	inline int RoundUpToMultiple(int value, int multiple)
	{
		if (multiple == 0)
			return value;

		int remainder = value % multiple;
		if (remainder == 0)
			return value;

		return value + (multiple - remainder);
	}

	inline float clampf(float x, float min, float max)
	{
		return (x<min  ? min : x<max ? x : max);
	}

	inline int clampi(int x, int min, int max)
	{
		return (x<min  ? min : x<max ? x : max);
	}

}