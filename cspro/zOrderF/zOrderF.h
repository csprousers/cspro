#pragma once


#ifdef CSPRO_NO_DLLS
    #define CLASS_DECL_ZORDERF
#elif defined(ZORDERF_IMPL)
    #define CLASS_DECL_ZORDERF __declspec(dllexport)
#else
    #define CLASS_DECL_ZORDERF __declspec(dllimport)
#endif
