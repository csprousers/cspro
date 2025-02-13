#pragma once

#ifdef WIN32
    #ifdef ZFREQO_EXPORTS
    #define ZFREQO_API __declspec(dllexport)
    #else
    #define ZFREQO_API __declspec(dllimport)
    #endif
#else
    #define ZFREQO_API
#endif
