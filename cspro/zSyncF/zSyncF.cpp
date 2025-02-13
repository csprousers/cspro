#include "stdafx.h"
#include <afxdllx.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
static AFX_EXTENSION_MODULE zSyncFDLL = { NULL, NULL };

extern "C" int APIENTRY
DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID /*lpReserved*/)
{
    if (dwReason == DLL_PROCESS_ATTACH)
    {
        TRACE0("zSyncF.DLL Initializing!\n");

        // Extension DLL one-time initialization
        AfxInitExtensionModule(zSyncFDLL,
            hInstance);

        // Insert this DLL into the resource chain
        new CDynLinkLibrary(zSyncFDLL);
    }
    else if (dwReason == DLL_PROCESS_DETACH)
    {
        TRACE0("zSyncF.DLL Terminating!\n");
    }
    return 1;   // ok
}
