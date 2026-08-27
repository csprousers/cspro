#pragma once


#ifdef CSPRO_NO_DLLS
    #define ZAPPO_API
#elif defined(ZAPPO_EXPORTS)
    #define ZAPPO_API __declspec(dllexport)
#else
    #define ZAPPO_API __declspec(dllimport)
#endif
