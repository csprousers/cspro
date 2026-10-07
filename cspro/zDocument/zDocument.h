#pragma once


#ifdef CSPRO_NO_DLLS
    #define ZDOCUMENT_API
#elif defined(ZDOCUMENT_EXPORTS)
    #define ZDOCUMENT_API __declspec(dllexport)
#else
    #define ZDOCUMENT_API __declspec(dllimport)
#endif
