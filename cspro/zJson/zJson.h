#pragma once

#ifdef WIN32
    #ifdef ZJSON_EXPORTS
    #define ZJSON_API __declspec(dllexport)
    #else
    #define ZJSON_API __declspec(dllimport)
    #endif
#else
    #define ZJSON_API
#endif
