#pragma once


//---------------------------------------------------------------------------
//  File name: zTbdO.h
//
//  Description: Contain all the headers necesaries to use the zTbd.dll app.
//
//  History:    Date       Author   Comment
//              ---------------------------
//              24 Jul 01   DVB     Created
//
//---------------------------------------------------------------------------

#ifdef CSPRO_NO_DLLS
    #define CLASS_DECL_ZTBDO
#elif defined(ZTBDO_IMPL)
    #define CLASS_DECL_ZTBDO __declspec(dllexport)
#else
    #define CLASS_DECL_ZTBDO __declspec(dllimport)
#endif
