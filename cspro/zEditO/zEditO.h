#pragma once


#ifdef CSPRO_NO_DLLS
    #define CLASS_DECL_ZEDITO
#elif defined(ZEDITO_IMPL)
    #define CLASS_DECL_ZEDITO __declspec(dllexport)
#else
    #define CLASS_DECL_ZEDITO __declspec(dllimport)
#endif
