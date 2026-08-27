#pragma once


#ifdef CSPRO_NO_DLLS
    #define CLASS_DECL_ZZIP
#elif defined(ZZIP_IMPL)
    #define CLASS_DECL_ZZIP __declspec(dllexport)
#else
    #define CLASS_DECL_ZZIP __declspec(dllimport)
#endif
