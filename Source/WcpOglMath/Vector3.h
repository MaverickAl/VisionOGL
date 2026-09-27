#ifndef _WCPOGL_Vector3
#define _WCPOGL_Vector3

#include <cmath>
#include <iostream>
#include "WcpOglPreComp.h"
using namespace std;


class Vector3  
{
	friend Vector3 
	ScalarVectorMultiply(
		const Vector3 &multVector, const float &scalarValue );

	friend Vector3
	ScalarVectorDivide(
		const Vector3 &divVector, const float &scalarValue );

	friend istream& 
	operator >>(
		istream &is,Vector3 &v );

public:
	Vector3()
		{ SetUp( 0.0f, 0.0f, 0.0f); }
	
	Vector3( float ax, float ay, float az )
		{ SetUp( ax, ay, az); }
	
	~Vector3(){}

	void SetUp( float ax, float ay, float az )
		{ x() = ax; y() = ay; z() = az; }

	void operator *=( const float &scalarValue );
	void operator +=( const Vector3 &addVector3 );
	bool operator ==( const Vector3 &compVector3 ) const;

	void operator -=( const Vector3 &subVector3 );
	void operator /=( const float &scalarValue );

	//Addition and subtraction
	Vector3 operator +( const Vector3 &addVector3 ) const;
	Vector3 operator -( const Vector3 &subVector3 ) const;


	void Normalise();

	float Magnitude() const;
	float MagnitudeSq() const;

	float GetSquaredDistanceFrom(const Vector3 &vec) const;

	// Only used for copying the data, e.g. to a shader
	// For setting the data use SetUp()
	const float* xyz() const { return m_xyz; }

	// Acessor functions
	float& x()				{ return m_xyz[0]; }
	float& y()				{ return m_xyz[1]; }
	float& z()				{ return m_xyz[2]; }
	const float& x() const { return m_xyz[0]; }
	const float& y() const { return m_xyz[1]; }
	const float& z() const { return m_xyz[2]; }

	float m_xyz[3];

};

inline istream& operator >>( istream &input, Vector3 &v )
{
      input >> v.x();
	  input >> v.y();
	  input >> v.z();
      return input;
}

inline Vector3
ScalarVectorMultiply(
	const Vector3 &multVector, const float &scalarValue )
{
	Vector3 tempVector3;
	tempVector3.x() = multVector.x()*scalarValue;
	tempVector3.y() = multVector.y()*scalarValue;
	tempVector3.z() = multVector.z()*scalarValue;

	return tempVector3;
}

inline Vector3 
ScalarVectorDivide(
	const Vector3 &divVector, const float &scalarValue )
{
	Vector3 tempVector3;
	tempVector3.x() = divVector.x()/scalarValue;
	tempVector3.y() = divVector.y()/scalarValue;
	tempVector3.z() = divVector.z()/scalarValue;

	return tempVector3;
}


inline bool Vector3::operator==(const Vector3 &compVector3) const
{
	if(compVector3.x() == x()){
		if(compVector3.y() == y()){
			if(compVector3.z() == z()){
				return true;
			}
		}
	}
	return false;

}

inline Vector3 operator*( const float &scalarValue, const Vector3 &multVector )
{
	return ScalarVectorMultiply(multVector, scalarValue);
}

inline Vector3 operator*( const Vector3 &multVector, const float &scalarValue )
{
	return ScalarVectorMultiply(multVector, scalarValue);
}


inline Vector3 operator/( const Vector3 &divVector, const float &scalarValue )
{
	return ScalarVectorDivide(divVector, scalarValue);
}

inline void Vector3::operator *=( const float &scalarValue )
{
	x() *= scalarValue;
	y() *= scalarValue;
	z() *= scalarValue;
}

inline void Vector3::operator /=( const float &scalarValue )
{
	x() /= scalarValue;
	y() /= scalarValue;
	z() /= scalarValue;
}



inline Vector3 Vector3::operator +( const Vector3 &addVector3 ) const
{
	Vector3 tempVector3;
	tempVector3.x() = addVector3.x() + x();
	tempVector3.y() = addVector3.y() + y();
	tempVector3.z() = addVector3.z() + z();

	return tempVector3;
}



inline void Vector3::operator +=( const Vector3 &addVector3 )
{
	x() += addVector3.x();
	y() += addVector3.y();
	z() += addVector3.z();
}

inline void Vector3::operator -=( const Vector3 &subVector3 )
{
	x() -= subVector3.x();
	y() -= subVector3.y();
	z() -= subVector3.z();
}


inline Vector3 Vector3::operator -( const Vector3 &subVector3 ) const
{
	Vector3 tempVector3;

	tempVector3.x() = x() - subVector3.x();
	tempVector3.y() = y() - subVector3.y();
	tempVector3.z() = z() - subVector3.z();

	return tempVector3;
}

inline void CrossProduct( const Vector3 &vec1, const Vector3 &vec2, Vector3 &out )
{
	out.x() = ( vec1.y() * vec2.z())-( vec1.z() * vec2.y() );
	out.y() = ( vec1.z() * vec2.x())-( vec1.x() * vec2.z() );
	out.z() = ( vec1.x() * vec2.y())-( vec1.y() * vec2.x() );
}

inline Vector3 CrossProduct( const Vector3 &vec1, const Vector3 &vec2 )
{
	Vector3	out;
	out.x() = ( vec1.y() * vec2.z())-( vec1.z() * vec2.y() );
	out.y() = ( vec1.z() * vec2.x())-( vec1.x() * vec2.z() );
	out.z() = ( vec1.x() * vec2.y())-( vec1.y() * vec2.x() );
	return out;
}



inline float DotProduct( const Vector3& vec1,const Vector3& vec2 )
{
	return
		((vec1.x() * vec2.x())+(vec1.y() * vec2.y())+(vec1.z() * vec2.z()));
}



inline void Vector3::Normalise()
{
	if(x()!= 0.0 || y()!=0.0 || z()!=0.0){
		float length = Magnitude();
	    
		x() /= length;
		y() /= length;
		z() /= length;
	}
}

inline float Vector3::Magnitude() const
{
	return 
		sqrt( ( x()*x() )
		+ ( y()*y() )
		+ ( z()*z() ) );
}

inline float Vector3::MagnitudeSq() const
{
	return ( x()*x() )
		+ ( y()*y() )
		+ ( z()*z() );
}


inline float Vector3::GetSquaredDistanceFrom( const Vector3 &vec ) const
{
	Vector3 tmpVec = *this - vec;
	return ((tmpVec.x()*tmpVec.x())+(tmpVec.y()*tmpVec.y())+(tmpVec.z()*tmpVec.z()));
}


#endif