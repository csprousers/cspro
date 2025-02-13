#pragma once

#ifdef WIN32
    #ifdef SQLITE_EXPORTS
        #define SQLITE_API __declspec(dllexport)
    #else
        #define SQLITE_API __declspec(dllimport)
    #endif
#else
    #define SQLITE_API
#endif
