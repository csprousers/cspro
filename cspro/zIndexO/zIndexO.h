#pragma once


#ifdef CSPRO_NO_DLLS
    #define ZINDEXO_API
#elif defined(ZINDEXO_EXPORTS)
    #define ZINDEXO_API __declspec(dllexport)
#else
    #define ZINDEXO_API __declspec(dllimport)
#endif
