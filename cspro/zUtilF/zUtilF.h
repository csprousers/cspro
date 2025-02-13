#pragma once

#ifdef WIN32
    #ifdef ZUTILF_IMPL
    #define CLASS_DECL_ZUTILF __declspec(dllexport)
    #else
    #define CLASS_DECL_ZUTILF __declspec(dllimport)
    #endif
#else
    #define CLASS_DECL_ZUTILF
#endif

#ifdef WIN_DESKTOP
extern AFX_EXTENSION_MODULE zUtilFDLL;
#endif
