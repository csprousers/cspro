#pragma once


#ifndef WIN32
#   define ZGIT_API
#elif defined(ZGIT_IMPL)
#   define ZGIT_API __declspec(dllexport)
#else
#   define ZGIT_API __declspec(dllimport)
#endif
