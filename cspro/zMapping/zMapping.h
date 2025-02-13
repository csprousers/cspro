#pragma once

#ifdef WIN32
    #ifdef ZMAPPING_EXPORTS
    #define ZMAPPING_API __declspec(dllexport)
    #else
    #define ZMAPPING_API __declspec(dllimport)
    #endif
#else
    #define ZMAPPING_API
#endif
