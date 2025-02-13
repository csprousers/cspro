#pragma once

#ifdef WIN32
    #ifdef ZCASEO_EXPORTS
    #define ZCASEO_API __declspec(dllexport)
    #else
    #define ZCASEO_API __declspec(dllimport)
    #endif
#else
    #define ZCASEO_API
#endif
