#pragma once

#ifdef WIN32
#ifdef ZTOOLSO_IMPL
#define CLASS_DECL_ZTOOLSO __declspec(dllexport)
#else
#define CLASS_DECL_ZTOOLSO __declspec(dllimport)
#endif
#else
#define CLASS_DECL_ZTOOLSO
#endif
