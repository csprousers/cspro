#pragma once

//***************************************************************************
//  File name: zBridgeO.h
//
//  Description:
//       Header for IMSA Bridge
//
//  History:    Date       Author     Comment
//              -----------------------------
//              1997         srs      created
//
//***************************************************************************

#ifdef CSPRO_NO_DLLS
    #define CLASS_DECL_ZBRIDGEO
#elif defined(ZBRIDGEO_IMPL)
    #define CLASS_DECL_ZBRIDGEO __declspec(dllexport)
#else
    #define CLASS_DECL_ZBRIDGEO __declspec(dllimport)
#endif


#ifdef WIN_DESKTOP
extern AFX_EXTENSION_MODULE zBridgeODLL;
#endif
