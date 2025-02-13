#pragma once

#ifdef WIN32
    #ifdef ZBATCHF_EXPORTS
    #define ZBATCHF_API __declspec(dllexport)
    #else
    #define ZBATCHF_API __declspec(dllimport)
    #endif
#else
    #define ZBATCHF_API
#endif
