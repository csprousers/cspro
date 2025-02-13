#include "StdAfx.h"
#include "zDesignerF.h"


AFX_EXTENSION_MODULE zDesignerFDLL = { NULL, NULL };

extern "C" int APIENTRY
DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID /*lpReserved*/)
{
    if( dwReason == DLL_PROCESS_ATTACH )
    {
        TRACE0("zDesignerF.DLL Initializing!\n");

        // Extension DLL one-time initialization
        if( !AfxInitExtensionModule(zDesignerFDLL, hInstance) )
            return 0;

        // Insert this DLL into the resource chain
        new CDynLinkLibrary(zDesignerFDLL);
    }

    else if( dwReason == DLL_PROCESS_DETACH )
    {
        TRACE0("zDesignerF.DLL Terminating!\n");

        // Terminate the library before destructors are called
        AfxTermExtensionModule(zDesignerFDLL);
    }

    return 1;   // ok
}
