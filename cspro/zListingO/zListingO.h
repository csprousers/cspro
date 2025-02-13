#pragma once

#ifdef WIN32
    #ifdef ZLISTINGO_EXPORTS
    #define ZLISTINGO_API __declspec(dllexport)
    #else
    #define ZLISTINGO_API __declspec(dllimport)
    #endif
#else
    #define ZLISTINGO_API
#endif
