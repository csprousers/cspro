﻿// zFormF.cpp : Defines the initialization routines for the DLL.
//

#include "StdAfx.h"
#include <afxdllx.h>


static AFX_EXTENSION_MODULE ZFormFDLL = { NULL, NULL };

HINSTANCE zFormF_hInstance;

extern "C" int APIENTRY
DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
    zFormF_hInstance = hInstance;

    // Remove this if you use lpReserved
    UNREFERENCED_PARAMETER(lpReserved);

    if (dwReason == DLL_PROCESS_ATTACH)
    {
        TRACE0("ZFORMF.DLL Initializing!\n");

        // Extension DLL one-time initialization
        if (!AfxInitExtensionModule(ZFormFDLL, hInstance))
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

        new CDynLinkLibrary(ZFormFDLL);
    }
    else if (dwReason == DLL_PROCESS_DETACH)
    {
        TRACE0("ZFORMF.DLL Terminating!\n");
        // Terminate the library before destructors are called
        AfxTermExtensionModule(ZFormFDLL);
    }
    return 1;   // ok
}
