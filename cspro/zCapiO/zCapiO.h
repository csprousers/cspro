#pragma once

#ifdef WIN32
#ifdef ZCAPIO_IMPL
    #define CLASS_DECL_ZCAPIO __declspec(dllexport)
#else
    #define CLASS_DECL_ZCAPIO __declspec(dllimport)
#endif
#else
#define CLASS_DECL_ZCAPIO
#endif
