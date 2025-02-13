#pragma once

#ifdef WIN32
    #ifdef ZCASETREEF_EXPORTS
    #define ZCASETREEF_API __declspec(dllexport)
    #else
    #define ZCASETREEF_API __declspec(dllimport)
    #endif
#else
    #define ZCASETREEF_API
#endif
