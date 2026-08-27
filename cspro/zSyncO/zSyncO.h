#pragma once


#ifdef CSPRO_NO_DLLS
    #define SYNC_API
#elif defined(ZSYNCO_EXPORTS)
    #define SYNC_API __declspec(dllexport)
#else
    #define SYNC_API __declspec(dllimport)
#endif
