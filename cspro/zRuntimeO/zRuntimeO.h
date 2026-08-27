#pragma once


#ifdef CSPRO_NO_DLLS
    #define ZRUNTIMEO_API
#elif defined(ZRUNTIMEO_EXPORTS)
    #define ZRUNTIMEO_API __declspec(dllexport)
#else
    #define ZRUNTIMEO_API __declspec(dllimport)
#endif
