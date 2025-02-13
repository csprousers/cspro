#pragma once

#ifdef WIN32
    #ifdef ZMESSAGEO_EXPORTS
    #define ZMESSAGEO_API __declspec(dllexport)
    #else
    #define ZMESSAGEO_API __declspec(dllimport)
    #endif
#else
    #define ZMESSAGEO_API
#endif
