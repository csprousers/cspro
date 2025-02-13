/* This file was added to the original rtf2html 0.2.0 code by
   Josh Handley for OECD/Paris21/IHSN.
   Copyright (c) 2009 OECD.
   Released under original rtf2html license (LGPL 2.1)
   See COPYING.LESSER for details.
*/

// The following ifdef block is the standard way of creating macros which make exporting
// from a DLL simpler. All files within this DLL are compiled with the RTF2HTML_DLL_EXPORTS
// symbol defined on the command line. this symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see
// RTF2HTML_DLL_API functions as being imported from a DLL, whereas this DLL sees symbols
// defined with this macro as being exported.
#ifdef WIN32
    #ifdef RTF2HTML_DLL_EXPORTS
        #define RTF2HTML_DLL_API __declspec(dllexport)
    #else
        #define RTF2HTML_DLL_API __declspec(dllimport)
    #endif
#else
    #define RTF2HTML_DLL_API
#endif
