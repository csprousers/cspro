#pragma once

#ifdef WIN32
    #ifdef ZPACKO_EXPORTS
    #define ZPACKO_API __declspec(dllexport)
    #else
    #define ZPACKO_API __declspec(dllimport)
    #endif
#else
    #define ZPACKO_API
#endif
