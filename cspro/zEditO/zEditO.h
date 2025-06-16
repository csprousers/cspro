#pragma once

#ifdef _WIN32
#ifdef ZEDITO_IMPL
    #define CLASS_DECL_ZEDITO __declspec(dllexport)
#else
    #define CLASS_DECL_ZEDITO __declspec(dllimport)
#endif
#else
#define CLASS_DECL_ZEDITO
#endif
