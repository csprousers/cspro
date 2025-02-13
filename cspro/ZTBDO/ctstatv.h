#pragma once
//---------------------------------------------------------------------------
//  File name: CtStatV.h
//
//  Description:
//          Header for CtStatVar class
//          This class keep a crosstab Stat Var
//
//  History:    Date       Author   Comment
//              ---------------------------
//              01 Jul 02   RHF     Created
//
//---------------------------------------------------------------------------
#include <ZTBDO/zTbdO.h>

class CLASS_DECL_ZTBDO CtStatVar {
private:
    int         m_iSubtableNumber;
    int         m_iCoordNumber;
    int         m_iSymVar;
    bool        m_bOverlappedCat;

public:
    CtStatVar();

    void Init();

    void    SetSubtableNumber( int iSubTableNumber );
    void    SetCoordNumber( int iCoordNumber );
    void    SetSymVar( int iSymVar );
    void    SetHasOverlappedCat( bool bOverlapped );

    int     GetSubtableNumber();
    int     GetCoordNumber();
    int     GetSymVar();
    bool    GetHasOverlappedCat();
#if defined(USE_BINARY) || defined(GENERATE_BINARY)
    CtStatVar( CtStatVar& other );
    void operator=( CtStatVar& other );

private:
    void cloneFrom( CtStatVar& other );
#endif // USE_BINARY || GENERATE_BINARY
};
