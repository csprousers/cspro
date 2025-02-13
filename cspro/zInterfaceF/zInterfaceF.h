#pragma once

#ifdef _WIN32
#ifdef ZINTERFACEF_EXPORTS
    #define CLASS_DECL_ZINTERFACEF __declspec(dllexport)
#else
    #define CLASS_DECL_ZINTERFACEF __declspec(dllimport)
#endif
#else
#define CLASS_DECL_ZINTERFACEF
#endif
