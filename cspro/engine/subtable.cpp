#include "StandardSystemIncludes.h"

#if defined(USE_BINARY) // IGNORE_CTAB
#else

#include "Comp.h"
#include "CtUnit.h"
#include "Ctab.h"
#include <zEngineO/ValueSet.h>


void  CSubTable::Init() {
    SetSymCtab( 0 );
    SetUsed( false );
    m_csName = _T("");

    m_aItemSymbols.RemoveAll();
    m_aStatValue.RemoveAll();


    m_aCtCoordNumber.RemoveAll();
    m_mapSubTableToTable.RemoveAll();
    m_mapTableToSubTable.RemoveAll();

    SetAuxSymCtab( 0 );
    SetStatType( CTSTAT_NONE );
//    SetSymStatVar( 0 );
    SetStatNumber( -1 );
    SetSymStatDim( DIM_NODIM, -1 );

    SetStatBase( NULL );

    SetRightCells( 0 );

    for( int iDim=0; iDim < TBD_MAXDIM; iDim++ ) {
        for( int iDepth=0; iDepth < TBD_MAXDEPTH; iDepth++ ) {
            m_iCtTree[iDim][iDepth] = -1;
        }
    }
}

void CSubTable::Copy( CSubTable& cSubTable ) {
    SetSymCtab( cSubTable.GetSymCtab() );
    m_bIsUsed = cSubTable.m_bIsUsed;
    m_csName = cSubTable.m_csName;

    // m_aItemSymbols
    m_aItemSymbols.RemoveAll();
    for( int iSymbol=0; iSymbol < cSubTable.m_aItemSymbols.GetSize(); iSymbol++ ) {
        m_aItemSymbols.Add( cSubTable.m_aItemSymbols.ElementAt(iSymbol) );
    }

    //m_aStatValue
    m_aStatValue.RemoveAll();
    for( int iValue=0; iValue < cSubTable.m_aStatValue.GetSize(); iValue++ ) {
        m_aStatValue.Add( cSubTable.m_aStatValue.ElementAt(iValue) );
    }


    // m_aCtCoordNumber
    m_aCtCoordNumber.RemoveAll();

    POSITION pos = cSubTable.m_aCtCoordNumber.GetStartPosition();
    int iCoordNumber1, iCoordNumber2;

    while( pos ) {
        cSubTable.m_aCtCoordNumber.GetNextAssoc( pos, iCoordNumber1, iCoordNumber2 );
        m_aCtCoordNumber.SetAt( iCoordNumber1, iCoordNumber2 );
    }

    // m_mapSubTableToTable
    int          iRemapCoordNumber1;
    CRemapCoord  iRemapCoordNumber2;

    m_mapSubTableToTable.RemoveAll();
    pos = cSubTable.m_mapSubTableToTable.GetStartPosition();
    while( pos ) {
        cSubTable.m_mapSubTableToTable.GetNextAssoc( pos, iRemapCoordNumber1, iRemapCoordNumber2 );
        m_mapSubTableToTable.SetAt( iRemapCoordNumber1, iRemapCoordNumber2 );
    }

    // m_mapTableToSubTable
    m_mapTableToSubTable.RemoveAll();
    pos = cSubTable.m_mapTableToSubTable.GetStartPosition();
    while( pos ) {
        cSubTable.m_mapTableToSubTable.GetNextAssoc( pos, iRemapCoordNumber1, iRemapCoordNumber2 );
        m_mapTableToSubTable.SetAt( iRemapCoordNumber1, iRemapCoordNumber2 );
    }

    // Stat
    SetAuxSymCtab( cSubTable.GetAuxSymCtab() );
    SetStatType( cSubTable.GetStatType() );
//    SetSymStatVar( cSubTable.GetSymStatVar() );
    SetStatNumber( cSubTable.GetStatNumber() );

    int     iStatDim, iStatDepth;

    iStatDim = cSubTable.GetSymStatDim( &iStatDepth );
    SetSymStatDim( iStatDim, iStatDepth );

    SetStatBase( cSubTable.GetStatBase() );
    SetRightCells( cSubTable.GetRightCells() );

    // m_iCtTree
    for( int iDim=0; iDim < TBD_MAXDIM; iDim++ ) {
        for( int iDepth=0; iDepth < TBD_MAXDEPTH; iDepth++ ) {
            m_iCtTree[iDim][iDepth] = cSubTable.m_iCtTree[iDim][iDepth];
        }
    }

}

void CSubTable::GenName( CEngineArea* m_pEngineArea, int* pNodeBase[TBD_MAXDIM],  CString* csLongSubtableName )
{
    auto GetSymbolTable = [&]() -> const Logic::SymbolTable& { return m_pEngineArea->GetSymbolTable(); };

    CString csName[TBD_MAXDIM*TBD_MAXDEPTH];
    int     iSymbol[TBD_MAXDIM*TBD_MAXDEPTH];
    int     iSeq[TBD_MAXDIM*TBD_MAXDEPTH];
    int     i=0;

    for( int iDim=0; iDim < TBD_MAXDIM; iDim++ ) {
        for( int iDepth=0; iDepth < TBD_MAXDEPTH; iDepth++ ) {
            if( this->m_iCtTree[iDim][iDepth] >= 0 ) {
                CTNODE*     pNode=(CTNODE *) (pNodeBase[iDim] + this->m_iCtTree[iDim][iDepth] );

                ASSERT( pNode->isVarNode() ); // rcl, May 2005
                csName[i].Format( _T("%s(%d.%d)"), NPT(pNode->m_iSymbol)->GetName().c_str(), pNode->m_iSymbol, pNode->m_iNumCells );
                if( pNode->m_iStatType != CTSTAT_NONE ) {
                    csName[i] += _T("(") + CtStatBase::GetStatName((CStatType)pNode->m_iStatType) + _T(")");
                }
                iSymbol[i] = pNode->m_iSymbol;
                iSeq[i] = pNode->m_iSeqNumber;
            }
            else {
                csName[i] = _T("");
                iSymbol[i] = 0;
                iSeq[i] = 0;
            }
            i++;
        }
    }

    /*
    csLongSubtableName.Format( "(%s,%d,%d)%c(%s,%d,%d) BY\n(%s,%d,%d)%c(%s,%d,%d) BY\n(%s,%d,%d)%c(%s,%d,%d)",
    csName[0], iSymbol[0], iSeq[0], (csName[1].GetLength()>0) ? '*' : ' ', csName[1], iSymbol[1], iSeq[1],
    csName[2], iSymbol[2], iSeq[2], (csName[3].GetLength()>0) ? '*' : ' ', csName[3], iSymbol[3], iSeq[3],
    csName[4], iSymbol[4], iSeq[4], (csName[5].GetLength()>0) ? '*' : ' ', csName[5], iSymbol[5], iSeq[5] );
    */



    CString   csSubtableName1;

    csSubtableName1.Format( _T("%s%c%s %s %s%c%s %s %s%c%s"),
        csName[0].GetString(), (csName[1].GetLength()>0) ? _T('*') : _T(' '), csName[1].GetString(),
        (csName[2].GetLength()>0) ? _T("BY") : _T(""),
        csName[2].GetString(), (csName[3].GetLength()>0) ? _T('*') : _T(' '), csName[3].GetString(),
        (csName[4].GetLength()>0) ? _T("BY") : _T(""),
        csName[4].GetString(), (csName[5].GetLength()>0) ? _T('*') : _T(' '), csName[5].GetString() );

    csSubtableName1.TrimRight();

#ifdef _DEBUG
    CString   csSubtableName2;

    csSubtableName2.Format( _T("(%d,%d)%c(%d,%d) BY (%d,%d)%c(%d,%d) BY (%d,%d)%c(%d,%d)"),
        iSymbol[0], iSeq[0], (csName[1].GetLength()>0) ? _T('*') : _T(' '),  iSymbol[1], iSeq[1],
        iSymbol[2], iSeq[2], (csName[3].GetLength()>0) ? _T('*') : _T(' '),  iSymbol[3], iSeq[3],
        iSymbol[4], iSeq[4], (csName[5].GetLength()>0) ? _T('*') : _T(' '),  iSymbol[5], iSeq[5] );

    csSubtableName2.TrimRight();
#endif

    if( csLongSubtableName != NULL ) {
#ifdef _DEBUG
        *csLongSubtableName = csSubtableName1 + _T("\n") + csSubtableName2;
#else
        *csLongSubtableName = csSubtableName1;
#endif
    }

    m_csName = csSubtableName1;
}


void CSubTable::MakeItemSymbols( CEngineArea* m_pEngineArea, int* pNodeBase[TBD_MAXDIM] )
{
    auto GetSymbolTable = [&]() -> const Logic::SymbolTable& { return m_pEngineArea->GetSymbolTable(); };

    m_aItemSymbols.RemoveAll();

    for( int iDim=0; iDim < TBD_MAXDIM; iDim++ ) {
        for( int iDepth=0; iDepth < TBD_MAXDEPTH; iDepth++ ) {
            if( this->m_iCtTree[iDim][iDepth] >= 0 ) {
                CTNODE* pNode = (CTNODE *) (pNodeBase[iDim] + this->m_iCtTree[iDim][iDepth] );
                ASSERT( pNode->isVarNode() );
                int     iMultSymbol=pNode->m_iSymbol;
                VART*   pVarT;

                if( NPT(iMultSymbol)->IsA(SymbolType::Variable) )
                    pVarT = VPT(iMultSymbol);
                else {
                    ASSERT( NPT(iMultSymbol)->GetType() == SymbolType::ValueSet );
                    const ValueSet* pValueSetVar = &GetSymbolValueSet(iMultSymbol);
                    ASSERT(!pValueSetVar->IsDynamic());
                    pVarT = pValueSetVar->GetVarT();
                }


                int iItemSymbol = pVarT->GetSymbolIndex();

                m_aItemSymbols.Add( iItemSymbol );
            }
        }
    }
}

void CSubTable::GetMultItemSymbols( CEngineArea* m_pEngineArea, int* pNodeBase[TBD_MAXDIM], CStMultVarSpecArray& aMultItemSymbols )
{
    auto GetSymbolTable = [&]() -> const Logic::SymbolTable& { return m_pEngineArea->GetSymbolTable(); };

    for( int iDim=0; iDim < TBD_MAXDIM; iDim++ ) {
        for( int iDepth=0; iDepth < TBD_MAXDEPTH; iDepth++ ) {
            int iOffset = this->m_iCtTree[iDim][iDepth];
            if( iOffset >= 0 ) {
                CTNODE* pNode = (CTNODE *) ( pNodeBase[iDim] + iOffset );
                ASSERT( pNode->isVarNode() );
                int     iMultSymbol = pNode->m_iSymbol;
                VART*   pVarT;

                if( NPT(iMultSymbol)->IsA(SymbolType::Variable) )
                    pVarT = VPT(iMultSymbol);
                else {
                    ASSERT( NPT(iMultSymbol)->GetType() == SymbolType::ValueSet );
                    const ValueSet* pValueSetVar = &GetSymbolValueSet(iMultSymbol);
                    ASSERT(!pValueSetVar->IsDynamic());
                    pVarT = pValueSetVar->GetVarT();
                }

                int iVarSymbol = pVarT->GetSymbolIndex();

                // Multiple without use of parentesis
                if( pVarT->IsArray() && pNode->m_iCtOcc < 0 )
                    aMultItemSymbols.Add( CStMultVarSpec( iDim, iOffset, iVarSymbol ) );
            }
        }
    }
}

void CSubTable::MakeCoordNumberMap( CTAB* pCtab, int* pNodeBase[TBD_MAXDIM] ) {
    m_aCtCoordNumber.RemoveAll();

    for( int iDim=0; iDim < TBD_MAXDIM; iDim++ ) {
        for( int iDepth=0; iDepth < TBD_MAXDEPTH; iDepth++ ) {
            if( m_iCtTree[iDim][iDepth] >= 0 ) {
                CTNODE* pNode = (CTNODE *) (pNodeBase[iDim] + m_iCtTree[iDim][iDepth] );
                m_aCtCoordNumber.SetAt( pNode->m_iCoordNumber, pNode->m_iCoordNumber );
            }
        }
    }
}


bool CSubTable::UseCoordNumber( int iCoordNumber ) {
    ASSERT( iCoordNumber != 0 );
    int     iDummy;
    return m_aCtCoordNumber.Lookup( iCoordNumber, iDummy ) == TRUE;
}


void CSubTable::CleanVectorItems( CEngineArea* m_pEngineArea )
{
    auto GetSymbolTable = [&]() -> const Logic::SymbolTable& { return m_pEngineArea->GetSymbolTable(); };

    for( int i=0; i < m_aItemSymbols.GetSize(); i++ ) {
        int&        iVarSymbol=m_aItemSymbols.ElementAt(i);

        VART*       pVarT = VPT(iVarSymbol);

        pVarT->VectorClean();
    }
}

void CSubTable::GetAuxCtabDims( CEngineArea* m_pEngineArea, int iDim[TBD_MAXDIM] )
{
    auto GetSymbolTable = [&]() -> const Logic::SymbolTable& { return m_pEngineArea->GetSymbolTable(); };

    for( int i=0; i < TBD_MAXDIM; i++ )
        iDim[i] = 0;

    if( GetAuxSymCtab() <= 0 )
        return;

    CTAB* pAuxCtab=XPT(GetAuxSymCtab());

    for( int i=0; i < TBD_MAXDIM; i++ )
        iDim[i] = pAuxCtab->GetDimSize(i);
}


void CSubTable::GenRemapCoord( CEngineArea* m_pEngineArea )
{
    auto GetSymbolTable = [&]() -> const Logic::SymbolTable& { return m_pEngineArea->GetSymbolTable(); };

    //Generate SubTable to Table map
    m_mapSubTableToTable.RemoveAll();

    //Generate Table to SubTable map
    m_mapTableToSubTable.RemoveAll();

    CTAB* pCtab=XPT(GetSymCtab());
    pCtab->GenRemapCoord( this );
}

bool CSubTable::GetSubTableCoord( int &iSubTableI, int &iSubTableJ, int &iSubTableK, const int iTableOffset ) {
    CRemapCoord     cSubTableCoord;

    if( m_mapTableToSubTable.Lookup( iTableOffset, cSubTableCoord ) == TRUE ) {
        iSubTableI = cSubTableCoord.m_iRow;
        iSubTableJ = cSubTableCoord.m_iCol;
        iSubTableK = cSubTableCoord.m_iLay;

        return true;
    }

    return false;
}

bool CSubTable::GetTableCoord( int &iTableI, int &iTableJ, int &iTableK, const int iSubTableOffset ) {
    CRemapCoord     cTableCoord;

    if( m_mapSubTableToTable.Lookup( iSubTableOffset, cTableCoord ) == TRUE ) {
        iTableI = cTableCoord.m_iRow;
        iTableJ = cTableCoord.m_iCol;
        iTableK = cTableCoord.m_iLay;

        return true;
    }

    return false;
}

#endif
