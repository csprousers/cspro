#pragma once


#ifdef CSPRO_NO_DLLS
    #define ZMULTIMEDIAO_API
#elif defined(ZMULTIMEDIAO_EXPORTS)
    #define ZMULTIMEDIAO_API __declspec(dllexport)
#else
    #define ZMULTIMEDIAO_API __declspec(dllimport)
#endif
