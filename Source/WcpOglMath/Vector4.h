#ifndef _WCPOGL_Vector4
#define	_WCPOGL_Vector4
#include "WcpOglUtility.h"
#include "Vector3.h"


class Vector4
{
public:
	Vector4(void) { Assign(0.0f, 0.0f, 0.0f, 1.0f); }
	Vector4(float x, float y, float z, float w) { Assign(x, y, z, w); }
	~Vector4(void) {}
	
	const float* xyzw() const { return m_xyzw; }
	void Assign(float x, float y, float z, float w);
	void Assign(Vector3& xyz, float w);
	void operator /=( const float &scalarValue );


	float& x()				{ return m_xyzw[0]; }
	float& y()				{ return m_xyzw[1]; }
	float& z()				{ return m_xyzw[2]; }
	float& w()				{ return m_xyzw[3]; }
	const float& x() const	{ return m_xyzw[0]; }
	const float& y() const	{ return m_xyzw[1]; }
	const float& z() const	{ return m_xyzw[2]; }
	const float& w() const	{ return m_xyzw[3]; }

private:
	float m_xyzw[4];
};


inline void Vector4::Assign(float x, float y, float z, float w)
{
	m_xyzw[0] = x;
	m_xyzw[1] = y;
	m_xyzw[2] = z;
	m_xyzw[3] = w;
}

inline void Vector4::Assign(Vector3& xyz, float w)
{
	Assign(xyz.x(), xyz.y(), xyz.z(), w);
}

inline void Vector4::operator /=( const float &scalarValue )
{
	x() /= scalarValue;
	y() /= scalarValue;
	z() /= scalarValue;
}


inline float DotProduct( const Vector4& vec1,const Vector4& vec2 )
{
	return
		( ( vec1.x() * vec2.x() )
		+ ( vec1.y() * vec2.y() )
		+ ( vec1.z() * vec2.z() )
		+ ( vec1.w() * vec2.w() ) );
}



#endif