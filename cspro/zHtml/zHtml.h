#pragma once

#ifdef WIN32
    #ifdef ZHTML_EXPORTS
    #define ZHTML_API __declspec(dllexport)
    #else
    #define ZHTML_API __declspec(dllimport)
    #endif
#else
    #define ZHTML_API
#endif

#ifdef WIN_DESKTOP
extern AFX_EXTENSION_MODULE zHtmlDLL;
#endif
