#pragma once


#ifdef CSPRO_NO_DLLS
    #define ZHTML_API
#elif defined(ZHTML_EXPORTS)
    #define ZHTML_API __declspec(dllexport)
#else
    #define ZHTML_API __declspec(dllimport)
#endif


#ifdef WIN_DESKTOP
extern AFX_EXTENSION_MODULE zHtmlDLL;
#endif
