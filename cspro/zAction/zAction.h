#pragma once

#ifdef WIN32
    #ifdef ZACTION_EXPORTS
    #define ZACTION_API __declspec(dllexport)
    #else
    #define ZACTION_API __declspec(dllimport)
    #endif
#else
    #define ZACTION_API
#endif
