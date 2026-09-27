#pragma once
#include "CommonInputStructs.h"

class CommonBase
{
public:
	int 		Type;		// 2=camera, 3=light
	int 		Handle;

	uint8		unknown[36];
	ProphMatrix	modelMatrix;

	virtual ~CommonBase(void);

	virtual	void VTableFunction();
};

