// subexpr.cpp: implementation of the CSubExpr class.
//
//////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "subexpr.h"
#include <zLogicO/SymbolTable.h>

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]= __FILE__;
#define new DEBUG_NEW
#endif


void CSubExpresion::Copy( CSubExpresion& rSubExpr ) {
    SetNumDim( rSubExpr.GetNumDim() );

    for( int iDim=0; iDim < TBD_MAXDIM; iDim++ ) {
        SetDepth( iDim, rSubExpr.GetDepth( iDim ) );

        RemoveBaseIndexes( iDim );
        for( int iSubPart=0; iSubPart < rSubExpr.GetNumBaseIndexes( iDim ); iSubPart++ )
            AddBaseIndex( iDim, rSubExpr.GetBaseIndex( iDim, iSubPart ) );

        for( int iDepth=0; iDepth < TBD_MAXDEPTH; iDepth++ ) {
            SetSymCoord( iDim, iDepth, rSubExpr.GetSymCoord( iDim, iDepth ) );
            // depth  doesn't depend of iDepth
            SetNumCells( iDim, iDepth, rSubExpr.GetNumCells( iDim, iDepth ) );

            RemoveSubRanges( iDim, iDepth );

            for( int iSubRange=0; iSubRange < rSubExpr.GetNumSubRanges( iDim, iDepth ); iSubRange++ ) {
                CSubRange*  pRange=rSubExpr.GetSubRange( iDim, iDepth, iSubRange );
                ASSERT( pRange != NULL );
                AddSubRange( iDim, iDepth, *pRange );
            }
        }
    }

    /*
    SetStat( rSubExpr.GetStat() );
    SetCount( rSubExpr.GetCount() );
    SetPct( rSubExpr.GetPct() );
    SetRowPct( rSubExpr.GetRowPct() );
    SetColPct( rSubExpr.GetColPct() );
    SetLayPct( rSubExpr.GetLayPct() );
    SetTotPct( rSubExpr.GetTotPct() );
    SetMin( rSubExpr.GetMin() );
    SetMax( rSubExpr.GetMax() );
    SetMode( rSubExpr.GetMode() );
    SetMean( rSubExpr.GetMean() );
    SetMedian( rSubExpr.GetMedian() );
    SetPercentile( rSubExpr.GetPercentile() );
    SetStdDev( rSubExpr.GetStdDev() );
    SetVariance( rSubExpr.GetVariance() );
    SetStdErr( rSubExpr.GetStdErr() );
    SetPctValidCases( rSubExpr.GetPctValidCases() );

    RemovePropRanges();
    for( int iSubRange=0; iSubRange < rSubExpr.GetNumPropRanges(); iSubRange++ ) {
        CSubRange*  pRange=rSubExpr.GetPropRange( iSubRange );

        ASSERT( pRange != NULL );
        AddPropRange( *pRange );
    }

    SetPropPct( rSubExpr.GetPropPct() );
    */

    //m_aOtherParameters.RemoveAll();
    for( int iDim=0; iDim < TBD_MAXDIM; iDim++ ) {
        for( int iDepth=0; iDepth < TBD_MAXDEPTH; iDepth++ ) {
            RemoveFunParams( iDim, iDepth );
            for( int iParam = 0; iParam < rSubExpr.GetNumFunParam( iDim, iDepth ); iParam++ ) {
                CString* pParam=rSubExpr.GetFunParam( iDim, iDepth, iParam );

                ASSERT( pParam != NULL );
                AddFunParam( iDim,  iDepth, *pParam );
            }
        }
    }
}

void CSubExpresion::Init() {
    SetNumDim( 0 );

    for( int iDim=0; iDim < TBD_MAXDIM; iDim++ ) {
        SetDepth( iDim, 0 );

        RemoveBaseIndexes( iDim );

        for( int iDepth=0; iDepth < TBD_MAXDEPTH; iDepth++ ) {
            SetSymCoord( iDim, iDepth, 0 );
            // Depth doesn't depend on iDepth
            SetNumCells( iDim, iDepth, 0 );
            // Base indexes doesn't depend on iDepth
            RemoveSubRanges( iDim, iDepth );
        }
    }

    /*
    SetStat( false );
    SetCount( false );
    SetPct( false );
    SetRowPct( false );
    SetColPct( false );
    SetLayPct( false );
    SetTotPct( false );
    SetMin( false );
    SetMax( false );
    SetMode( false );
    SetMean( false );
    SetMedian( false );
    SetPercentile( 0 );
    SetStdDev( false );
    SetVariance( false );
    SetStdErr( false );
    SetPctValidCases( false );

    RemovePropRanges();

    SetPropPct( false );
    */

    //m_aOtherParameters.RemoveAll();

    for( int iDim=0; iDim < TBD_MAXDIM; iDim++ )
        for( int iDepth=0; iDepth < TBD_MAXDEPTH; iDepth++ )
            RemoveFunParams( iDim, iDepth );
}


CString CSubExpresion::GetLabel( Logic::SymbolTable* pSymbolTable ) {
    Symbol*  pSymbol;
    CString         csName;
    CString         csLabel;

    for( int iDim=0; iDim < GetNumDim(); iDim++ ) {
        if( iDim >= 1 )
            csLabel = csLabel + _T(" BY ");

        for( int iDepth=0; iDepth < GetDepth(iDim); iDepth++ ) {
            // Symbol Name
            int iSymCoord = this->GetSymCoord( iDim, iDepth );

            ASSERT( iSymCoord > 0 );

            pSymbol = (iSymCoord > 0) ? &pSymbolTable->GetAt(iSymCoord) : NULL;

            csName = (pSymbol==NULL) ? _T("none") : WS2CS(pSymbol->GetName());

            if( iDepth == 0 )
                csLabel = csLabel + csName;
            else
                csLabel = csLabel + CString(_T(" * ")) + csName;

        }
    }

    return csLabel;
}

#ifdef _DEBUG
void CSubExpresion::Dump( CArray<CString, CString>& aDump, Logic::SymbolTable* pSymbolTable, CString csPrefix ) {
    CString     csMsg;
    Symbol* pSymbol;
    int         iSymCoord;


    for( int iDim=0; iDim < TBD_MAXDIM; iDim++ ) {
        csMsg.Format( _T("%sSTART DIMENSION %d"), csPrefix.GetString(), iDim+1 );
        aDump.Add( csMsg );

        for( int iDepth=0; iDepth < TBD_MAXDEPTH; iDepth++ ) {

            // Symbol Name
            iSymCoord = this->GetSymCoord( iDim, iDepth );

            pSymbol = (iSymCoord > 0) ? &pSymbolTable->GetAt(iSymCoord) : NULL;
            csMsg.Format( _T("%sName=%s"), csPrefix.GetString(), (pSymbol==NULL) ? _T("none") : pSymbol->GetName().c_str() );
            aDump.Add( csMsg );


            // Cell numbers
            csMsg.Format( _T("%sNumCells=%d"), csPrefix.GetString(), this->GetNumCells(iDim, iDepth) );
            aDump.Add( csMsg );

            // Base Index
            if( iDepth == 0 ) {
                int     iBaseIndex;

                for( int iSubPart=0; iSubPart < this->GetNumBaseIndexes( iDim ); iSubPart++ ) {
                    iBaseIndex = this->GetBaseIndex(iDim, iSubPart );

                    csMsg.Format( _T("%sStart Index (sub-part %d of %d)=%d"), csPrefix.GetString(), iSubPart+1, this->GetNumBaseIndexes( iDim ), iBaseIndex );
                    aDump.Add( csMsg );
                }
            }


            // Sub-Ranges
            for( int i = 0; i < this->GetNumSubRanges( iDim, iDepth ); i++ ) {
                CSubRange*  pSubRange= this->GetSubRange( iDim, iDepth, i );
                bool    bImplicit;
                int     iLow = (int) pSubRange->GetLow();
                int     iHigh = (int) pSubRange->GetHigh( &bImplicit );
                int     iCollapsed=pSubRange->GetCollapsed();

                if( bImplicit )
                    csMsg.Format( _T("%sSubRange %d (Collapsed=%d)=%d"), csPrefix.GetString(), i+1, iCollapsed, iLow );
                else
                    csMsg.Format( _T("%sSubRange %d (Collapsed=%d)=%d:%d"), csPrefix.GetString(), i+1, iCollapsed, iLow, iHigh );

                aDump.Add( csMsg );
            }
        }

        // Stats


        // Function Parameters
        for( int iDepth=0; iDepth < TBD_MAXDEPTH; iDepth++ ) {
            for( int i = 0; i < this->GetNumFunParam( iDim, iDepth ); i++ ) {
                CString*  pParam= this->GetFunParam( iDim, iDepth, i );
                csMsg.Format( _T("%sParam %d=%s"), csPrefix.GetString(), i+1, pParam->GetString() );
                aDump.Add( csMsg );
            }
        }

        csMsg.Format( _T("%sEND DIMENSION %d\n"), csPrefix.GetString(), iDim+1 );
        aDump.Add( csMsg );
    }

}
#endif
