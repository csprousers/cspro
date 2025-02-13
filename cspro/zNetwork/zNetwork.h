#pragma once

#ifdef WIN32
    #ifdef ZNETWORK_EXPORTS
    #define ZNETWORK_API __declspec(dllexport)
    #else
    #define ZNETWORK_API __declspec(dllimport)
    #endif
#else
    #define ZNETWORK_API
#endif
