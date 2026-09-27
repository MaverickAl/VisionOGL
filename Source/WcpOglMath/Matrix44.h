#pragma once
#include "Vector3.h"
#include "Vector4.h"
#include "CommonInputStructs.h"

typedef unsigned int uint;

class Matrix44
{
public:
	Matrix44() { LoadIdentity(); }
	~ Matrix44() {}

	void LoadIdentity();

	Matrix44 operator*( const Matrix44 &rhs ) const;

	Matrix44 operator *= (Matrix44 matr);

	Vector3 operator*( const Vector3 &in ) const;

	void GetAxisI(Vector3 &axis) const;
	void GetAxisJ(Vector3 &axis) const;
	void GetAxisK(Vector3 &axis) const;

	void
	CameraMatrix( 
		const Vector3 &right, const Vector3 &up, const Vector3 &view, const Vector3 &pos );

	void GetTranslation( Vector3 &pos ) const ;

	void SetTranslation(const Vector3& pos);

	void FillFromProph( const ProphMatrix &prophIn );

	Vector3 ModelTransform(const Vector3& pos);

	//! Accessor methods
	const float* operator[]( uint column ) const 
	{ return &m[column*4]; }
	
	float* operator[]( uint column) 
	{ return &m[column*4]; }

	const float* ColumnMajor() const
	{ return m; }
	
	float* ColumnMajor()
	{ return m; }



private:
	union
	{
		float m[16];
		struct
		{
			float C11, C12, C13, C14;
			float C21, C22, C23, C24;
			float C31, C32, C33, C34;
			float C41, C42, C43, C44;
		};
	};
};


inline Vector3 Matrix44::ModelTransform(const Vector3& in)
{
	return Vector3(in.m_xyz[0] * m[0] + in.m_xyz[1] * m[4] + in.m_xyz[2] * m[8] + m[12],
	in.m_xyz[0] * m[1] + in.m_xyz[1] * m[5] + in.m_xyz[2] * m[9] + m[13],
	in.m_xyz[0] * m[2] + in.m_xyz[1] * m[6] + in.m_xyz[2] * m[10] + m[14]);
}


inline Vector3 Matrix44::operator*(const Vector3& in) const
{
	Vector3	result;
	float		w;

	result.x() = in.x() * m[0] + in.y() * m[4] + in.z() * m[8] + m[12];
	result.y() = in.x() * m[1] + in.y() * m[5] + in.z() * m[9] + m[13];
	result.z() = in.x() * m[2] + in.y() * m[6] + in.z() * m[10] + m[14];

	// For proph guaranteed to be 0, 0, 0, 1
	w = in.x() * m[3] + in.y() * m[7] + in.z() * m[11] + m[15];

	result.x() /= w;
	result.y() /= w;
	result.z() /= w;

	return result;
}


inline void Matrix44::GetTranslation( Vector3 &pos ) const
{
	pos.x() = (*this)[3][0];
	pos.y() = (*this)[3][1];
	pos.z() = (*this)[3][2];
}

inline void Matrix44::SetTranslation(const Vector3& pos)
{
	(*this)[3][0] = pos.x();
	(*this)[3][1] = pos.y();
	(*this)[3][2] = pos.z();
}

inline void Matrix44::GetAxisI(Vector3 &axis) const
{
	axis.SetUp((*this)[0][0],(*this)[0][1],(*this)[0][2]);
}

inline void Matrix44::GetAxisJ(Vector3 &axis) const
{
	axis.SetUp((*this)[1][0],(*this)[1][1],(*this)[1][2]);
}

inline void Matrix44::GetAxisK(Vector3 &axis) const
{
	axis.SetUp((*this)[2][0],(*this)[2][1],(*this)[2][2]);
}

inline void Matrix44::FillFromProph(const ProphMatrix &prophIn)
{
	assert(sizeof(ProphMatrix) == sizeof(float) * 12);

	const float* matrix = reinterpret_cast<const float*>(&prophIn);

	m[0] = matrix[0];	m[4] = matrix[1];	m[8] = matrix[2];	m[12] = matrix[3];
	m[1] = matrix[4];	m[5] = matrix[5];	m[9] = matrix[6];	m[13] = matrix[7];
	m[2] = matrix[8];	m[6] = matrix[9];	m[10] = matrix[10];	m[14] = matrix[11];
	m[3] = 0.0f;		m[7] = 0.0f;		m[11] = 0.0f;		m[15] = 1.0f;

}

