//---------------------------------------------------------------------------
//  File name: CtUnit.cpp
//
//  Description:
//          Crosstab Unit Implementation
//
//  History:    Date       Author   Comment
//              ---------------------------
//              10 Jun 02  RHF     Creation
//              29 Nov 04  rcl     Extensions for 3d handling
//
//---------------------------------------------------------------------------

#include "StandardSystemIncludes.h"
#include "Comp.h"
#include "CtUnit.h"
#include "subtable.h"
#include "Ctab.h"


void CtUnit::Copy( CtUnit& ctUnit ) {
    SetNumber( ctUnit.GetNumber() );
    SetOwnerCtabSymbol( ctUnit.GetOwnerCtabSymbol() );
    SetInGlobal( ctUnit.InGlobal() );

    m_aSubTableIndex.RemoveAll();
    for( int i=0; i < ctUnit.m_aSubTableIndex.GetSize(); i++ )
        m_aSubTableIndex.Add( ctUnit.m_aSubTableIndex.ElementAt(i) );

    // Copy CoordNumber
    m_aCtCoordNumber.RemoveAll();

    POSITION pos = ctUnit.m_aCtCoordNumber.GetStartPosition();
    int iCoordNumber1, iCoordNumber2;

    while( pos ) {
        ctUnit.m_aCtCoordNumber.GetNextAssoc( pos, iCoordNumber1, iCoordNumber2 );
        m_aCtCoordNumber.SetAt( iCoordNumber1, iCoordNumber2 );
    }

    m_iUnitSymbol = ctUnit.m_iUnitSymbol;
    m_iLoopSymbol = ctUnit.m_iLoopSymbol;
    m_iCurrentIndex = ctUnit.m_iCurrentIndex;
    m_iSelectExpr = ctUnit.m_iSelectExpr;
    m_iWeightExpr = ctUnit.m_iWeightExpr;
    m_iTabLogicExpr = ctUnit.m_iTabLogicExpr;
    m_iUnitType = ctUnit.m_iUnitType;

    // rcl, 3d handling extensions
    m_bUseExtraInfo  = ctUnit.isUsingExtraInfo();
    if( isUsingExtraInfo() )
        setExtraInfo( ctUnit.getExtraInfoNode() );

    SetDefaultUnit( ctUnit.GetDefaultUnit() );
}

void CtUnit::Init() {
    SetNumber(-1);
    SetOwnerCtabSymbol(-1);
    SetInGlobal( false );

    m_aSubTableIndex.RemoveAll();
    m_aCtCoordNumber.RemoveAll();

    SetUnitSymbol(-1,-1);
    SetLoopSymbol(-1);
    SetCurrentIndex( -1 );
    SetSelectExpr(0);
    SetWeightExpr(0);
    SetTabLogicExpr(0);

    removeExtraInfo(); // rcl, Nov 2004

    SetDefaultUnit( false );
}

void CtUnit::MakeCoordNumberMap( CTAB* pCtab, int* pNodeBase[TBD_MAXDIM] ) {
    m_aCtCoordNumber.RemoveAll();

    for( int i=0; i < m_aSubTableIndex.GetSize(); i++ ) {
        int         iSubTable=m_aSubTableIndex.GetAt(i);
        CSubTable&  cSubTable=pCtab->GetSubTable(iSubTable);

        // Fill SubTable
        cSubTable.MakeCoordNumberMap( pCtab, pNodeBase );

        // Fill Unit
        for( int iDim=0; iDim < TBD_MAXDIM; iDim++ ) {
            for( int iDepth=0; iDepth < TBD_MAXDEPTH; iDepth++ ) {
                if( cSubTable.m_iCtTree[iDim][iDepth] >= 0 ) {
                    CTNODE* pNode = (CTNODE *) (pNodeBase[iDim] + cSubTable.m_iCtTree[iDim][iDepth] );
                    m_aCtCoordNumber.SetAt( pNode->m_iCoordNumber, pNode->m_iCoordNumber );
                }
            }
        }
    }
}

bool CtUnit::UseCoordNumber( int iCoorNumber ) {
    ASSERT( iCoorNumber >= 1 );
    int     iDummy;
    return m_aCtCoordNumber.Lookup( iCoorNumber, iDummy ) == TRUE;
}

void CtUnit::CleanVectorItems( CTAB* pCtab, CEngineArea* m_pEngineArea ) {
    for( int i=0; i < m_aSubTableIndex.GetSize(); i++ ) {
        int         iSubTable=m_aSubTableIndex.GetAt(i);

        CSubTable&  cSubTable=pCtab->GetSubTable(iSubTable);
        cSubTable.CleanVectorItems( m_pEngineArea );
    }
}

bool CtUnit::HasSubTable( int iSubTable ) {
    // TODO: Optimize
    for( int i=0; i < m_aSubTableIndex.GetSize(); i++ ) {
        if( iSubTable == m_aSubTableIndex.GetAt(i) )
            return true;
    }

    return false;
}
