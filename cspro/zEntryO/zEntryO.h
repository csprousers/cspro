#pragma once

#ifdef _WIN32
#ifdef ZENTRYO_IMPL
#define CLASS_DECL_ZENTRYO __declspec(dllexport)
#else
#define CLASS_DECL_ZENTRYO __declspec(dllimport)
#endif
#else
#define CLASS_DECL_ZENTRYO
#endif
