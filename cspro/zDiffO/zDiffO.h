#pragma once

#ifdef WIN32
    #ifdef ZDIFFO_EXPORTS
    #define ZDIFFO_API __declspec(dllexport)
    #else
    #define ZDIFFO_API __declspec(dllimport)
    #endif
#else
    #define ZDIFFO_API
#endif
