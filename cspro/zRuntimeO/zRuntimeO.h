#pragma once

#ifdef WIN32
    #ifdef ZRUNTIMEO_EXPORTS
    #define ZRUNTIMEO_API __declspec(dllexport)
    #else
    #define ZRUNTIMEO_API __declspec(dllimport)
    #endif
#else
    #define ZRUNTIMEO_API
#endif
