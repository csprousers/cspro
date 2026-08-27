#pragma once


#ifdef CSPRO_NO_DLLS
    #define CLASS_DECL_ZTABLEF
#elif defined(ZTABLEF_IMPL)
    #define CLASS_DECL_ZTABLEF __declspec(dllexport)
#else
    #define CLASS_DECL_ZTABLEF __declspec(dllimport)
#endif
