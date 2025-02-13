#pragma once

//---------------------------------------------------------------------------
//  File name: CLinkTerm.h
//
//  Description:
//          Header for CLinkTerm class
//
//  History:    Date       Author   Comment
//              ---------------------------
//              25 Feb 03   RHF      Created
//
//---------------------------------------------------------------------------

#include <ZTBDO/zTbdO.h>
#include <ZTBDO/cLinkVar.h>
#include <ZTBDO/cttree.h>


class CLASS_DECL_ZTBDO CLinkTerm
{
private:
    // Expresion coordinates
    CArray<CLinkVar, CLinkVar>  m_CoorExpr[TBD_MAXDEPTH]; // Left & Right expresion

    void           Init();
    void           Copy( CLinkTerm& rLinkTerm );

public:
    CLinkTerm();
    CLinkTerm( CLinkTerm& rLinkTerm);
    void operator=(CLinkTerm& rLinkTerm);
    ~CLinkTerm();

    // Get
    int         GetNumLinkVar( int iDepth );
    CLinkVar&   GetLinkVar( int iDepth, int iVar );

    void        AddLinkVar( CLinkVar& cLinkVar, int iDepth );

    int         GetNumCells();
};
