#pragma once

#ifdef  ZTABLEF_IMPL
    #define CLASS_DECL_ZTABLEF __declspec(dllexport)
#else
    #define CLASS_DECL_ZTABLEF __declspec(dllimport)
#endif
