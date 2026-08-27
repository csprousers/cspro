#pragma once


#ifdef CSPRO_NO_DLLS
    #define CLASS_DECL_ZDICTF
#elif defined(ZDICTF_IMPL)
    #define CLASS_DECL_ZDICTF __declspec(dllexport)
#else
    #define CLASS_DECL_ZDICTF __declspec(dllimport)
#endif
