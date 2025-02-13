#pragma once

#ifdef _WIN32
#ifdef ZUTILO_IMPL
    #define CLASS_DECL_ZUTILO __declspec(dllexport)
#else
    #define CLASS_DECL_ZUTILO __declspec(dllimport)
#endif
#else
#define CLASS_DECL_ZUTILO
#endif
