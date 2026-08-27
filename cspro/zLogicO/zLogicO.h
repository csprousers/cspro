#pragma once


#ifdef CSPRO_NO_DLLS
    #define ZLOGICO_API
#elif defined(ZLOGICO_EXPORTS)
    #define ZLOGICO_API __declspec(dllexport)
#else
    #define ZLOGICO_API __declspec(dllimport)
#endif
