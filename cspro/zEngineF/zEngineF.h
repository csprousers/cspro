#pragma once

#ifdef _WIN32
#ifdef ZENGINEF_EXPORTS
    #define CLASS_DECL_ZENGINEF __declspec(dllexport)
#else
    #define CLASS_DECL_ZENGINEF __declspec(dllimport)
#endif
#else
#define CLASS_DECL_ZENGINEF
#endif
