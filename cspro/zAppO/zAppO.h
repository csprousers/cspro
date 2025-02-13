#pragma once

#ifdef WIN32
    #ifdef ZAPPO_EXPORTS
    #define ZAPPO_API __declspec(dllexport)
    #else
    #define ZAPPO_API __declspec(dllimport)
    #endif
#else
    #define ZAPPO_API
#endif
