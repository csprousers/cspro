#pragma once


#ifdef CSPRO_NO_DLLS
    #define CLASS_DECL_ZTOOLSO
#elif defined(ZTOOLSO_IMPL)
    #define CLASS_DECL_ZTOOLSO __declspec(dllexport)
#else
    #define CLASS_DECL_ZTOOLSO __declspec(dllimport)
#endif
