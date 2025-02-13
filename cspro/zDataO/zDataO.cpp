#include "stdafx.h"
#include "zDataO.h"
#include <afxdllx.h>

AFX_EXTENSION_MODULE zDataODLL = { NULL, NULL };

extern "C" int APIENTRY
DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID /*lpReserved*/)
{
    if( dwReason == DLL_PROCESS_ATTACH )
    {
        TRACE0("zDataO.DLL Initializing!\n");

        // Extension DLL one-time initialization
        if( !AfxInitExtensionModule(zDataODLL, hInstance) )
            return 0;

        // Insert this DLL into the resource chain
        new CDynLinkLibrary(zDataODLL);
    }

    else if( dwReason == DLL_PROCESS_DETACH )
    {
        TRACE0("zDataO.DLL Terminating!\n");
        AfxTermExtensionModule(zDataODLL);
    }

    return 1;   // ok
}
