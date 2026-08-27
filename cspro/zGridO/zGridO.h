#pragma once


#ifdef CSPRO_NO_DLLS
    #define CLASS_DECL_ZGRIDO
#elif defined(ZGRIDO_IMPL)
    #define CLASS_DECL_ZGRIDO __declspec(dllexport)
#else
    #define CLASS_DECL_ZGRIDO __declspec(dllimport)
#endif
