#pragma once


#ifdef CSPRO_NO_DLLS
    #define ZCONCATO_API
#elif defined(ZCONCATO_EXPORTS)
    #define ZCONCATO_API __declspec(dllexport)
#else
    #define ZCONCATO_API __declspec(dllimport)
#endif
