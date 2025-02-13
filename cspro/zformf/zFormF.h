#pragma once

#ifdef ZFORMF_IMPL
    #define CLASS_DECL_ZFORMF __declspec(dllexport)
#else
    #define CLASS_DECL_ZFORMF __declspec(dllimport)
#endif

extern HINSTANCE zFormF_hInstance;
