#pragma once


#ifdef CSPRO_NO_DLLS
    #define ZREPORTO_API
#elif defined(ZREPORTO_EXPORTS)
    #define ZREPORTO_API __declspec(dllexport)
#else
    #define ZREPORTO_API __declspec(dllimport)
#endif
