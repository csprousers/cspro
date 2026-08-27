#pragma once


#ifdef CSPRO_NO_DLLS
    #define ZHELP_API
#elif defined(ZHELP_EXPORTS)
    #define ZHELP_API __declspec(dllexport)
#else
    #define ZHELP_API __declspec(dllimport)
#endif
