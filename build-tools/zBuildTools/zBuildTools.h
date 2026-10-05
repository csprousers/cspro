#pragma once


#ifdef CSPRO_NO_DLLS
    #define ZBUILDTOOLS_API
#elif defined(ZBUILDTOOLS_EXPORTS)
    #define ZBUILDTOOLS_API __declspec(dllexport)
#else
    #define ZBUILDTOOLS_API __declspec(dllimport)
#endif
