//---------------------------------------------------------------------------
//  File name: Do_xtab.cpp
//
//  Description:
//          Implementation xtab driver
//
//  History:    Date       Author   Comment
//              ---------------------------
//              15 Jul 99   RHF     Basic conversion
//              20 Nov 00   RHF     Redo for Hotdeck support
//              03 Jul 01   RHF     Redo for Multi tally support
//              14 Aug 02   RHF     Add Unit,tablogic, statistics support
//
//---------------------------------------------------------------------------
#include "StandardSystemIncludes.h"
#include "INTERPRE.H"
#include "Engine.h"
#include "CBorder.h"
#include "Ctab.h"
#include "Ctab_Helper.h"
#include "ProgramControl.h"
#include "VTSTRUCT.h"
#include <zEngineO/ValueSet.h>
#include <zEngineO/WorkVariable.h>


#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]= __FILE__;
#define new DEBUG_NEW
#endif


// Return:
//      the first 'invalid' do_onetab code.
//      10 when is not possible to create the iterator
//      0 if everything was ok
double CIntDriver::DoXtab( CTAB* pCtab, double dTableWeight, int iTabLogicExpr, LIST_NODE* pListNode ) {
    double      dRet=0;

    // RHF INIC Jan 30, 2003
    if( pCtab->GetCtabGrandTotal() != NULL ) { // RHF Apr 17, 2003
        ASSERT( pCtab->GetCtabGrandTotal() != NULL && pCtab->GetCtabGrandTotal()->GetTableType() == CTableDef::Ctab_GrandTotal ); // Grand Total

        CBorder*            pTotalBorder=pCtab->GetCtabGrandTotal()->m_pBorder;
        ASSERT( pTotalBorder );

        pTotalBorder->CleanArrays(false);
    } // RHF Apr 17, 2003
    // RHF END Jan 30, 2003

    // UNITS
    int     iNumUnits;
    int     iSingleUnit=-1;
    if( (iNumUnits=pCtab->GetNumUnits()) > 0 ) {
        for( int iUnit=0; iUnit < iNumUnits; iUnit++ ) {
            CtUnit &ctUnit=pCtab->GetUnit( iUnit );
            int     iUnitTabLogicExpr;

            //ctUnit.CleanVectorItems( pCtab, m_pEngineArea );

            pCtab->SetCurrentUnit( &ctUnit );

            // RHF INIC Jul 18, 2005
            if( ctUnit.GetTabLogicExpr() != 0 )
                iUnitTabLogicExpr = ctUnit.GetTabLogicExpr();
            else
                iUnitTabLogicExpr = iTabLogicExpr;
            // RHF END Jul 18, 2005

            int     iType=ctUnit.GetUnitType();
            double  dUnitRet=0;

            // if unit specified, use proper iterator
            switch( iType) {
            case (int)SymbolType::Relation:// RELATION
            case (int)SymbolType::Section: // RECORD
            case (int)SymbolType::Variable:        // ITEM
                dUnitRet = DoXtabRelUnit( pCtab, dTableWeight, iUnitTabLogicExpr, pListNode );
                break;
            case (int)SymbolType::Group:       // GROUP
                dUnitRet = DoXtabGroupUnit( pCtab, dTableWeight, iUnitTabLogicExpr, pListNode );
                break;
            default:                                // no UNIT specified - behave as before
                ASSERT( iSingleUnit == -1 );
                iSingleUnit = iUnit;
                break;
            }

            if( dRet == 0 )
                dRet = dUnitRet;
        }

        if( iSingleUnit >= 0 ) {
            CtUnit &ctUnit=pCtab->GetUnit( iSingleUnit );
            pCtab->SetCurrentUnit( &ctUnit );
        }
        else
            return dRet;
    }
    else {
        ASSERT(0);
        pCtab->SetCurrentUnit( NULL );
    }

    // Single unit
    if( pCtab->GetCurrentUnit() != NULL ) {
        pCtab->GetCurrentUnit()->CleanVectorItems( pCtab, m_pEngineArea );
    }

    dRet = DoOneXtab( pCtab, dTableWeight, iTabLogicExpr, pListNode );

    return dRet;
}

double CIntDriver::DoXtabRelUnit( CTAB* pCtab, double dTableWeight, int iTabLogicExpr, LIST_NODE* pListNode ) {
    double      dRet;
    FORRELATION_NODE ForNode;

    CtUnit*  pTabUnit=pCtab->GetCurrentUnit();
    ASSERT( pTabUnit );

    ForNode.iCtab          = pTabUnit->GetOwnerCtabSymbol();
    ForNode.forVarIdx      = pTabUnit->GetLoopSymbol();
    ForNode.forRelIdx      = pTabUnit->GetUnitSymbol();
    ForNode.forWhereExpr   = pTabUnit->GetSelectExpr();
    ForNode.CtabWeightExpr = pTabUnit->GetWeightExpr();
    ForNode.CtabTablogicExpr = pTabUnit->GetTabLogicExpr();

    //////////////////////////////////////////////////////////////////////////
    // 3d Extensions, rcl, Nov 2004
    ForNode.isUsingExtraInfo = pTabUnit->isUsingExtraInfo();
    ForNode.extraInfoNode    = pTabUnit->getExtraInfoNode();
    ForNode.m_iForRelNodeIdx = 0;
    //////////////////////////////////////////////////////////////////////////

    ForNode.forBlock = -1;

    dRet = exdofor_relation( &ForNode, &dTableWeight, &iTabLogicExpr, pListNode  ); // Call to DoOneXTab

    return dRet;
}

double CIntDriver::DoXtabGroupUnit( CTAB* pCtab, double dTableWeight, int iTabLogicExpr, LIST_NODE* pListNode ) {
    double      dRet=0;
    CtUnit*  pTabUnit=pCtab->GetCurrentUnit();

    ASSERT( pTabUnit );

    int         iSymGroup  = pTabUnit->GetUnitSymbol();
    int         iSymForVar = pTabUnit->GetLoopSymbol();
    int         iWhereExpr = pTabUnit->GetSelectExpr();
    int         iWeightExpr = pTabUnit->GetWeightExpr();
    int         iUnitTablogicExpr = pTabUnit->GetTabLogicExpr();

    double*     pLoopVar   = GetSymbolWorkVariable(iSymForVar).GetValueAddress();
    GROUPT*     pGroupT    = GPT(iSymGroup);
    int         iGrpNumDim = pGroupT->GetNumDim();
    double      dOneRet;

    // push Group into Runtime Stack for Group' looping
    ASSERT( m_iForStackNext < FOR_STACK_MAX );
    if( m_iForStackNext >= FOR_STACK_MAX ) {
        // TODO - what should be done here?
    }

    FOR_STACK*  pForStack = &ForStack[m_iForStackNext++];

    pForStack->forVarIdx = iSymForVar;
    pForStack->forGrpIdx = iSymGroup;
    pForStack->forType   = _T('G');

    // remark - all GROUP' occurrences are tallied
    double  dInitValue = 1;
    double  dMaxValue  = pGroupT->GetTotalOccurrences();
    double  dWhereExprValue;

    *pLoopVar = NOTAPPL;

    for( double dCount = dInitValue; dCount <= dMaxValue; dCount++ ) {
        *pLoopVar = dCount;

        if( iWhereExpr == 0 )
            dWhereExprValue = 1;
        else if( iWhereExpr > 0 ) {
            dWhereExprValue = evalexpr( iWhereExpr );
        }
        else {
            dWhereExprValue = evalexpr( -iWhereExpr );
        }

        if( ConditionalValueIsTrue(dWhereExprValue) && dWhereExprValue > -MAXVALUE ) {
            double      dSubTableWeight;

            if( iWeightExpr == 0 )
                dSubTableWeight = 1;
            else if( iWeightExpr > 0 ) {
                dSubTableWeight = evalexpr( iWeightExpr );
            }
            else {
                dSubTableWeight = evalexpr( -iWeightExpr );
            }

            // tally this instance
            if( iWeightExpr != 0 && !IsSpecial(dSubTableWeight) && dSubTableWeight > -MAXVALUE ||
                iWeightExpr == 0 && !IsSpecial(dTableWeight) && dTableWeight > -MAXVALUE ) {

                // RHF COM Jul 15, 2005 dOneRet = DoOneXtab( pCtab, (iWeightExpr!=0) ? dSubTableWeight : dTableWeight,
                // RHF COM Jul 15, 2005    iTabLogicExpr, pListNode  ); // sub-table weight by table weight
                dOneRet = DoOneXtab( pCtab, (iWeightExpr!=0) ? dSubTableWeight * dTableWeight : dTableWeight,
                                            (iUnitTablogicExpr!=0) ? iUnitTablogicExpr : iTabLogicExpr, pListNode  );
                if( dRet == 0 ) dRet = dOneRet; // The first error is returned
            }
        }

        if( m_iStopExec )
            break;
    }

    // Restore original area
    //PPTBASE = iOldProg;

    // pop Group from Runtime Stack
    if( m_iForStackNext > 0 )
        m_iForStackNext--;

    return dRet;
}

bool CIntDriver::ExecTabLogic( CTAB* pCtab, int iTabLogicExpr ) {
    CtUnit* pCurrentUnit=pCtab->GetCurrentUnit();
    ASSERT( pCurrentUnit != NULL ); // RHF Jan 28, 2003
    bool    bRequestIssued=false;

    try
    {
        if( iTabLogicExpr == 0 )
            ;
        else if( iTabLogicExpr < 0 ) {
            if( pCurrentUnit != NULL )
                pCurrentUnit->CleanVectorItems( pCtab, m_pEngineArea );

            bRequestIssued = ExecuteProgramStatements( -iTabLogicExpr );
        }
        else {// iTabLogicExpr > 0
            if( pCurrentUnit != NULL )
                pCurrentUnit->CleanVectorItems( pCtab, m_pEngineArea );

            bRequestIssued = ExecuteProgramStatements( iTabLogicExpr );
        }
    }

    // an exit statement terminates tab logic
    catch( const ExitProgramControlException& ) { }

    return bRequestIssued;
}

// local
double CIntDriver::DoOneXtab( CTAB* pCtab, double dWeight, int iTabLogicExpr, LIST_NODE* pListNode ) {
    //--------------------------------------------------------
    // 'rc' return codes (the greater is returned)
    //
    //  0   no problem
    //  5   cell overflow for int/long cells
    //  6   weight value has decimals for int/long cells
    //  7   dep. var has special value  - no xtab performed
    //  8   tabvaloff can't locate cell - no xtab performed
    //  9   unknown type of table       - no xtab performed
    //--------------------------------------------------------


    CtUnit* pCurrentUnit=pCtab->GetCurrentUnit();

    ASSERT( pCurrentUnit != NULL ); // RHF Jan 28, 2003

    double      dRet=0;
    double      dOneRet;
    int         iNumElems= (pListNode == NULL) ? 0 : pListNode->iNumElems;


    ExecTabLogic( pCtab, iTabLogicExpr ); // RHF Jul 29, 2005

    if( iNumElems == 0 ) {// Use all SubTables in the current unit
        if( pCurrentUnit == NULL )
            dRet = DoOneXtabForSubTable( pCtab, dWeight, NULL, iTabLogicExpr );
        else {
            dOneRet = DoOneXtabFor4AllSubTablesOfCurUnit( pCtab, dWeight,iTabLogicExpr );
        }
    }
    else {
        if( pCurrentUnit == NULL )
            dRet = DoOneXtabForSubTable( pCtab, dWeight, NULL, iTabLogicExpr );
        else {
            for( int i=0; i < iNumElems; i++ ) {
                int     iSubTable=pListNode->iSym[i];

                if( pCurrentUnit->HasSubTable( iSubTable ) ) {
                    CSubTable& cSubTable=pCtab->GetSubTable(iSubTable);

                    dOneRet = DoOneXtabForSubTable( pCtab, dWeight, &cSubTable, iTabLogicExpr );
                    if( dRet == 0 ) dRet = dOneRet; // The first error is returned
                }
            }
        }
    }

    return dRet;
}

void CTAB::FillTotals( double dWeight, int iSubTableI, int iSubTableJ, int iSubTableK ) {
    CBorder*            pTotalBorder=this->m_pBorder;
    ASSERT( pTotalBorder );

    int                 iNumRows=pTotalBorder->GetNumRows();
    int                 iNumCols=pTotalBorder->GetNumCols();
    int                 iNumLayers=pTotalBorder->GetNumLayers();

    bool                bJustFilled[2]={true,true};

    // RHF INIC Jun 03, 2003
    ASSERT( GetTableType() == CTableDef::Ctab_Percent || GetTableType() == CTableDef::Ctab_GrandTotal );
    bool    bSubTable=(GetTableType() == CTableDef::Ctab_Percent);
    // RHF END Jun 03, 2003

    for( int iDim=0; iDim < 2; iDim++ ) {
        int     iRow, iCol, iLayer;
        if( iDim == 0 ) {
            iRow = iNumRows;
            iCol = iSubTableJ;
            iLayer = iSubTableK;
        }
        else if( iDim == 1 ) {
            iRow = iSubTableI;
            iCol = iNumCols;
            iLayer = iSubTableK;
        }
        else
            ASSERT(0);

        bJustFilled[iDim] = pTotalBorder->AddValue( dWeight, iRow, iCol, iLayer );
    }

    // Row Border in total layer
    if( bSubTable||bJustFilled[0] )
        pTotalBorder->AddValue( dWeight, iNumRows, iSubTableJ, iNumLayers );

    // Col Border in total layer
    if( bSubTable||bJustFilled[1] )
        pTotalBorder->AddValue( dWeight, iSubTableI, iNumCols, iNumLayers );

    if( bSubTable||bJustFilled[0] || bJustFilled[1] )
        // Total layer
        pTotalBorder->AddValue( dWeight, iSubTableI, iSubTableJ, iNumLayers );

    //Layer corner
    pTotalBorder->AddValue( dWeight, iNumRows, iNumCols, iSubTableK );

    //Total Layer corner
    pTotalBorder->AddValue( dWeight, iNumRows, iNumCols, iNumLayers );

}
// RHF END Jan 29, 2003

double CIntDriver::DoOneXtabForSubTable( CTAB* pCtab, double dWeight, CSubTable* pSubTable, int iTabLogicExpr ) {
    csprochar*       pValue;
    int         ctvector[DIM_MAXDIM][MAX_TALLYCELLS+1], isym, r, rc, i, j, k;
    int         iValue;
    long        longValue;
    double      depvar[2];
    MEANACUM*   pacum;
    SMEANACUM*  spacum;
    Symbol* pSymbol;
    bool        bKind[4] = {false,false,false,false};
    CTAB*       pCtabAux=NULL;
    double*     pdValueAux;
    bool        bOkCoord;
    //int         iRightCells=0;
    int         iNumSubCells[TBD_MAXDIM]={1,1,1};
    CtStatBase* pStatBase=NULL;
    int         iSubCells=0;
    bool        bIsPercent=(pSubTable != NULL && pSubTable->GetStatType() == CTSTAT_PERCENT );// RHF Jan 30, 2003

    // RHF INIT Sep 07, 2004
    bool        bNeedExtraCellDisplacment=
                (pSubTable != NULL &&
                (
                pSubTable->GetStatType() == CTSTAT_MODE ||
                pSubTable->GetStatType() == CTSTAT_MEDIAN ||
                pSubTable->GetStatType() == CTSTAT_PTILE
                )
                );// RHF Sep 07, 2004

    bool        bNeedExtraOneCellDisplacment=
                (
                pSubTable != NULL &&
                (
                pSubTable->GetStatType() == CTSTAT_PROP ||
                pSubTable->GetStatType() == CTSTAT_MEAN ||
                pSubTable->GetStatType() == CTSTAT_STDEV ||
                pSubTable->GetStatType() == CTSTAT_VARIANCE ||
                pSubTable->GetStatType() == CTSTAT_STDERR
                )
                );

    // RHF END Sep 07, 2004

    CStatValue  cEmptyStat;
    double      dDummyValue=0;

    // MEAN or SMEAN or HOTDECK: verify that dependent variables have non-special values
    if( pCtab->GetTableType() != CTableDef::Ctab_Crosstab && pCtab->GetTableType() != CTableDef::Ctab_STable ) {
        depvar[1] = 1.0;

        for( i = 0; i <= 1 && pCtab->GetDepSymVar(i) != -1; i++ ) {
            isym = pCtab->GetDepSymVar(i);
            pSymbol = NPT(isym);

            if( pSymbol->GetType() == SymbolType::WorkVariable ) {
                depvar[i] = GetSymbolWorkVariable(isym).GetValue();
            }
            else {
                VARX*   pVarX;
                VART*   pVarT;

                pVarT = GetVarT( pSymbol );
                pVarX = pVarT->GetVarX();

                if( pCtab->GetDepVarOcc(i) == 0 ) {
                    depvar[i] = svarvalue( pVarX );
                }
                else {
                    depvar[i] = mvarvalue( pVarX, (double) pCtab->GetDepVarOcc(i) );
                }
            }

            if( IsSpecial(depvar[i]) )
                return( (double) 7 );
        }

        bKind[3] = false;
    }

    else if( pSubTable != NULL && pSubTable->GetStatType() != CTSTAT_NONE &&
// RHF COM Jan 30, 2003        !bIsPercent &&
        pSubTable->GetStatType() != CTSTAT_FREQ // RHF Oct 07, 2002
        && pSubTable->GetStatType() != CTSTAT_TOTAL // RHF Jun 11, 2003
        ) {

        if( pSubTable->GetStatType() == CTSTAT_OVERLAPPED )
            return 0;

        bKind[3] = true;

        ASSERT( pSubTable->GetAuxSymCtab() > 0 );
        pCtabAux = XPT(pSubTable->GetAuxSymCtab());

        //iRightCells = pSubTable->GetRightCells();
        //iRightCells = (iRightCells <= 0 ) ? 1 : iRightCells;

        pStatBase=pSubTable->GetStatBase();
        ASSERT( pStatBase != NULL);

        iSubCells=pStatBase->GetSubCells();
        ASSERT( iSubCells >= 1 );
        if( pSubTable->GetSymStatDim() == DIM_ROW ) {
            iNumSubCells[0] = iSubCells;
        }
        else if( pSubTable->GetSymStatDim() == DIM_COL ) {
            iNumSubCells[1] = iSubCells;
        }
        else if( pSubTable->GetSymStatDim() == DIM_LAYER ) {
            iNumSubCells[2] = iSubCells;
        }
        else
            ASSERT(0);

        // RHF INIC Jan 29, 2003
        if( bIsPercent ) {
            CBorder*            pTotalBorder=pCtabAux->m_pBorder;
            ASSERT( pTotalBorder );

            pTotalBorder->CleanArrays(false);
        }
        // RHF END Jan 29, 2003
    }

    // int/long cells: verify decimals in weight value
    if( pCtab->GetAcumType() == sizeof(short) && dWeight != (short) dWeight )
        rc = 6;
    else if( pCtab->GetAcumType() == sizeof(long) && dWeight != (long) dWeight )
        rc = 6;
    else
        rc = 0;

    // # of elements in row, col, layer
    for( i = 0; i < DIM_MAXDIM; i++ ) {
        ctvector[i][0] = 1;
        ctvector[i][1] = 0;
    }


    for( i = 0; i < pCtab->GetNumDim(); i++ ) {

        CtPos( pCtab, pCtab->GetNodeExpr(i), ctvector[i], pSubTable, NULL, false );
    }


    if( !bKind[3] ) {
        bKind[0] =  ( pCtab->GetTableType() == CTableDef::Ctab_Crosstab || pCtab->GetTableType() == CTableDef::Ctab_STable || pCtab->GetTableType() == CTableDef::Ctab_Hotdeck );
        bKind[1] =  ( pCtab->GetTableType() == CTableDef::Ctab_Mean );
        bKind[2] =  ( pCtab->GetTableType() == CTableDef::Ctab_SMean );
#ifdef _DEBUG
        if( !bKind[0] && !bKind[1] && !bKind[2] ) {
            rc = 9;
            ASSERT(0);
            return( (double) rc );
        }
#endif
    }

    bool    bHotDeck=(pCtab->GetTableType() == CTableDef::Ctab_Hotdeck);
    bool    bValidValue;

    int     iTallyNumber; // RHF Jul 29, 2002

    int     iRow=0, iColumn=0, iLayer=0;
    for( k = 1; k <= ctvector[2][0]; k++ ) { // Layer
        if( ctvector[2][k] == -1 )
            continue;
        iLayer++;
        iColumn = 0;
        for( j = 1; j <= ctvector[1][0]; j++ ) { // Column
            if( ctvector[1][j] == -1 )
                continue;
            iColumn++;
            iRow = 0;
            for( i = 1; i <= ctvector[0][0]; i++ ) { // Row
                if( ctvector[0][i] == -1 )
                    continue;

                iRow++;
                pValue = (csprochar*) pCtab->m_pAcum.GetValue( ctvector[0][i], ctvector[1][j], ctvector[2][k] );

                r = 0;
                if( pValue != NULL ) {
                    // RHF INIC Jan 30, 2003
                    // Refresh TOTAL table always

                    if( pCtab->GetCtabGrandTotal() != NULL ) { // RHF Apr 17, 2003
                        ASSERT( pCtab->GetCtabGrandTotal() != NULL && pCtab->GetCtabGrandTotal()->GetTableType() == CTableDef::Ctab_GrandTotal ); // Grand Total

                        //Fill TABLE Total if the corresponding border is not used
                        pCtab->GetCtabGrandTotal()->FillTotals( dWeight, ctvector[0][i], ctvector[1][j], ctvector[2][k] );
                        // RHF END Jan 30, 2003
                    } // RHF Apr 17, 2003

                    if( bKind[0] ) {
                        switch( pCtab->GetAcumType() ) {
                        case sizeof(short):
                            if( bHotDeck ) {
                                bValidValue = ( depvar[0] >= SHRT_MIN && depvar[0] <= SHRT_MAX );
                                if( bValidValue ) {
                                    iValue = (short) depvar[0];
                                    bValidValue = ( iValue == depvar[0] );
                                }

                                if( !bValidValue )
                                    r = 5;
                                else
                                    *( (short *) pValue ) = depvar[0];
                            }
                            else {
                                iValue = *( (short *) pValue );
                                if( iValue + (short) dWeight < iValue )
                                    r = 5;

                                *( (short *) pValue ) += dWeight;
                            }
                            break;
                        case sizeof(long):
                            if( bHotDeck ) {
                                bValidValue = ( depvar[0] >= INT_MIN && depvar[0] <= INT_MAX );
                                if( bValidValue ) {
                                    longValue = (long) depvar[0];
                                    bValidValue = ( longValue == depvar[0] );
                                }

                                if( !bValidValue )
                                    r = 5;
                                else
                                    *( (long *) pValue ) = depvar[0];
                            }
                            else {
                                longValue = *( (long *) pValue );
                                if( longValue + (long) dWeight < longValue )
                                    r = 5;

                                *( (long *) pValue ) += dWeight;
                            }
                            break;
                        case sizeof(double):
                            if( bHotDeck )
                                *( (double *) pValue ) = depvar[0];
                            else
                                *( (double *) pValue ) += dWeight;
                            break;
                        }
                    }
                    else if( bKind[1] ) {
                        pacum = (MEANACUM *) pValue;
                        pacum->m_dSumX   += depvar[0];
                        pacum->m_dSumW   += dWeight;
                        pacum->m_dSumXW  += depvar[0] * dWeight;
                        pacum->m_dSumXW2 += depvar[0] * depvar[0] * dWeight;
                    }
                    else if( bKind[2] ) {
                        spacum = (SMEANACUM *) pValue;

                        spacum->m_dNumWeight += depvar[0] * dWeight;
                        spacum->m_dNumSquare += depvar[0] * depvar[0] * dWeight;
                        spacum->m_dDenWeight += depvar[1] * dWeight;
                        spacum->m_dDenSquare += depvar[1] * depvar[1] * dWeight;
                        spacum->m_dDenUnWeight += depvar[1];
                        spacum->m_dCumWeight += dWeight;
                        spacum->m_dCrosProd += depvar[0] * dWeight * depvar[1];
                        spacum->m_dCumCases += 1;
                    }
                    else if( bKind[3] ) {
                        int         iSubTableI, iSubTableJ, iSubTableK;
                        int         iTableOffSet;
                        double      dSubTableValue;
                        CStatType   eStatType = pSubTable->GetStatType();

                        ASSERT( pSubTable != 0 );
                        if( pSubTable->GetSymStatDim() == DIM_ROW )
                            //iTallyNumber = i - 1;
                            iTallyNumber = iRow-1;
                        else if( pSubTable->GetSymStatDim() == DIM_COL )
                            //iTallyNumber = j - 1;
                            iTallyNumber = iColumn-1;
                        else if( pSubTable->GetSymStatDim() == DIM_LAYER )
                            //iTallyNumber = k - 1;
                            iTallyNumber = iLayer - 1;
                        else
                            ASSERT(0);

                        // RHF COM Jan 30, 2003 CStatValue&  cStatValue=pSubTable->GetStatValue(iTallyNumber);

                        CStatValue&  cStatValue=bIsPercent?cEmptyStat:pSubTable->GetStatValue(iTallyNumber);

                        dSubTableValue = cStatValue.m_dValue;

                        // RHF INIT Sep 07, 2004
                        int             iExtraCellDisplacment=0;
                        if( bNeedExtraCellDisplacment || bNeedExtraOneCellDisplacment ) {

                            if( bNeedExtraCellDisplacment )
                                ASSERT( cStatValue.m_iPos >= 0 && cStatValue.m_iPos < iSubCells );

                            if( pSubTable->GetSymStatDim() == DIM_ROW )
                                iExtraCellDisplacment = bNeedExtraCellDisplacment ? cStatValue.m_iPos :
                                                                                                                1;
                            else if( pSubTable->GetSymStatDim() == DIM_COL )
                                     iExtraCellDisplacment = bNeedExtraCellDisplacment ? (cStatValue.m_iPos * pCtabAux->m_pAcum.GetNumRows()) :
                                                                                         pCtabAux->m_pAcum.GetNumRows();
                                 else if( pSubTable->GetSymStatDim() == DIM_LAYER )
                                          iExtraCellDisplacment = bNeedExtraCellDisplacment ? (cStatValue.m_iPos * pCtabAux->m_pAcum.GetNumRows() * pCtabAux->m_pAcum.GetNumCols()) :
                                                                                                                                                                        (pCtabAux->m_pAcum.GetNumRows() * pCtabAux->m_pAcum.GetNumCols());
                                      else
                                          ASSERT(0);
                         }
                         // RHF END Sep 07, 2004

                        iTableOffSet = pCtab->m_pAcum.GetOffSetPos( ctvector[0][i], ctvector[1][j], ctvector[2][k] );

                        iTableOffSet /= sizeof(double);

                        ASSERT( pCtabAux != NULL );

                        if( pCtab->GetNumCells() <= MAXCELL_REMAP )
                            bOkCoord = pSubTable->GetSubTableCoord( iSubTableI, iSubTableJ, iSubTableK, iTableOffSet );
                        else
                            bOkCoord = pCtab->GetSubTableCoordOnLine( iSubTableI, iSubTableJ, iSubTableK, iTableOffSet, pSubTable );

                            ASSERT( bOkCoord );

                            // RHF INIC Jul 07, 2005
                            if( eStatType == CTSTAT_PTILE || eStatType == CTSTAT_PROP ) {
                                int        iFinalCells=1;

                                if( eStatType == CTSTAT_PTILE ) {
                                    CtStatPTile*  pStatPTile=(CtStatPTile*) pStatBase;

                                    ASSERT( pStatPTile->GetNumTiles() >= 2 );

                                    if( pStatPTile->GetNumTiles() > 2 )
                                        iFinalCells=(pStatPTile->GetNumTiles() - 1);
                                }
                                else if( eStatType == CTSTAT_PROP ) {
                                    CtStatProp*  pStatProp=(CtStatProp*) pStatBase;

                                    if( pStatProp->GetPropType() & CTSTAT_PROP_TOTAL )
                                        iFinalCells = 2;
                                }
                                else
                                    ASSERT(0);

                                if( iFinalCells > 1 ) {
                                    if( pSubTable->GetSymStatDim() == DIM_ROW ) {
                                        iSubTableI /= iFinalCells;
                                    }
                                    else if( pSubTable->GetSymStatDim() == DIM_COL ) {
                                        iSubTableJ /= iFinalCells;
                                    }
                                    else if( pSubTable->GetSymStatDim() == DIM_LAYER ) {
                                        iSubTableK /= iFinalCells;
                                    }
                                    else
                                        ASSERT(0);
                                }
                            }
                            // RHF END Jul 07, 2005

                        iSubTableI *= iNumSubCells[0];
                        iSubTableJ *= iNumSubCells[1];
                        iSubTableK *= iNumSubCells[2];

                        if( !bOkCoord )
                            pdValueAux = NULL;
                        else {
                            if( bIsPercent )
                                pdValueAux = &dDummyValue; // Not used in percent
                            else
                                pdValueAux = (double*) pCtabAux->m_pAcum.GetValue( iSubTableI, iSubTableJ, iSubTableK );
                        }

                            ASSERT(pdValueAux);


                        if( pdValueAux != NULL ) {
#ifdef _DEBUG
                            double  dCurrentValue=*pdValueAux;
#endif
                            eStatType = pSubTable->GetStatType();

                            switch( eStatType ) {
                            case CTSTAT_OVERLAPPED: // OK Never executed
                                ASSERT(0);
                                //*(pdValueAux) = NOTAPPL;
                                break;

                            case CTSTAT_FREQ: // OK Never executed
                                ASSERT(0);
                                //*(pdValueAux) += dWeight;
                                break;

                            case CTSTAT_TOTAL:
                                ASSERT(0); // RHF Jun 11, 2003
                                // RHF COM Jun 02, 2003 *(pdValueAux) += dSubTableValue * dWeight;
                                *(pdValueAux) += dWeight;
                                break;

                            case CTSTAT_PERCENT:
                                // RHF INIC Jan 29, 2003
                                *( (double *) pValue ) += dWeight; // Fill freq on the base table

                                //Fill SubTable Total if the corresponding border is not used
                                pCtabAux->FillTotals( dWeight, iSubTableI, iSubTableJ, iSubTableK );
                                // RHF END Jan 29, 2003
                                break;
                            case CTSTAT_PROP:
                                {
                                    ASSERT( iSubCells == 2 );
                                    bool    bMatch=false;
                                    CtStatProp*     pStatProp=(CtStatProp*) pStatBase;
                                    ASSERT( pStatBase != 0 );
                                    ASSERT( pStatProp != 0 );
                                    ASSERT( pStatBase->GetStatType() == CTSTAT_PROP );
                                    for( int iRange=0; !bMatch && iRange < pStatProp->GetNumRanges(); iRange++ ) {
                                        CSubRange& cSubRange=pStatProp->GetRange(iRange);

                                        if( dSubTableValue >= cSubRange.GetLow() && dSubTableValue <= cSubRange.GetHigh() )
                                            bMatch = true;
                                    }

                                    if( bMatch )
                                        (*pdValueAux) += dWeight; // Matches

                                    // RHF COM Sep 07, 2004 pdValueAux++;
                                    pdValueAux += iExtraCellDisplacment; //RHF Sep 07, 2004

                                    (*pdValueAux) += dWeight; // N
                                }
                                break;
                            case CTSTAT_MIN:
                                ASSERT( iSubCells == 1 );
                                // Init pCtabAux with NOTAPPL
                                *(pdValueAux) = std::min( *(pdValueAux), dSubTableValue );
                                break;
                            case CTSTAT_MAX:
                                ASSERT( iSubCells == 1 );
                                // Init pCtabAux with -NOTAPPL
                                *(pdValueAux) = std::max( *(pdValueAux), dSubTableValue );
                                break;
                            case CTSTAT_MODE:
                                // Init pCtabAux with -NOTAPPL
                                ASSERT( cStatValue.m_iPos >= 0 && cStatValue.m_iPos < iSubCells );

                                // RHF COM Sep 07, 2004 pdValueAux += cStatValue.m_iPos;
                                pdValueAux += iExtraCellDisplacment; //RHF Sep 07, 2004

                                if( (*pdValueAux) == -NOTAPPL )
                                    (*pdValueAux) = dWeight;
                                else
                                    (*pdValueAux) += dWeight;

                                break;
                            case CTSTAT_MEAN:
                                {
                                    ASSERT( iSubCells == 2 );

                                    (*pdValueAux) += dSubTableValue*dWeight; // Sum X

                                    // RHF COM Sep 07, 2004 pdValueAux++;
                                    pdValueAux += iExtraCellDisplacment; //RHF Sep 07, 2004

                                    (*pdValueAux) += dWeight; // N
                                }
                                break;

                            case CTSTAT_MEDIAN:
                            case CTSTAT_PTILE:
                                // Init pCtabAux with -NOTAPPL
                                ASSERT( cStatValue.m_iPos >= 0 && cStatValue.m_iPos < iSubCells );

                                // RHF COM Sep 07, 2004 pdValueAux += cStatValue.m_iPos;
                                pdValueAux += iExtraCellDisplacment; //RHF Sep 07, 2004

                                if( (*pdValueAux) == -NOTAPPL )
                                    (*pdValueAux) = dWeight;
                                else
                                    (*pdValueAux) += dWeight;

                                break;
                            case CTSTAT_STDEV:
                            case CTSTAT_VARIANCE:
                            case CTSTAT_STDERR:
                                {
                                    ASSERT( iSubCells == 3 );
                                    (*pdValueAux) += dSubTableValue*dWeight; // Sum X

                                    // RHF COM Sep 07, 2004 pdValueAux++;
                                    pdValueAux += iExtraCellDisplacment; //RHF Sep 07, 2004

                                    (*pdValueAux) += dSubTableValue*dSubTableValue*dWeight; // Sum X2

                                    // RHF COM Sep 07, 2004 pdValueAux++;
                                    pdValueAux += iExtraCellDisplacment; //RHF Sep 07, 2004

                                    (*pdValueAux) += dWeight; // N
                                }
                                break;
                            case CTSTAT_VPCT: // TODO
                                {
                                }
                                break;
                            default:
                                ASSERT(0);
                            }

                        }
                    }
                }
                else // pValue is NULL
                    r = 8;
                rc = ( r > rc ) ? r : rc;
            } // For Row
        } // For Column
    } // For Layer


    return( (double) rc );
}




//////////////////////////////////////////////////////////////////////////
// CtPos refactoring -> CtPos_Add + CtPos_Mul + CtPos_Val
//
// rcl, Oct 26, 2004
void CIntDriver::CtPos_Add( CTAB* pCtab, CTNODE* pNode, int *vector, CSubTable* pSubTable,
                            CCoordValue* pCoordValue, bool bMarkAllPos )
{

    ASSERT( pNode->isOperNode(CTNODE_OP_ADD) );
    ASSERT( vector != 0 );

    // GHM 20090923 the program ran out of memory and crashed for large tables
    // int v1[MAX_TALLYCELLS+1],
        //v2[MAX_TALLYCELLS+1];
    int * v1 = (int *)malloc( ( MAX_TALLYCELLS+1) * sizeof(int));
    int * v2 = (int *)malloc( ( MAX_TALLYCELLS+1) * sizeof(int));

    int leftcells;
    int i;


    CtPos( pCtab, pNode->m_iCtLeft, v1, pSubTable, pCoordValue, bMarkAllPos );
    CtPos( pCtab, pNode->m_iCtRight, v2, pSubTable, pCoordValue, bMarkAllPos );

    leftcells = ctcell( CtNodebase, pNode->m_iCtLeft );

    vector[0] = v1[0] + v2[0];

    ASSERT( v1[0] < MAX_TALLYCELLS );
    for( i = 1; i <= v1[0]; i++ )
        vector[i] = v1[i];

    ASSERT( vector[0] < MAX_TALLYCELLS );
    for( i = i; i <= vector[0]; i++ ) {
        ASSERT( (i - v1[0]) >= 0 );

        if( v2[i - v1[0]] == -1 )
            vector[i] = -1;
        else
            vector[i] = v2[i - v1[0]] + leftcells;
    }

    free(v1); // GHM 20090923
    free(v2);
}

void CIntDriver::CtPos_Mul( CTAB* pCtab, CTNODE* pNode, int *vector, CSubTable* pSubTable,
                            CCoordValue* pCoordValue, bool bMarkAllPos )
{
    ASSERT( pNode->isOperNode(CTNODE_OP_MUL) ); // rcl, May 2005
    ASSERT( vector != 0 );

    // GHM 20090923 the program ran out of memory and crashed for large tables
    // int v1[MAX_TALLYCELLS+1],
        //v2[MAX_TALLYCELLS+1];
    int * v1 = (int *)malloc( ( MAX_TALLYCELLS+1) * sizeof(int));
    int * v2 = (int *)malloc( ( MAX_TALLYCELLS+1) * sizeof(int));

    int rightcells;
    int i, j, k;


    CtPos( pCtab, pNode->m_iCtLeft, v1, pSubTable, pCoordValue, bMarkAllPos );
    CtPos( pCtab, pNode->m_iCtRight, v2, pSubTable, pCoordValue, bMarkAllPos );

    rightcells = ctcell( CtNodebase, pNode->m_iCtRight );

    vector[0] = v1[0] * v2[0];
    k = 1;
    ASSERT( v1[0] < MAX_TALLYCELLS );
    for( i = 1; i <= v1[0]; i++ ) {
        ASSERT( v2[0] < MAX_TALLYCELLS );
        for( j = 1; j <= v2[0]; j++ ) {
            ASSERT( k < MAX_TALLYCELLS );
            if( v2[j] == -1 || v1[i] == -1 )
                vector[k++] = -1;
            else
                vector[k++] = v2[j] + v1[i] * rightcells;
        }
    }

    free(v1); // GHM 20090923
    free(v2);
}

//////////////////////////////////////////////////////////////////////////

// When executing CtPos_FillIndexArray() for the same unit some
// changes are made to some MVAR_NODEs, which are stored in a local
// [and global] cache [named g_cache and defined below].
// Later on, when the Unit has changed, we restore
// the saved value until a new change is needed.
//                                                               rcl, Nov 2004
class CacheMVarNode
{
    std::map<int,MVAR_NODE> m_cache;
public:
    bool isCached( int iOccExpr )
    {
        return m_cache.find( iOccExpr ) != m_cache.end();
    }

    void insertIntoCache( int iOccExpr, MVAR_NODE mnode )
    {
        m_cache[iOccExpr] = mnode;
    }

    void getFromCache( int iOccExpr, MVAR_NODE* pnode )
    {
        ASSERT( pnode != 0 );
        ASSERT( isCached(iOccExpr) );
        *pnode = m_cache[iOccExpr];
    }

    void clearCache() { m_cache.clear(); }
};

CacheMVarNode g_cache;

// class DimDataKeeper
// needed in CtPos_FillIndexArray() to keep changed dim value and restore it later
class DimDataKeeper
{
    int m_iDimUsed, m_iDimType, m_iDimValue;
public:
    DimDataKeeper()  { m_iDimUsed = -1; m_iDimType = m_iDimValue = 0; }
    void SaveDimData_Change( int iDimUsed, MVAR_NODE* pmvar, int iNewConstValue )
    {
        ASSERT( pmvar != 0 );
        m_iDimUsed = iDimUsed;
        m_iDimType = pmvar->m_iVarSubindexType[iDimUsed];
        m_iDimValue = pmvar->m_iVarSubindexExpr[iDimUsed];
        pmvar->m_iVarSubindexExpr[iDimUsed] = iNewConstValue;
        pmvar->m_iVarSubindexType[iDimUsed] = MVAR_CONSTANT;
    }
    void RestoreDimData( MVAR_NODE* pmvar )
    {
        ASSERT( pmvar != 0 );
        if( hasSomething() )
        {
            pmvar->m_iVarSubindexExpr[m_iDimUsed] = m_iDimValue;
            pmvar->m_iVarSubindexType[m_iDimUsed] = m_iDimType;
            m_iDimUsed = -1;
        }
    }
    bool hasSomething() { return (m_iDimUsed != -1); }
};

void CIntDriver::CtPos_FillIndexArray( CTAB* ct, VART* pVarT, int iOccExpr )
{
    ASSERT( ct != 0 );
    MVAR_NODE* ptrvar = (MVAR_NODE*) (PPT(iOccExpr));

    ///////////////////////////////////////////////////////////////////////////
    // Change of unit detected? -> restore saved value,
    //                             if there is something saved there
    // Change of crosstab detected? -> clear cache
    //                                                           rcl, Nov 2004
    static CtUnit* pCurrentUnit = 0;
    static CTAB* lastCrosstabUsed = 0;
    if( lastCrosstabUsed != ct )
    {
        g_cache.clearCache();
        lastCrosstabUsed = ct;
        pCurrentUnit = ct->GetCurrentUnit();
    }

    if( ct->GetCurrentUnit() != pCurrentUnit )
    {
        pCurrentUnit = ct->GetCurrentUnit();
        if( g_cache.isCached( iOccExpr ) )
            g_cache.getFromCache( iOccExpr, ptrvar );
    }
    ///////////////////////////////////////////////////////////////////////////


    CIterator cTableIterator;
    cTableIterator.SetEngineDriver(m_pEngineDriver);

    bool bEvaluateItem = true;
    bool bUseTotalRecord = true;

    // we will eventually need to change one of the dim's value,
    // so we will save which one, and what value was there before
    // to restore it at the end
    DimDataKeeper dimKeeper;

    int iLoopVarSymbol   = ct->GetCurrentUnit()->GetLoopSymbol();
    double dLoopVarValue = GetSymbolWorkVariable(iLoopVarSymbol).GetValue();
    ASSERT( dLoopVarValue != NOTAPPL );

    int iSymVar = ptrvar->m_iVarIndex;

// Has Unit any constant we should use?
    int iLowIndex = 0;
    int iNumDim = pVarT->GetNumDim();

    if( pCurrentUnit->isUsingExtraInfo() )
    {
        MVAR_NODE* pMvarNode = m_pEngineDriver->m_pEngineCompFunc->getLocalMVarNodePtr( pCurrentUnit->getExtraInfoNode() );
        iLowIndex = pMvarNode->m_iSubindexNumber;
        if( iLowIndex > 0 )
        {
            ///////////////////////////////////////////////////////////
            // About to make changes to MVAR_NODE
            // -> keep original in cache
            g_cache.insertIntoCache( iOccExpr, *ptrvar );

            for( int i = 0; i < iLowIndex; i++ )
            {
                ASSERT( pMvarNode->m_iVarSubindexType[i] = MVAR_CONSTANT );
                ptrvar->m_iVarSubindexExpr[i] = pMvarNode->m_iVarSubindexExpr[i];
                ptrvar->m_iVarSubindexType[i] = pMvarNode->m_iVarSubindexType[i];
            }

            for( int i = iLowIndex; i < iNumDim; i++ )
            {
                if( ptrvar->m_iVarSubindexType[i] == MVAR_USE_DYNAMIC_CALC )
                    ptrvar->m_iVarSubindexType[i] = MVAR_GROUP;
            }
        }
    }

    if( iLowIndex < iNumDim && m_iForStackNext > 0 )
    {
        // exdofor_relation pushed the relation unit into forstack
        ASSERT( m_iForStackNext > 0 );
        FOR_STACK *pForStack = &ForStack[m_iForStackNext-1];
        int iForType = pForStack->forType;
        ASSERT( iForType == _T('R') || iForType == 'G' );

        // Definitions used in For-Relations case
        // [NULL is assigned otherwise]
        RELT* pRelT = iForType == _T('R') ? RLT( pForStack->forRelIdx ) : 0;
        RELATED aRelated;

        for( int iDim = iLowIndex; iDim < iNumDim; iDim++ )
        {
            int iIndexToUse = -1;

            if( iForType == 'G' ) {
                if( pForStack->forGrpIdx == ptrvar->m_iVarSubindexExpr[iDim] ) {
                    iIndexToUse = pForStack->forGrpIdx;
                }
            }
            else
            {
                ASSERT( iForType == 'R' );
                RELATED* pRelated = pRelT->GetRelated( &aRelated, iSymVar, pVarT->GetDimType( iDim ) );
                if( pRelated != NULL )
                { // variable reached by relation
                    if( pRelated->iRelatedRelationType == USE_WHERE_RELATION_MULTIPLE )
                    {
                        // Not ready tested, maybe do nothing?
                        ASSERT(0);
                    }
                    else
                    {
                        ASSERT( pRelated->iRelatedRelationType != USE_WHERE_RELATION_MULTIPLE );
                        iIndexToUse = pRelated->iRelatedItemIndex;
                        if( pRelated->iRelatedRelationType == USE_LINK_RELATION )
                        {
                            // Use link value, not loop value
                            dLoopVarValue = GetSymbolWorkVariable(pRelated->iRelatedWVarIdx).GetValue(); // rcl, Sept 2005
                        }
                    }
                }
            }

            // Any dynamic dimension? -> Time to make 'permanent' changes
            // with information from stack
            if( iIndexToUse != -1 && hasAnyDynamicDim( ptrvar, iNumDim ) )
            {
                ///////////////////////////////////////////////////////////
                // About to make changes to MVAR_NODE
                // -> keep original in cache
                if( !g_cache.isCached(iOccExpr) )
                    g_cache.insertIntoCache( iOccExpr, *ptrvar );

                // and now safely do the changes ..
                ct->CompleteDimensionsUsingThisUnit( ptrvar, iNumDim, iIndexToUse );
            }

            // If a group appears at this time, change it
            // to a constant value and exit this loop
            if( ptrvar->m_iVarSubindexType[iDim] == MVAR_GROUP )
            {
                dimKeeper.SaveDimData_Change( iDim, ptrvar, (int) dLoopVarValue );
                // only 1 unit is used, so when this change is ready
                // we can safely break this loop
                break;
            }
        }
    }

    if( !dimKeeper.hasSomething() ) // no changes made yet?
    {
        // search first MVAR_GROUP to change
        for( int i = iLowIndex; i < iNumDim; i++ )
        {
            if( ptrvar->m_iVarSubindexType[i] == MVAR_GROUP )
            {
                dimKeeper.SaveDimData_Change( i, ptrvar, (int) dLoopVarValue );
                break; // only 1 change allowed
            }
        }
    }

#ifdef _DEBUG
    {
        bool bAllConstants = true;
        for( int i = 0; bAllConstants && i < iNumDim; i++ )
            bAllConstants = (ptrvar->m_iVarSubindexType[i] == MVAR_CONSTANT);
        ASSERT( bAllConstants || dimKeeper.hasSomething() );
    }
#endif

    bool bBoundsOk =
        cTableIterator.MakeTableIterator( ptrvar, bEvaluateItem, bUseTotalRecord );
    int iHowMany = cTableIterator.size(); // number of elements inside iterator, useful later
                                          // to get each set of indexes

    //TRACE( _T("~~ CtPos_FillIndexArray: Number of loops: %d\n"), iHowMany );
    CString csVarName = WS2CS(pVarT->GetName());

    double dOccur[DIM_MAXDIM]; // 1-based double indexes

    for( int i = 0; i < iHowMany; i++ )
    {
        cTableIterator.getIndexes( i, dOccur );

        // mvarvalue() not only gets the variable value, but it also
        // considers color information, and change the real value to NOTAPPL
        // according to the situation. Instead of repeating the same
        // code here we give the data to mvarvalue and wait for his
        // calculation and changes.
        //
        // rcl, Oct 2004

        double dValue = mvarvalue( pVarT->GetVarX(), dOccur );

        // Alternative:
        //    a. Get integer indexes from iterator
        //          int aIndex[DIM_MAXDIM];    // 0-based int indexes
        //          cTableIterator.getIndexes( i, aIndex );
        //          TRACE( "Use index %s( %d, %d, %d )\n", csVarName,
        //                  aIndex[0]. aIndex[1], aIndex[2] );
        //    b. Build the expanded index in theIndex
        //       [record in dim 0, item in dim 1, subitem in dim 2]
        //       The call
        //           theIndex.setIndexes( aIndex );
        //       will not be useful because aIndex is in compact notation
        //       See  CIntDriver::mvarvalue( VARX* pVarX, double dOccur[] )
        //       for hints on how to do it.
        //    c. call GetVarFloatValue( pVarT, theIndex )
        //   d1. do some color calculation, or
        //   d2. just ignore colors
        //
        // rcl, Oct 2004

        // RTODO: Check if last occurrence is what is really needed in the
        //        vector
        double dLastOccurrence = dOccur[pVarT->GetNumDim()-1];
        pVarT->VectorAdd( dValue, dLastOccurrence );
    }

    // restore original values, if changed
    dimKeeper.RestoreDimData( ptrvar );
}

const double NO_VALUE_ASSIGNED_YET = -INVALIDINDEX;

void CIntDriver::CtPos_Var( CTAB* pCtab, int iCtNode, int *vector, CSubTable* pSubTable,
                            CCoordValue* pCoordValue, bool bMarkAllPos )
{
    CTNODE* pNode = (CTNODE *) ( CtNodebase + iCtNode );

    ASSERT( pNode->isVarNode() && !pNode->isOperNode() );

    double dValue = -1234.5; // magic number to detect non assignments in debug time.
    int    iNumMatches = 0;
    bool   bUseCoordNumber = false;

    if( pSubTable != NULL )
        bUseCoordNumber = pSubTable->UseCoordNumber( pNode->m_iCoordNumber );

#ifdef _DEBUG
    if( pSubTable == NULL )
        ASSERT( pCoordValue != NULL  );
#endif

    // RHF INIC Aug 01, 2002 A coordinate replace by a STAT 'without' freq. See CEngineCompFunc::AddStatCoordinate
    if( pNode->m_iNumCells == 0 ) {
        vector[0] = 0;
        return;
    }

    if( bMarkAllPos && bUseCoordNumber ) {
        iNumMatches = pNode->m_iNumCells;

        // Stat Node must have 1 cell at least
        if( pNode->m_iStatType != CTSTAT_NONE ) {
            ASSERT( iNumMatches >= 1 );
            //iNumMatches = 1; // ONLY MARK THE FIRST SUB-CELL for remapping
        }

        ASSERT( iNumMatches < MAX_TALLYCELLS );
        // could 'never' happen, but ...       rcl, May 2005
        if( iNumMatches >= MAX_TALLYCELLS )
        {
            iNumMatches = MAX_TALLYCELLS - 1;
        }

        for( int i=1; i <= iNumMatches; i++ )
            vector[i] = i-1;
    }
    // RHF END Aug 01, 2002

    else if( pCoordValue != NULL ) {
#ifdef _DEBUG
        VART*   pVarT = GetVarT( NPT(pNode->m_iSymbol) );
#endif
        if( iCtNode == pCoordValue->m_iCtNodeLeft ) {
            dValue = pCoordValue->m_dValueLeft;
            vector[0] = 1;
            vector[1] = ctvarpos( iCtNode, dValue, pCoordValue->m_iSeqLeft );
            iNumMatches = (vector[1] >= 0) ? 1 : 0;
        }
        else if( iCtNode == pCoordValue->m_iCtNodeRight ) {
            dValue = pCoordValue->m_dValueRight;
            vector[0] = 1;
            vector[1] = ctvarpos( iCtNode, dValue, pCoordValue->m_iSeqRight );
            iNumMatches = (vector[1] >= 0) ? 1 : 0;
        }
        else {
            iNumMatches = 0;
        }

        // DoTally( iCtNode, dValue, vector, &iNumMatches, pCoordValue );
    }
    else if( !bUseCoordNumber ) {
        ; // iNumMatches = 0;
    }
    else {

        double dValueAux = NO_VALUE_ASSIGNED_YET; // See Fixing point later

        // Get VART/VARX
        VART* pVarT = GetVarT( NPT(pNode->m_iSymbol) );
        VARX* pVarX = pVarT->GetVarX();

#ifdef _DEBUG
        {
            CString csMsg;
            csMsg.Format( _T("CtPos: VarName=(%s) CoorNumber=%d, SeqNumber=%d"), pVarT->GetName().c_str(), pNode->m_iCoordNumber, pNode->m_iSeqNumber );
            //TRACE( csMsg );
        }
#endif
        int  iOccExpr = pNode->m_iCtOcc;
        bool bFitOcc  = false;

        int  iNumTallys = pVarT->VectorSize();
        iOccExpr = abs(iOccExpr);

        if( iOccExpr != 0 ) // RHF Jul 16, 2004
            iOccExpr--; // RHF Jul 16, 2004

        bool bMultiValue = false;
        // RHF INIC Jul 16, 2002 Check Multiple relation
        // It is not possible to have a vector in a multiple relation. See message 33200
        if( iNumTallys == 0 && pVarT->IsArray() )
        {
            CtPos_FillIndexArray( pCtab, pVarT, iOccExpr );

            switch( pVarT->VectorSize() )
            {
            default: // other than 0 or 1 [hopefully bigger than 1]
                bMultiValue = true;
                iNumTallys = pVarT->VectorSize();
                break;
            case 1:
                {
                    // When vector has only 1 element
                    double dOccur;
                    int iOcc =0;
                    dValueAux = pVarT->VectorGet(iOcc, dOccur );

                    // the only one element inside vector must be erased
                    // to keep backward compatibility with original code
                    // [When there was only 1 element it was *not* inserted
                    //  into VarT's vector]
                    pVarT->VectorClean();
                }
                break;
            case 0:
                // when vector has no elements we change the former
                // NO_VALUE_ASSIGNED_YET value to INVALIDINDEX so that
                // later we know that we do *not* have to fix it
                // by evaluating expression in ptrvar
                // [See "FIXING point" (many lines) below]
                dValueAux = INVALIDINDEX;
                break;
            }
        }
        // RHF END Jul 16, 2002
        ASSERT( pSubTable != 0 );
        bool bIsStatItem  = ( pSubTable != NULL && pSubTable->GetStatNumber() >= 1 && pSubTable->GetStatCtTree() == iCtNode );// RHF Aug 09, 2002
        bool bStatPercent = ( bIsStatItem && pSubTable->GetStatType() == CTSTAT_PERCENT );

        //bStatPercent = false; // RHF Jan 29, 2003
        int  iLocalMatches;

        if( bIsStatItem )
            pSubTable->RemoveAllStatValues();

        // There is a valid vector
        if( iNumTallys >= 1 ) {
            double  dOccur = 0.0;
            double  dVectorOcc;

            if( pNode->m_iCtOcc <= 0 ) { // Multiple without indexes or single
                bFitOcc = true;
            }
            else {
                ASSERT( pVarT->IsArray() );
                ASSERT( *PPT(iOccExpr) == MVAR_CODE );

                MVAR_NODE*  ptrvar = (MVAR_NODE*) (PPT(iOccExpr));
                double      dIndex[DIM_MAXDIM];
#ifdef _DEBUG
                int     iIndexType[DIM_MAXDIM];
                mvarGetSubindexes( ptrvar, dIndex, iIndexType );

                // Multiple relation unsupported when a multiple var is used with subindexes
                if( iIndexType[0] == _T('M') || iIndexType[1] == _T('M') || iIndexType[2] == 'M' )
                    ASSERT(0);
#else
                mvarGetSubindexes( ptrvar, dIndex );
#endif
                dOccur = dIndex[0];
            }

            for( int iVector=0; iVector < iNumTallys; iVector++ ) {
                dValue = pVarT->VectorGet(iVector, dVectorOcc );

                if( bFitOcc || dVectorOcc == dOccur ) {
                    iLocalMatches = DoTally( iCtNode, dValue, vector, &iNumMatches );

                    // RHF INIC Jul 29, 2002
                    if( bIsStatItem && !bStatPercent && iLocalMatches > 0 ) {
                        for( int j=0; j < iLocalMatches; j++ ) {
                            pSubTable->AddStatValue( dValue, vector[j+1] );
                            vector[j+1] = 0; // All stat use slot 0. Except percent! // RHF Sep 17, 2002
                        }
                    }
                    // RHF END Jul 29, 2002
                }
            }

            if( bMultiValue ) // Relation Multiple
                pVarT->VectorClean();
        }
        else { // No Tally vector
            // RHF COM Jul 16, 2004 if( iOccExpr == 0 )   // ... single
            if( !pVarT->IsArray() ) // RHF Jul 16, 2004
                dValue = svarvalue( pVarX );
            else { // Multiple

                ///////////////////////////////////////////////////////////////
                // FIXING point here
                if( dValueAux != NO_VALUE_ASSIGNED_YET )
                    dValue = dValueAux;
                else
                    dValue = evalexpr( iOccExpr );
                ///////////////////////////////////////////////////////////////
            }

            iLocalMatches = DoTally( iCtNode, dValue, vector, &iNumMatches );

            // RHF INIC Jul 29, 2002
            if( bIsStatItem && !bStatPercent && iLocalMatches > 0 ) {
                for( int j=0; j < iLocalMatches; j++ ) {
                    pSubTable->AddStatValue( dValue, vector[j+1] );
                    vector[j+1] = 0; // All stat use slot 0. Except percent! // RHF Sep 17, 2002
                }
            }
            // RHF END Jul 29, 2002
        }
    }

    // set the number of relative positions
    if( iNumMatches == 0 )
        vector[0] = 1;
    else
        vector[0] = iNumMatches;

    ASSERT( iNumMatches < MAX_TALLYCELLS );
    // could 'never' happen, but ...       rcl, May 2005
    if( iNumMatches >= MAX_TALLYCELLS )
    {
        iNumMatches = MAX_TALLYCELLS - 1;
    }

    vector[iNumMatches+1] = -1;
}

//------------------------------------------------------------------------
// Calculates positions to tally.  At first time "iCtNode" is the root
// of the corresponding (row,col,layer) expression 'ctab->CtNodeexpr[i]'
//------------------------------------------------------------------------
// local
void CIntDriver::CtPos( CTAB* pCtab, int iCtNode, int *vector, CSubTable* pSubTable,
                         CCoordValue* pCoordValue, bool bMarkAllPos )
{
    CTNODE* pNode = (CTNODE *) ( CtNodebase + iCtNode );
    if( pNode->isVarNode() ) // Variable or value-set
    {
        CtPos_Var( pCtab, iCtNode, vector, pSubTable, pCoordValue, bMarkAllPos );
    }
    else
    {
        switch( pNode->getOperator() )
        {
        case CTNODE_OP_ADD:
            CtPos_Add( pCtab, pNode, vector, pSubTable, pCoordValue, bMarkAllPos );
            break;
        case CTNODE_OP_MUL:
            CtPos_Mul( pCtab, pNode, vector, pSubTable, pCoordValue, bMarkAllPos );
            break;
        default:
            ;
        }
    }
}

// local
int CIntDriver::DoTally( int iCtNode, double dValue, int* iVector, int* iNumMatches ) {
    int         i, iLocalMatches=0;
    CTNODE*     pNode=(CTNODE *) ( CtNodebase + iCtNode );
    CTRANGE*    pRange;

    double  rValue = ct_ivalue( iCtNode, dValue ); // see HINT in tabfuncs.cpp

    int  ini_range = iCtNode + CTNODE_SLOTS;
    int  end_range = ini_range + CTRANGE_SLOTS * pNode->m_iCtNumRanges;
    int  relpos = 0;
    int  lastrelpos = 0;

    bool bStatNode = (pNode->m_iStatType != CTSTAT_NONE); // RHF Aug 13, 2002

    // RHF INIC Jun 09, 2003
    bool bTotal=(pNode->m_iStatType == CTSTAT_TOTAL);
    // RHF END Jun 09, 2003

    bool bFitRange = false;
    int  j = 0;

    bool    bProcessRange=true; // RHF May 09, 2006
    for( i = ini_range; i < end_range; i += CTRANGE_SLOTS ) { // Variable Mapping
        pRange = (CTRANGE *) ( CtNodebase + i );
        bFitRange = pRange->fitInside(rValue);
        if( pRange->m_iRangeCollapsed <= 1 ) bProcessRange=true; // RHF May 09, 2006
        if( bFitRange ) {
            // RHF NEW May 09, 2006
            if( bProcessRange ) {
                (*iNumMatches)++;
                iLocalMatches++;
                bProcessRange = false;
            }
            // RHF NEW May 09, 2006

            /* RHF COM May 09, 2006
            (*iNumMatches)++;
            iLocalMatches++;
            */

            // All stats fit in position zero. All the stats cells are together in the respective subtable.
            if( false && bStatNode ) { // RHF Sep 11, 2002 add false. Now ctvector is clean outside this method
                iVector[*iNumMatches] = 0;
            }
            else {
                if( pRange->m_iRangeCollapsed == 0 ) {// No collapse
                    lastrelpos = relpos;
                    relpos += rValue - pRange->m_iRangeLow;
                }
                else if( pRange->m_iRangeCollapsed == 1 ) // Only the first collapsed range count a cell
                    relpos++;
                else // Do nothing with the rest collapsed ranges
                    ;

                ASSERT( *iNumMatches < MAX_TALLYCELLS );
                if( pRange->m_iRangeCollapsed >= 1 ) {
                    ASSERT( relpos >= 1 );
                    iVector[*iNumMatches] = relpos-1;
                }
                else {
                    iVector[*iNumMatches] = relpos;
                }

                // RHF INIC Jun 09, 2003
                if( bTotal )
                    iVector[*iNumMatches] = 0;
                // RHF END Jun 09, 2003

                // Advance the full range if no collapsed
                if( pRange->m_iRangeCollapsed == 0 ) {
                    relpos = lastrelpos;
                    relpos += pRange->m_iRangeHigh - pRange->m_iRangeLow + 1;
                }
            }
        }
        else {
            if( false && bStatNode ) { // RHF Sep 11, 2002 add false. Now ctvector is clean outside this method
                ;
            }
            else {
                if( pRange->m_iRangeCollapsed == 0 ) // No collapse
                    relpos += pRange->m_iRangeHigh - pRange->m_iRangeLow + 1;
                else if( pRange->m_iRangeCollapsed == 1 ) // Only the first collapsed range count a cell
                    relpos++;
                else // Do nothing with the rest collapsed ranges
                    ;
            }
        }
    }


    return iLocalMatches;
}


// local
VART* CIntDriver::GetVarT(Symbol* pSymbol)
{
    if( pSymbol->IsA(SymbolType::Variable) )
        return (VART*)pSymbol;

    else if( pSymbol->IsA(SymbolType::ValueSet) )
        return ((ValueSet*)pSymbol)->GetVarT();

    ASSERT(false);
    return nullptr;
}

//SAVY 4 SPEED BEGIN
double CIntDriver::DoOneXtabFor4AllSubTablesOfCurUnit( CTAB* pCtab, double dWeight, int iTabLogicExpr )
{
    CSubTable* pSubTable = NULL;
    double      depvar[2];
    double      rc=0;

    double dRet = 0;
    int iSubTbl=0;
    int i=0;
    //Get the array of subtables
    std::vector<std::shared_ptr<VTSTRUCT>>& arrVectors = pCtab->m_arrVectors;
    CtUnit* pCurrentUnit= pCtab->GetCurrentUnit();
    for( iSubTbl=0; iSubTbl < pCurrentUnit->GetNumSubTableIndex(); iSubTbl++ ) {
        std::shared_ptr<VTSTRUCT> pVtStruct = GetVTStructFromPool();
        pVtStruct->Init();
        pVtStruct->pSubTable = &(pCtab->GetSubTable(pCurrentUnit->GetSubTableIndex(iSubTbl)));
        //arrVectors.Add(pVtStruct);
        if( iSubTbl >= arrVectors.size() ) {
            arrVectors.resize(iSubTbl + 1);
        }
        arrVectors[iSubTbl] = pVtStruct;
        //      if( dRet == 0 ) dRet = dOneRet; // The first error is returned SAVY do send the first error
    }

    //For each subtable
    for(iSubTbl =(int)arrVectors.size()-1; iSubTbl >=0; iSubTbl--){
        if(arrVectors[iSubTbl] == NULL){
            arrVectors.erase(arrVectors.begin() + iSubTbl);
            continue;
        }
        VTSTRUCT& vtStruct = *arrVectors[iSubTbl];
        CTAB* pCtabAux=NULL;
        pSubTable = vtStruct.pSubTable;
        // MEAN or SMEAN or HOTDECK: verify that dependent variables have non-special values
        //SAVY try and optimize this stuff . This being done repeatedly table level
        if( pCtab->GetTableType() != CTableDef::Ctab_Crosstab && pCtab->GetTableType() != CTableDef::Ctab_STable ) {
            depvar[1] = 1.0;

            for(i = 0; i <= 1 && pCtab->GetDepSymVar(i) != -1; i++ ) {
                int isym = pCtab->GetDepSymVar(i);
                Symbol* pSymbol= NPT(isym);

                if( pSymbol->GetType() == SymbolType::WorkVariable ) {
                    depvar[i] = GetSymbolWorkVariable(isym).GetValue();
                }
                else {
                    VARX*   pVarX;
                    VART*   pVarT;

                    pVarT = GetVarT( pSymbol );
                    pVarX = pVarT->GetVarX();

                    if( pCtab->GetDepVarOcc(i) == 0 ) {
                        depvar[i] = svarvalue( pVarX );
                    }
                    else {
                        depvar[i] = mvarvalue( pVarX, (double) pCtab->GetDepVarOcc(i) );
                    }
                }

                if( IsSpecial(depvar[i]) ){
                    //  return( (double) 7 );
                    //New mechanism to return error .Do not exit as other subtables are being processed
                    //Remove this subtable from the list to be processed
                    if( dRet == 0 ) dRet = (double) 7 ;
                    AddVTStructToPool(arrVectors[iSubTbl]);
                    arrVectors.erase(arrVectors.begin() + iSubTbl);
                    continue;
                }
            }

            vtStruct.bKind[3] = false;
        }
        else{
            if(pSubTable->GetStatType() != CTSTAT_NONE &&
                // RHF COM Jan 30, 2003        !bIsPercent &&
                pSubTable->GetStatType() != CTSTAT_FREQ // RHF Oct 07, 2002
                && pSubTable->GetStatType() != CTSTAT_TOTAL // RHF Jun 11, 2003
                ) {

                    if( pSubTable->GetStatType() == CTSTAT_OVERLAPPED ){
                        // return 0; do not return continue; with other
                        AddVTStructToPool(arrVectors[iSubTbl]);
                        arrVectors.erase(arrVectors.begin() + iSubTbl);
                        continue;
                    }

                    vtStruct.bKind[3] = true;

                    ASSERT( pSubTable->GetAuxSymCtab() > 0 );
                    pCtabAux = XPT(pSubTable->GetAuxSymCtab());

                    //iRightCells = pSubTable->GetRightCells();
                    //iRightCells = (iRightCells <= 0 ) ? 1 : iRightCells;

                    CtStatBase* pStatBase=pSubTable->GetStatBase();
                    ASSERT( pStatBase != NULL);

                    int iSubCells=pStatBase->GetSubCells();
                    ASSERT( iSubCells >= 1 );
                    if( pSubTable->GetSymStatDim() == DIM_ROW ) {
                        vtStruct.iNumSubCells[0] = iSubCells;
                    }
                    else if( pSubTable->GetSymStatDim() == DIM_COL ) {
                        vtStruct.iNumSubCells[1] = iSubCells;
                    }
                    else if( pSubTable->GetSymStatDim() == DIM_LAYER ) {
                        vtStruct.iNumSubCells[2] = iSubCells;
                    }
                    else
                        ASSERT(0);

                    // RHF INIC Jan 29, 2003
                    if( pSubTable->GetStatType() == CTSTAT_PERCENT) {
                        CBorder*            pTotalBorder=pCtabAux->m_pBorder;
                        ASSERT( pTotalBorder );

                        pTotalBorder->CleanArrays(false);
                    }
                    // RHF END Jan 29, 2003
                }
        }
    }

    // int/long cells: verify decimals in weight value
    if( pCtab->GetAcumType() == sizeof(short) && dWeight != (short) dWeight )
        rc = 6;
    else if( pCtab->GetAcumType() == sizeof(long) && dWeight != (long) dWeight )
        rc = 6;
    else
        rc = 0;

    // # of elements in row, col, layer
    //Now init is in the structure
#ifdef _DELME
    for( i = 0; i < DIM_MAXDIM; i++ ) {
        ctvector[i][0] = 1;
        ctvector[i][1] = 0;
    }
#endif
    //TALLY part is getting done here for all subtable of the current unit
    //Find "Vectors" to reuse
#ifdef _INCLUDE
    CIMSAString sName;
    CIMSAString sFrqName;
    CIMSAString sTotName;
    bool bFound = false;
    for(iSubTbl =0; iSubTbl < (int)arrVectors.size(); iSubTbl++){
        sName = arrVectors.GetAt(iSubTbl)->pSubTable->GetName();
        if(sName.Find(_T("(Percent)")) != -1){
            bFound = false;
            sTotName = sFrqName = sName;
            sFrqName.Replace(_T("(Percent)"),_T("(Freq)"));
            for(i=0; i< (int)arrVectors.size();i++){
                if(arrVectors[i]->pSubTable->GetName().CompareNoCase(sFrqName)==0){
                    arrVectors.GetAt(iSubTbl)->iReUseIndex = i;
                    bFound = true;
                    break;
                }
            }
            if(!bFound){
                sTotName.Replace(_T("(Percent)"),_T("(Total)"));
                for(i=0; i< (int)arrVectors.size();i++){
                    if(arrVectors[i]->pSubTable->GetName().CompareNoCase(sTotName)==0){
                        arrVectors.GetAt(iSubTbl)->iReUseIndex = i;
                        bFound = true;
                        break;
                    }
                }
            }
            if(!bFound){
                arrProcessVectors.Add(arrVectors.GetAt(iSubTbl));
            }
        }
        else {
            arrProcessVectors.Add(arrVectors.GetAt(iSubTbl));
        }
    }
#endif
    for(i = 0; i < pCtab->GetNumDim(); i++ ) {
        for(iSubTbl =0; iSubTbl < (int)arrVectors.size(); iSubTbl++){
            arrVectors[iSubTbl]->iUseDim = i;
        }
        CtPos( pCtab, pCtab->GetNodeExpr(i),arrVectors, NULL, false );
        //CtPos( pCtab, pCtab->GetNodeExpr(i),arrProcessVectors, NULL, false );
    }

    int iUseSubTbl = -1;
    TCHAR*       pValue;
    //int         /*ctvector[DIM_MAXDIM][MAX_TALLYCELLS+1],moved to VTSTRUCT*/ isym, r, rc, i, j, k;
    int         /*ctvector[DIM_MAXDIM][MAX_TALLYCELLS+1],moved to VTSTRUCT*/ isym, r,j, k;
    int         iValue;
    long        longValue;
    MEANACUM*   pacum;
    SMEANACUM*  spacum;
    Symbol* pSymbol;
    // bool        bKind[4] = {false,false,false,false};//moved to VTSTRUCT
    double*     pdValueAux;
    bool        bOkCoord;
    //  int         iNumSubCells[TBD_MAXDIM]={1,1,1}; //moved to VTSTRUCT
    CtStatBase* pStatBase=NULL;
    int         iSubCells=0;
    bool bIsPercent = false;

    double      dDummyValue=0;
    bool    bValidValue,bHotDeck;

    int     iTallyNumber; // RHF Jul 29, 2002
    int     iRow=0, iColumn=0, iLayer=0;
    CStatType   eStatType;
    int         iSubTableI, iSubTableJ, iSubTableK;
    int         iTableOffSet;
    double      dSubTableValue;


    for(iSubTbl = 0; iSubTbl < (int)arrVectors.size(); iSubTbl++){
        int rc = 0;
        longValue = iValue = isym= r= i= j= k=-1;
        pStatBase = NULL;
        pdValueAux = NULL;
        pSymbol  = NULL;
        pValue = NULL;
        spacum= NULL;
        pacum = NULL;
        bOkCoord = false;
        //  int         iNumSubCells[TBD_MAXDIM]={1,1,1}; //moved to VTSTRUCT
        dDummyValue = iSubCells=0;

        // bool        bIsPercent=(pSubTable != NULL && pSubTable->GetStatType() == CTSTAT_PERCENT );// RHF Jan 30, 2003




        CStatValue  cEmptyStat;
        //arrVectors[iSubTbl]->iReUseIndex != -1 ?  iUseSubTbl = arrVectors[iSubTbl]->iReUseIndex : iUseSubTbl =iSubTbl;
        VTSTRUCT& vtStruct = *arrVectors[iSubTbl];//subtable may be different from vector being used
        VTSTRUCT& vtStruct4Vector = *arrVectors[iSubTbl];//subtable may be different from vector being used
        pSubTable = arrVectors[iSubTbl]->pSubTable;

        pStatBase=pSubTable->GetStatBase();



        CTAB* pCtabAux = NULL;
        if(pSubTable->GetStatType() != CTSTAT_NONE &&
                // RHF COM Jan 30, 2003        !bIsPercent &&
                pSubTable->GetStatType() != CTSTAT_FREQ // RHF Oct 07, 2002
                && pSubTable->GetStatType() != CTSTAT_TOTAL // RHF Jun 11, 2003
                ){
                    ASSERT( pSubTable->GetAuxSymCtab() > 0 );
                    pCtabAux = XPT(pSubTable->GetAuxSymCtab());
                }
        bIsPercent = pSubTable->GetStatType() == CTSTAT_PERCENT;

        if( !vtStruct.bKind[3] ) {
            vtStruct.bKind[0] =  ( pCtab->GetTableType() == CTableDef::Ctab_Crosstab || pCtab->GetTableType() == CTableDef::Ctab_STable || pCtab->GetTableType() == CTableDef::Ctab_Hotdeck );
            vtStruct.bKind[1] =  ( pCtab->GetTableType() == CTableDef::Ctab_Mean );
            vtStruct.bKind[2] =  ( pCtab->GetTableType() == CTableDef::Ctab_SMean );
#ifdef _DEBUG
            if( !vtStruct.bKind[0] && !vtStruct.bKind[1] && !vtStruct.bKind[2] ) {
                rc = 9;
                ASSERT(0);
                //SAVY now do not return . instead continue with other subtables
                continue;
                //              return( (double) rc );
            }
#endif
        }

        bHotDeck=(pCtab->GetTableType() == CTableDef::Ctab_Hotdeck);
        bValidValue = false;
        iRow= iColumn=iLayer=0;

        for( k = 1; k <= vtStruct4Vector.ctvector[2][0]; k++ ) { // Layer
            if( vtStruct4Vector.ctvector[2][k] == -1 )
                continue;
            iLayer++;
            iColumn = 0;
            for( j = 1; j <= vtStruct4Vector.ctvector[1][0]; j++ ) { // Column
                if( vtStruct4Vector.ctvector[1][j] == -1 )
                    continue;
                iColumn++;
                iRow = 0;
                for( i = 1; i <= vtStruct4Vector.ctvector[0][0]; i++ ) { // Row
                    if( vtStruct4Vector.ctvector[0][i] == -1 )
                        continue;

                    iRow++;
                    pValue = (TCHAR*) pCtab->m_pAcum.GetValue( vtStruct4Vector.ctvector[0][i], vtStruct4Vector.ctvector[1][j], vtStruct4Vector.ctvector[2][k] );

                    r = 0;
                    if( pValue != NULL ) {
                        // RHF INIC Jan 30, 2003
                        // Refresh TOTAL table always

                        if( pCtab->GetCtabGrandTotal() != NULL ) { // RHF Apr 17, 2003
                            ASSERT( pCtab->GetCtabGrandTotal() != NULL && pCtab->GetCtabGrandTotal()->GetTableType() == CTableDef::Ctab_GrandTotal ); // Grand Total

                            //Fill TABLE Total if the corresponding border is not used
                            pCtab->GetCtabGrandTotal()->FillTotals( dWeight, vtStruct4Vector.ctvector[0][i], vtStruct4Vector.ctvector[1][j], vtStruct4Vector.ctvector[2][k] );
                            // RHF END Jan 30, 2003
                        } // RHF Apr 17, 2003

                        if( vtStruct.bKind[0] ) {
                            switch( pCtab->GetAcumType() ) {
                        case sizeof(short):
                            if( bHotDeck ) {
                                bValidValue = ( depvar[0] >= SHRT_MIN && depvar[0] <= SHRT_MAX );
                                if( bValidValue ) {
                                    iValue = (short) depvar[0];
                                    bValidValue = ( iValue == depvar[0] );
                                }

                                if( !bValidValue )
                                    r = 5;
                                else
                                    *( (short *) pValue ) = depvar[0];
                            }
                            else {
                                iValue = *( (short *) pValue );
                                if( iValue + (short) dWeight < iValue )
                                    r = 5;

                                *( (short *) pValue ) += dWeight;
                            }
                            break;
                        case sizeof(long):
                            if( bHotDeck ) {
                                bValidValue = ( depvar[0] >= INT_MIN && depvar[0] <= INT_MAX );
                                if( bValidValue ) {
                                    longValue = (long) depvar[0];
                                    bValidValue = ( longValue == depvar[0] );
                                }

                                if( !bValidValue )
                                    r = 5;
                                else
                                    *( (long *) pValue ) = depvar[0];
                            }
                            else {
                                longValue = *( (long *) pValue );
                                if( longValue + (long) dWeight < longValue )
                                    r = 5;

                                *( (long *) pValue ) += dWeight;
                            }
                            break;
                        case sizeof(double):
                            if( bHotDeck )
                                *( (double *) pValue ) = depvar[0];
                            else
                                *( (double *) pValue ) += dWeight;
                            break;
                            }
                        }
                        else if( vtStruct.bKind[1] ) {
                            pacum = (MEANACUM *) pValue;
                            pacum->m_dSumX   += depvar[0];
                            pacum->m_dSumW   += dWeight;
                            pacum->m_dSumXW  += depvar[0] * dWeight;
                            pacum->m_dSumXW2 += depvar[0] * depvar[0] * dWeight;
                        }
                        else if( vtStruct.bKind[2] ) {
                            spacum = (SMEANACUM *) pValue;

                            spacum->m_dNumWeight += depvar[0] * dWeight;
                            spacum->m_dNumSquare += depvar[0] * depvar[0] * dWeight;
                            spacum->m_dDenWeight += depvar[1] * dWeight;
                            spacum->m_dDenSquare += depvar[1] * depvar[1] * dWeight;
                            spacum->m_dDenUnWeight += depvar[1];
                            spacum->m_dCumWeight += dWeight;
                            spacum->m_dCrosProd += depvar[0] * dWeight * depvar[1];
                            spacum->m_dCumCases += 1;
                        }
                        else if( vtStruct.bKind[3] ) {
                            dSubTableValue = iTableOffSet = iSubTableI =iSubTableJ= iSubTableK =-1;
                            eStatType = pSubTable->GetStatType();

                            ASSERT( pSubTable != 0 );
                            if( pSubTable->GetSymStatDim() == DIM_ROW )
                                //iTallyNumber = i - 1;
                                iTallyNumber = iRow-1;
                            else if( pSubTable->GetSymStatDim() == DIM_COL )
                                //iTallyNumber = j - 1;
                                iTallyNumber = iColumn-1;
                            else if( pSubTable->GetSymStatDim() == DIM_LAYER )
                                //iTallyNumber = k - 1;
                                iTallyNumber = iLayer - 1;
                            else
                                ASSERT(0);

                            // RHF COM Jan 30, 2003 CStatValue&  cStatValue=pSubTable->GetStatValue(iTallyNumber);

                            CStatValue&  cStatValue=bIsPercent?cEmptyStat:pSubTable->GetStatValue(iTallyNumber);

                            dSubTableValue = cStatValue.m_dValue;

                            // RHF INIT Sep 07, 2004
                            int             iExtraCellDisplacment=0;
                            //SAVY moved the subtable related code here
                            // RHF INIT Sep 07, 2004
                            bool        bNeedExtraCellDisplacment=
                                (pSubTable != NULL &&
                                (
                                pSubTable->GetStatType() == CTSTAT_MODE ||
                                pSubTable->GetStatType() == CTSTAT_MEDIAN ||
                                pSubTable->GetStatType() == CTSTAT_PTILE
                                )
                                );// RHF Sep 07, 2004

                            bool        bNeedExtraOneCellDisplacment=
                                (
                                pSubTable != NULL &&
                                (
                                pSubTable->GetStatType() == CTSTAT_PROP ||
                                pSubTable->GetStatType() == CTSTAT_MEAN ||
                                pSubTable->GetStatType() == CTSTAT_STDEV ||
                                pSubTable->GetStatType() == CTSTAT_VARIANCE ||
                                pSubTable->GetStatType() == CTSTAT_STDERR
                                )
                                );

                            // RHF END Sep 07, 2004
                            if( bNeedExtraCellDisplacment || bNeedExtraOneCellDisplacment ) {
                                ASSERT(pStatBase);
                                iSubCells=pStatBase->GetSubCells();
                                if( bNeedExtraCellDisplacment )
                                    ASSERT( cStatValue.m_iPos >= 0 && cStatValue.m_iPos < iSubCells );

                                if( pSubTable->GetSymStatDim() == DIM_ROW )
                                    iExtraCellDisplacment = bNeedExtraCellDisplacment ? cStatValue.m_iPos :
                                1;
                                else if( pSubTable->GetSymStatDim() == DIM_COL )
                                    iExtraCellDisplacment = bNeedExtraCellDisplacment ? (cStatValue.m_iPos * pCtabAux->m_pAcum.GetNumRows()) :
                                pCtabAux->m_pAcum.GetNumRows();
                                else if( pSubTable->GetSymStatDim() == DIM_LAYER )
                                    iExtraCellDisplacment = bNeedExtraCellDisplacment ? (cStatValue.m_iPos * pCtabAux->m_pAcum.GetNumRows() * pCtabAux->m_pAcum.GetNumCols()) :
                                (pCtabAux->m_pAcum.GetNumRows() * pCtabAux->m_pAcum.GetNumCols());
                                else
                                    ASSERT(0);
                            }
                            // RHF END Sep 07, 2004

                            iTableOffSet = pCtab->m_pAcum.GetOffSetPos( vtStruct4Vector.ctvector[0][i], vtStruct4Vector.ctvector[1][j], vtStruct4Vector.ctvector[2][k] );

                            iTableOffSet /= sizeof(double);

                            ASSERT( pCtabAux != NULL );

                            if( pCtab->GetNumCells() <= MAXCELL_REMAP )
                                bOkCoord = pSubTable->GetSubTableCoord( iSubTableI, iSubTableJ, iSubTableK, iTableOffSet );
                            else
                                bOkCoord = pCtab->GetSubTableCoordOnLine( iSubTableI, iSubTableJ, iSubTableK, iTableOffSet, pSubTable );

                            ASSERT( bOkCoord );

                            // RHF INIC Jul 07, 2005
                            if( eStatType == CTSTAT_PTILE || eStatType == CTSTAT_PROP ) {
                                int        iFinalCells=1;

                                if( eStatType == CTSTAT_PTILE ) {
                                    CtStatPTile*  pStatPTile=(CtStatPTile*) pStatBase;

                                    ASSERT( pStatPTile->GetNumTiles() >= 2 );

                                    if( pStatPTile->GetNumTiles() > 2 )
                                        iFinalCells=(pStatPTile->GetNumTiles() - 1);
                                }
                                else if( eStatType == CTSTAT_PROP ) {
                                    CtStatProp*  pStatProp=(CtStatProp*) pStatBase;

                                    if( pStatProp->GetPropType() & CTSTAT_PROP_TOTAL )
                                        iFinalCells = 2;
                                }
                                else
                                    ASSERT(0);

                                if( iFinalCells > 1 ) {
                                    if( pSubTable->GetSymStatDim() == DIM_ROW ) {
                                        iSubTableI /= iFinalCells;
                                    }
                                    else if( pSubTable->GetSymStatDim() == DIM_COL ) {
                                        iSubTableJ /= iFinalCells;
                                    }
                                    else if( pSubTable->GetSymStatDim() == DIM_LAYER ) {
                                        iSubTableK /= iFinalCells;
                                    }
                                    else
                                        ASSERT(0);
                                }
                            }
                            // RHF END Jul 07, 2005

                            iSubTableI *= vtStruct.iNumSubCells[0];
                            iSubTableJ *= vtStruct.iNumSubCells[1];
                            iSubTableK *= vtStruct.iNumSubCells[2];

                            if( !bOkCoord )
                                pdValueAux = NULL;
                            else {
                                if( bIsPercent )
                                    pdValueAux = &dDummyValue; // Not used in percent
                                else
                                    pdValueAux = (double*) pCtabAux->m_pAcum.GetValue( iSubTableI, iSubTableJ, iSubTableK );
                            }

                            ASSERT(pdValueAux);


                            if( pdValueAux != NULL ) {
#ifdef _DEBUG
                                double  dCurrentValue=*pdValueAux;
#endif
                                eStatType = pSubTable->GetStatType();

                                switch( eStatType ) {
                            case CTSTAT_OVERLAPPED: // OK Never executed
                                ASSERT(0);
                                //*(pdValueAux) = NOTAPPL;
                                break;

                            case CTSTAT_FREQ: // OK Never executed
                                ASSERT(0);
                                //*(pdValueAux) += dWeight;
                                break;

                            case CTSTAT_TOTAL:
                                ASSERT(0); // RHF Jun 11, 2003
                                // RHF COM Jun 02, 2003 *(pdValueAux) += dSubTableValue * dWeight;
                                *(pdValueAux) += dWeight;
                                break;

                            case CTSTAT_PERCENT:
                                // RHF INIC Jan 29, 2003
                                *( (double *) pValue ) += dWeight; // Fill freq on the base table

                                //Fill SubTable Total if the corresponding border is not used
                                pCtabAux->FillTotals( dWeight, iSubTableI, iSubTableJ, iSubTableK );
                                // RHF END Jan 29, 2003
                                break;
                            case CTSTAT_PROP:
                                {
                                    ASSERT( iSubCells == 2 );
                                    bool    bMatch=false;
                                    CtStatProp*     pStatProp=(CtStatProp*) pStatBase;
                                    ASSERT( pStatBase != 0 );
                                    ASSERT( pStatProp != 0 );
                                    ASSERT( pStatBase->GetStatType() == CTSTAT_PROP );
                                    for( int iRange=0; !bMatch && iRange < pStatProp->GetNumRanges(); iRange++ ) {
                                        CSubRange& cSubRange=pStatProp->GetRange(iRange);

                                        if( dSubTableValue >= cSubRange.GetLow() && dSubTableValue <= cSubRange.GetHigh() )
                                            bMatch = true;
                                    }

                                    if( bMatch )
                                        (*pdValueAux) += dWeight; // Matches

                                    // RHF COM Sep 07, 2004 pdValueAux++;
                                    pdValueAux += iExtraCellDisplacment; //RHF Sep 07, 2004

                                    (*pdValueAux) += dWeight; // N
                                }
                                break;
                            case CTSTAT_MIN:
                                ASSERT( iSubCells == 1 );
                                // Init pCtabAux with NOTAPPL
                                *(pdValueAux) = std::min( *(pdValueAux), dSubTableValue );
                                break;
                            case CTSTAT_MAX:
                                ASSERT( iSubCells == 1 );
                                // Init pCtabAux with -NOTAPPL
                                *(pdValueAux) = std::max( *(pdValueAux), dSubTableValue );
                                break;
                            case CTSTAT_MODE:
                                // Init pCtabAux with -NOTAPPL
                                ASSERT( cStatValue.m_iPos >= 0 && cStatValue.m_iPos < iSubCells );

                                // RHF COM Sep 07, 2004 pdValueAux += cStatValue.m_iPos;
                                pdValueAux += iExtraCellDisplacment; //RHF Sep 07, 2004

                                if( (*pdValueAux) == -NOTAPPL )
                                    (*pdValueAux) = dWeight;
                                else
                                    (*pdValueAux) += dWeight;

                                break;
                            case CTSTAT_MEAN:
                                {
                                    ASSERT( iSubCells == 2 );

                                    (*pdValueAux) += dSubTableValue*dWeight; // Sum X

                                    // RHF COM Sep 07, 2004 pdValueAux++;
                                    pdValueAux += iExtraCellDisplacment; //RHF Sep 07, 2004

                                    (*pdValueAux) += dWeight; // N
                                }
                                break;

                            case CTSTAT_MEDIAN:
                            case CTSTAT_PTILE:
                                // Init pCtabAux with -NOTAPPL
                                ASSERT( cStatValue.m_iPos >= 0 && cStatValue.m_iPos < iSubCells );

                                // RHF COM Sep 07, 2004 pdValueAux += cStatValue.m_iPos;
                                pdValueAux += iExtraCellDisplacment; //RHF Sep 07, 2004

                                if( (*pdValueAux) == -NOTAPPL )
                                    (*pdValueAux) = dWeight;
                                else
                                    (*pdValueAux) += dWeight;

                                break;
                            case CTSTAT_STDEV:
                            case CTSTAT_VARIANCE:
                            case CTSTAT_STDERR:
                                {
                                    ASSERT( iSubCells == 3 );
                                    (*pdValueAux) += dSubTableValue*dWeight; // Sum X

                                    // RHF COM Sep 07, 2004 pdValueAux++;
                                    pdValueAux += iExtraCellDisplacment; //RHF Sep 07, 2004

                                    (*pdValueAux) += dSubTableValue*dSubTableValue*dWeight; // Sum X2

                                    // RHF COM Sep 07, 2004 pdValueAux++;
                                    pdValueAux += iExtraCellDisplacment; //RHF Sep 07, 2004

                                    (*pdValueAux) += dWeight; // N
                                }
                                break;
                            case CTSTAT_VPCT: // TODO
                                {
                                }
                                break;
                            default:
                                ASSERT(0);
                                }

                            }
                        }
                    }
                    else // pValue is NULL
                        r = 8;
                    rc = ( r > rc ) ? r : rc;
                } // For Row
            } // For Column
        } // For Layer

    }//Add a loop for each subtable
    //clean array
    for(int iIndex =0; iIndex < (int)arrVectors.size(); iIndex++){
//      arrVectors[iIndex]->Clean();
        AddVTStructToPool(arrVectors[iIndex]);
        arrVectors[iIndex] = NULL;
    }
    //arrVectors.RemoveAll();
    return( (double) rc );
}

void CIntDriver::CtPos( CTAB* pCtab, int iCtNode,std::vector<std::shared_ptr<VTSTRUCT>>& arrVectors,
                         CCoordValue* pCoordValue, bool bMarkAllPos )
{
    CTNODE* pNode = (CTNODE *) ( CtNodebase + iCtNode );
    if( pNode->isVarNode() ) // Variable or value-set
    {
        CtPos_Var( pCtab, iCtNode, arrVectors, pCoordValue, bMarkAllPos );
    }
    else
    {
        switch( pNode->getOperator() )
        {
        case CTNODE_OP_ADD:
            CtPos_Add( pCtab, pNode, arrVectors, pCoordValue, bMarkAllPos );
            break;
        case CTNODE_OP_MUL:
            CtPos_Mul( pCtab, pNode, arrVectors, pCoordValue, bMarkAllPos );
            break;
        default:
            ;
        }
    }
}
//////////////////////////////////////////////////////////////////////////
// CtPos refactoring -> CtPos_Add + CtPos_Mul + CtPos_Val
//
// rcl, Oct 26, 2004
void CIntDriver::CtPos_Add( CTAB* pCtab, CTNODE* pNode, std::vector<std::shared_ptr<VTSTRUCT>>& arrVectors,
                            CCoordValue* pCoordValue, bool bMarkAllPos )
{
    ASSERT( pNode->isOperNode(CTNODE_OP_ADD) );
//    ASSERT( vector != 0 );

    /*int v1[MAX_TALLYCELLS+1],
        v2[MAX_TALLYCELLS+1];*/ //We are doing the recursice stuff in this using temp arrays int VTSTRUCT
    //simulating the push and pop similar to a Stack.
    //Refer to original CtPos_Add for implementation of this
    int leftcells;
    int i;
    bool bUseV1;
    std::shared_ptr<std::vector<int>> pArrV1;
    std::shared_ptr<std::vector<int>> pArrV2;
    int* vector = NULL;
    for(int iIndex =0; iIndex < (int)arrVectors.size(); iIndex++){

        VTSTRUCT& vtStruct = *arrVectors[iIndex];
        pArrV1= GetIntArrFromPool();
        if(pArrV1->empty()){
            pArrV1->resize(MAX_TALLYCELLS+1);
        }
        //vtStruct.arrV1s.Add(pArrV1);
        vtStruct.AddV1Arr(pArrV1);
        //vtStruct.bUseV1 = true;
        bUseV1 = true;
        //vtStruct.arrbUseV1.Add(bUseV1);
        vtStruct.AddUseFlag(bUseV1);
    }
    CtPos( pCtab, pNode->m_iCtLeft,arrVectors,pCoordValue, bMarkAllPos );
    for(int iIndex =0; iIndex < (int)arrVectors.size(); iIndex++){

        VTSTRUCT& vtStruct = *arrVectors[iIndex];
        //vtStruct.arrbUseV1.RemoveAt(vtStruct.arrbUseV1.GetCount()-1);
        vtStruct.RemoveUseFlag();
    }

    for(int iIndex =0; iIndex < (int)arrVectors.size(); iIndex++){

        VTSTRUCT& vtStruct = *arrVectors[iIndex];
        pArrV2= GetIntArrFromPool();
        if(pArrV2->empty()){
            pArrV2->resize(MAX_TALLYCELLS+1);
        }
        //vtStruct.arrV2s.Add(pArrV2);
        vtStruct.AddV2Arr(pArrV2);
        //vtStruct.bUseV1 = false;
        bUseV1 = false;
        //vtStruct.arrbUseV1.Add(bUseV1);
        vtStruct.AddUseFlag(bUseV1);
    }
    CtPos( pCtab, pNode->m_iCtRight,arrVectors,pCoordValue, bMarkAllPos );
    for(int iIndex =0; iIndex < (int)arrVectors.size(); iIndex++){

        VTSTRUCT& vtStruct = *arrVectors[iIndex];
        //vtStruct.arrbUseV1.RemoveAt(vtStruct.arrbUseV1.GetCount()-1);
        vtStruct.RemoveUseFlag();
    }
    leftcells = ctcell( CtNodebase, pNode->m_iCtLeft );

    for(int iIndex =0; iIndex < (int)arrVectors.size(); iIndex ++){//for each subtable for the unit
        VTSTRUCT& vtStruct = *arrVectors[iIndex]; //Get the vector
        vector = NULL;
        bUseV1 = true;
        if(vtStruct.iUseIndex >= 0){
            bUseV1 = vtStruct.arrbUseV1[vtStruct.iUseIndex];
        }
        if(bUseV1){
            //if(vtStruct.arrV1s.GetCount() ==1) {
            if(vtStruct.iV1Index  == 0) {//only one element
                vector = vtStruct.ctvector[vtStruct.iUseDim]; //use the dimension which is currently in use
            }
            else {
                //vector = (vtStruct.arrV1s[vtStruct.arrV1s.GetCount()-2])->GetData();
                vector = vtStruct.arrV1s[vtStruct.iV1Index-1]->data();
            }
        }
        else {
            //if(vtStruct.arrV2s.GetCount() ==1) {
            if(vtStruct.iV2Index  == 0) {//only one element
                vector = vtStruct.ctvector[vtStruct.iUseDim]; //use the dimension which is currently in use
            }
            else {
                vector =  vtStruct.arrV2s[vtStruct.iV2Index-1]->data();
            }
        }
        //get the last v1
        std::vector<int>& arrV1 = *vtStruct.arrV1s[vtStruct.iV1Index];
        //get the last v2
        std::vector<int>& arrV2 = *vtStruct.arrV2s[vtStruct.iV2Index];

        vector[0] = arrV1.front() + arrV2.front();

        ASSERT( arrV1[0] < MAX_TALLYCELLS );
        for( i = 1; i <= arrV1[0]; i++ )
            vector[i] = arrV1[i];

        ASSERT( vector[0] < MAX_TALLYCELLS );
        for( i = i; i <= vector[0]; i++ ) {
            ASSERT( (i - arrV1.front()) >= 0 );

            if( arrV2[i - arrV1.front()] == -1 )
                vector[i] = -1;
            else
                vector[i] = arrV2[i - arrV1.front()] + leftcells;
        }

        //pop the last v1
        AddIntArrToPool(vtStruct.arrV1s[vtStruct.iV1Index]);
        vtStruct.RemoveArrV1();
        //pop the last v2
        AddIntArrToPool(vtStruct.arrV2s[vtStruct.iV2Index]);
        vtStruct.RemoveArrV2();
    }
    /* Original Method commented. Look at this for implementation details
    Do not delete this part .
    int v1[MAX_TALLYCELLS+1],
        v2[MAX_TALLYCELLS+1];
    int leftcells;
    int i;


    CtPos( pCtab, pNode->m_iCtLeft, v1, pSubTable, pCoordValue, bMarkAllPos );
    CtPos( pCtab, pNode->m_iCtRight, v2, pSubTable, pCoordValue, bMarkAllPos );

    leftcells = ctcell( CtNodebase, pNode->m_iCtLeft );

    vector[0] = v1[0] + v2[0];

    ASSERT( v1[0] < MAX_TALLYCELLS );
    for( i = 1; i <= v1[0]; i++ )
        vector[i] = v1[i];

    ASSERT( vector[0] < MAX_TALLYCELLS );
    for( i = i; i <= vector[0]; i++ ) {
        ASSERT( (i - v1[0]) >= 0 );

        if( v2[i - v1[0]] == -1 )
            vector[i] = -1;
        else
            vector[i] = v2[i - v1[0]] + leftcells;
    }*/
}
void CIntDriver::CtPos_Mul( CTAB* pCtab, CTNODE* pNode, std::vector<std::shared_ptr<VTSTRUCT>>& arrVectors,
                            CCoordValue* pCoordValue, bool bMarkAllPos )
{
    ASSERT( pNode->isOperNode(CTNODE_OP_MUL) ); // rcl, May 2005
    //    ASSERT( vector != 0 );

    /*int v1[MAX_TALLYCELLS+1],
    v2[MAX_TALLYCELLS+1];*/  //We are doing the recursice stuff in this using temp arrays int VTSTRUCT
    //simulating the push and pop similar to a Stack.
    //Refer to original CtPos_Mul for implementation of this
    int rightcells;
    int i, j, k;
    bool bUseV1;

    for(int iIndex =0; iIndex < (int)arrVectors.size(); iIndex++){
        VTSTRUCT& vtStruct = *arrVectors[iIndex];
        std::shared_ptr<std::vector<int>> pArrV1= GetIntArrFromPool();
        if(pArrV1->empty()){
            pArrV1->resize(MAX_TALLYCELLS+1);
        }
        //vtStruct.arrV1s.Add(pArrV1);
        vtStruct.AddV1Arr(pArrV1);
        bUseV1 = true;
        vtStruct.AddUseFlag(bUseV1);
    }
    CtPos( pCtab, pNode->m_iCtLeft,arrVectors, pCoordValue, bMarkAllPos );
    for(int iIndex =0; iIndex < (int)arrVectors.size(); iIndex++){
        VTSTRUCT& vtStruct = *arrVectors[iIndex];
        //vtStruct.arrbUseV1.RemoveAt(vtStruct.arrbUseV1.GetCount()-1);
        vtStruct.RemoveUseFlag();
    }
    for(int iIndex =0; iIndex < (int)arrVectors.size(); iIndex++){
        VTSTRUCT& vtStruct =*arrVectors[iIndex];
        std::shared_ptr<std::vector<int>> pArrV2= GetIntArrFromPool();
        if(pArrV2->empty()){
            pArrV2->resize(MAX_TALLYCELLS+1);
        }
        //vtStruct.arrV2s.Add(pArrV2);
        vtStruct.AddV2Arr(pArrV2);
        //vtStruct.bUseV1 = false;
        bUseV1 = false;
        //vtStruct.arrbUseV1.Add(bUseV1);
        vtStruct.AddUseFlag(bUseV1);
    }

    CtPos( pCtab, pNode->m_iCtRight, arrVectors, pCoordValue, bMarkAllPos );
    for(int iIndex =0; iIndex < (int)arrVectors.size(); iIndex++){
        VTSTRUCT& vtStruct = *arrVectors[iIndex];
        //vtStruct.arrbUseV1.RemoveAt(vtStruct.arrbUseV1.GetCount()-1);
        vtStruct.RemoveUseFlag();
    }
    rightcells = ctcell( CtNodebase, pNode->m_iCtRight );


    for(int iIndex =0; iIndex < (int)arrVectors.size(); iIndex ++){//for each subtable for the unit
        VTSTRUCT& vtStruct = *arrVectors[iIndex]; //Get the vector
        int* vector = NULL;
        bUseV1 = true;
        if(vtStruct.iUseIndex >= 0){
            bUseV1 = vtStruct.arrbUseV1[vtStruct.iUseIndex];
        }
        if(bUseV1){
            //if(vtStruct.arrV1s.GetCount() ==1) {
            if(vtStruct.iV1Index  == 0) {//only one element
                vector = vtStruct.ctvector[vtStruct.iUseDim]; //use the dimension which is currently in use
            }
            else {
                //vector = (vtStruct.arrV1s[vtStruct.arrV1s.GetCount()-2])->GetData();
                vector = (vtStruct.arrV1s[vtStruct.iV1Index-1])->data();
            }
        }
        else {
            //if(vtStruct.arrV2s.GetCount() ==1) {
            if(vtStruct.iV2Index  == 0) {//only one element
                vector = vtStruct.ctvector[vtStruct.iUseDim]; //use the dimension which is currently in use
            }
            else {
                //vector = (vtStruct.arrV2s[vtStruct.arrV2s.GetCount()-2])->GetData();
                vector =  (vtStruct.arrV2s[vtStruct.iV2Index-1])->data();
            }
        }

        //get the last v1
        std::vector<int>& arrV1 = *vtStruct.arrV1s[vtStruct.iV1Index];
        //get the last v2
        std::vector<int>& arrV2 = *vtStruct.arrV2s[vtStruct.iV2Index];

        vector[0] = arrV1[0] * arrV2[0];
        k = 1;
        ASSERT( arrV1[0] < MAX_TALLYCELLS );
        for( i = 1; i <= arrV1[0]; i++ ) {
            ASSERT( arrV2[0] < MAX_TALLYCELLS );
            for( j = 1; j <= arrV2[0]; j++ ) {
                ASSERT( k < MAX_TALLYCELLS );
                if( arrV2[j] == -1 || arrV1[i] == -1 )
                    vector[k++] = -1;
                else
                    vector[k++] = arrV2[j] + arrV1[i] * rightcells;
            }
        }
        //pop the last v1
        AddIntArrToPool(vtStruct.arrV1s[vtStruct.iV1Index]);
        vtStruct.RemoveArrV1();
        //pop the last v2
        AddIntArrToPool(vtStruct.arrV2s[vtStruct.iV2Index]);
        vtStruct.RemoveArrV2();
    }

    /*{Original Method commented. Look at this for implementation details
    Do not delete this part .
    ASSERT( pNode->isOperNode(CTNODE_OP_MUL) ); // rcl, May 2005
    ASSERT( vector != 0 );

    int v1[MAX_TALLYCELLS+1],
    v2[MAX_TALLYCELLS+1];
    int rightcells;
    int i, j, k;


    CtPos( pCtab, pNode->m_iCtLeft, v1, pSubTable, pCoordValue, bMarkAllPos );
    CtPos( pCtab, pNode->m_iCtRight, v2, pSubTable, pCoordValue, bMarkAllPos );

    rightcells = ctcell( CtNodebase, pNode->m_iCtRight );

    vector[0] = v1[0] * v2[0];
    k = 1;
    ASSERT( v1[0] < MAX_TALLYCELLS );
    for( i = 1; i <= v1[0]; i++ ) {
    ASSERT( v2[0] < MAX_TALLYCELLS );
    for( j = 1; j <= v2[0]; j++ ) {
    ASSERT( k < MAX_TALLYCELLS );
    if( v2[j] == -1 || v1[i] == -1 )
    vector[k++] = -1;
    else
    vector[k++] = v2[j] + v1[i] * rightcells;
    }
    }
    }*/
}
void CIntDriver::CtPos_Var(  CTAB* pCtab, int iCtNode, std::vector<std::shared_ptr<VTSTRUCT>>& arrVectors,CCoordValue* pCoordValue, bool bMarkAllPos )
{
    CTNODE* pNode = (CTNODE *) ( CtNodebase + iCtNode );

    ASSERT( pNode->isVarNode() && !pNode->isOperNode() );
    VART*   pCleanVarT = GetVarT( NPT(pNode->m_iSymbol) );
    bool bUseArray = false;
    bool bMultiValue = false;
    bool bFirstTime = true; //temp stuff

    bool bIsStatItem = false;
    bool bStatPercent = false;
    int* pVector = NULL;
    double dValue = -1234.5; // magic number to detect non assignments in debug time.
    int    iNumMatches =0;
    int j,i= 0;
    bool   bUseCoordNumber = false;
    int iSubTbl = 0;
    CSubTable* pSubTable = NULL;
    bool bHasStuffToProcess = false;
    for(iSubTbl =0; iSubTbl < (int)arrVectors.size(); iSubTbl++){

        VTSTRUCT& vtStruct = *arrVectors[iSubTbl];
        pSubTable  = vtStruct.pSubTable;
        vtStruct.bUse4Processing = false;
        vtStruct.bUseV1 = true;
        if(vtStruct.iUseIndex  >= 0){
            //vtStruct.bUseV1 = vtStruct.arrbUseV1.GetAt(vtStruct.arrbUseV1.GetCount()-1);//=vtStruct.bUseV1;
            vtStruct.bUseV1 = vtStruct.arrbUseV1[vtStruct.iUseIndex];//=vtStruct.bUseV1;
        }
        if(vtStruct.bUseV1){
            //if(vtStruct.arrV1s.GetCount()-1 == -1){//this happens when pcts and totals are not to gether ctpo_var is called first with out ctpos_add
            if(vtStruct.iV1Index == -1){//this happens when pcts and totals are not to gether ctpo_var is called first with out ctpos_add
                pVector = vtStruct.ctvector[vtStruct.iUseDim];
            }
            else {
                //pVector = (int*)vtStruct.arrV1s[vtStruct.arrV1s.GetCount()-1]->GetData();
                pVector = vtStruct.arrV1s[vtStruct.iV1Index]->data();
            }
        }
        else {
            //if(vtStruct.arrV2s.GetCount()-1 == -1){//this happens when pcts and totals are not to gether ctpo_var is called first with out ctpos_add
            if(vtStruct.iV2Index  == -1){//this happens when pcts and totals are not to gether ctpo_var is called first with out ctpos_add
                pVector = vtStruct.ctvector[vtStruct.iUseDim];
            }
            else {
                //pVector = (int*)vtStruct.arrV2s[vtStruct.arrV2s.GetCount()-1]->GetData();
                pVector = vtStruct.arrV2s[vtStruct.iV2Index]->data();
            }
        }

        if( pSubTable != NULL ){
            bUseCoordNumber = pSubTable->UseCoordNumber( pNode->m_iCoordNumber );
        }

#ifdef _DEBUG
        if( pSubTable == NULL )
            ASSERT( pCoordValue != NULL  );
#endif

        // RHF INIC Aug 01, 2002 A coordinate replace by a STAT 'without' freq. See CEngineCompFunc::AddStatCoordinate
        if( pNode->m_iNumCells == 0 ) {
            (pVector)[0] = 0;
             //return;
            continue;
        }

        if( bMarkAllPos && bUseCoordNumber ) {
            iNumMatches = pNode->m_iNumCells;

            // Stat Node must have 1 cell at least
            if( pNode->m_iStatType != CTSTAT_NONE ) {
                ASSERT( iNumMatches >= 1 );
                //iNumMatches = 1; // ONLY MARK THE FIRST SUB-CELL for remapping
            }

            ASSERT( iNumMatches < MAX_TALLYCELLS );
            // could 'never' happen, but ...       rcl, May 2005
            if( iNumMatches >= MAX_TALLYCELLS )
            {
                iNumMatches = MAX_TALLYCELLS - 1;
            }

            for( i=1; i <= iNumMatches; i++ )
                (pVector)[i] = i-1;

            //BEGIN COMMON STUFF
            if( iNumMatches == 0 )
                (pVector)[0] = 1;
            else
                (pVector)[0] = iNumMatches;

            ASSERT( iNumMatches < MAX_TALLYCELLS );
            // could 'never' happen, but ...       rcl, May 2005
            if( iNumMatches >= MAX_TALLYCELLS )
            {
                iNumMatches = MAX_TALLYCELLS - 1;
            }

            (pVector)[iNumMatches+1] = -1;
            //END COMMON STUFF

        }
        // RHF END Aug 01, 2002

        else if( pCoordValue != NULL ) {
#ifdef _DEBUG
            VART*   pVarT = GetVarT( NPT(pNode->m_iSymbol) );
#endif
            if( iCtNode == pCoordValue->m_iCtNodeLeft ) {
                dValue = pCoordValue->m_dValueLeft;
                (pVector)[0] = 1;
                (pVector)[1] = ctvarpos( iCtNode, dValue, pCoordValue->m_iSeqLeft );
                iNumMatches = ((pVector)[1] >= 0) ? 1 : 0;
            }
            else if( iCtNode == pCoordValue->m_iCtNodeRight ) {
                dValue = pCoordValue->m_dValueRight;
                (pVector)[0] = 1;
                (pVector)[1] = ctvarpos( iCtNode, dValue, pCoordValue->m_iSeqRight );
                iNumMatches = ((pVector)[1] >= 0) ? 1 : 0;
            }
            else {
                iNumMatches = 0;
            }

            // DoTally( iCtNode, dValue, vector, &iNumMatches, pCoordValue );

            //BEGIN COMMON STUFF
            if( iNumMatches == 0 )
                (pVector)[0] = 1;
            else
                (pVector)[0] = iNumMatches;

            ASSERT( iNumMatches < MAX_TALLYCELLS );
            // could 'never' happen, but ...       rcl, May 2005
            if( iNumMatches >= MAX_TALLYCELLS )
            {
                iNumMatches = MAX_TALLYCELLS - 1;
            }

            (pVector)[iNumMatches+1] = -1;
            //END COMMON STUFF

        }
        else if( !bUseCoordNumber ) {
            ; // iNumMatches = 0;

            //BEGIN COMMON STUFF
            if( iNumMatches == 0 )
                (pVector)[0] = 1;
            else
                (pVector)[0] = iNumMatches;

            ASSERT( iNumMatches < MAX_TALLYCELLS );
            // could 'never' happen, but ...       rcl, May 2005
            if( iNumMatches >= MAX_TALLYCELLS )
            {
                iNumMatches = MAX_TALLYCELLS - 1;
            }

            (pVector)[iNumMatches+1] = -1;
            //END COMMON STUFF
        }
        else {
            //arrProcessSubTables.Add(&vtStruct);
            bHasStuffToProcess = true;
            vtStruct.bUse4Processing = true;
            // RHF END Jul 16, 2002
            ASSERT( vtStruct.pSubTable != 0 );
            bIsStatItem  = ( vtStruct.pSubTable != NULL && vtStruct.pSubTable->GetStatNumber() >= 1 && vtStruct.pSubTable->GetStatCtTree() == iCtNode );// RHF Aug 09, 2002
            bStatPercent = ( bIsStatItem && vtStruct.pSubTable->GetStatType() == CTSTAT_PERCENT );

            if( bIsStatItem )
                vtStruct.pSubTable->RemoveAllStatValues();
        }
    }
    if(bHasStuffToProcess){

        double dValueAux = NO_VALUE_ASSIGNED_YET; // See Fixing point later

        // Get VART/VARX
        VART* pVarT = GetVarT( NPT(pNode->m_iSymbol) );
        VARX* pVarX = pVarT->GetVarX();

#ifdef _DEBUG
        {
            CString csMsg;
            csMsg.Format( _T("CtPos: VarName=(%s) CoorNumber=%d, SeqNumber=%d"), pVarT->GetName().c_str(), pNode->m_iCoordNumber, pNode->m_iSeqNumber );
            //TRACE( csMsg );
        }
#endif

        int  iOccExpr = pNode->m_iCtOcc;
        bool bFitOcc  = false;

        int  iNumTallys = pVarT->VectorSize();
        iOccExpr = abs(iOccExpr);

        if( iOccExpr != 0 ) // RHF Jul 16, 2004
            iOccExpr--; // RHF Jul 16, 2004

        //          bool bMultiValue = false;
        // RHF INIC Jul 16, 2002 Check Multiple relation
        // It is not possible to have a vector in a multiple relation. See message 33200
        if( iNumTallys == 0 && pVarT->IsArray() && bUseArray == false){//fill the array first time
            CtPos_FillIndexArray( pCtab, pVarT, iOccExpr );
            /*double  dValue = 19.00;
            double  dLastOccurrence = 1;
            pVarT->VectorAdd( dValue, dLastOccurrence );*/
            bUseArray = true;
        }
        if(bUseArray){
            switch( pVarT->VectorSize() )
            {
            default: // other than 0 or 1 [hopefully bigger than 1]
                bMultiValue = true;
                iNumTallys = pVarT->VectorSize();
                break;
            case 1:
                {
                    // When vector has only 1 element
                    double dOccur;
                    int iOcc =0;
                    iNumTallys = 0; //Reset to execute the correct block down the line
                    dValueAux = pVarT->VectorGet( iOcc, dOccur );

                    // the only one element inside vector must be erased
                    // to keep backward compatibility with original code
                    // [When there was only 1 element it was *not* inserted
                    //  into VarT's vector]
                    //SAVY commented out clean . Will do in the end
                    if(iSubTbl == (int)arrVectors.size() -1){
                        pVarT->VectorClean();
                    }
                }
                break;
            case 0:
                // when vector has no elements we change the former
                // NO_VALUE_ASSIGNED_YET value to INVALIDINDEX so that
                // later we know that we do *not* have to fix it
                // by evaluating expression in ptrvar
                // [See "FIXING point" (many lines) below]
                dValueAux = INVALIDINDEX;
                break;
            }
        }
        // RHF END Jul 16, 2002
        int  iLocalMatches;

        // There is a valid vector
        if( iNumTallys >= 1 ) {
            double  dOccur = 0.0;
            double  dVectorOcc;

            if( pNode->m_iCtOcc <= 0 ) { // Multiple without indexes or single
                bFitOcc = true;
            }
            else {
                ASSERT( pVarT->IsArray() );
                ASSERT( *PPT(iOccExpr) == MVAR_CODE );

                MVAR_NODE*  ptrvar = (MVAR_NODE*) (PPT(iOccExpr));
                double      dIndex[DIM_MAXDIM];
#ifdef _DEBUG
                int     iIndexType[DIM_MAXDIM];
                mvarGetSubindexes( ptrvar, dIndex, iIndexType );

                // Multiple relation unsupported when a multiple var is used with subindexes
                if( iIndexType[0] == _T('M') || iIndexType[1] == _T('M') || iIndexType[2] == 'M' )
                    ASSERT(0);
#else
                mvarGetSubindexes( ptrvar, dIndex );
#endif
                dOccur = dIndex[0];
            }
            CStatValue  cStatValue;
            for( int iVector=0; iVector < iNumTallys; iVector++ ) {
                dValue = pVarT->VectorGet(iVector, dVectorOcc );

                if( bFitOcc || dVectorOcc == dOccur ) {
                    //iLocalMatches = DoTally( iCtNode, dValue, (int*)pVector->GetData(), &iNumMatches );
                    iLocalMatches = DoTally( iCtNode, dValue, arrVectors, &iNumMatches );

                    // RHF INIC Jul 29, 2002
                    for(iSubTbl =0; iSubTbl < (int)arrVectors.size(); iSubTbl++){
                        VTSTRUCT& vtStruct = *arrVectors[iSubTbl];
                        pSubTable  = vtStruct.pSubTable;
                        if(!vtStruct.bUse4Processing){
                            continue;
                        }
                        if(vtStruct.bUseV1){
                            if(vtStruct.iV1Index  == -1){//this happens when pcts and totals are not to gether ctpo_var is called first with out ctpos_add
                                pVector = vtStruct.ctvector[vtStruct.iUseDim];
                            }
                            else {
                                pVector = vtStruct.arrV1s[vtStruct.iV1Index]->data();
                            }
                        }
                        else {
                            if(vtStruct.iV2Index == -1){//this happens when pcts and totals are not to gether ctpo_var is called first with out ctpos_add
                                pVector = vtStruct.ctvector[vtStruct.iUseDim];
                            }
                            else{
                                pVector = vtStruct.arrV2s[vtStruct.iV2Index]->data();
                            }
                        }
                        bIsStatItem  = ( vtStruct.pSubTable != NULL && vtStruct.pSubTable->GetStatNumber() >= 1 && vtStruct.pSubTable->GetStatCtTree() == iCtNode );// RHF Aug 09, 2002
                        bStatPercent = ( bIsStatItem && vtStruct.pSubTable->GetStatType() == CTSTAT_PERCENT );

                        if( bIsStatItem && !bStatPercent && iLocalMatches > 0 ) {
                            for( j=0; j < iLocalMatches; j++ ) {
                                cStatValue.m_dValue = dValue;
                                cStatValue.m_iPos = (pVector)[j+1];
                                //pSubTable->AddStatValue( dValue, (*pVector)[j+1] );
                                pSubTable->AddStatValue(cStatValue);
                                (pVector)[j+1] = 0; // All stat use slot 0. Except percent! // RHF Sep 17, 2002
                            }
                        }
                        //BEGIN COMMON STUFF
                        if( iNumMatches == 0 )
                            (pVector)[0] = 1;
                        else
                            (pVector)[0] = iNumMatches;

                        ASSERT( iNumMatches < MAX_TALLYCELLS );
                        // could 'never' happen, but ...       rcl, May 2005
                        if( iNumMatches >= MAX_TALLYCELLS )
                        {
                            iNumMatches = MAX_TALLYCELLS - 1;
                        }

                        (pVector)[iNumMatches+1] = -1;
                        //END COMMON STUFF
                    }
                }
            }
        }
        else { // No Tally vector
            // RHF COM Jul 16, 2004 if( iOccExpr == 0 )   // ... single
            if( !pVarT->IsArray() ) // RHF Jul 16, 2004
                dValue = svarvalue( pVarX );
            else { // Multiple

                ///////////////////////////////////////////////////////////////
                // FIXING point here
                if( dValueAux != NO_VALUE_ASSIGNED_YET )
                    dValue = dValueAux;
                else
                    dValue = evalexpr( iOccExpr );
                ///////////////////////////////////////////////////////////////
            }
            //iLocalMatches = DoTally( iCtNode, dValue, (int*)pVector->GetData(), &iNumMatches );
            iLocalMatches = DoTally( iCtNode, dValue,arrVectors, &iNumMatches );

            // RHF INIC Jul 29, 2002

            CStatValue  cStatValue;
            for(iSubTbl =0; iSubTbl < (int)arrVectors.size(); iSubTbl++){
                VTSTRUCT& vtStruct = *arrVectors[iSubTbl];
                if(!vtStruct.bUse4Processing){
                    continue;
                }
                pSubTable  = vtStruct.pSubTable;
                if(vtStruct.bUseV1){
                    //if(vtStruct.arrV1s.GetCount()-1 == -1){//this happens when pcts and totals are not to gether ctpo_var is called first with out ctpos_add
                    if(vtStruct.iV1Index  == -1){//this happens when pcts and totals are not to gether ctpo_var is called first with out ctpos_add
                        pVector = vtStruct.ctvector[vtStruct.iUseDim];
                    }
                    else {
                        pVector = vtStruct.arrV1s[vtStruct.iV1Index]->data();
                    }
                }
                else {
                    //if(vtStruct.arrV2s.GetCount()-1 == -1){//this happens when pcts and totals are not to gether ctpo_var is called first with out ctpos_add
                    if(vtStruct.iV2Index == -1){//this happens when pcts and totals are not to gether ctpo_var is called first with out ctpos_add
                        pVector = vtStruct.ctvector[vtStruct.iUseDim];
                    }
                    else{
                        pVector = vtStruct.arrV2s[vtStruct.iV2Index]->data();
                    }
                }
                bIsStatItem  = ( vtStruct.pSubTable != NULL && vtStruct.pSubTable->GetStatNumber() >= 1 && vtStruct.pSubTable->GetStatCtTree() == iCtNode );// RHF Aug 09, 2002
                bStatPercent = ( bIsStatItem && vtStruct.pSubTable->GetStatType() == CTSTAT_PERCENT );

                if( bIsStatItem && !bStatPercent && iLocalMatches > 0 ) {
                    for(j=0; j < iLocalMatches; j++ ) {
                        cStatValue.m_dValue = dValue;
                        cStatValue.m_iPos = (pVector)[j+1];
                        //pSubTable->AddStatValue( dValue, (*pVector)[j+1] );
                        pSubTable->AddStatValue(cStatValue);
                        (pVector)[j+1] = 0; // All stat use slot 0. Except percent! // RHF Sep 17, 2002
                    }
                }
                //BEGIN COMMON STUFF
                if( iNumMatches == 0 )
                (pVector)[0] = 1;
                else
                    (pVector)[0] = iNumMatches;

                ASSERT( iNumMatches < MAX_TALLYCELLS );
                // could 'never' happen, but ...       rcl, May 2005
                if( iNumMatches >= MAX_TALLYCELLS )
                {
                    iNumMatches = MAX_TALLYCELLS - 1;
                }

                (pVector)[iNumMatches+1] = -1;
            //END COMMON STUFF
            }
            // RHF END Jul 29, 2002
        }



        //SAVY Do this for all subtables which reach this level
        // set the number of relative positions
        //SAVY DO FOR ALL THE PROCESS SUBTABLES ARR with appropriate pVector
#ifdef _DEL_ME
        for(iSubTbl =0; iSubTbl < arrProcessSubTables.GetCount(); iSubTbl++){
            VTSTRUCT& vtStruct = *(arrProcessSubTables[iSubTbl]);
            pSubTable  = vtStruct.pSubTable;
            if(vtStruct.bUseV1){
                pVector = vtStruct.arrV1s[vtStruct.arrV1s.GetCount()-1];
            }
            else {
                pVector = vtStruct.arrV2s[vtStruct.arrV2s.GetCount()-1];
            }
            if( iNumMatches == 0 )
                (pVector)[0] = 1;
            else
                (pVector)[0] = iNumMatches;

            ASSERT( iNumMatches < MAX_TALLYCELLS );
            // could 'never' happen, but ...       rcl, May 2005
            if( iNumMatches >= MAX_TALLYCELLS )
            {
                iNumMatches = MAX_TALLYCELLS - 1;
            }

            (pVector)[iNumMatches+1] = -1;
        }
#endif
    }

    if((bUseArray || bMultiValue) && pCleanVarT){
        pCleanVarT->VectorClean();
    }
}

int CIntDriver::DoTally( int iCtNode, double dValue, std::vector<std::shared_ptr<VTSTRUCT>>& arrVectors, int* iNumMatches ) {
    int         i, iLocalMatches=0;
    CTNODE*     pNode=(CTNODE *) ( CtNodebase + iCtNode );
    CTRANGE*    pRange;

    double  rValue = ct_ivalue( iCtNode, dValue ); // see HINT in tabfuncs.cpp

    int  ini_range = iCtNode + CTNODE_SLOTS;
    int  end_range = ini_range + CTRANGE_SLOTS * pNode->m_iCtNumRanges;
    int  relpos = 0;
    int  lastrelpos = 0;

    bool bStatNode = (pNode->m_iStatType != CTSTAT_NONE); // RHF Aug 13, 2002

    // RHF INIC Jun 09, 2003
    bool bTotal=(pNode->m_iStatType == CTSTAT_TOTAL);
    // RHF END Jun 09, 2003

    bool bFitRange = false;
    int  j = 0;
    int iSubTbl =0;
    int* iVector = NULL;
    std::shared_ptr<std::vector<int>> pVector;
    int iAssign =0;
    bool bProcessRange=true; // RHF May 09, 2006
    for( i = ini_range; i < end_range; i += CTRANGE_SLOTS ) { // Variable Mapping
        pRange = (CTRANGE *) ( CtNodebase + i );
        bFitRange = pRange->fitInside(rValue);
        if( pRange->m_iRangeCollapsed <= 1 ) bProcessRange=true; // RHF May 09, 2006
        if( bFitRange ) {
            // RHF NEW May 09, 2006
            if( bProcessRange ) {
                (*iNumMatches)++;
                iLocalMatches++;
                bProcessRange = false;
            }
            // RHF NEW May 09, 2006

            /* RHF COM May 09, 2006
                (*iNumMatches)++;
                iLocalMatches++;
             */

            // All stats fit in position zero. All the stats cells are together in the respective subtable.
            if( false && bStatNode ) { // RHF Sep 11, 2002 add false. Now ctvector is clean outside this method
                for(iSubTbl =0; iSubTbl < (int)arrVectors.size(); iSubTbl++){
                    VTSTRUCT& vtStruct = *arrVectors[iSubTbl];
                    if(!vtStruct.bUse4Processing){
                        continue;
                    }
                    if(vtStruct.bUseV1){
                        //begin
                        //if(vtStruct.arrV1s.GetCount()-1 == -1){//this happens when pcts and totals are not to gether ctpo_var is called first with out ctpos_add
                        if(vtStruct.iV1Index  == -1){//this happens when pcts and totals are not to gether ctpo_var is called first with out ctpos_add
                            iVector = vtStruct.ctvector[vtStruct.iUseDim];
                        }
                        else {
                            pVector = vtStruct.arrV1s[vtStruct.iV1Index];
                            iVector = pVector->data();
                        }
                        //end
                    }
                    else {
                        //begin
                        //if(vtStruct.arrV2s.GetCount()-1 == -1){//this happens when pcts and totals are not to gether ctpo_var is called first with out ctpos_add
                        if(vtStruct.iV2Index  == -1){
                            iVector = vtStruct.ctvector[vtStruct.iUseDim];
                        }
                        else {
                            pVector = vtStruct.arrV2s[vtStruct.iV2Index];
                            iVector = pVector->data();
                        }
                        //end
                    }
                    iVector[*iNumMatches] = 0;
                }
            }
            else {
                if( pRange->m_iRangeCollapsed == 0 ) {// No collapse
                    lastrelpos = relpos;
                    relpos += rValue - pRange->m_iRangeLow;
                }
                else if( pRange->m_iRangeCollapsed == 1 ) // Only the first collapsed range count a cell
                    relpos++;
                else // Do nothing with the rest collapsed ranges
                    ;

                ASSERT( *iNumMatches < MAX_TALLYCELLS );
                iAssign =0;
                if( pRange->m_iRangeCollapsed >= 1 ) {
                    ASSERT( relpos >= 1 );
                    iAssign = relpos-1;
                }
                else {
                    iAssign = relpos;
                }
                iVector = NULL;
                for(iSubTbl =0; iSubTbl < (int)arrVectors.size(); iSubTbl++){
                    VTSTRUCT& vtStruct = *arrVectors[iSubTbl];
                    if(!vtStruct.bUse4Processing){
                        continue;
                    }
                    if(vtStruct.bUseV1){
                        //if(vtStruct.arrV1s.GetCount()-1 == -1){//this happens when pcts and totals are not to gether ctpo_var is called first with out ctpos_add
                        if(vtStruct.iV1Index  == -1){//this happens when pcts and totals are not to gether ctpo_var is called first with out ctpos_add
                            iVector = vtStruct.ctvector[vtStruct.iUseDim];
                        }
                        else {
                            pVector = vtStruct.arrV1s[vtStruct.iV1Index];
                            iVector = pVector->data();
                        }
                    }
                    else {
                        //begin
                        //if(vtStruct.arrV2s.GetCount()-1 == -1){//this happens when pcts and totals are not to gether ctpo_var is called first with out ctpos_add
                        if(vtStruct.iV2Index == -1){//this happens when pcts and totals are not to gether ctpo_var is called first with out ctpos_add
                            iVector = vtStruct.ctvector[vtStruct.iUseDim];
                        }
                        else {
                            pVector = vtStruct.arrV2s[vtStruct.iV2Index];
                            iVector = pVector->data();
                        }
                        //end

                    }
                    iVector[*iNumMatches] = iAssign;
                     // RHF INIC Jun 09, 2003
                     if( bTotal )
                        iVector[*iNumMatches] = 0;
                    // RHF END Jun 09, 2003

                }

                // Advance the full range if no collapsed
                if( pRange->m_iRangeCollapsed == 0 ) {
                    relpos = lastrelpos;
                    relpos += pRange->m_iRangeHigh - pRange->m_iRangeLow + 1;
                }
            }
        }
        else {
            if( false && bStatNode ) { // RHF Sep 11, 2002 add false. Now ctvector is clean outside this method
                ;
            }
            else {
                if( pRange->m_iRangeCollapsed == 0 ) // No collapse
                    relpos += pRange->m_iRangeHigh - pRange->m_iRangeLow + 1;
                else if( pRange->m_iRangeCollapsed == 1 ) // Only the first collapsed range count a cell
                    relpos++;
                else // Do nothing with the rest collapsed ranges
                    ;
            }
        }
    }


    return iLocalMatches;
}
//SAVY 4 SPEED END
