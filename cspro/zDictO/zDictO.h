#pragma once


#ifdef CSPRO_NO_DLLS
    #define CLASS_DECL_ZDICTO
#elif defined(ZDICTO_IMPL)
    #define CLASS_DECL_ZDICTO __declspec(dllexport)
#else
    #define CLASS_DECL_ZDICTO __declspec(dllimport)
#endif
