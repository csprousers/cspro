#pragma once


#ifdef CSPRO_NO_DLLS
    #define ZPARADATAO_API
#elif defined(ZPARADATAO_EXPORTS)
    #define ZPARADATAO_API __declspec(dllexport)
#else
    #define ZPARADATAO_API __declspec(dllimport)
#endif
