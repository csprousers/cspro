#pragma once

#ifdef WIN32
    #ifdef ZFORMATTERO_EXPORTS
    #define ZFORMATTERO_API __declspec(dllexport)
    #else
    #define ZFORMATTERO_API __declspec(dllimport)
    #endif
#else
    #define ZFORMATTERO_API
#endif
