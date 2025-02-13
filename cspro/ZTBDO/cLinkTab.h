#pragma once

#if defined(USE_BINARY) // IGNORE_CTAB
#else

//---------------------------------------------------------------------------
//  File name: CLinkTab.h
//
//  Description:
//          Header for CLinkTable class
//
//  History:    Date       Author   Comment
//              ---------------------------
//              25 Feb 03   RHF      Created
//
//---------------------------------------------------------------------------
#include <ZTBDO/zTbdO.h>
#include <ZTBDO/cLinkTer.h>
#include <ZTBDO/cLinkUnt.h>
#include <ZTBDO/cLinkSta.h>
#include <zToolsO/StringPos.h>
#include <zEngineO/AllSymbolDeclarations.h>


class CLASS_DECL_ZTBDO CLinkTable {
private:
    // CStrinPos members
    CStringPos     m_csFullCommand;
    CStringPos     m_csName;
    CStringPos     m_csRowExpr;
    CStringPos     m_csColExpr;
    CStringPos     m_csLayExpr;
    CStringPos     m_csSelect;
    CStringPos     m_csWeight;
    CStringPos     m_csTabLogic;
    CStringPos     m_csInclude;
    CStringPos     m_csExclude;
    CStringPos     m_csLevel;

    CStringPos     m_csTitle;
    CStringPos     m_csStubTitle;
    CStringPos     m_csNoBreak;
    CStringPos     m_csNoPrint;


    // Other members
    CString        m_csProcName; // The PROC name where the table is declared
    bool           m_bHasSyntaxError; // When true use only m_csFullCommand


    // Expresion coordinates
    CArray<CLinkTerm, CLinkTerm>  m_RowTermExpr;
    CArray<CLinkTerm, CLinkTerm>  m_ColTermExpr;
    CArray<CLinkTerm, CLinkTerm>  m_LayTermExpr;

    // Unit
    CArray<CLinkUnit, CLinkUnit>  m_aUnits;

    // Stat
    //Savy changed this to pointer
    //CArray<CLinkStat, CLinkStat>  m_aStats;
    CArray<CLinkStat*, CLinkStat*>  m_aStats;



    // Units
    void AddUnit( CLinkUnit& rLinkUnit );

    // Stats
    //void AddStat( CLinkStat& rLinkStat );
    void AddStat( CLinkStat* pLinkStat );


    // Init
    void           Init();



public:
    CLinkTable();
    ~CLinkTable();

    // ----- CStringPos based methods
    // Get
    CStringPos  GetFullCommand();
    CStringPos  GetName();
    CStringPos  GetRowExpr();
    CStringPos  GetColExpr();
    CStringPos  GetLayExpr();
    CStringPos  GetSelect();
    CStringPos  GetWeight();
    CStringPos  GetTablogic();
    CStringPos  GetInclude();
    CStringPos  GetExclude();
    CStringPos  GetLevel();

    CStringPos  GetTitle();
    CStringPos  GetStubTitle();
    CStringPos  GetNoBreak();
    CStringPos  GetNoPrint();




    //---- Other methods
    bool        GetHasSyntaxError();  // Only filled by the scanner
    CString     GetProcName();// Only filled by the scanner

    // Row,Col,Layer expresions
    CArray<CLinkTerm, CLinkTerm>& GetRowTermExpr();
    CArray<CLinkTerm, CLinkTerm>& GetColTermExpr();
    CArray<CLinkTerm, CLinkTerm>& GetLayTermExpr();

    // Unit
    int        GetNumUnit();
    CLinkUnit& GetUnit( int iUnit );

    // Stats
    int        GetNumStat();
    //Savy changed this to pointer
    CLinkStat* GetStat( int iStat );

    // Dimension
    int         GetNumCells( int iDim );


    friend      CTAB;
    friend      class CEngineCompFunc;

};

#endif
