#pragma once
// zTableO.h : main header file for the ZTABLEO DLL
//

#ifdef WIN32

#ifdef ZTABLEO_IMPL
    #define CLASS_DECL_ZTABLEO __declspec(dllexport)
#else
    #define CLASS_DECL_ZTABLEO __declspec(dllimport)
#endif

#else
    #define CLASS_DECL_ZTABLEO

#endif
