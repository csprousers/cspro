#pragma once

#ifdef WIN32
    #ifdef ZBATCHO_EXPORTS
    #define ZBATCHO_API __declspec(dllexport)
    #else
    #define ZBATCHO_API __declspec(dllimport)
    #endif
#else
    #define ZBATCHO_API
#endif
