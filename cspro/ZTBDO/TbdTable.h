#pragma once
//---------------------------------------------------------------------------
//  File name: TbdTable.h
//
//  Description:
//          Header for CTbdTable class
//          This class is intended define the table structure
//
//  History:    Date       Author   Comment
//              ---------------------------
//              3 Jul 01   DVB      Created
//
//---------------------------------------------------------------------------

#include <ZTBDO/zTbdO.h>
#include <ZTBDO/basetab2.h>
#include <ZTBDO/BreakIt.h>

// ---------------------------------
// Defines
// ---------------------------------
#define TBD_TT_NAMELEN 256

// ---------------------------------
// Binary Table Structure
// ---------------------------------
typedef struct {
    csprochar cTableName[TBD_TT_NAMELEN];
    csprochar cNextTableName[TBD_TT_NAMELEN];
    csprochar cType;
    csprochar cNumDim;
    csprochar cNumTableBreaks;
    csprochar cOtherInfo;
    long lCellSize;
} TBD_TT;


class CLASS_DECL_ZTBDO CTbdTable : public CBaseTable2 {
protected:
    CString                 m_csName;
    CString                 m_csNextName;
    int                     m_iTbdFileBreakKeyLen; // RHF Sep 27, 2005

    // Pointers to  CTbdFile::m_aBreaKItem and if equal to number of breaks defined for the table
    CArray<CBreakItem*, CBreakItem*> m_aTableBreak;

public:
    CTbdTable(CString csName, CString csNextName, CArray<int,int>& aDim, CTableDef::ETableType eTableType, int iCellSize, csprochar cOtherInfo, int iTbdFileBreakKeyLen=-1);
    CTbdTable( CTbdTable& ctTable);
    CTbdTable();
    virtual ~CTbdTable();

    bool AddBreak( CBreakItem* pBreak );

    void                SetTableName( CString csName );

    CBreakItem*         GetBreak( int iBreakNum );
    CString             GetTableName();
    CString             GetNextTableName();
    int                 GetNumBreak();
    int                 GetDimSize(int iDim);
    int                 GetDimSize( int* aIndex );
    int                 GetDimSize( CArray<int,int>& rDim );

    void                SetTbdFileBreakKeyLen( int iTbdFileBreakKeyLen );
    int                 GetTbdFileBreakKeyLen();

    const CTbdTable& operator= (const CTbdTable& ctTable);
    //savy
    CArray<CBreakItem*, CBreakItem*>& GetBreakArray() { return m_aTableBreak;}
};
