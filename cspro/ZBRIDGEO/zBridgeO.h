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

#ifdef WIN32
#ifdef ZBRIDGEO_IMPL
    #define CLASS_DECL_ZBRIDGEO __declspec(dllexport)
#else
    #define CLASS_DECL_ZBRIDGEO __declspec(dllimport)
#endif
#else
#define CLASS_DECL_ZBRIDGEO
#endif
