#pragma once

//---------------------------------------------------------------------------
//  File name: CLinkVar.h
//
//  Description:
//          Header for CLinkVar class
//
//  History:    Date       Author   Comment
//              ---------------------------
//              25 Feb 03   RHF      Created
//
//---------------------------------------------------------------------------

#include <ZTBDO/zTbdO.h>
#include <ZTBDO/subrange.h>


class CLASS_DECL_ZTBDO CLinkVar
{
private:
    CString        m_csName;
    CString        m_csOcc;

    CArray<CSubRange,CSubRange>  m_aRanges;

    void           Init();
    void           Copy(const CLinkVar& rLinkVar );

public:
    CLinkVar();
    CLinkVar(const CLinkVar& rLinkVar );
    void operator=(const CLinkVar& rLinkVar );
    ~CLinkVar();

    CString     GetName() const;
    CString     GetOcc() const;
    int         GetNumRanges() const;
    CSubRange&  GetRange( int i );
    const CSubRange&  GetRange( int i ) const;
    int         GetNumCells();

    void        SetName( CString csName );
    void        SetOcc( CString csOcc );
    void        AddRange( const CSubRange& cRange );

    friend      class  CSymbolCtab; //changed CTAB to CSymbolCtab for porting
    friend      class  CEngineCompFunc;

};
