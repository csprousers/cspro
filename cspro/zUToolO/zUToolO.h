#pragma once

/*************************************************************************************
Header file for the utool box stuff
**************************************************************************************/

#ifdef CSPRO_NO_DLLS
    #define OX_CLASS_DECL
#elif defined(ZUTOOLO_IMPL)
    #define OX_CLASS_DECL __declspec(dllexport)
#else
    #define OX_CLASS_DECL __declspec(dllimport)
#endif
