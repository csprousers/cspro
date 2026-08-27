#pragma once


#ifdef CSPRO_NO_DLLS
    #define CLASS_DECL_ZINTERFACEF
#elif defined(ZINTERFACEF_EXPORTS)
    #define CLASS_DECL_ZINTERFACEF __declspec(dllexport)
#else
    #define CLASS_DECL_ZINTERFACEF __declspec(dllimport)
#endif
