#include ".\LightGL.h"
#include "GLOpenGL.h"
#include "Light.h"
#include "Colour.h"
#include "Matrix44.h"

LightGL::LightGL(void):
m_number(-1),
m_linearAttenuation(0.0f), 
m_constantAttenuation(0.0f),
m_quadraticAttenuation(0.0f),
m_cutoffDistSq(10000.f),
m_cutoffDist(1000.0f),
m_bIsCutoffPoint(false)
{
}

LightGL::~LightGL(void)
{
}

Vector3 LightGL::GetPos() const
{
	const float* pos = m_position.xyzw();
	return Vector3(pos[0], pos[1], pos[2]);
}

void LightGL::Init(Light* prophLight)
{
	LightType type = prophLight->lightType;
	Matrix44 lightMtx;
	lightMtx.FillFromProph(prophLight->modelMatrix);
	Vector3 pos;

	Colour lightCol = prophLight->colour;
	lightCol.a() = 1.0f;


	// Set direction
	if(type == LT_DIRECTIONAL )
	{
		lightMtx.GetAxisK(pos);
		pos.Normalise();	// Just to be safe
		SetPosition( Vector4(-pos.x(), -pos.y(), -pos.z(), 0.0f) );
		SetDiffuse(lightCol);
		SetAmbient(Colour(0.0f, 0.0f, 0.0f));
		//SetSpecular(Colour(1.0f, 1.0f, 1.0f));
		SetAttenuation(1.0f, 0.f, 0.f);

	}
	else if( type == LT_LASER)
	{
		lightMtx.GetTranslation(pos);
		// Handling all point lights the same
		SetPosition( Vector4(pos.x(), pos.y(), pos.z(), 1.0f) );
		SetDiffuse(lightCol);
		// The lasers don't care about direction although I can only replicate with shaders
		SetAmbient(Colour(0.0f, 0.0f, 0.0f));
		SetCutoffDistSq(prophLight->cutoffDistSq);
		SetCutoffDist(prophLight->cutoffDist);

		SetAttenuation(prophLight->constantAttenuation, prophLight->linearAttenuation, prophLight->quadraticAttenuation);

		if (prophLight->constantAttenuation <= 0.0f && prophLight->linearAttenuation <= 0.0f)
		{
			m_bIsCutoffPoint = true;
		}
	}
	else if( type == LT_AMBIENT )
	{
		// TODO: Hack global comes in at 255 on some menus
		SetPosition( Vector4(0.0f, 0.0f, 0.0f, 0.0f) );
		Colour tmp = lightCol;
		tmp.r() = wcpogl::clampf(lightCol.r(), 0.0f, 1.0f);
		tmp.g() = wcpogl::clampf(lightCol.g(), 0.0f, 1.0f);
		tmp.b() = wcpogl::clampf(lightCol.b(), 0.0f, 1.0f);
		tmp.a() = 1.0f;
		SetAmbient(tmp);

		SetAttenuation(1.0f, 0.f, 0.f);

	}
	else
	{
		//  Unknown light
		printf("Unknown light");
	}
}

void LightGL::SetAttenuation(GLfloat constant, GLfloat linear, GLfloat quadratic)
{
	m_constantAttenuation = constant;
	m_linearAttenuation = linear;
	m_quadraticAttenuation = quadratic;
}

void LightGL::SetUpLight(Vector3 vEyePos) const
{
	glLightfv(m_number, GL_AMBIENT, m_ambient.rgba());
	glLightfv(m_number, GL_DIFFUSE, m_diffuse.rgba());
	glLightfv(m_number, GL_SPECULAR, m_specular.rgba());
	#if ORIGIN_OFFSET
	Vector4 vPos = m_position;
	vPos.x() -= vEyePos.x();
	vPos.y() -= vEyePos.y();
	vPos.z() -= vEyePos.z();

	glLightfv(m_number, GL_POSITION, vPos.xyzw());
	#else
	glLightfv(m_number, GL_POSITION, m_position.xyzw());
	#endif

	glLightf(m_number, GL_SPOT_CUTOFF,180.f);

	if (m_bIsCutoffPoint)
	{
		float quadratic = 1.0f / (m_cutoffDist * m_cutoffDist);

		glLightf(m_number, GL_LINEAR_ATTENUATION, 0.0f);
		glLightf(m_number, GL_CONSTANT_ATTENUATION, 1.0f);
		glLightf(m_number, GL_QUADRATIC_ATTENUATION, quadratic);
	}
	else
	{
		glLightf(m_number, GL_LINEAR_ATTENUATION, m_linearAttenuation);
		glLightf(m_number, GL_CONSTANT_ATTENUATION, m_constantAttenuation);
		glLightf(m_number, GL_QUADRATIC_ATTENUATION, m_quadraticAttenuation);
	}
}

void LightGL::SwitchOn() const
{
	glEnable(m_number);
}

void LightGL::SwitchOff() const
{
	glDisable(m_number);
}

