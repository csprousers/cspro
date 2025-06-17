#pragma once

//---------------------------------------------------------------------------
//  File name: CLinkSub.h
//
//  Description:
//          Header for CLinkSubTable class
//
//  History:    Date       Author   Comment
//              ---------------------------
//              25 Feb 03   RHF      Created
//
//---------------------------------------------------------------------------
#include <ZTBDO/zTbdO.h>
#include <ZTBDO/cttree.h>
#include <engine/dimens.h>

//class CLinkSubTable;
class CLASS_DECL_ZTBDO CLinkSubTableDim {
private:
    int             m_iDim; // DIM_ROW, DIM_COL, DIM_LAYER, DIM_NONE
    CString         m_csVarName[TBD_MAXDEPTH];
    int             m_iSeqNumber[TBD_MAXDEPTH]; // >= 1.

    void    Init();

public:
    CLinkSubTableDim();
    ~CLinkSubTableDim();

    void        SetDim( int iDim );
    void        SetVarName( CString csVarName, int iDepth );
    void        SetSeqNumber( int iSeqNumber, int iDepth );

    int         GetDim();
    CString     GetVarName( int iDepth );
    int         GetSeqNumber( int iDepth );

    friend     class CLinkSubTable;

};


class CLASS_DECL_ZTBDO CLinkSubTable {
private:
    CLinkSubTableDim    m_cDims[TBD_MAXDIM];

    void    Init();
    void    Copy( const CLinkSubTable& rLinkSubTable );
public:
    CLinkSubTable();
    ~CLinkSubTable();

    CLinkSubTable( const CLinkSubTable& rLinkSubTable );
    void operator=( const CLinkSubTable& rLinkSubTable );

    void AddSubTableDim( const CLinkSubTableDim& rLinkSubTableDim, int iDim );
    CLinkSubTableDim&   GetSubTableDim( int iDim );

};
