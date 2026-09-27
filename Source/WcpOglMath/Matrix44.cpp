#include "Matrix44.h"
#include "WcpOglUtility.h"

void Matrix44::LoadIdentity()
{
	m[0] = 1.0f; m[4] = 0.0f; m[8] =  0.0f; m[12] = 0.0f;
	m[1] = 0.0f; m[5] = 1.0f; m[9] =  0.0f; m[13] = 0.0f;
	m[2] = 0.0f; m[6] = 0.0f; m[10] = 1.0f; m[14] = 0.0f;
	m[3] = 0.0f; m[7] = 0.0f; m[11] = 0.0f; m[15] = 1.0f;
}


Matrix44 Matrix44::operator*( const Matrix44 &rhs ) const
{
	Matrix44 tmpMatrix;
	ZeroMemory(&tmpMatrix, sizeof(Matrix44) );

	// Column
	for(unsigned char i=0; i<4; i++)
	{
		// Row
		for(unsigned char j=0; j<4; j++)
		{
			tmpMatrix[i][j] += (*this)[i][0] * rhs[0][j];
			tmpMatrix[i][j] += (*this)[i][1] * rhs[1][j];
			tmpMatrix[i][j] += (*this)[i][2] * rhs[2][j];
			tmpMatrix[i][j] += (*this)[i][3] * rhs[3][j];
		}
	}
	return tmpMatrix;
}



Matrix44 Matrix44::operator *= ( Matrix44 rhs )
{
	*this = *this * rhs;
	
	return *this;
}


void
Matrix44::CameraMatrix( 
	const Vector3 &right, const Vector3 &up, const Vector3 &view, const Vector3 &pos )
{
	// Get the inverse of the view (right handed co-ordinate system)
	Vector3 negView;
	negView.x() -= view.x();
	negView.y() -= view.y();
	negView.z() -= view.z();

	m[0] = right.x();	m[4] = right.y();	m[8 ] = right.z();
	m[1] = up.x();		m[5] = up.y();		m[9 ] = up.z();
	m[2] = negView.x();	m[6] = negView.y();	m[10] = negView.z();
	m[3] = 0.0f;		m[7] = 0.0f;		m[11] = 0.0f;

	// Determine the position
	float x, y, z;
	x = -DotProduct( pos, right );
	y = -DotProduct( pos, up );
	z = -DotProduct( pos, negView );

	m[12] = x;	m[13] = y;	m[14] = z;	m[15] = 1.0f;


}






