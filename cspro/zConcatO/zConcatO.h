#pragma once

#ifdef WIN32
    #ifdef ZCONCATO_EXPORTS
    #define ZCONCATO_API __declspec(dllexport)
    #else
    #define ZCONCATO_API __declspec(dllimport)
    #endif
#else
    #define ZCONCATO_API
#endif
