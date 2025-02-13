#pragma once

#ifdef WIN32
    #ifdef ZINDEXO_EXPORTS
    #define ZINDEXO_API __declspec(dllexport)
    #else
    #define ZINDEXO_API __declspec(dllimport)
    #endif
#else
    #define ZINDEXO_API
#endif
