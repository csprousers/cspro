#pragma once
//---------------------------------------------------------------------------
//  File name: BaseTab2.h
//
//  Description:
//          Header for CBaseTable2 class
//          This class will be used by interface and engine
//
//  History:    Date       Author   Comment
//              ---------------------------
//              01 Jul 01   RHF      Created
//
//---------------------------------------------------------------------------

#include <ZTBDO/zTbdO.h>
#include <engine/ttype.h>

class CTableAcum;
#include <ZTBDO/cttree.h>


// To be used by TBD and interpreter
class CLASS_DECL_ZTBDO CBaseTable2 {

private:

    // m_pAcum is allocated for tables/sub-tables.
    // m_pAcum::m_pData is allocated only for a table
    CTableAcum*                                 m_pAcum;

    //   bool                                       m_bSubTable;

//    CArray<CBaseTable2*,CBaseTable2*>   m_aRelatedTables;
    CBaseTable2*                                            m_pParentRelatedTable;

    //Parent table: Null if m_SubTable is false (i.e. NULL if is a table)
    CBaseTable2*                                                m_pParentTable;

    // Indexes relative to m_pParentTable
    // if m_bSubTable is false, m_aBaseIndex contains only 0's.
    CArray< CArray<int,int>, CArray<int,int> >  m_iBaseIndexes;


protected:
    CArray<int,int>                     m_aDim; // Dimensions
    CTableDef::ETableType                       m_eTableType;
    int                                                 m_iCellSize;
    csprochar                                                m_cOtherInfo;



private:
    // Init
    void Init();


public:
    CBaseTable2();

    // Create as table
    // Only the tables have related tables
    CBaseTable2( CArray<int,int>& aDim, CTableDef::ETableType eTableType, int iCellSize, csprochar cOtherInfo, CBaseTable2* pParentRelatedTable=NULL );

    /*
    // Create as sub-table
    CBaseTable2( CArray<int,int>& aDim, CArray< CArray<int,int>, CArray<int,int> >& aBaseIndexes, CBaseTable2* pParentTable );
    */

    virtual ~CBaseTable2();


    // Dimensions
    void SetNumDims( CArray<int,int>& aDim );
    int GetNumDims() const;

    int GetDimSize( int iDim ) const;
    int GetDimSize( int* aIndex ) const;

    int GetDimSize( CArray<int,int>& aDim) const;

    void SetDimSize(CArray<int,int>& aDim);

    // Table Type
    CTableDef::ETableType GetTableType();
    void SetTableType( CTableDef::ETableType eTableType );

    // Cell Size
    unsigned int     GetCellSize();
    void             SetCellSize( int iSize );

    // Other Info
    csprochar            GetOtherInfo();
    void            SetOtherInfo( csprochar cOtherInfo );



    void                SetParentRelatedTable( CBaseTable2* pParentRelatedTable );
    CBaseTable2*            GetParentRelatedTable();

    void                SetParentTable( CBaseTable2* pParentTable);
    CBaseTable2*            GetParentTable();


    // Base indexes
    int GetBaseIndex( int iDim, int iSubPart );
    void AddBaseIndex( int iDim, int iIndex );
    int GetNumBaseIndexes( int iDim );

    void RemoveBaseIndexes( int iDim );
};
