#pragma once


#ifdef CSPRO_NO_DLLS
    #define CLASS_DECL_ZENTRYO
#elif defined(ZENTRYO_IMPL)
    #define CLASS_DECL_ZENTRYO __declspec(dllexport)
#else
    #define CLASS_DECL_ZENTRYO __declspec(dllimport)
#endif
