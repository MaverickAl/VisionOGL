#pragma once

class Palette
{
public:
	virtual	void HasVTable();

	char*		name;	// Usually null
	Palette*	chain;	

	int         int1;
	int 		int2;

	int			colourCount;	// 256
	uint8*		data;		// brightness adjusted
	uint8*		rawData;	// unadjusted values

	void*		pPointer1;
};


class TextureMaterial
{
public:
	int					unknown[3];
	TextureMaterial*	chain;

	int					unknown4;
	void*				matGL;
	uint8				red;
	uint8				green;
	uint8				blue;
	uint8				alpha;
	int					width;
	int					height;
	uint8*				rgbMap;
	uint8*				alphaMap;
	const Palette*		palette;
	bool				transparency;
	bool				translucency;

	virtual ~TextureMaterial();
	virtual void					VirFunc1() const;
	// Usually returns self, but in the case of the alien wormhole you must call this
	virtual const TextureMaterial*	GetActiveMat() const;
	virtual void*					GetMaterialGL() const;
	virtual void					VirFunc4() const;
};
