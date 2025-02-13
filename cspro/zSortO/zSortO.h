#pragma once

#ifdef WIN32
    #ifdef ZSORTO_EXPORTS
    #define ZSORTO_API __declspec(dllexport)
    #else
    #define ZSORTO_API __declspec(dllimport)
    #endif
#else
    #define ZSORTO_API
#endif
