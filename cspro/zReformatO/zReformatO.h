#pragma once


#ifdef CSPRO_NO_DLLS
    #define ZREFORMATO_API
#elif defined(ZREFORMATO_EXPORTS)
    #define ZREFORMATO_API __declspec(dllexport)
#else
    #define ZREFORMATO_API __declspec(dllimport)
#endif
