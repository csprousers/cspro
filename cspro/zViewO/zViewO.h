#pragma once

#ifdef WIN32
    #ifdef ZVIEWO_EXPORTS
    #define ZVIEWO_API __declspec(dllexport)
    #else
    #define ZVIEWO_API __declspec(dllimport)
    #endif
#else
    #define ZVIEWO_API
#endif
