#pragma once


#ifdef CSPRO_NO_DLLS
    #define ZFORMATTERO_API
#elif defined(ZFORMATTERO_EXPORTS)
    #define ZFORMATTERO_API __declspec(dllexport)
#else
    #define ZFORMATTERO_API __declspec(dllimport)
#endif
