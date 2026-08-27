#pragma once


#ifdef CSPRO_NO_DLLS
    #define ZACTION_API
#elif defined(ZACTION_EXPORTS)
    #define ZACTION_API __declspec(dllexport)
#else
    #define ZACTION_API __declspec(dllimport)
#endif
