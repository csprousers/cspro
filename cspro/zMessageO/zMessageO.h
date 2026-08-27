#pragma once


#ifdef CSPRO_NO_DLLS
    #define ZMESSAGEO_API
#elif defined(ZMESSAGEO_EXPORTS)
    #define ZMESSAGEO_API __declspec(dllexport)
#else
    #define ZMESSAGEO_API __declspec(dllimport)
#endif
