#include "stdafx.h"
#include "zAction.h"


AFX_EXTENSION_MODULE zActionDLL = { FALSE };


extern "C" int APIENTRY
DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID /*lpReserved*/)
{
    if( dwReason == DLL_PROCESS_ATTACH )
    {
        TRACE0("zAction.DLL Initializing!\n");

        // Extension DLL one-time initialization
        if (!AfxInitExtensionModule(zActionDLL, hInstance) )
            return 0;

        // Insert this DLL into the resource chain
        // NOTE: If this Extension DLL is being implicitly linked to by
        //  an MFC Regular DLL (such as an ActiveX Control)
        //  instead of an MFC application, then you will want to
        //  remove this line from DllMain and put it in a separate
        //  function exported from this Extension DLL.  The Regular DLL
        //  that uses this Extension DLL should then explicitly call that
        //  function to initialize this Extension DLL.  Otherwise,
        //  the CDynLinkLibrary object will not be attached to the
        //  Regular DLL's resource chain, and serious problems will
        //  result.

        new CDynLinkLibrary(zActionDLL);
    }

    else if( dwReason == DLL_PROCESS_DETACH )
    {
        TRACE0("zAction.DLL Terminating!\n");
        // Terminate the library before destructors are called
        AfxTermExtensionModule(zActionDLL);
    }

    return 1;   // ok
}
