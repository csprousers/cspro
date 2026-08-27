#pragma once


#ifdef CSPRO_NO_DLLS
    #define CLASS_DECL_ZTABLEO
#elif defined(ZTABLEO_IMPL)
    #define CLASS_DECL_ZTABLEO __declspec(dllexport)
#else
    #define CLASS_DECL_ZTABLEO __declspec(dllimport)
#endif
