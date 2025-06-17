#pragma once

#ifdef WIN32
    #ifdef ZZIP_IMPL
        #define CLASS_DECL_ZZIP __declspec(dllexport)
    #else
        #define CLASS_DECL_ZZIP __declspec(dllimport)
    #endif
#else
    #define CLASS_DECL_ZZIP
#endif
