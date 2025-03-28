#pragma once


#ifdef WIN32
    #ifdef ZSQL_EXPORTS
        #define ZSQL_API __declspec(dllexport)
    #else
        #define ZSQL_API __declspec(dllimport)
    #endif
#else
    #define ZSQL_API
#endif

#define SQLITE_API ZSQL_API


// SQLite compile-time options
#define SQLITE_ENABLE_MATH_FUNCTIONS
