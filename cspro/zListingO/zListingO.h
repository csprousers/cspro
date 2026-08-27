#pragma once


#ifdef CSPRO_NO_DLLS
    #define ZLISTINGO_API
#elif defined(ZLISTINGO_EXPORTS)
    #define ZLISTINGO_API __declspec(dllexport)
#else
    #define ZLISTINGO_API __declspec(dllimport)
#endif
