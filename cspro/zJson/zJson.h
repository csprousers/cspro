#pragma once


#ifdef CSPRO_NO_DLLS
    #define ZJSON_API
#elif defined(ZJSON_EXPORTS)
    #define ZJSON_API __declspec(dllexport)
#else
    #define ZJSON_API __declspec(dllimport)
#endif
