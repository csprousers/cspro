#pragma once


#ifdef CSPRO_NO_DLLS
    #define CLASS_DECL_ZEXTAB
#elif defined(ZEXTAB_IMPL)
    #define CLASS_DECL_ZEXTAB __declspec(dllexport)
#else
    #define CLASS_DECL_ZEXTAB __declspec(dllimport)
#endif
