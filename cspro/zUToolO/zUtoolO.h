#pragma once

/*************************************************************************************
Header file for the utool box stuff
**************************************************************************************/
#ifdef ZUTOOLO_IMPL
    #define OX_CLASS_DECL __declspec(dllexport)
#else
    #define OX_CLASS_DECL __declspec(dllimport)
#endif
