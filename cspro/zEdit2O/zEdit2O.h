#pragma once

#ifdef _WIN32
#ifdef ZEDIT2O_IMPL
    #define CLASS_DECL_ZEDIT2O __declspec(dllexport)
#else
    #define CLASS_DECL_ZEDIT2O __declspec(dllimport)
#endif
#else
#define CLASS_DECL_ZEDIT2O
#endif
