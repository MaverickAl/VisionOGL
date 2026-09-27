#pragma once
#include "CommonBase.h"
#include "CommonInputStructs.h"

class Light : public CommonBase
{
public:
	LightType	lightType;
	Colour		colour;
	float		cutoffDist;
	float		cutoffDistSq;
	float		constantAttenuation;
	float		linearAttenuation;
	float		quadraticAttenuation;

	virtual ~Light(void);

	virtual	void HasVTable();
};
