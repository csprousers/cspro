#pragma once

#ifdef WIN32
    #ifdef ZZIPO_IMPL
        #define CLASS_DECL_ZZIPO __declspec(dllexport)
    #else
        #define CLASS_DECL_ZZIPO __declspec(dllimport)
    #endif
#else
    #define CLASS_DECL_ZZIPO
#endif
