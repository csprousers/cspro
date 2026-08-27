#pragma once


#ifdef CSPRO_NO_DLLS
    #define ZNETWORK_API
#elif defined(ZNETWORK_EXPORTS)
    #define ZNETWORK_API __declspec(dllexport)
#else
    #define ZNETWORK_API __declspec(dllimport)
#endif
