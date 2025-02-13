#pragma once

#ifdef ZORDERF_IMPL
    #define CLASS_DECL_ZORDERF __declspec(dllexport)
#else
    #define CLASS_DECL_ZORDERF __declspec(dllimport)
#endif
