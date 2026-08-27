#pragma once


#ifdef CSPRO_NO_DLLS
    #define CLASS_DECL_ZCAPIO
#elif defined(ZCAPIO_IMPL)
    #define CLASS_DECL_ZCAPIO __declspec(dllexport)
#else
    #define CLASS_DECL_ZCAPIO __declspec(dllimport)
#endif
