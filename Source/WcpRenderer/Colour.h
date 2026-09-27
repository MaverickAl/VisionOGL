#pragma once
#include <iostream>
#include "GLExt.h"
using namespace std;

class Colour
{
public:
	Colour(void) { Assign(0.0f, 0.0f, 0.0f, 0.0f); }
	Colour(GLfloat rgba[]);
	Colour(GLfloat r, GLfloat g, GLfloat b, GLfloat a = 1.0f);
	~Colour(void) {}
	//Getter
	const GLfloat* rgba() const { return m_rgba; }
	//Setter
	void Assign(GLfloat r, GLfloat g, GLfloat b, GLfloat a);

	void Assign565(int color565);

	const float &r() const { return m_rgba[0]; }
	const float &g() const { return m_rgba[1]; }
	const float &b() const { return m_rgba[2]; }
	const float &a() const { return m_rgba[3]; }

	float &r() { return m_rgba[0]; }
	float &g() { return m_rgba[1]; }
	float &b() { return m_rgba[2]; }
	float &a() { return m_rgba[3]; }

	void operator +=( const Colour &addColour );

	const Colour operator *( const float multiplier ) const;

private:
	enum
	{
		R_PLACE_SHIFT = 11,
		G_PLACE_SHIFT = 5,
		B_PLACE_SHIFT = 0,

		R_SIZE_SHIFT  = 3,
		G_SIZE_SHIFT  = 2,
		B_SIZE_SHIFT  = 3
	};

	GLfloat m_rgba[4];
	
};


inline const Colour Colour::operator *( const float multiplier ) const
{
	Colour out;
	out.r() = m_rgba[0]*multiplier;
	out.g() = m_rgba[1]*multiplier;
	out.b() = m_rgba[2]*multiplier;
	out.a() = m_rgba[3]*multiplier;
	return out;
}

inline void Colour::operator +=( const Colour &addColour )
{
	r() += addColour.r();
	g() += addColour.g();
	b() += addColour.b();
	a() += addColour.a();
}

inline Colour::Colour(GLfloat r, GLfloat g, GLfloat b, GLfloat a)
{
	Assign(r, g, b, a);
}

inline Colour::Colour(GLfloat rgba[])
{
	Assign(rgba[0], rgba[1], rgba[2], rgba[3]);
}

inline void Colour::Assign(GLfloat r, GLfloat g, GLfloat b, GLfloat a)
{ 
	m_rgba[0] = r;
	m_rgba[1] = g; 
	m_rgba[2] = b;
	m_rgba[3] = a;
}


inline void Colour::Assign565(int color565)
{
	m_rgba[0] = (float)( ((color565 >> R_PLACE_SHIFT) << R_SIZE_SHIFT) & 0xff)/255.0f;
	m_rgba[1] = (float)( ((color565 >> G_PLACE_SHIFT) << G_SIZE_SHIFT) & 0xff)/255.0f;
	m_rgba[2] = (float)( ((color565 >> B_PLACE_SHIFT) << B_SIZE_SHIFT) & 0xff)/255.0f;
	m_rgba[3] = 1.0f;
}
