#pragma once


#ifdef CSPRO_NO_DLLS
    #define ZBATCHF_API
#elif defined(ZBATCHF_EXPORTS)
    #define ZBATCHF_API __declspec(dllexport)
#else
    #define ZBATCHF_API __declspec(dllimport)
#endif
