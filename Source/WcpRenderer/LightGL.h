#pragma once
#include "WcpOglUtility.h"
#include "Vector4.h"
#include "Vector3.h"
#include "Colour.h"

class Light;
class SpecularEffect;


class LightGL
{
	friend SpecularEffect;
public:
	LightGL(void);
	~LightGL(void);
	void Init(Light* prophLight);
	void SetAmbient(const Colour &ambient) { m_ambient = ambient; }
	void SetDiffuse(const Colour &diffuse) { m_diffuse = diffuse; }
	void SetSpecular(const Colour &specular) { m_specular = specular; }
	void SetPosition(const Vector4 &position) { m_position = position; }
	Vector3 GetPos() const;
	const Colour& GetAmbient() { return m_ambient; }
	void SetNumber(GLuint lightNo) { m_number = GL_LIGHT0 + lightNo; }
	
	void SetAttenuation(GLfloat constant, GLfloat linear, GLfloat quadratic);
	void SetUpLight(Vector3 vEyePos) const;

	void SetCutoffDist(float cutoffDist)	{ m_cutoffDist = cutoffDist; }
	void SetCutoffDistSq(float csq) { m_cutoffDistSq = csq; }
	float GetRangeSquare() const { return m_cutoffDistSq; }

    void SwitchOn() const;
	void SwitchOff() const;

private:
	Colour		m_diffuse;
	Colour		m_ambient;
	Colour		m_specular;
	Vector4		m_position;
	GLfloat		m_linearAttenuation;
	GLfloat		m_constantAttenuation;
	GLfloat		m_quadraticAttenuation;
	float		m_cutoffDistSq;
	float		m_cutoffDist;
	GLuint		m_number;
	bool		m_bIsCutoffPoint;
};
