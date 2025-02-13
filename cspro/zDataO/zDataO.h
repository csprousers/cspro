#pragma once

// The following ifdef block is the standard way of creating macros which make exporting
// from a DLL simpler. All files within this DLL are compiled with the ZDATAO_EXPORTS
// symbol defined on the command line. This symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see
// ZDATAO_API functions as being imported from a DLL, whereas this DLL sees symbols
// defined with this macro as being exported.

#ifdef WIN32
    #if defined(ZDATAO_EXPORTS) || defined(ZEXPORTO_EXPORTS)
    #define ZDATAO_API __declspec(dllexport)
    #else
    #define ZDATAO_API __declspec(dllimport)
    #endif
#else
    #define ZDATAO_API
#endif

#ifdef WIN_DESKTOP
extern AFX_EXTENSION_MODULE zDataODLL;
#endif
