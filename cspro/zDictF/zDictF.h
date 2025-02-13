#pragma once

#ifdef ZDICTF_IMPL
    #define CLASS_DECL_ZDICTF __declspec(dllexport)
#else
    #define CLASS_DECL_ZDICTF __declspec(dllimport)
#endif
