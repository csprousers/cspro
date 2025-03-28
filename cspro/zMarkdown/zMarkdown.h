#pragma once

#ifdef WIN32
    #ifdef ZMARKDOWN_EXPORTS
    #define ZMARKDOWN_API __declspec(dllexport)
    #else
    #define ZMARKDOWN_API __declspec(dllimport)
    #endif
#else
    #define ZMARKDOWN_API
#endif
