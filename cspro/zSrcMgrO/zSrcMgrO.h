#pragma once


#ifdef CSPRO_NO_DLLS
    #define CLASS_DECL_ZSRCMGR
#elif defined(ZSRCMGR_IMPL)
    #define CLASS_DECL_ZSRCMGR __declspec(dllexport)
#else
    #define CLASS_DECL_ZSRCMGR __declspec(dllimport)
#endif
