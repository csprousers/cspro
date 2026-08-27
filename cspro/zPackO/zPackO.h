#pragma once


#ifdef CSPRO_NO_DLLS
    #define ZPACKO_API
#elif defined(ZPACKO_EXPORTS)
    #define ZPACKO_API __declspec(dllexport)
#else
    #define ZPACKO_API __declspec(dllimport)
#endif
