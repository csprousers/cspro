#pragma once


#ifdef CSPRO_NO_DLLS
    #define CLASS_DECL_ZDESIGNERF
#elif defined(ZDESIGNERF_EXPORTS)
    #define CLASS_DECL_ZDESIGNERF __declspec(dllexport)
#else
    #define CLASS_DECL_ZDESIGNERF __declspec(dllimport)
#endif


#ifdef WIN_DESKTOP
extern AFX_EXTENSION_MODULE zDesignerFDLL;
#endif
