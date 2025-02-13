#pragma once

#ifdef _WIN32
    #ifdef ZDICTO_IMPL
        #define CLASS_DECL_ZDICTO __declspec(dllexport)
    #else
        #define CLASS_DECL_ZDICTO __declspec(dllimport)
    #endif
#else
    #define CLASS_DECL_ZDICTO
#endif
