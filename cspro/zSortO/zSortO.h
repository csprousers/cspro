#pragma once


#ifdef CSPRO_NO_DLLS
    #define ZSORTO_API
#elif defined(ZSORTO_EXPORTS)
    #define ZSORTO_API __declspec(dllexport)
#else
    #define ZSORTO_API __declspec(dllimport)
#endif
