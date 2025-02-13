#pragma once

#ifdef WIN32
#ifdef ZLOGICO_EXPORTS
    #define ZLOGICO_API __declspec(dllexport)
#else
    #define ZLOGICO_API __declspec(dllimport)
#endif // ZLOGICO_EXPORTS
#else
#define ZLOGICO_API
#endif
