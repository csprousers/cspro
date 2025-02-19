#pragma once

#ifdef WIN32
    #ifdef ZXML_EXPORTS
    #define ZXML_API __declspec(dllexport)
    #else
    #define ZXML_API __declspec(dllimport)
    #endif
#else
    #define ZXML_API
#endif
