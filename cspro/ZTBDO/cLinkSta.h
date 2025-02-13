#pragma once

//---------------------------------------------------------------------------
//  File name: CLinkSta.h
//
//  Description:
//          Header for CLinkStat class
//
//  History:    Date       Author   Comment
//              ---------------------------
//              25 Feb 03   RHF      Created
//
//---------------------------------------------------------------------------

#include <ZTBDO/zTbdO.h>

#ifndef USE_BINARY // IGNORE_CTAB

#include <ZTBDO/ctstat.h>
#include <ZTBDO/cLinkSub.h>

class CLASS_DECL_ZTBDO CLinkStatVar {
public:
    CString     m_csVarName;
    int         m_iSeqNumber;

    void        Init();
public:
    CLinkStatVar();
    ~CLinkStatVar();

    void        SetName( CString csVarName );
    void        SetSeqNumber( int iSeqNumber );

    CString     GetName();
    int         GetSeqNumber();
};

class CLASS_DECL_ZTBDO CLinkStat {
private:
    CArray<CLinkSubTable,CLinkSubTable> m_aSubTables;
    CArray<CLinkStatVar,CLinkStatVar>   m_aStatVars;
    //Savy for SCanTables in Batch --CArray<CtStatBase,CtStatBase>       m_aStatBase;
    CArray<CtStatBase*,CtStatBase*>       m_aStatBase;

    void                    Copy( CLinkStat& rLinkStat );

public:
    CLinkStat();
    /*CLinkStat( CLinkStat& rLinkStat);
    void operator=(CLinkStat& rLinkStat);*/

    void                    Init();

    ~CLinkStat();
    void AddSubTable( CLinkSubTable& rLinkSubTable );
    int  GetNumSubTable();
    CLinkSubTable&  GetSubTable( int i );

    void AddStatVar( CLinkStatVar& rLinkStatVar );
    int  GetNumStatVar();
    CLinkStatVar&   GetStatVar( int i );

    //Savy Changed these to Pointer
    //void        AddStat( CtStatBase& rStatBase );
    //CtStatBase&  GetStat( int iStat );
    int         GetNumStat();

    void        AddStat( CtStatBase*  pStatBase );
    CtStatBase* GetStat( int i );

    void  RemoveAllStat();

};

#endif
