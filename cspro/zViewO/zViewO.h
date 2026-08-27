#pragma once


#ifdef CSPRO_NO_DLLS
    #define ZVIEWO_API
#elif defined(ZVIEWO_EXPORTS)
    #define ZVIEWO_API __declspec(dllexport)
#else
    #define ZVIEWO_API __declspec(dllimport)
#endif
