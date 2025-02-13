#pragma once
//---------------------------------------------------------------------------
//  File name: CCross.h
//
//  Description:
//          Header for CCrossTable class
//          This class is intended to be shared by interface and engine
//
//  History:    Date       Author   Comment
//              ---------------------------
//              30 Jul 01   RHF      Created
//
//---------------------------------------------------------------------------
#include <ZTBDO/zTbdO.h>
#include <ZTBDO/basetab2.h>
#include <ZTBDO/subexpr.h>
#include <ZTBDO/coordmem.h>


/* definition to expand macro then apply to pragma message */
#define VALUE_TO_STRING(x) #x
#define VALUE(x) VALUE_TO_STRING(x)
#define VAR_NAME_VALUE(var) #var "="  VALUE(var)


class CLASS_DECL_ZTBDO CCrossTable : public CBaseTable2
{
private:

#if defined(USE_BINARY) // IGNORE_CTAB
#else
    CArray<CSubExpresion,CSubExpresion>   m_aSubExpresion;
#endif

    int     m_iTableLevel;
    bool    m_bApplDeclared;    // true declared in APP; false in other proc


    CArray<CString,CString>     m_csTitle;
    CArray<CString,CString>     m_csStubTitle;
    int     m_iTitleLen;        // Title, disk bytes used by title lines
    int     m_iStubLen;         // Stub,  disk bytes used by stub  lines

    int     m_iPageWidth;
    int     m_iStubWidth;
    int     m_iPageLen;
    int     m_iPercentDecs;



    void ExpandPolinom( CArray<CCoordMember,CCoordMember>& aCoordMember, int* pCtNodebase, const Logic::SymbolTable* pSymbolTable, int iOffset );

    void Init();

public:
    CCrossTable(); // : CBaseTable2();

    CCrossTable( CArray<int,int>& aDim, CTableDef::ETableType eTableType, int iCellSize, CBaseTable2* pParentRelatedTable=NULL );
        // : CBaseTable2( aDim, eTableType, iCellSize, pParentRelatedTable );

    virtual ~CCrossTable();

    // Fill m_aSubExpresion
    void MakeSubExpresion( CArray<int,int>& aNodeExpr, int* pCtNodebase, const Logic::SymbolTable* pSymbolTable );

    virtual int GetTableLevel() const { return m_iTableLevel; } // marked as virtual only so that it is accessible to zEngineO's SymbolCalculator
    void SetTableLevel(int iLevel)    { m_iTableLevel = iLevel; }


    bool IsApplDeclared();
    void SetApplDeclared( bool bApplDeclared );


    CArray<CString,CString>&    GetTitle();
    CArray<CString,CString>&    GetStubTitle();


    int   GetTitleLen();
    void  SetTitleLen( int iTitleLen );

    int   GetStubLen();
    void  SetStubLen( int iStubLen );

    int  GetPageWidth();
    void SetPageWidth( int iPageWidth );

    int  GetStubWidth();
    void SetStubWidth( int iStubWidth );

    int  GetPageLen();
    void SetPageLen( int iPageLen );

    int  GetPercentDecs();
    void SetPercentDecs( int iPercentDecs );
};
