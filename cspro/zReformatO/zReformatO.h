#pragma once

#ifdef WIN32
    #ifdef ZREFORMATO_EXPORTS
    #define ZREFORMATO_API __declspec(dllexport)
    #else
    #define ZREFORMATO_API __declspec(dllimport)
    #endif
#else
    #define ZREFORMATO_API
#endif
