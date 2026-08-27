#pragma once


#ifdef CSPRO_NO_DLLS
    #define ZMAPPING_API
#elif defined(ZMAPPING_EXPORTS)
    #define ZMAPPING_API __declspec(dllexport)
#else
    #define ZMAPPING_API __declspec(dllimport)
#endif
