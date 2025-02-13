#pragma once

#ifdef _WIN32
#ifdef ZDESIGNERF_EXPORTS
    #define CLASS_DECL_ZDESIGNERF __declspec(dllexport)
#else
    #define CLASS_DECL_ZDESIGNERF __declspec(dllimport)
#endif
#else
#define CLASS_DECL_ZDESIGNERF
#endif

#ifdef WIN_DESKTOP
extern AFX_EXTENSION_MODULE zDesignerFDLL;
#endif
