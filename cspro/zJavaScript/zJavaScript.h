#pragma once

#ifdef WIN32
    #ifdef ZJAVASCRIPT_EXPORTS
    #define ZJAVASCRIPT_API __declspec(dllexport)
    #else
    #define ZJAVASCRIPT_API __declspec(dllimport)
    #endif
#else
    #define ZJAVASCRIPT_API
#endif
