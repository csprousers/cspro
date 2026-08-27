#pragma once


#ifdef CSPRO_NO_DLLS
    #define CLASS_DECL_ZFORMO
#elif defined(ZFORMO_IMPL)
    #define CLASS_DECL_ZFORMO __declspec(dllexport)
#else
    #define CLASS_DECL_ZFORMO __declspec(dllimport)
#endif
