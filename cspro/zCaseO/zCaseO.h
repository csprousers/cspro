#pragma once


#ifdef CSPRO_NO_DLLS
    #define ZCASEO_API
#elif defined(ZCASEO_EXPORTS)
    #define ZCASEO_API __declspec(dllexport)
#else
    #define ZCASEO_API __declspec(dllimport)
#endif
