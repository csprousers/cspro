#pragma once

#ifdef WIN32
    #ifdef ZPARADATAO_EXPORTS
    #define ZPARADATAO_API __declspec(dllexport)
    #else
    #define ZPARADATAO_API __declspec(dllimport)
    #endif
#else
    #define ZPARADATAO_API
#endif
