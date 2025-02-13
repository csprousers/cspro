//#ifndef _CBaseTab2_included
//#define _CBaseTab2_included
//---------------------------------------------------------------------------
//  File name: BaseTab2.cpp
//
//  Description:
//          Implementation for CBaseTable2 class
//
//  History:    Date       Author   Comment
//              ---------------------------
//              24 Oct 01   RHF      Created
//
//---------------------------------------------------------------------------
#include "StdAfx.h"
#include "basetab2.h"


void CBaseTable2::Init() {
    m_pAcum = NULL;
    m_pParentRelatedTable = NULL;
    m_pParentTable = NULL;

    m_aDim.RemoveAll();
    SetTableType( CTableDef::Ctab_NoType );
    SetCellSize( 0 );
    SetOtherInfo( 0 );
    m_iBaseIndexes.SetSize( TBD_MAXDIM );
    //        m_bSubTable = false;
}


CBaseTable2::CBaseTable2(){
    Init();
}


// Create as table
// Only the tables have related tables
CBaseTable2::CBaseTable2( CArray<int,int>& aDim, CTableDef::ETableType eTableType, int iCellSize, csprochar cOtherInfo, CBaseTable2* pParentRelatedTable ) {
    Init();

    ASSERT( aDim.GetSize() >= 1 && aDim.GetSize() <= TBD_MAXDIM );

    SetNumDims( aDim );

    // Base index begin in 0 in tables
    //for( int iDim=0; iDim < aDim.GetSize(); iDim++ )
    for( int iDim=0; iDim < TBD_MAXDIM; iDim++ )
        m_iBaseIndexes[iDim].Add( 0 );

    SetTableType( eTableType );
    SetCellSize( iCellSize );
    SetOtherInfo( cOtherInfo );
    //        m_bSubTable = false;
    m_pParentRelatedTable = pParentRelatedTable;
}

/*
// Create as sub-table
CBaseTable2::CBaseTable2( CArray<int,int>& aDim, CArray< CArray<int,int>, CArray<int,int> >& aBaseIndexes, CBaseTable2* pParentTable ) {
Init();

  ASSERT( pParentTable != NULL );
  ASSERT( aDim.GetSize() == pParentTable->GetNumDims() );
  ASSERT( aDim.GetSize() == aBaseIndexes.GetSize() );

    SetNumDims( aDim );
    m_aDim.Append( aDim );

      ASSERT( aBaseIndexes.GetSize() > 0 && aBaseIndexes.GetSize() <= TBD_MAXDIM );

        for( int iDim=0; iDim < aBaseIndexes.GetSize(); iDim++ ) {
        for( int iSubPart=0; iSubPart < aBaseIndexes[iDim].GetSize(); iSubPart++ )
        AddBaseIndex( iDim, aBaseIndexes[iDim].GetAt(iSubPart) );
        }

          SetTableType( pParentTable->GetTableType() );
          SetCellSize( pParentTable->GetCellSize() );
          //m_bSubTable = true;
          m_pParentTable = pParentTable;
          }
*/

CBaseTable2::~CBaseTable2(){}


// Dimensions
void CBaseTable2::SetNumDims( CArray<int,int>& aDim ) {
    if( aDim.GetSize() <= 0 || aDim.GetSize() > TBD_MAXDIM ) {
        ASSERT(0);
        return;
    }

    m_aDim.RemoveAll();
    m_aDim.Append( aDim );


    // Base index begin in 0 in tables
    //for( int iDim=0; iDim < aDim.GetSize(); iDim++ ) {
    for( int iDim=0; iDim < TBD_MAXDIM; iDim++ ) {
        m_iBaseIndexes[iDim].RemoveAll();
        m_iBaseIndexes[iDim].Add( 0 );
    }

}

int CBaseTable2::GetNumDims()  const
{
    return m_aDim.GetSize();
}

int CBaseTable2::GetDimSize( int iDim ) const
{
    if( iDim < 0 || iDim >= m_aDim.GetSize() )
        return 0;
    return m_aDim.GetAt( iDim );
}

int CBaseTable2::GetDimSize( int* aIndex ) const
{
    for( int iDim=0; iDim < m_aDim.GetSize(); iDim++ )
        *(aIndex + iDim ) = m_aDim.GetAt(iDim);

    return GetNumDims();
}

int CBaseTable2::GetDimSize( CArray<int,int>& aDim) const
{
    aDim.RemoveAll();
    aDim.Append( m_aDim );

    return GetNumDims();
}

void CBaseTable2::SetDimSize(CArray<int,int>& aDim) {
    m_aDim.RemoveAll();
    m_aDim.Append( aDim );
}

//  CArray<int,int>&  GetDimSize() { return m_aDim; }

// Table Type
CTableDef::ETableType CBaseTable2::GetTableType() { return m_eTableType; }
void CBaseTable2::SetTableType( CTableDef::ETableType eTableType ) { m_eTableType = eTableType; }

// Cell Size
unsigned int  CBaseTable2::GetCellSize() { return m_iCellSize; }
void          CBaseTable2::SetCellSize( int iSize ) { m_iCellSize = iSize; }

// Other Info
csprochar            CBaseTable2::GetOtherInfo() {  return m_cOtherInfo;}
void            CBaseTable2::SetOtherInfo( csprochar cOtherInfo ) { m_cOtherInfo = cOtherInfo; }



void                CBaseTable2::SetParentRelatedTable( CBaseTable2* pParentRelatedTable ) { m_pParentRelatedTable = pParentRelatedTable; }
CBaseTable2*            CBaseTable2::GetParentRelatedTable() { return m_pParentRelatedTable; }

void                CBaseTable2::SetParentTable( CBaseTable2* pParentTable)        { m_pParentTable = pParentTable; }
CBaseTable2*            CBaseTable2::GetParentTable()        { return m_pParentTable; }


// Base indexes
int CBaseTable2::GetBaseIndex( int iDim, int iSubPart ) {
    if( iDim < 0 || iDim >= TBD_MAXDIM ) {
        ASSERT(0);
        return 0;
    }

    if( iSubPart < 0 || iSubPart >= m_iBaseIndexes[iDim].GetSize() ) {
        ASSERT(0);
        return 0;
    }
    return m_iBaseIndexes[iDim].ElementAt(iSubPart);
}

void CBaseTable2::AddBaseIndex( int iDim, int iIndex ) {
    if( iDim < 0 || iDim >= TBD_MAXDIM ) {
        ASSERT(0);
        return;
    }
    m_iBaseIndexes[iDim].Add( iIndex );
}

int CBaseTable2::GetNumBaseIndexes( int iDim ) {
    if( iDim < 0 || iDim >= TBD_MAXDIM ) {
        ASSERT(0);
        return 0;
    }

    return m_iBaseIndexes[iDim].GetSize();
}

void CBaseTable2::RemoveBaseIndexes( int iDim ) {
    if( iDim < 0 || iDim >= TBD_MAXDIM ) {
        ASSERT(0);
        return;
    }

    m_iBaseIndexes[iDim].RemoveAll();
}


