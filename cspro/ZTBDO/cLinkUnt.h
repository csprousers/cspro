#pragma once

//---------------------------------------------------------------------------
//  File name: CLinkUnt.h
//
//  Description:
//          Header for CLinkUnit class
//
//  History:    Date       Author   Comment
//              ---------------------------
//              25 Feb 03   RHF      Created
//
//---------------------------------------------------------------------------
#include <ZTBDO/zTbdO.h>
#include <zToolsO/Tools.h>

class CLinkSubTable;

class CLASS_DECL_ZTBDO CLinkUnit {
private:
    CArray<CLinkSubTable,CLinkSubTable> m_aSubTables;

    CString                 m_csRepeatingKeyword;
    CString                 m_csRepeatingName;
    CString                 m_csSelectExpr;
    CString                 m_csWeightExpr;
    CString                 m_csTablogicExpr;

    void                    Copy( CLinkUnit& rLinkUnit );

public:
    CLinkUnit();
    ~CLinkUnit();
    CLinkUnit( CLinkUnit& rLinkUnit);
    void operator=(CLinkUnit& rLinkUnit);

    void                    Init();

    void AddSubTable( CLinkSubTable& rLinkSubTable );
    int  GetNumSubTable();
    CLinkSubTable&  GetSubTable( int i );

    void SetRepeatingKeyword( CString csRepeatingKeyword );
    void SetRepeatingName( CString csRepeatingName );
    void SetSelectExpr( CString csSelectExpr );
    void SetWeightExpr( CString csWeightExpr );
    void SetTablogicExpr( CString csTablogcExpr );


    CString GetRepeatingKeyword();
    CString GetRepeatingName();
    CString GetSelectExpr();
    CString GetWeightExpr();
    CString GetTablogicExpr();

};
