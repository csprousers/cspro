#pragma once


#ifdef CSPRO_NO_DLLS
    #define ZEXCELO_API
#elif defined(ZEXCELO_EXPORTS)
    #define ZEXCELO_API __declspec(dllexport)
#else
    #define ZEXCELO_API __declspec(dllimport)
#endif
