#pragma once


#ifdef CSPRO_NO_DLLS
    #define CLASS_DECL_ZUTILF
#elif defined(ZUTILF_IMPL)
    #define CLASS_DECL_ZUTILF __declspec(dllexport)
#else
    #define CLASS_DECL_ZUTILF __declspec(dllimport)
#endif


#ifdef WIN_DESKTOP
extern AFX_EXTENSION_MODULE zUtilFDLL;
#endif
