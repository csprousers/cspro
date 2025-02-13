#pragma once

#ifdef WIN32
#ifdef ZMULTIMEDIAO_EXPORTS
#define ZMULTIMEDIAO_API __declspec(dllexport)
#else
#define ZMULTIMEDIAO_API __declspec(dllimport)
#endif
#else
#define ZMULTIMEDIAO_API
#endif
