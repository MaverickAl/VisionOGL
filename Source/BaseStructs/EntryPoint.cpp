#include "EntryPoint.h"
#include <windows.h>
#include "WcpOglUtility.h"
#include "RenderingConstants.h"
#include "GLExt.h"
#include "GLBaseClass.h"
#include "CGShader.h"


BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    // Perform actions based on the reason for calling.
    switch( fdwReason ) 
    { 
        case DLL_PROCESS_ATTACH:
         	// Initialize once for each new process.
         	// Return FALSE to fail DLL load.
            break;

        case DLL_THREAD_ATTACH:
        	 // Do thread-specific initialization.
            break;

        case DLL_THREAD_DETACH:
         	// Do thread-specific cleanup.
            break;

        case DLL_PROCESS_DETACH:
        
            if (lpvReserved != nullptr)
            {
                break; // do not do cleanup if process termination scenario
            }
            
         	// Perform any necessary cleanup.
            break;
    }
    return TRUE;  // Successful DLL_PROCESS_ATTACH.
}

void DestroyGl(GLBaseClass *glbc)
{
	delete glbc;
	CGShader::ShutdownCG();
}

void InitGl(const InitUnknown *init)
{
#ifdef _DEBUG
	// _CRTDBG_CHECK_ALWAYS_DF
	//_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_CHECK_CRT_DF | _CRTDBG_LEAK_CHECK_DF | _CRTDBG_DELAY_FREE_MEM_DF);

	// Turn on heap tail checking
	//HeapSetInformation(NULL, HeapEnableTerminationOnCorruption, NULL, sizeof(HeapEnableTerminationOnCorruption));
#endif
}
