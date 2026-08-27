#pragma once


#ifdef CSPRO_NO_DLLS
    #define ZFREQO_API
#elif defined(ZFREQO_EXPORTS)
    #define ZFREQO_API __declspec(dllexport)
#else
    #define ZFREQO_API __declspec(dllimport)
#endif
