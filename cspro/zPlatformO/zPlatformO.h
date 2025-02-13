#pragma once

#ifdef WIN32
#ifdef ZPLATFORMO_IMPL
    #define CLASS_DECL_ZPLATFORMO_IMPL __declspec(dllexport)
#else
    #define CLASS_DECL_ZPLATFORMO_IMPL __declspec(dllimport)
#endif
#else
//No MFC style export needed for .so shared objects
#define CLASS_DECL_ZPLATFORMO_IMPL
#endif
