#pragma once


#ifdef CSPRO_NO_DLLS
    #define CLASS_DECL_ZFORMF
#elif defined(ZFORMF_IMPL)
    #define CLASS_DECL_ZFORMF __declspec(dllexport)
#else
    #define CLASS_DECL_ZFORMF __declspec(dllimport)
#endif


extern HINSTANCE zFormF_hInstance;
