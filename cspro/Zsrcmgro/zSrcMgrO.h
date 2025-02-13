#pragma once

#ifdef WIN32
    #ifdef ZSRCMGR_IMPL
    #define CLASS_DECL_ZSRCMGR __declspec(dllexport)
    #else
    #define CLASS_DECL_ZSRCMGR __declspec(dllimport)
    #endif
#else
    #define CLASS_DECL_ZSRCMGR
#endif
