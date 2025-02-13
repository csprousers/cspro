#pragma once

#ifdef WIN32
    #ifdef ZENGINEO_EXPORTS
    #define ZENGINEO_API __declspec(dllexport)
    #else
    #define ZENGINEO_API __declspec(dllimport)
    #endif
#else
    #define ZENGINEO_API
#endif
