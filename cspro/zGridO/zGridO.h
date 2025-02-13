#pragma once

#ifdef ZGRIDO_IMPL
    #define CLASS_DECL_ZGRIDO __declspec(dllexport)
#else
    #define CLASS_DECL_ZGRIDO __declspec(dllimport)
#endif
