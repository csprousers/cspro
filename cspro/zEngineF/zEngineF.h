#pragma once


#ifdef CSPRO_NO_DLLS
    #define CLASS_DECL_ZENGINEF
#elif defined(ZENGINEF_EXPORTS)
    #define CLASS_DECL_ZENGINEF __declspec(dllexport)
#else
    #define CLASS_DECL_ZENGINEF __declspec(dllimport)
#endif
