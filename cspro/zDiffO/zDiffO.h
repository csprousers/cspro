#pragma once


#ifdef CSPRO_NO_DLLS
    #define ZDIFFO_API
#elif defined(ZDIFFO_EXPORTS)
    #define ZDIFFO_API __declspec(dllexport)
#else
    #define ZDIFFO_API __declspec(dllimport)
#endif
