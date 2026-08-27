#pragma once


#ifdef CSPRO_NO_DLLS
    #define ZXML_API
#elif defined(ZXML_EXPORTS)
    #define ZXML_API __declspec(dllexport)
#else
    #define ZXML_API __declspec(dllimport)
#endif
