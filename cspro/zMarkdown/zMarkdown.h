#pragma once


#ifdef CSPRO_NO_DLLS
    #define ZMARKDOWN_API
#elif defined(ZMARKDOWN_EXPORTS)
    #define ZMARKDOWN_API __declspec(dllexport)
#else
    #define ZMARKDOWN_API __declspec(dllimport)
#endif
