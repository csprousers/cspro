#pragma once

#ifdef WIN32
    #ifdef ZEXCELO_EXPORTS
    #define ZEXCELO_API __declspec(dllexport)
    #else
    #define ZEXCELO_API __declspec(dllimport)
    #endif
#else
    #define ZEXCELO_API
#endif
