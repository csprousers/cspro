#pragma once


#ifdef CSPRO_NO_DLLS
    #define ZSQL_API
#elif defined(ZSQL_EXPORTS)
    #define ZSQL_API __declspec(dllexport)
#else
    #define ZSQL_API __declspec(dllimport)
#endif


#define SQLITE_API ZSQL_API


// SQLite compile-time options
#define SQLITE_ENABLE_MATH_FUNCTIONS
