#pragma once


#ifdef CSPRO_NO_DLLS
    #define ZDATAO_API
#elif defined(ZDATAO_EXPORTS)
    #define ZDATAO_API __declspec(dllexport)
#else
    #define ZDATAO_API __declspec(dllimport)
#endif


#ifdef WIN_DESKTOP
extern AFX_EXTENSION_MODULE zDataODLL;
#endif
