#pragma once

#ifdef WIN32
    #ifdef ZREPORTO_EXPORTS
    #define ZREPORTO_API __declspec(dllexport)
    #else
    #define ZREPORTO_API __declspec(dllimport)
    #endif
#else
    #define ZREPORTO_API
#endif
