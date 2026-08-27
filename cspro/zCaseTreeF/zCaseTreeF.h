#pragma once


#ifdef CSPRO_NO_DLLS
    #define ZCASETREEF_API
#elif defined(ZCASETREEF_EXPORTS)
    #define ZCASETREEF_API __declspec(dllexport)
#else
    #define ZCASETREEF_API __declspec(dllimport)
#endif
