#pragma once


#ifdef CSPRO_NO_DLLS
    #define ZJAVASCRIPT_API
#elif defined(ZJAVASCRIPT_EXPORTS)
    #define ZJAVASCRIPT_API __declspec(dllexport)
#else
    #define ZJAVASCRIPT_API __declspec(dllimport)
#endif
