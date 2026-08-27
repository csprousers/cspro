#pragma once


#ifdef CSPRO_NO_DLLS
    #define CLASS_DECL_ZUTILO
#elif defined(ZUTILO_IMPL)
    #define CLASS_DECL_ZUTILO __declspec(dllexport)
#else
    #define CLASS_DECL_ZUTILO __declspec(dllimport)
#endif
