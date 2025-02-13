#include "StdAfx.h"
#include "zEngineF.h"

AFX_EXTENSION_MODULE zEngineF = { NULL, NULL };

extern "C" int APIENTRY
DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
    // Remove this if you use lpReserved
    UNREFERENCED_PARAMETER(lpReserved);

    if (dwReason == DLL_PROCESS_ATTACH)
    {
        TRACE0("zEngineF.DLL Initializing!\n");

        // Extension DLL one-time initialization
        if (!AfxInitExtensionModule(zEngineF, hInstance))
            return 0;

        new CDynLinkLibrary(zEngineF);
    }
    else if (dwReason == DLL_PROCESS_DETACH)
    {
        TRACE0("zEngineF.DLL Terminating!\n");
        // Terminate the library before destructors are called
        AfxTermExtensionModule(zEngineF);
    }
    return 1;   // ok
}
