#pragma once
#include "CommonBase.h"

class Camera : public CommonBase
{
public:
	int			viewLeft;
	int			viewTop;
	int			viewRight;
	int			viewBottom;
	int			halfWidth;
	int			halfHeight;
	int			posX;
	int			posY;
	float		nearDist;
	float		farDist;
	float		fov;

	float		oneOverFrustumWidth;
	float		oneOverFrustumHeight;

	float		unknown1;
	float		unknown2;

	Vector3		viewNormals[6];
	float		viewDistances[6];

	int			number1;
	int			number2;

	void*		pointer1;
	void*		pointer2;
};
