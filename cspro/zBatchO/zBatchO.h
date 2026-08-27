#pragma once


#ifdef CSPRO_NO_DLLS
    #define ZBATCHO_API
#elif defined(ZBATCHO_EXPORTS)
    #define ZBATCHO_API __declspec(dllexport)
#else
    #define ZBATCHO_API __declspec(dllimport)
#endif
