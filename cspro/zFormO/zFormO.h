#pragma once

#ifdef _WIN32
    #ifdef ZFORMO_IMPL
        #define CLASS_DECL_ZFORMO __declspec(dllexport)
    #else
        #define CLASS_DECL_ZFORMO __declspec(dllimport)
    #endif
#else
    #define CLASS_DECL_ZFORMO
#endif
