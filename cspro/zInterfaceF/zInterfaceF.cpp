#include "StdAfx.h"
#include "zInterfaceF.h"

static AFX_EXTENSION_MODULE zInterfaceFDLL = { NULL, NULL };

extern "C" int APIENTRY
DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID /*lpReserved*/)
{
    if( dwReason == DLL_PROCESS_ATTACH )
    {
        TRACE0("zInterfaceF.DLL Initializing!\n");

        // Extension DLL one-time initialization
        if( !AfxInitExtensionModule(zInterfaceFDLL, hInstance) )
            return 0;

        // Insert this DLL into the resource chain
        new CDynLinkLibrary(zInterfaceFDLL);
    }

    else if( dwReason == DLL_PROCESS_DETACH )
    {
        TRACE0("zInterfaceF.DLL Terminating!\n");

        // Terminate the library before destructors are called
        AfxTermExtensionModule(zInterfaceFDLL);
    }

    return 1;   // ok
}
