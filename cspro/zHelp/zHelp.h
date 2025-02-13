#pragma once

#ifdef WIN32
    #ifdef ZHELP_EXPORTS
    #define ZHELP_API __declspec(dllexport)
    #else
    #define ZHELP_API __declspec(dllimport)
    #endif
#else
    #define ZHELP_API
#endif
