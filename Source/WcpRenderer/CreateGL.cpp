#include "EntryPoint.h"
#include "GLOpenGL.h"

GLBaseClass *CreateGl()
{
	return new GLOpenGL();
}
