#pragma once


#ifdef CSPRO_NO_DLLS
    #define ZENGINEO_API
#elif defined(ZENGINEO_EXPORTS)
    #define ZENGINEO_API __declspec(dllexport)
#else
    #define ZENGINEO_API __declspec(dllimport)
#endif
