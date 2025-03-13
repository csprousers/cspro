/* This file was added to the original rtf2html 0.2.0 code by
   Josh Handley for OECD/Paris21/IHSN.
   Copyright (c) 2009 OECD.
   Released under original rtf2html license (LGPL 2.1)
   See COPYING.LESSER for details.
*/

// rtf2html_dll.cpp : Defines the entry point for the DLL application.
//

#include "windows.h"
#include "rtf2html_dll.h"

#ifdef _MANAGED
#pragma managed(push, off)
#endif

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

#ifdef _MANAGED
#pragma managed(pop)
#endif
