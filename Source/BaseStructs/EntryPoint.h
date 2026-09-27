#pragma once
class  GLBaseClass;
struct InitUnknown;	

#define DllExport extern "C" _declspec(dllexport)

DllExport void 			InitGl(const InitUnknown *init);
DllExport GLBaseClass* 	CreateGl();
DllExport void 			DestroyGl(GLBaseClass *gl);
