#include "StandardSystemIncludes.h"
#include "Comp.h"
#include "Ctab.h"
#include "CBorder.h"
#include "Ctab_Helper.h"
#include "IntDrive.h"
#include <zToolsO/Tools.h>
#include <zLogicO/SymbolTableIterator.h>
#include <zEngineO/ValueSet.h>

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]= __FILE__;
#define new DEBUG_NEW
#endif


const int INTERNAL_COMPILER_ERROR = 99999;

//---------------------------------------------------------------------------
//  File name: Ctab.cpp
//
//  Description:
//          Header for Crosstab classes
//          * ctmaketerm is external
//                  - ctnterm & ctaddterm: internal use only
//
//
//  History:    Date       Author   Comment
//              ---------------------------
//              15 Jul 99   RHF     Basic conversion
//              17 Jul 01   RHF     Redo for multy-tally support
//              27 Jun 02   RHF     Redo for unit support
//              14 Ago 02   RHF     Add Statistics support
//---------------------------------------------------------------------------

static bool     bMetaTable = true; // use false when expresion for sub-tables are exact

CTTERM *CEngineCompFunc::ctmaketerm( int ictnode ) {
    int     ctaux[2048], ctncells[2048], nctaux, i, last, j, ntc;
    CTNODE  *pnode;
    CTTERM  *ctt;

    i = ctnterm( ictnode );
    ctt = (CTTERM *) calloc( i, sizeof( CTTERM ) );
    if( ctt == NULL ) {  // no mem for CROSSTABs structures
        if( Issamod == ModuleType::Designer ) {
            SetSyntErr(642);
            return ctt;
        }

        issaerror( MessageType::Abort, 642 );        // in EX or BEX executors
    }

    pnode = CTN(ictnode);
    nctaux = 0;
    ntc = 0;

    while( pnode->isOperNode(CTNODE_OP_ADD) ) {
        ctncells[nctaux] = pnode->getNumCells() - CTN( pnode->m_iCtRight )->getNumCells();
        ctaux[nctaux++] = pnode->m_iCtRight;
        pnode = CTN( ( ictnode = pnode->m_iCtLeft ) );
    }
    ctncells[nctaux] = 0;
    ctaux[nctaux++] = ictnode;

    for( i = 0; i < nctaux; i++ )
        ctaddterm( ctt, &ntc, 0, ctaux[nctaux-i-1], ctncells[nctaux-i-1] );
    ctaddterm( ctt, &ntc, -1, -1, -1 );

    last = ntc;

    for( i = 0; i < last; i++ ) {
        if( CTN( (ctt+i)->m_iCtNode )->isOperNode(CTNODE_OP_MUL) ){
            (ctt+i)->m_iNodeId = ntc;
            ictnode = (ctt+i)->m_iCtNode;
            pnode = CTN(ictnode);
            ctaddterm( ctt, &ntc, 0, pnode->m_iCtLeft, CTN(pnode->m_iCtLeft)->getNumCells() );
            pnode = CTN( ( ictnode = pnode->m_iCtRight ) );
            nctaux = 0;
            while( pnode->isOperNode(CTNODE_OP_ADD) ){
                ctncells[nctaux] = pnode->getNumCells() - CTN(pnode->m_iCtRight)->getNumCells();
                ctaux[nctaux++] = pnode->m_iCtRight;
                pnode = CTN( ( ictnode = pnode->m_iCtLeft ) );
            }
            ctncells[nctaux] = 0;
            ctaux[nctaux++] = ictnode;
            for( j = 0; j < nctaux; j++ )
                ctaddterm( ctt, &ntc, 0, ctaux[nctaux - j - 1], ctncells[nctaux - j - 1] );
            ctaddterm( ctt, &ntc, -1, -1, -1 );
        }
    }

    return( ctt );
}

/* static */
int CEngineCompFunc::ctnterm( int ictnode ) {
    CTNODE  *pnode, *qnode;
    int     nctt;

    pnode = CTN(ictnode);
    nctt = 0;

    while( pnode->isOperNode(CTNODE_OP_ADD) ) {
        if( CTN( pnode->m_iCtRight )->isOperNode(CTNODE_OP_MUL) ) {
            nctt++;
            qnode = CTN( pnode->m_iCtRight );
            qnode = CTN( qnode->m_iCtRight );
            while( qnode->isOperNode(CTNODE_OP_ADD) ) {
                nctt++;
                qnode = CTN( qnode->m_iCtLeft );
            }
            nctt += 2;
            //nctt += CTNODE_SLOTS - 2;
        }
        nctt++;
        pnode = CTN( pnode->m_iCtLeft );
    }

    nctt += 2;
    //nctt += CTNODE_SLOTS - 2;
    if( pnode->isOperNode(CTNODE_OP_MUL) ) {
        nctt++;
        qnode = CTN( pnode->m_iCtRight );
        while( qnode->isOperNode(CTNODE_OP_ADD) ) {
            nctt++;
            qnode = CTN( qnode->m_iCtLeft );
        }
        nctt += 2;
        //nctt += CTNODE_SLOTS - 2;
    }

    return( nctt );
}

/* static */
void CEngineCompFunc::ctaddterm( CTTERM *ctt, int *ntc, int iNodeId, int iCtNode, int iPrevCells ) {
    CTTERM  *pt;

    pt = ctt + *ntc;
    pt->m_iNodeId = iNodeId;
    pt->m_iCtNode = iCtNode;
    pt->m_iPrevCells = iPrevCells;
    *ntc += 1;
}

/*******************************************************************************
DESCRIPCION DE CTTERM: RHF 27/9/96
----------------------------------

  pctt tiene la sgte. estructura:

    cada nodeid es 0 para las sub-tablas de 1 nivel o un desplazamiento en el
    arreglo para las tablas de mas de un nivel.

      Ejemplo:

        k1 + k2 *( k3+k4+k5) + k6

          genera los sgtes. nodeid.


              i          pctt->nodeid      NPT( CTN(pcct->nodeid)->m_iNodeType )->GetName()
              ------------------------------------------------------------------------------
              0           0                k1                   1a sub-tabla
              1           4                desplazamiento de la 2a sub-tabla
              2           0                k6                   3a sub-tabla
              3          -1                                    indica fin de las submatrices
              4           0                k2
              5           0                k3
              6           0                k4
              7           0                k5
              8          -1                                    indica fin del arreglo
*******************************************************************************/

void CTAB::InitBorder( int iNumRows, int iNumCols, int iNumLayers ) {
    ASSERT( m_pBorder == NULL );
    ASSERT( GetTableType() == CTableDef::Ctab_Percent || GetTableType() == CTableDef::Ctab_GrandTotal ); // Grand Total

    m_pBorder = new CBorder;

    m_pBorder->SetDims( iNumRows, iNumCols, iNumLayers );
}

bool CTAB::AllocBorder() {
    ASSERT( m_pBorder != NULL );
    ASSERT( GetTableType() == CTableDef::Ctab_Percent || GetTableType() == CTableDef::Ctab_GrandTotal ); // Grand total

    ASSERT( m_pAcum.GetNumCells() == m_pBorder->GetNumCells() );
    ASSERT( m_pAcum.GetNumCols() == 1 );
    ASSERT( m_pAcum.GetNumLayers() == 1);

    return m_pBorder->Alloc( (double*) m_pAcum.GetAcumArea() );
}

CTAB::CSymbolCtab(std::wstring name)
    :   Symbol(std::move(name), SymbolType::Crosstab)
{
    SetTbdSlice( NULL );

    SetTableType( CTableDef::Ctab_NoType );

    for( int i = 0; i < 4; i++ )
        m_uEnviron[i] = 0;
    resetOptions();

    SetNumDim( 0 );

    for( int i = 0; i < TBD_MAXDIM; i++ ) {
        SetTotDim( 0, i );
        SetNodeExpr( -1, i );
        SetTerm( NULL, i );
    }

    SetNumDec( 0 );
    SetAcumType( sizeof( double ) );
    SetAcumArea( NULL );

    for( int i = 0; i < 2; i++ ) {
        SetDepSymVar( -1, i );// RHF Jan 10, 2002
        SetDepVarOcc( 0, i );
    }

    SetTableNumber( 0 );
    SetRunTimeVersion( 2 );

    // RHF INIT Aug 02, 2005
    if( CSettings::m_bNewTbd )
        SetRunTimeVersion( 3 );
    // RHF END Aug 02, 2005

    SetCurrentUnit( NULL );

    SetCurrentCoordNumber(0);

    RemoveAllUnits();
    RemoveAllSubTables();
    RemoveAllStats();
    RemoveAllAuxCtabs();

    // --- engine links
    m_pEngineDriver  = NULL;

    SetSelectExpr( 0 );
    SetWeightExpr( 0 );
    SetTabLogicExpr( 0 );

    SetAuxiliarTable( false );

    SetHasSomePercent( false );

    SetPreDeclared( false );// RHF Oct 31, 2002

    m_pBorder = NULL;

    SetCtabGrandTotal( NULL );

// SERPRO_CALC
    SetHasSyntaxError( false );

    m_pLinkTable = NULL;

    SetNumBreaks( 0 ); // RHF Apr 16, 2003
// SERPRO_CALC
}

CTAB::~CSymbolCtab()
{
    RemoveAllStats();
    delete m_pBorder;
    delete m_pLinkTable;
}


Logic::SymbolTable& CTAB::GetSymbolTable() const
{
    return m_engineData->symbol_table;
}


void CTAB::SetEngineDriver( CEngineDriver* pEngineDriver )
{
    ASSERT(pEngineDriver != nullptr);

    m_pEngineDriver = pEngineDriver;
    m_pEngineArea = pEngineDriver->getEngineAreaPtr();
    m_engineData = m_pEngineArea->m_engineData.get();
}


UINT CTAB::GetCellSize() {
    UINT    uCellSize;

    switch( GetTableType() ) {
    // Normal tables
    case CTableDef::Ctab_Crosstab:
    case CTableDef::Ctab_STable:
    case CTableDef::Ctab_Hotdeck:

    // Stat tables
    case CTableDef::Ctab_NotAppl:
    case CTableDef::Ctab_Freq:
    case CTableDef::Ctab_Total:
    case CTableDef::Ctab_GrandTotal:
    case CTableDef::Ctab_Percent:
    case CTableDef::Ctab_Prop:
    case CTableDef::Ctab_MinTable:
    case CTableDef::Ctab_MaxTable:
    case CTableDef::Ctab_Mode:
    case CTableDef::Ctab_StatMean:
    case CTableDef::Ctab_Median:
    case CTableDef::Ctab_Percentil:
    case CTableDef::Ctab_StdDev         :
    case CTableDef::Ctab_Variance:
    //case CTableDef::Ctab_ValidPct:
        uCellSize = GetAcumType();
        break;
    case CTableDef::Ctab_Mean:
        uCellSize = sizeof( MEANACUM );
        break;
    case CTableDef::Ctab_SMean:
        uCellSize = sizeof( SMEANACUM );
        break;
    default:
        // RHF COM Oct 23, 2001 uCellSize = 0;
        uCellSize = CBaseTable2::GetCellSize();// RHF Oct 23, 2001
        //ASSERT(0); //// RHF Aug 09, 2002
    }

    return uCellSize;
}

UINT CTAB::GetNumCells() {
    UINT    uNumCells;

    switch( GetNumDim() ) {
    case 1:
        uNumCells = GetTotDim(0);
        break;
    case 2:
        uNumCells = (long) GetTotDim(0) * GetTotDim(1);
        break;
    case 3:
        uNumCells = (long) GetTotDim(0) * GetTotDim(1) * GetTotDim(2);
        break;
    default:
        uNumCells = 0;
    }

    return uNumCells;
}

UINT CTAB::GetAcumSize() {
    UINT     uCellSize;
    UINT     uNumCells;

    if( ( uCellSize = GetCellSize() ) == 0 )
        return( (long) 0 );

    if( ( uNumCells = GetNumCells() ) == 0 )
        return( (long) 0 );

    return( uNumCells * uCellSize );
}

UINT CTAB::GetNumRows() {
    return GetTotDim(0);
}

UINT CTAB::GetNumCols() {
    return (GetTotDim(1) <= 0 ) ? 1 : GetTotDim(1);
}

UINT CTAB::GetNumLayers() {
    return (GetTotDim(2) <= 0 ) ? 1 : GetTotDim(2);
}


void CTAB::SetCtabGrandTotal( CTAB* pCtabGrandTotal ) { m_pCtabGrandTotal = pCtabGrandTotal; }
CTAB* CTAB::GetCtabGrandTotal() { return m_pCtabGrandTotal; }

CCrossTable* CTAB::MakeSubExpresions() {
    CCrossTable*    pTable=new CCrossTable;
    CArray<int,int> aNodeExpr;

    for( int iDim=0; iDim < GetNumDim(); iDim++ )
        aNodeExpr.Add( GetNodeExpr(iDim) );

    pTable->MakeSubExpresion( aNodeExpr, CtNodebase, &GetSymbolTable() );

    // See init
    pTable->SetTableLevel( GetTableLevel() );
    pTable->SetApplDeclared( IsApplDeclared() );
    pTable->SetTitleLen( GetTitleLen() );
    pTable->SetStubLen( GetStubLen() );
    pTable->SetPageWidth( GetPageWidth() );
    pTable->SetStubWidth( GetStubWidth() );
    pTable->SetPageLen( GetPageLen() );
    pTable->SetPercentDecs( GetPercentDecs() );


    return pTable;
}

// Return true if iSymbol is found in the expresions.
// Use iDimType=0 for ignore dimension
// Use iSeq=0 for ignore secuence
bool CTAB::HasItem( int* pNodeBase[TBD_MAXDIM],
                   int iDimType, int iDepthType,
                   int iSymbol, int iSeq, int iSubTableNum, int* iCtVarNode, int* iCtDim ) {

    ASSERT( iDimType == DIM_SOMEDIM || iDimType == DIM_ROW || iDimType == DIM_COL || iDimType == DIM_LAYER );
    ASSERT( iDepthType == DIM_SOMEDEPTH|| iDepthType == DIM_LEFTDEPTH || iDepthType == DIM_RIGHTDEPTH  );
    ASSERT( iSymbol != 0 );
    ASSERT( iSeq >= 0 );

    int     iMin, iMax;

    if( iSubTableNum == 0 ) {
        iMin = 0;
        iMax = GetNumSubTables();
    }
    else {
        iMin = iSubTableNum-1;
        iMax = std::min(iMin+1,GetNumSubTables());
    }


    if( iCtVarNode != NULL )
        *iCtVarNode = -1;

    if( iCtDim != NULL )
        *iCtDim = -1;

    for( int iSubTable = iMin; iSubTable < iMax; iSubTable++ ) {
        CSubTable& cSubTable=GetSubTable(iSubTable);

        for( int iDim=0; iDim < TBD_MAXDIM; iDim++ ) {
            if( iDimType == DIM_SOMEDIM || iDimType == iDim + 1 ) {
                for( int iDepth=0; iDepth < TBD_MAXDEPTH; iDepth++ ) {
                    if( iDepthType == DIM_SOMEDEPTH || iDepthType == iDepth + 1 ) {
                        if( cSubTable.m_iCtTree[iDim][iDepth] >= 0 ) {
                            CTNODE*     pCtNode=(CTNODE *) (pNodeBase[iDim] + cSubTable.m_iCtTree[iDim][iDepth] );

                            if( ( pCtNode->hasThisVariable(iSymbol) || iSymbol == -1 ) &&
                                ( iSeq == 0 || pCtNode->m_iSeqNumber == iSeq ) ) {
                                if( iCtVarNode != NULL ) {
                                    *iCtVarNode = cSubTable.m_iCtTree[iDim][iDepth];
                                    *iCtDim = iDim;
                                }
                                return true;
                            }
                        }
                        else if( iSymbol == -1 && iSeq == 0 ) {
                            if( iCtVarNode != NULL ) {
                                *iCtVarNode = -1;
                                *iCtDim = iDim;
                            }
                            return true;
                        }
                    }
                }
            }
        }
    }

    return false;
}


// Return the number of sub-tables found (0 to n). If iDim/iSeq1/iSeq2=0 then the dimension or sequence has not been specified.
// Return -1 if iDim not found, -2 iVar1 not found, -3 iSeq1 not found, -4 if iVar2 not found,
// -5 if iSeq2 not found
// Fill cSubTable when return 1.
int CTAB::SearchSubTables( int* pNodeBase[TBD_MAXDIM], int iDimType[TBD_MAXDIM],
                           int iVar[TBD_MAXDIM][TBD_MAXDEPTH], int iSeq[TBD_MAXDIM][TBD_MAXDEPTH],
                           std::vector<int>& aSubTables, int iTheDimType )
{
    int     iNumSubTables=0;
    int     iMaxDim= ( iTheDimType < 0 ) ? TBD_MAXDIM : 1;


    for( int iDim=0; iDim < iMaxDim; iDim++ )
        ASSERT( iDimType[iDim] != DIM_SOMEDIM );

    // Structural checking
    for( int iDim = 0; iDim < iMaxDim && iDimType[iDim] >= 0 && iNumSubTables == 0; iDim++ ) {
        if( iDimType[iDim] > 0 && iDimType[iDim] > GetNumDim() )
            iNumSubTables = -1;
        else{
            bool bHasSomeLeft  = HasItem( pNodeBase, iDimType[iDim], DIM_LEFTDEPTH, iVar[iDim][0], 0, 0 );
            bool bHasSomeRight = HasItem( pNodeBase, iDimType[iDim], DIM_RIGHTDEPTH, iVar[iDim][1], 0, 0 );
            bool bHasLeft      = HasItem( pNodeBase, iDimType[iDim], DIM_LEFTDEPTH, iVar[iDim][0], iSeq[iDim][0], 0 );
            bool bHasRight     = HasItem( pNodeBase, iDimType[iDim], DIM_RIGHTDEPTH, iVar[iDim][1], iSeq[iDim][1], 0 );

            if( bMetaTable ) {
                bHasLeft = bHasSomeLeft;
                bHasRight = bHasSomeRight;
            }

            if( !bHasSomeLeft )
                iNumSubTables = -2;
            else if( !bHasSomeRight )
                iNumSubTables = -4;
            else if( !bHasLeft )
                iNumSubTables = -3;
            else if( !bHasRight )
                iNumSubTables = -5;

        }
    }

    if( iNumSubTables != 0 )
        return iNumSubTables;


    for( int iSubTable = 0; iSubTable < GetNumSubTables(); iSubTable++ ) {
        CSubTable& cSubTable=GetSubTable(iSubTable);

        //Check symbols against dimensions
        bool    bFit[TBD_MAXDIM];
        bool    bFitSubTable;

        for( int iDim=0; iDim < TBD_MAXDIM; iDim++ )
            bFit[iDim] = true;

        for( int iDim=0; iDim < iMaxDim; iDim++ ) {
            bFit[iDim] = false;

            if( iDimType[iDim] == DIM_NODIM ) {
                bFit[iDim] = true;
                continue;
            }

            bool    bHas[TBD_MAXDEPTH];

            bHas[0] = HasItem( pNodeBase, iDimType[iDim], DIM_LEFTDEPTH,  iVar[iDim][0], iSeq[iDim][0], iSubTable+1 );
            bHas[1] = HasItem( pNodeBase, iDimType[iDim], DIM_RIGHTDEPTH, iVar[iDim][1], iSeq[iDim][1], iSubTable+1 );

            // Don't fit on Left and only has one variable --> Check if fits in right the first variable
            bool bFitInRight = false;
            if( bMetaTable && !bHas[0] && iVar[iDim][1] == -1 )
                bFitInRight = HasItem( pNodeBase, iDimType[iDim], DIM_RIGHTDEPTH,  iVar[iDim][0], iSeq[iDim][0], iSubTable+1 );

            if( bHas[0] && bHas[1] || bFitInRight )
                bFit[iDim] = true;
        }

        bFitSubTable = (bFit[0] && bFit[1] && bFit[2]);


        if( bFitSubTable ) {
            iNumSubTables++;
            aSubTables.emplace_back( iSubTable );
        }
    }

    return iNumSubTables;
}

void CTAB::MakeSubTables( int* pNodeBase[TBD_MAXDIM], int iRoot[TBD_MAXDIM] )
{
    std::vector<Range<int>> aVector[TBD_MAXDIM];

    m_aSubTables.RemoveAll();

    int iDim;
    for( iDim=0; iDim < GetNumDim(); iDim++ )
        MakeSubTableDim( pNodeBase[iDim], iRoot[iDim], aVector[iDim] );

    // Add an empty range when dimension doesnt't exist
    for( ;iDim < TBD_MAXDIM; iDim++ ) {
        aVector[iDim].emplace_back(-1, -1);
    }

    CSubTable cSubTable;
    cSubTable.SetSymCtab( GetSymbolIndex() );

    for( size_t i = 0; i < aVector[0].size(); i++ ) {
        const Range<int>& cRange0 = aVector[0][i];
        cSubTable.m_iCtTree[0][0] = cRange0.low;
        cSubTable.m_iCtTree[0][1] = cRange0.high;

        for( size_t j = 0; j < aVector[1].size(); j++ ) {
            const Range<int>& cRange1 = aVector[1][j];
            cSubTable.m_iCtTree[1][0] = cRange1.low;
            cSubTable.m_iCtTree[1][1] = cRange1.high;

            for( size_t k = 0; k < aVector[2].size(); k++ ) {
                const Range<int>& cRange2 = aVector[2][k];
                cSubTable.m_iCtTree[2][0] = cRange2.low;
                cSubTable.m_iCtTree[2][1] = cRange2.high;

                cSubTable.GenName( m_pEngineArea, pNodeBase );

                cSubTable.MakeItemSymbols( m_pEngineArea, pNodeBase );

                // Assign Subtable type
                int             iStatDim=DIM_NODIM, iStatDepth=-1;
                CStatType       cStatType=CTSTAT_NONE;
                CtStatBase*     pStatBase=NULL;
                int             iStatVar=0;
                bool            bSomeCoordZero=false;// RHF Oct 08, 2002
                for( int iDim=0; iDim < GetNumDim(); iDim++ ) {
                    for( int iDepth=0; iDepth < TBD_MAXDEPTH; iDepth++ ) {
                        int         iCtNode=cSubTable.m_iCtTree[iDim][iDepth];
                        CTNODE*     pCtNode=(iCtNode >= 0 ) ? (CTNODE *) (pNodeBase[iDim] + iCtNode ) : NULL;

                        if( pCtNode != NULL && pCtNode->getNumCells() == 0 )// RHF Oct 08, 2002
                            bSomeCoordZero = true;// RHF Oct 08, 2002

                        if( pCtNode != NULL && pCtNode->m_iStatType != CTSTAT_NONE ) {
                            if( cStatType == CTSTAT_NONE ) {
                                cStatType = (CStatType) pCtNode->m_iStatType;
                                ASSERT( pCtNode->isVarNode() );
                                iStatVar = pCtNode->m_iSymbol;
                                pStatBase = pCtNode->m_pStatBase;
                            }

                            // RHF INIC Jun 09, 2003. Freq Intersection
                            else if( pCtNode->m_iStatType == CTSTAT_FREQ || pCtNode->m_iStatType == CTSTAT_TOTAL ) { // RHF Jun 11, 2003 Add pCtNode->m_iStatType == CTSTAT_TOTAL
                                continue;
                            }
                            /* RHF COM INIC Jun 11, 2003
                            else if( pCtNode->m_iStatType == CTSTAT_TOTAL && cStatType == CTSTAT_TOTAL ) {
                                continue;
                            }
                           RHF COM END Jun 11, 2003 */
                            else if( cStatType == CTSTAT_FREQ || cStatType == CTSTAT_TOTAL )  { // RHF Jun 11, 2003 Add cStatType == CTSTAT_TOTAL
                                cStatType = (CStatType) pCtNode->m_iStatType;
                                ASSERT( pCtNode->isVarNode() );
                                iStatVar = pCtNode->m_iSymbol;
                                pStatBase = pCtNode->m_pStatBase;
                            }
                            // RHF END Jun 09, 2003
                            else {
                                cStatType = CTSTAT_OVERLAPPED;
                                // RHF COM Jul 13, 2005 iStatVar = 0;
                                // RHF COM Jul 13, 2005 pStatBase = NULL;
                                iStatVar = pCtNode->m_iSymbol; // RHF Jul 13, 2005
                                pStatBase = pCtNode->m_pStatBase; // RHF Jul 13, 2005
                            }

                            iStatDim = iDim+1;
                            iStatDepth = iDepth;
                        }
                    }
                }

                // RHF INIC Oct 08, 2002
                if( bSomeCoordZero )
                    cStatType = CTSTAT_NONE;
                // RHF END Oct 08, 2002

                cSubTable.SetStatType( cStatType );
//                cSubTable.SetSymStatVar( iStatVar );
                cSubTable.SetSymStatDim( iStatDim, iStatDepth );
                cSubTable.SetStatBase( pStatBase );

                // Add to array
                m_aSubTables.Add( cSubTable );
            }
        }
    }



#ifdef _DEBUG
    DumpSubTables( pNodeBase );
#endif
}

#ifdef _DEBUG
void CTAB::DumpSubTables( int* pNodeBase[TBD_MAXDIM] ) {
    CString     csMsg;
    CString     csLongName;

    for( int iSubTable=0; iSubTable < m_aSubTables.GetSize(); iSubTable++ ) {
        CSubTable&  cSubTable=m_aSubTables.ElementAt(iSubTable);
        csLongName = _T(""); // RHF Jun 30, 2004
        cSubTable.GenName( m_pEngineArea, pNodeBase, &csLongName );

        csMsg.Format( _T("%3d) %s"), iSubTable+1, csLongName.GetString() );

        TRACE( csMsg );
    }
}
#endif

void CTAB::MakeSubTableDim( int* pNodeBase, int iCtNode, std::vector<Range<int>>& aVector ) {
    ASSERT( iCtNode >= 0 );
    std::vector<Range<int>> aVector1;
    std::vector<Range<int>> aVector2;
    CTNODE*     pCtNode=(CTNODE *) (pNodeBase + iCtNode );

    if( pCtNode->isVarNode() ) // Variable or value-set
    {
        aVector.emplace_back(iCtNode, -1);
    }
    else
    {
        switch( pCtNode->getOperator() )
        {
        case CTNODE_OP_ADD:
            {
                MakeSubTableDim( pNodeBase, pCtNode->m_iCtLeft, aVector1 );
                MakeSubTableDim( pNodeBase, pCtNode->m_iCtRight, aVector2 );

                for( size_t i=0; i < aVector1.size(); i++ )
                    aVector.emplace_back( aVector1[i] );
                for( size_t j=0; j < aVector2.size(); j++ )
                    aVector.emplace_back( aVector2[j] );
            }
            break;

        case CTNODE_OP_MUL:
            {
                MakeSubTableDim( pNodeBase, pCtNode->m_iCtLeft, aVector1 );
                MakeSubTableDim( pNodeBase, pCtNode->m_iCtRight, aVector2 );

                for( size_t i=0; i < aVector1.size(); i++ ) {
                    const Range<int>& rRange=aVector1[i];
                    for( size_t j=0; j < aVector2.size(); j++ ) {
                        const Range<int>& lRange=aVector2[j];
                        aVector.emplace_back(rRange.low, lRange.low);
                    }
                }
            }
            break;

        default:
            break;
        }
    }
}

// Return > 0 if all variables fit in some relation
int CTAB::SearchRelation( CSubTable::CStMultVarSpecArray& aMultItemSymbols )
{
    return GetSymbolTable().ForeachSymbol<RELT, true>(
        [&](RELT& relation)
        {
            bool bFitAllVarsInRelation=true;
            bool bSomeVarInSource=false;

            for( int iVar=0; iVar < aMultItemSymbols.GetSize() && bFitAllVarsInRelation; iVar++ )
            {
                int     iSymVar=aMultItemSymbols.GetAt(iVar).m_iVarSymbol;
                VART    *pVarT=VPT(iSymVar);
                RELATED *pRelated, aRelated;

                bool    bFitVarInRelation=true;
                for( int iDim = 0; iDim < pVarT->GetNumDim() && bFitVarInRelation; iDim++ ) {
                    pRelated = relation.GetRelated( &aRelated, iSymVar, pVarT->GetDimType( iDim ) );
                    if( pRelated == NULL ) { // variable not reached by relation
                        bFitVarInRelation = false;
                    }
                }

                if( !bFitVarInRelation ) {
                    bFitAllVarsInRelation = false;
                }
                else { // Check some variable belong to origin
                    ASSERT( relation.GetBaseObjType() == SymbolType::Section || relation.GetBaseObjType() == SymbolType::Variable );
                    SECT*   pSecT;

                    if( relation.GetBaseObjType() == SymbolType::Section ) {
                        int     iSec = relation.GetBaseObjIndex();
                        pSecT = SPT(iSec);

                        if( pVarT->GetSPT() == pSecT ) // Belong to the source section
                            bSomeVarInSource = true;
                    }
                    else { // SymbolType::Variable
                        int     iVarTSource=relation.GetBaseObjIndex();

                        if( iSymVar == iVarTSource )
                            bSomeVarInSource = true;
                        else {
                            int     iSymVarParent  = pVarT->GetOwnerSymItem();
                            bool    bIsSubItem=( pVarT->GetOwnerSymItem() > 0 );

                            if( bIsSubItem && iSymVarParent == iVarTSource )
                                bSomeVarInSource = true;
                        }
                    }
                }
            }

            if( bFitAllVarsInRelation && bSomeVarInSource ) // found the relation
                return false;

            return true; // keep on searching
        });
}

//////////////////////////////////////////////////////////////////////////

int CTAB::getOwnerGrpIdxForVarOrRecord( int iVar )
{
    int iGroup = MAGIC_NUMBER;
    if( NPT(iVar)->IsA(SymbolType::Variable) )
    {
        GROUPT* pGroupT = LocalGetParent(VPT(iVar));
        iGroup = pGroupT->GetSymbol();
    }
    else
    {
        ASSERT( NPT(iVar)->IsA(SymbolType::Group) );
        iGroup = iVar;
        TRACE( _T("Dynamic completion -> %d %s"), iGroup, GPT(iVar)->GetName().c_str() );
    }

    ASSERT( iGroup != MAGIC_NUMBER );
    return iGroup;
}

#define MTYPE(i) pMVarNode->m_iVarSubindexType[i]
#define MVAL(i)  pMVarNode->m_iVarSubindexExpr[i]
#define MTYPE2(i) pNode2->m_iVarSubindexType[i]
#define MVAL2(i)  pNode2->m_iVarSubindexExpr[i]

void CTAB::CompleteDimensionsUsingThisUnit( MVAR_NODE* pMVarNode, int iNumDim, int iUnitSymbol )
{
    ASSERT( pMVarNode != 0 );
    ASSERT( iNumDim <= 2 );  // Only implemented for 2 dims
    if( iNumDim > 2 )
        return;

    VART* pVarT = VPT(pMVarNode->m_iVarIndex);
    ASSERT( pVarT->GetNumDim() == iNumDim );

    if( hasAnyDynamicDim( pMVarNode, iNumDim ) )
    {
        int iGroup = getOwnerGrpIdxForVarOrRecord(iUnitSymbol);
        CDimension::VDimType vType = GPT(iGroup)->GetDimType();

        switch( iNumDim )
        {
        case 1:
            MTYPE(0) = MVAR_GROUP;
            ASSERT( MVAL(0) == iGroup );
            MVAL(0) = iGroup;
            break;
        case 2:
            {
                // Special case:
                if( MTYPE(0) == MVAR_CONSTANT &&
                    MTYPE(1) == MVAR_USE_DYNAMIC_CALC )
                {
                    // Move constant to the second dimension
                    MTYPE(1) = MVAR_CONSTANT;
                    MVAL(1)  = MVAL(0);
                    MTYPE(0) = MVAR_GROUP;
                    MVAL(0)  = iGroup; // this value will be overwritten after
                                       // calling this method
                    break;
                }

                int iFixIndex = 0;
                if( vType == pVarT->GetDimType(1) )
                    iFixIndex = 1;

                int iOtherIndex = 1 - iFixIndex;

                if( MTYPE(iFixIndex) != MVAR_CONSTANT )
                {
                    MTYPE(iFixIndex) = MVAR_GROUP;
                    MVAL(iFixIndex) = iGroup;

                    if( MTYPE(iOtherIndex) == MVAR_USE_DYNAMIC_CALC )
                        MTYPE(iOtherIndex) = MVAR_GROUP;
                }
                else
                {
                    // given that one of the indexes must be
                    // dynamic and the one we are trying is used
                    // by a constant, we prefer the constant, but
                    // before leaving, lets change the "dynamic" flag
                    // to a MVAR_GROUP type
                    MTYPE(iOtherIndex) = MVAR_GROUP;
                }
            } // case 2 dims
            break;
        default:
            ASSERT(0);
            break;
        } // switch
    }

    ASSERT( MTYPE(0) != MVAR_USE_DYNAMIC_CALC );
    ASSERT( MTYPE(1) != MVAR_USE_DYNAMIC_CALC );
}

// 1 dimension case [cspro v2.5]
int CTAB::SolveSymbolFor1Dim( VART* pVarT )
{
    ASSERT( pVarT != 0 );
    int iSymbol = MAGIC_NUMBER;
    SECT* pSecT = pVarT->GetSPT();

    // Multi Record
    if( pSecT->GetMaxOccs() >= 2 )
    {
        ASSERT( pVarT->GetDimType(0) == CDimension::Record );
        iSymbol = pSecT->GetSymbolIndex();
    }
    else // Multi var
        iSymbol = pVarT->GetSymbolIndex();

    ASSERT( iSymbol != MAGIC_NUMBER );
    return iSymbol;
}

// Sometimes we need to get MVAR_NODE data, but it is not always available
// [GENCODE compilation only]
// so we define a method to obtain the data, independently of compilation mode
// Currently it is being used by SolveSymbolFor2Dims()
MVAR_NODE* CTAB::getMVarNode( int iMVarNodeIndex )
{
#ifdef GENCODE
    return reinterpret_cast<MVAR_NODE*>(m_pEngineArea->GetLogicByteCode().GetCodeAtPosition(iMVarNodeIndex));
#else
    // When GENCODE is not available we do not have Progbase, but
    // we have a replacement using getLocalMVarNodePtr()
    return m_pEngineDriver->m_pEngineCompFunc->getLocalMVarNodePtr(iMVarNodeIndex);
#endif
}

// 2 dimension case [cspro 3.0]
int CTAB::SolveSymbolFor2Dims( CTNODE* pNode, VART* pVarT, int* piExtraInfo )
{
    int iSymbol = 0;

    ASSERT( pNode != 0 );
    MVAR_NODE* pMVarNode = 0;
    int iCtOcc = pNode->m_iCtOcc;
    if( iCtOcc != 0 )
    {
        if( iCtOcc < 0 )
            iCtOcc = -iCtOcc;

        // Fix because in compilation phase, iCtOcc was incremented by 1
        iCtOcc--;

        pMVarNode = getMVarNode(iCtOcc);

        if( pMVarNode == 0 )
        {
            // Error generation! Cannot be null.
            // something wrong happen in the MVAR_NODE info kept in
            // CTNODE, something is corrupt there
            ASSERT(0);
            return iSymbol = -1;
        }

        // 2 dimensions:
        // Both indexes consts -> use 1st dim [record/item]
        // only the second is constant -> use 1st dim [record/item]
        // dynamic -> use 1st dim [record/item] then fix dimension
        // only 1st is constant -> use 2nd dim
        //

        if( MTYPE(0) == MVAR_CONSTANT && MTYPE(1) == MVAR_GROUP )
        {
            iSymbol = getSymbolToUseAsUnitForSecondDim( pVarT );

            if( piExtraInfo != 0 )
            {
                VART* pVarT = VPT(iSymbol);
                ASSERT( pVarT->GetNumDim() == 2 );
                if( pVarT->GetNumDim() == 2 )
                {
                    int iMvarNode = m_pEngineDriver->m_pEngineCompFunc->getNewMVarNodeIndex();
                    *piExtraInfo = iMvarNode;
                    MVAR_NODE* pNode2 = m_pEngineDriver->m_pEngineCompFunc->getLocalMVarNodePtr( iMvarNode );
                    pNode2->m_iVarIndex = iSymbol;
                    MVAL2(0)  = MVAL(0);
                    MTYPE2(0) = MVAR_CONSTANT;

                    // Complete the other dimensions
                    MVAL2(1)  = MVAL2(2) = 1;
                    MTYPE2(1) = MTYPE2(2) = MVAR_CONSTANT;
                }
            }
        }
        else
        {
            iSymbol = getSymbolToUseAsUnitForFirstDim( pVarT );
            // Special case:
            // Help interpreter make the right decision
            // by moving constant to last position, because
            // unit will iterate through the first dimension.
            if( MTYPE(1) == MVAR_USE_DYNAMIC_CALC && MTYPE(0) == MVAR_CONSTANT )
            {
                MTYPE(1) = MVAR_CONSTANT;
                MVAL(1)  = MVAL(0);
                MTYPE(0) = MVAR_GROUP;
                MVAL(0)  = iSymbol;
            }
            // Will not complete in compilation time now  // rcl, November 2004
            // CompleteDimensionsUsingThisUnit( pMVarNode, 2, iSymbol ); // 2 because this case is for 2 dim variables
            // ASSERT( MTYPE(0) != MVAR_USE_DYNAMIC_CALC );
            //ASSERT( MTYPE(1) != MVAR_USE_DYNAMIC_CALC );
        }
    }

    return iSymbol;
}

int CTAB::SolveSymbolFor3Dims( CTNODE* pNode, VART* pVarT )
{
    int iSymbol = 0;


    return iSymbol;
}

//////////////////////////////////////////////////////////////////////////

// Generates the default unit for each sub-table that has not been specified in the UNIT commands
int CTAB::AddDefaultUnits( int* pNodeBase[TBD_MAXDIM]) {
    int         iSyntErr=0;

    for( int iSubTable=0; iSubTable < GetNumSubTables() && iSyntErr == 0; iSubTable++ ) {
        CSubTable& cSubTable = GetSubTable(iSubTable);

        if( cSubTable.IsUsed() )
            continue;
        cSubTable.SetUsed( true );

        // Rules for default UNIT
        // 1) If all items are single or use an explicit sub-index--> add to the single unit.
        // 2) else if (1 multiple item)
        //   2.1) if record multiple       --> use default RECORD relation
        //   2.2) else                     --> use default ITEM   relation
        //   //2.2) else if item (is mult) |sub-item multiple         --> use default ITEM relation
        //   //2.3) else if sub-item (ASSERT single & item multiple)  --> use default ITEM relation (symbol parent sub-item)
        // 3) else if (2 or more multiples. In theory til 6 but layer doesn't have '*' so only 5).
        //   3.1) if all items multiples belong to the same record  --> use default RECORD relation
        //   3.2) else search first relation that fit with all repetitive items (try with all permutations).
        //        3.21) Relation found    --> use it!
        //        3.22) No relation found --> ERROR

        CSubTable::CStMultVarSpecArray aMultItemSymbols;
        bool  bSingleUnitFound = false;
        int   iRelSymbol = -1;
        int   iSymForVar = -1;
        int   iExtraInfo = -1;

        cSubTable.GetMultItemSymbols( m_pEngineArea, pNodeBase, aMultItemSymbols );

        if( aMultItemSymbols.GetSize() == 0 ) {
            for( int iUnit=0; iUnit < GetNumUnits(); iUnit++ ) {
                CtUnit&   cSingleUnit=GetUnit(iUnit);

                if( cSingleUnit.GetUnitSymbol() == -1 ) { // Single Unit
                    cSingleUnit.AddSubTableIndex( iSubTable );
                    cSingleUnit.MakeCoordNumberMap( this, pNodeBase );
                    bSingleUnitFound = true;
                }
            }

            iRelSymbol = -1; // No unit type
            iSymForVar = -1;
        }
        else if( aMultItemSymbols.GetSize() == 1 ) {
            CSubTable::CStMultVarSpec varSpec = aMultItemSymbols[0];
            int     iMultSymbol = varSpec.m_iVarSymbol;
            VART*   pVarT = VPT(iMultSymbol);
            CTNODE* pNode = 0;

            int iSymbol = 0;

            // Analyzing number of dimensions now:
            switch( pVarT->GetNumDim() )
            {
            case 1: iSymbol = SolveSymbolFor1Dim( pVarT ); break;
            case 2: pNode = (CTNODE *) ( pNodeBase[varSpec.m_iDim] + varSpec.m_iOffset );
                    iSymbol = SolveSymbolFor2Dims( pNode, pVarT, &iExtraInfo );
                break;
            case 3: pNode = (CTNODE *) ( pNodeBase[varSpec.m_iDim] + varSpec.m_iOffset );
                    iSymbol = SolveSymbolFor3Dims( pNode, pVarT );
                break;
            }

            if( iSymbol >= 0 )
            iRelSymbol = m_pEngineDriver->m_pEngineCompFunc->GetRelationSymbol( iSymbol );
            else
            {
                ASSERT(0); // got an ASSERT? see below
                // the reason is inside SolveSymbolFor2Dims()
                // when trying to get the MVAR_NODE*, we got a NULL pointer
                bSingleUnitFound = true; // to prevent execution of the code below
                iSyntErr = INTERNAL_COMPILER_ERROR;
            }
        }
        else {
            ASSERT( aMultItemSymbols.GetSize() >= 2 && aMultItemSymbols.GetSize() <= 5 );

            SECT*   pSecT=NULL;
            bool    bBelongToSameRecord=true;

            // Check if all multiple items belong to the same record
            for( int i=0; i < aMultItemSymbols.GetSize() && bBelongToSameRecord; i++ ) {
                int     iMultSymbol = aMultItemSymbols.GetAt(i).m_iVarSymbol;
                const VART* pVarT;

                if( NPT(iMultSymbol)->IsA(SymbolType::Variable) )
                    pVarT = VPT(iMultSymbol);
                else {
                    ASSERT( NPT(iMultSymbol)->GetType() == SymbolType::ValueSet );
                    const ValueSet* pValueSetVar = &GetSymbolValueSet(iMultSymbol);
                    ASSERT(!pValueSetVar->IsDynamic());
                    pVarT = pValueSetVar->GetVarT();
                }

                SECT* pNewSecT = pVarT->GetSPT();

                if( pSecT == NULL )
                    pSecT = pNewSecT;
                else if( pSecT != pNewSecT )
                    bBelongToSameRecord = false;
            }

            if( bBelongToSameRecord ) {
                ASSERT( pSecT != 0 );
                iRelSymbol = m_pEngineDriver->m_pEngineCompFunc->GetRelationSymbol( pSecT->GetSymbolIndex() );
            }
            else {
                // Search relation
                iRelSymbol = SearchRelation( aMultItemSymbols );

                if( iRelSymbol <= 0 ) {
                    if( Issamod == ModuleType::Designer )
                        iSyntErr = 8420;
                    else
                        issaerror( MessageType::Abort, 8421, cSubTable.GetName().GetString() );
                    continue;
                }
            }
        }

        if( !bSingleUnitFound ) {
            // RHF INIT Sep 08, 2004
            // Search similar unit first
            bool  bFoundSimilarUnit = false;
            for( int iUnit=0; !bFoundSimilarUnit && iUnit < GetNumUnits(); iUnit++ ) {
                CtUnit&   cSingleUnit=GetUnit(iUnit);

                if( cSingleUnit.GetOwnerCtabSymbol() == GetSymbolIndex() &&
                    cSingleUnit.GetUnitSymbol() == iRelSymbol &&
                    //cSingleUnit.GetLoopSymbol() == iSymForVar &&
                    // RHF INIT Jul 04, 2005
                    cSingleUnit.GetSelectExpr() == 0 &&
                    cSingleUnit.GetWeightExpr() == 0 &&
                    // RHF END Jul 04, 2005
                    cSingleUnit.InGlobal() == IsApplDeclared()
                    ) {
                        cSingleUnit.AddSubTableIndex( iSubTable );
                        cSingleUnit.MakeCoordNumberMap( this, pNodeBase ); // RHF Sep 10, 2004
                        bFoundSimilarUnit = true;
                    }
            }
            if( bFoundSimilarUnit )
                continue;
            iSymForVar = m_pEngineDriver->m_pEngineCompFunc->MakeRelationWorkVar();
            // RHF END Sep 08, 2004

            CtUnit      ctUnit;
            int         iUnitNumber=GetNumUnits();

            ctUnit.SetNumber( iUnitNumber );
            ctUnit.SetOwnerCtabSymbol( GetSymbolIndex() );
            ctUnit.SetUnitSymbol( iRelSymbol, (iRelSymbol > 0) ? (int)NPT(iRelSymbol)->GetType() : -1 ); //
            ctUnit.SetLoopSymbol( iSymForVar ); //
            ctUnit.SetCurrentIndex(0);
            ctUnit.SetSelectExpr( 0 );
            ctUnit.SetWeightExpr( 0 );
            ctUnit.SetTabLogicExpr( 0 );

            ctUnit.AddSubTableIndex( iSubTable );

            ctUnit.MakeCoordNumberMap( this, pNodeBase );

            ctUnit.SetInGlobal( IsApplDeclared() );

            ctUnit.SetDefaultUnit( true );

            if( iExtraInfo != -1 )
                ctUnit.setExtraInfo( iExtraInfo );

            AddUnit( ctUnit );
        }
    }

    return iSyntErr;
}

// Get all nodes containing iVar/iSeq.
int CTAB::GetStatVar( CArray<CtStatVar,CtStatVar>& aStatVar, int* pNodeBase[TBD_MAXDIM],
                         int iVar, int iSeq, std::vector<int>* pSubTables ) {
    int         iCtNode, iCtDim;
    CtStatVar   ctStatVar;

    int         iNumSubTables=(pSubTables==NULL) ? GetNumSubTables() : (int)pSubTables->size();

    int         iNumMatches=0;
    for( int i=0; i < iNumSubTables; i++ ) {
        int     iSubTable;

        if( pSubTables == NULL )
            iSubTable = i+1;
        else
            iSubTable = pSubTables->at(i)+1;

        bool    bHasItem  = HasItem( pNodeBase, DIM_SOMEDIM, DIM_SOMEDEPTH,
                                     iVar, iSeq, iSubTable, &iCtNode, &iCtDim );

        if( bHasItem && iCtNode >= 0 ) {
            ASSERT( iCtDim >= 0 && iCtDim < TBD_MAXDIM );

            CTNODE*     pCtNode=(CTNODE *) (pNodeBase[iCtDim] + iCtNode );
            if( iSeq > 0 )
                ASSERT( iSeq == pCtNode->m_iSeqNumber );

            ctStatVar.SetSubtableNumber( iSubTable-1 );
            ctStatVar.SetCoordNumber( pCtNode->m_iCoordNumber );
            ASSERT( pCtNode->isVarNode() );
            ctStatVar.SetSymVar( pCtNode->m_iSymbol );

            if( HasOverlappedCat( pNodeBase[iCtDim], iCtNode ) )
                ctStatVar.SetHasOverlappedCat( true );
            else
                ctStatVar.SetHasOverlappedCat( false );

            aStatVar.Add( ctStatVar );
            iNumMatches++;
        }
    }

    return iNumMatches;
}


bool CTAB::UseCoordNumber( LIST_NODE* pListNode, int iCoordNumber ) {
    int     iNumElems;
    ASSERT( iCoordNumber >= 1 );

    if( pListNode == NULL || (iNumElems=pListNode->iNumElems) <= 0 )
        return true;

    for( int i=0; i < iNumElems; i++ ) {
        int     iSubTable=pListNode->iSym[i];
        CSubTable& cSubTable=GetSubTable(iSubTable);

        if( cSubTable.UseCoordNumber( iCoordNumber ) )
            return true;
    }

    return false;
}

// RHF INIC Jul 30, 2002

int CTAB::GetStatDimSize( int iCtNode, bool bAuxCtab ) {
    CTNODE*         pCtNode=(CTNODE*) (CtNodebase+iCtNode);
    int             iTotDim=0;
    ASSERT( pCtNode->isVarNode() );
    int             iSymVar=pCtNode->m_iSymbol;
    //VART*         pVarT = m_pEngineDriver->m_pIntDriver->GetVarT( NPT(iSymVar) ); // Value-Set or Var.
    //int           iVarLen=pVarT->GetLength();
    CStatType       eStatType=(CStatType)pCtNode->m_iStatType;
    CtStatBase*     pStatBase= pCtNode->m_pStatBase;

    ASSERT( eStatType == pStatBase->GetStatType() );

    //ASSERT( iVarLen >= 1 && iVarLen <= 4 );

    if( eStatType == CTSTAT_FREQ )
        //iTotDim = pCtNode->getNumCells();
        iTotDim = CtCells( iCtNode );
    else if( eStatType == CTSTAT_TOTAL )
        iTotDim = CtCells( iCtNode ); // RHF Jun 11, 2003
        // RHF COM Jun 11, 2003 iTotDim = 1; // N
    else if( eStatType == CTSTAT_PERCENT )
        if( bAuxCtab ) {
            iTotDim = 1; // Only for simetry. Percent doesn't have auxiliar tables.
        }
        else {
            iTotDim = CtCells( iCtNode );
        }
    else if( eStatType == CTSTAT_PROP ) {
        if( bAuxCtab )
            iTotDim = 2; // Matches & N
        else {
            iTotDim = 1;
            CtStatProp*  pStatProp=(CtStatProp*) pStatBase;

            int     iPropType=pStatProp->GetPropType();

            if( iPropType & CTSTAT_PROP_TOTAL )
                iTotDim++;
        }
    }
    else if( eStatType == CTSTAT_MIN )
        iTotDim = 1;  // Min
    else if( eStatType == CTSTAT_MAX )
        iTotDim = 1; // Max
    else if( eStatType == CTSTAT_MODE ) {
        if( bAuxCtab )
            //iTotDim = (double) pow( 10, iVarLen ); // Freq
            iTotDim = CtCells( iCtNode );
        else
            iTotDim = 1;
    }
    else if( eStatType == CTSTAT_MEAN )  {
        if( bAuxCtab )
            iTotDim = 2; // Sum & N
        else
            iTotDim = 1;
    }
    else if( eStatType == CTSTAT_MEDIAN )  {
        if( bAuxCtab )
            //iTotDim = (double) pow( 10, iVarLen ); // Freq
            iTotDim = CtCells( iCtNode );
        else
            iTotDim = 1;
    }
    else if( eStatType == CTSTAT_PTILE ) {
        if( bAuxCtab )
            //iTotDim = (double) pow( 10, iVarLen ); // Freq
            iTotDim = CtCells( iCtNode );
        else {
            CtStatPTile*  pStatPTile=(CtStatPTile*) pStatBase;

            if( pStatPTile->GetNumTiles() >= 2 && pStatPTile->GetNumTiles() <= 10 )
                iTotDim = pStatPTile->GetNumTiles() - 1;
            else
                ASSERT(0);
        }
    }
    else if( eStatType == CTSTAT_STDEV || eStatType == CTSTAT_VARIANCE || eStatType == CTSTAT_STDERR ) {
        if( bAuxCtab )
            iTotDim = 3; // Sum & Sum2 & N
        else
            iTotDim = 1;
    }
    else if( eStatType == CTSTAT_VPCT ) {
        if( bAuxCtab )
            iTotDim = 2; // Matches & N
        else
            iTotDim = 1;
    }
    else
        ASSERT(0);

    if( bAuxCtab )
        pStatBase->SetSubCells( iTotDim );

    return iTotDim;
}

// Get the number of cells based on list of ranges
int CTAB::CtCells( int iCtNode ) {
    CTNODE* pCtNode = (CTNODE *) ( CtNodebase + iCtNode );
    int     iFirstRange = iCtNode + CTNODE_SLOTS;
    int     iLastRange = iFirstRange + CTRANGE_SLOTS * ( pCtNode->m_iCtNumRanges );
    int     iNumCells=0;

    for( int iRange = iFirstRange; iRange < iLastRange; iRange += CTRANGE_SLOTS ) {
        CTRANGE*pRange = (CTRANGE *) ( CtNodebase + iRange );

        iNumCells += pRange->getNumCells();
    }

    return iNumCells;
}



#define TOTAL_TYPES  3
#define NUM_VALADDR   19

class CSubTableExpanded : public CSubTable {

private:
   // Percent stuff
    CAcum   m_expandedSubTableAcum;
    int     m_iNumTotals[TBD_MAXDIM][TOTAL_TYPES];
    int     m_iClassStep[TBD_MAXDIM];
    int     m_iExpandedCells[TBD_MAXDIM];

public:
    CSubTableExpanded();

    void Init(CTAB* pCtab, CSubTable* pSubTable, bool bAllTotals);

    // TOTAL_CLASS is used only when the Depth is 2 (i.e. * was used) and the stat is applyed to the depth 2 item
    enum TotalStyle { TOTALSTYLE_CLASS=0, TOTALSTYLE_SUBTABLE=1, TOTALSTYLE_TABLE=2 };

    // From outer: tables totals, then Sub-Table total, then (if used) Class Totals.
    // Total Layers is used only if there are 2 or more layers.
    int         GetNumTotals( int iDim );
    int         GetDimSize( int iDim );
    bool        GetTotals(  const int iNormalRow, const int iNormalCol, const int iNormalLayer,
                            const TotalStyle eTotalStyle, const bool bTotalLayer,
                            SUBTABLE_TOTALS* cTotal );

    bool        GetTotAddr(int iSubTableI, int iSubTableJ, int iSubTableK, double* dValAddr[NUM_VALADDR], int dValIndex[NUM_VALADDR][TBD_MAXDIM] );
    void        CalcTotals( CTAB* pCtab, CSubTable& cSubTable, bool bUseRemap, CTAB* pCtabAux ); // Fill m_expandedSubTableAcum & calculate Totals
    void        CalcPercents( CTAB* pCtab, CSubTable& cSubTable, bool bUseRemap, CtStatPercent* pStatPercent ); // Calculate the percents and fill pCtab->m_pAcum
    bool        AllocExpandedAcum();
    void        FreeExpandedAcum();
    CAcum&          GetExpandedAcum();
};

CSubTableExpanded::CSubTableExpanded() {
    Init(NULL, NULL, false);
}

void CSubTableExpanded::Init(CTAB* pCtab, CSubTable* pSubTable, bool bAllTotals ) {
    if( pSubTable == NULL ) {
        for( int iDim=0; iDim < TBD_MAXDIM; iDim++ ) {
            for( int i=0; i < TOTAL_TYPES; i++ ) {
                m_iNumTotals[iDim][i] = 0;
                m_iClassStep[iDim] = 0;
                m_iExpandedCells[iDim] = 0;
            }
        }
    }
    else {
        ASSERT( pSubTable->GetStatType() == CTSTAT_PERCENT );

        int     iCells[TBD_MAXDIM][TBD_MAXDEPTH];
        int     iExpandedCells[TBD_MAXDIM];
        int     iNumTotals[TBD_MAXDIM][TOTAL_TYPES];


        for( int iDim=0; iDim < TBD_MAXDIM; iDim++ ) {
            for( int iTotalType=0; iTotalType < TOTAL_TYPES; iTotalType++ ) {
                iNumTotals[iDim][iTotalType] = 0;
            }

            for( int iDepth=0; iDepth < TBD_MAXDEPTH; iDepth++ ) {
                iCells[iDim][iDepth] = ( pSubTable->m_iCtTree[iDim][iDepth] < 0 ) ? 1 : pCtab->CtCells(pSubTable->m_iCtTree[iDim][iDepth] );
            }


            iExpandedCells[iDim] = iCells[iDim][0] * iCells[iDim][1];
            ASSERT( iExpandedCells[iDim] >= 1 );

            if( bAllTotals || iExpandedCells[iDim] >= 2 ) {
                iNumTotals[iDim][CSubTableExpanded::TOTALSTYLE_SUBTABLE] = 1; // SubTable Total
            }

            iNumTotals[iDim][CSubTableExpanded::TOTALSTYLE_TABLE] = 1; // Table Total

            iExpandedCells[iDim] += iNumTotals[iDim][CSubTableExpanded::TOTALSTYLE_CLASS] +
                iNumTotals[iDim][CSubTableExpanded::TOTALSTYLE_SUBTABLE] +
                iNumTotals[iDim][CSubTableExpanded::TOTALSTYLE_TABLE];
        }


        for( int iDim=0; iDim < TBD_MAXDIM; iDim++ ) {
            for( int i=0; i < TOTAL_TYPES; i++ ) {
                m_iNumTotals[iDim][i] = iNumTotals[iDim][i];
                m_iClassStep[iDim] = 0; // See below
                m_iExpandedCells[iDim] = iExpandedCells[iDim];
            }
        }

        // m_iClassStep
        for( int iDim=0; iDim < TBD_MAXDIM; iDim++ ) {
            if( m_iNumTotals[iDim][TOTALSTYLE_CLASS] > 0 ) {
                int     iDimSize=GetDimSize(iDim) - GetNumTotals( iDim );
                m_iClassStep[iDim] = iDimSize / m_iNumTotals[iDim][TOTALSTYLE_CLASS];
                ASSERT( (double) m_iClassStep[iDim] == (double) iDimSize/ m_iNumTotals[iDim][TOTALSTYLE_CLASS] );
            }
        }
    }
}


int CSubTableExpanded::GetNumTotals( int iDim ) {
    ASSERT( iDim >= 0 && iDim < TBD_MAXDIM );

    return m_iNumTotals[iDim][TOTALSTYLE_CLASS]+
           m_iNumTotals[iDim][TOTALSTYLE_SUBTABLE]+
           m_iNumTotals[iDim][TOTALSTYLE_TABLE];
}


int CSubTableExpanded::GetDimSize( int iDim ) {
    int     iDimSize;

    ASSERT( iDim >=0 && iDim < TBD_MAXDIM );
    if( iDim == 0 )
        iDimSize = m_iExpandedCells[0];
    else if( iDim == 1 )
        iDimSize = m_iExpandedCells[1];
    else
        iDimSize = m_iExpandedCells[2];

    return iDimSize;
}


/*
Figure Number 1:
Example: the next subtable has 7 rows & 10 columns & 2 layers.
 0123456789
0vvvvvvvvvvCCCST
1vvvvvvvvvvCCCST
2vvvvvvvvvvCCCST
3vvvvvvvvvvCCCST
4vvvvvvvvvvCCCST
5vvvvvvvvvvCCCST
6vvvvvvvvvvCCCST
 CCCCCCCCCC-----
 CCCCCCCCCC-----
 SSSSSSSSSS---A-
 TTTTTTTTTT----Z


 0123456789
0xxxxxxxxxxCCCST
1xxxxxxxxxxCCCST
2xxxxxxxxxxCCCST
3xxxxxxxxxxCCCST
4xxxxxxxxxxCCCST
5xxxxxxxxxxCCCST
6xxxxxxxxxxCCCST
 CCCCCCCCCC-----
 CCCCCCCCCC-----
 SSSSSSSSSS---A-
 TTTTTTTTTT----Z

  The C border is the total of the CLASS.
  The S border is the total of the SUBTABLE.
  The T border is the total of the TABLE.
  The - are not used cells

  The Total Layer has the next shape ( t[i,j]=v[i,j]+x[i,j] ):

 0123456789
0ttttttttttCCCST
1ttttttttttCCCST
2ttttttttttCCCST
3ttttttttttCCCST
4ttttttttttCCCST
5ttttttttttCCCST
6ttttttttttCCCST
 CCCCCCCCCC-----
 CCCCCCCCCC-----
 SSSSSSSSSS---A-
 TTTTTTTTTT----Z
*/

// Cell(0)
// ClassTotalRow(1), ClassTotalCol(2), ClassTotalLay(3)
// SubTableRowTot(4), SubTableColTot(5), SubTableLayTot(6),
// TableRowTot(7),    TableColTot(8),    TableLayTot(9),
// LayerClassTotalRow(10), LayerClassTotalCol(11),   LayerClassTotalLay(12)
// LayerSubTableRowTot(13), LayerSubTableColTot(14), LayerSubTableLayTot(15),
// LayerTableRowTot(16),    LayerTableColTot(17),    LayerTableLayTot(18),

bool CSubTableExpanded::GetTotAddr(int iSubTableI, int iSubTableJ, int iSubTableK,
                                   double* dValAddr[NUM_VALADDR], int dValIndex[NUM_VALADDR][TBD_MAXDIM] ) {
    int     iExpandedNumRows   = m_expandedSubTableAcum.GetNumRows();
    int     iExpandedNumCols   = m_expandedSubTableAcum.GetNumCols();
    int     iExpandedNumLayers = m_expandedSubTableAcum.GetNumLayers();
    int     iNumRows   = iExpandedNumRows   - GetNumTotals( 0 );
    int     iNumCols   = iExpandedNumCols   - GetNumTotals( 1 );
    int     iNumLayers = iExpandedNumLayers - GetNumTotals( 2 );

    bool    bExpandedLayer=(iSubTableK>=iNumLayers);


    ASSERT( iSubTableI >= 0 && iSubTableI < iNumRows );
    ASSERT( iSubTableJ >= 0 && iSubTableJ < iNumCols );
    //ASSERT( iSubTableK >= 0 && iSubTableK < iNumLayers );

    ASSERT( iExpandedNumRows >= 3 );
    ASSERT( iExpandedNumCols >= 3 );
    ASSERT( iExpandedNumLayers >= 3 );

    // The cell
    dValIndex[0][0] = iSubTableI;
    dValIndex[0][1] = iSubTableJ;
    dValIndex[0][2] = iSubTableK;

    // ClassTotalRow(1), ClassTotalCol(2), ClassTotalLay(3)
    // LayerClassTotalRow(10), LayerClassTotalCol(11),   LayerClassTotalLay(12)
    // RHF COM Oct 30, 2002double*     pClassTotalCell=NULL;
    for( int iDim=0; iDim < TBD_MAXDIM; iDim++ ) {
        if( m_iClassStep[iDim] > 0 ) {
            int     iPosClassTotal;
            int     iClassTotalIndex;

            if( iDim == 0 ) {
                iPosClassTotal = iNumRows;
                iClassTotalIndex = iSubTableI / m_iClassStep[iDim];
                dValIndex[1+iDim][0] = iPosClassTotal+iClassTotalIndex;
                dValIndex[1+iDim][1] = iSubTableJ;
                dValIndex[1+iDim][2] = iSubTableK;

                dValIndex[10+iDim][0] = iPosClassTotal+iClassTotalIndex;
                dValIndex[10+iDim][1] = iSubTableJ;
                dValIndex[10+iDim][2] = iExpandedNumLayers-2;
            }
            else if( iDim == 1 ) {
                iPosClassTotal = iNumCols;
                iClassTotalIndex = iSubTableJ / m_iClassStep[iDim];
                dValIndex[1+iDim][0] = iSubTableI;
                dValIndex[1+iDim][1] = iPosClassTotal+iClassTotalIndex;
                dValIndex[1+iDim][2] = iSubTableK;

                dValIndex[10+iDim][0] = iSubTableI;
                dValIndex[10+iDim][1] = iPosClassTotal+iClassTotalIndex;
                dValIndex[10+iDim][2] = iExpandedNumLayers-2;
            }
            else if( bExpandedLayer ){
                dValIndex[1+iDim][0] = -1;
                dValIndex[1+iDim][1] = -1;
                dValIndex[1+iDim][2] = -1;

                dValIndex[10+iDim][0] = -1;
                dValIndex[10+iDim][1] = -1;
                dValIndex[10+iDim][2] = -1;

            }
            else {
                iPosClassTotal = iNumLayers;
                iClassTotalIndex = iSubTableK / m_iClassStep[iDim];
                dValIndex[1+iDim][0] = iSubTableI;
                dValIndex[1+iDim][1] = iSubTableJ;
                dValIndex[1+iDim][2] = iPosClassTotal+iClassTotalIndex;

                dValIndex[10+iDim][0] = iSubTableI;
                dValIndex[10+iDim][1] = iSubTableJ;
                dValIndex[10+iDim][2] = iExpandedNumLayers-2;
            }

            // RHF COM Oct 30, 2002ASSERT( pClassTotalCell != NULL );
            // RHF COM Oct 30, 2002dValAddr[1+iDim] = pClassTotalCell;

        }
        else {
            dValIndex[1+iDim][0] = -1;
            dValIndex[1+iDim][1] = -1;
            dValIndex[1+iDim][2] = -1;
        }
    }

    // SubTableRowTot, SubTableColTot, SubTableLayTot
    dValIndex[4][0] = iExpandedNumRows-2;
    dValIndex[4][1] = iSubTableJ;
    dValIndex[4][2] = iSubTableK;

    dValIndex[5][0] = iSubTableI;
    dValIndex[5][1] = iExpandedNumCols-2;
    dValIndex[5][2] = iSubTableK;

    dValIndex[6][0] = iExpandedNumRows-2;
    dValIndex[6][1] = iExpandedNumCols-2;
    dValIndex[6][2] = iSubTableK;

    if( bExpandedLayer ) {
        for( int i=7; i < NUM_VALADDR; i++ ) {
            dValIndex[i][0] = dValIndex[i][1] = dValIndex[i][2] = -1;
        }
    }
    else {
        // TableRowTot,    TableColTot,    TableLayTot
        dValIndex[7][0] = iExpandedNumRows-1;
        dValIndex[7][1] = iSubTableJ;
        dValIndex[7][2] = iSubTableK;

        dValIndex[8][0] = iSubTableI;
        dValIndex[8][1] = iExpandedNumCols-1;
        dValIndex[8][2] = iSubTableK;

        dValIndex[9][0] = iExpandedNumRows-1;
        dValIndex[9][1] = iExpandedNumCols-1;
        dValIndex[9][2] = iSubTableK;

        // LayerClassTotalRow, LayerClassTotalCol,   LayerClassTotalLay
        //See Loop above

        // LayerSubTableRowTot, LayerSubTableColTot, LayerSubTableLayTot
        dValIndex[13][0] = iExpandedNumRows-2;
        dValIndex[13][1] = iSubTableJ;
        dValIndex[13][2] = iExpandedNumLayers-2 ;

        dValIndex[14][0] = iSubTableI;
        dValIndex[14][1] = iExpandedNumCols-2;
        dValIndex[14][2] = iExpandedNumLayers-2 ;

        dValIndex[15][0] = iExpandedNumRows-2;
        dValIndex[15][1] = iExpandedNumCols-2;
        dValIndex[15][2] = iExpandedNumLayers-2 ;

        // LayerTableRowTot,    LayerTableColTot,    LayerTableLayTot
        dValIndex[16][0] = iExpandedNumRows-1;
        dValIndex[16][1] = iSubTableJ;
        dValIndex[16][2] = iExpandedNumLayers-1;

        dValIndex[17][0] = iSubTableI;
        dValIndex[17][1] = iExpandedNumCols-1;
        dValIndex[17][2] = iExpandedNumLayers-1;

        dValIndex[18][0] = iExpandedNumRows-1;
        dValIndex[18][1] = iExpandedNumCols-1;
        dValIndex[18][2] = iExpandedNumLayers-1;
    }


    for( int i=0; i < NUM_VALADDR; i++ ) {
        dValAddr[i] = (double*) m_expandedSubTableAcum.GetValue( dValIndex[i][0], dValIndex[i][1], dValIndex[i][2] );
    }

    return true;
}

// Fill Expanded matrix
void CSubTableExpanded::CalcTotals( CTAB* pCtab, CSubTable& cSubTable, bool bUseRemap, CTAB* pCtabAux ) {
    int     iTableI, iTableJ, iTableK;
    double* pdSubTableValue;
    double* pdTableValue;
    int     iSubTableOffSet = -1;
    bool    bOkCoord;
    int     iExpandedNumRows   = m_expandedSubTableAcum.GetNumRows();
    int     iExpandedNumCols   = m_expandedSubTableAcum.GetNumCols();
    int     iExpandedNumLayers = m_expandedSubTableAcum.GetNumLayers();
    int     iNumRows   = iExpandedNumRows   - GetNumTotals( 0 );
    int     iNumCols   = iExpandedNumCols   - GetNumTotals( 1 );
    int     iNumLayers = iExpandedNumLayers - GetNumTotals( 2 );


    ASSERT( iNumRows >= 1 );
    ASSERT( iNumCols >= 1 );
    ASSERT( iNumLayers >= 1 );

    ASSERT( iExpandedNumRows >= 3 );
    ASSERT( iExpandedNumCols >= 3 );
    ASSERT( iExpandedNumLayers >= 3 );

    // Total borders
    ASSERT( pCtab->GetCtabGrandTotal() );
    CBorder*    pTableTotalBorder=pCtab->GetCtabGrandTotal()->m_pBorder;
    ASSERT( pTableTotalBorder );

    ASSERT( pCtabAux != NULL  );
    CBorder*    pSubTableTotalBorder=pCtabAux->m_pBorder;
    ASSERT( pSubTableTotalBorder );


    int     iSubTableI, iSubTableJ, iSubTableK, iCellNum;
    bool    bInner, bSubTableTotal, bTableTotal;


    // Copy values from normal table, totals for subtable & total for table to expanded subtable
    for( iSubTableK=0; iSubTableK < iExpandedNumLayers; iSubTableK++ ) {
        for( iSubTableJ=0; iSubTableJ < iExpandedNumCols; iSubTableJ++ ) {
            for( iSubTableI=0; iSubTableI < iExpandedNumRows; iSubTableI++ ) {
                bInner = bSubTableTotal = bTableTotal = false;
                if( iSubTableI < iNumRows && iSubTableJ < iNumCols && iSubTableK < iNumLayers )
                    bInner = true;
                else if( iSubTableI == iExpandedNumRows - 1 || iSubTableJ == iExpandedNumCols - 1 || iSubTableK == iExpandedNumLayers - 1 )
                    bTableTotal = true;
                else {
                    ASSERT( iSubTableI == iExpandedNumRows - 2 || iSubTableJ == iExpandedNumCols - 2 || iSubTableK == iExpandedNumLayers - 2 );
                    bSubTableTotal = true;
                }

                // Auxiliar expanded Table cell
                pdSubTableValue = (double*) m_expandedSubTableAcum.GetValue( iSubTableI, iSubTableJ, iSubTableK );
                ASSERT( pdSubTableValue != NULL );

                if( bInner ) {
                    iSubTableOffSet++;

                    // Get iTableI, iTableJ & iTableK
                    if( bUseRemap )
                        bOkCoord = cSubTable.GetTableCoord( iTableI, iTableJ, iTableK, iSubTableOffSet );
                    else
                        bOkCoord = pCtab->GetTableCoordOnLine( iTableI, iTableJ, iTableK, iSubTableOffSet, &cSubTable );

                    ASSERT( bOkCoord );

                    // Table cell
                    pdTableValue = (double*) pCtab->m_pAcum.GetValue( iTableI, iTableJ, iTableK );
                    ASSERT( pdTableValue != NULL );

                    // Copy to expanded cell
                    *pdSubTableValue = *pdTableValue;
                }
                else if( bTableTotal ) {
                    bool    bTotRow=(iSubTableI == iExpandedNumRows - 1 );
                    bool    bTotCol=(iSubTableJ == iExpandedNumCols - 1 );
                    bool    bTotLay=(iSubTableK == iExpandedNumLayers - 1 );

                    int     iAuxSubTableI;
                    int     iAuxSubTableJ;
                    int     iAuxSubTableK;
                    int     iAuxSubTableOffSet;

                    iAuxSubTableI = std::min( iSubTableI, iNumRows - 1 );
                    iAuxSubTableJ = std::min( iSubTableJ, iNumCols - 1 );
                    iAuxSubTableK = std::min( iSubTableK, iNumLayers - 1 );

                    iAuxSubTableOffSet = iAuxSubTableK*iNumCols*iNumRows + iAuxSubTableJ * iNumRows + iAuxSubTableI;

                    // Get iTableI, iTableJ & iTableK
                    if( bUseRemap )
                        bOkCoord = cSubTable.GetTableCoord( iTableI, iTableJ, iTableK, iAuxSubTableOffSet );
                    else
                        bOkCoord = pCtab->GetTableCoordOnLine( iTableI, iTableJ, iTableK, iAuxSubTableOffSet, &cSubTable );

                    ASSERT( bOkCoord );

                    if( bTotRow )
                        iTableI = pCtab->GetNumRows();
                    if( bTotCol )
                        iTableJ = pCtab->GetNumCols();
                    if( bTotLay )
                        iTableK = pCtab->GetNumLayers();

                    iCellNum = pTableTotalBorder->GetCellNum( iTableI, iTableJ, iTableK );
                    ASSERT( iCellNum >= 0 );

                    *pdSubTableValue = pTableTotalBorder->GetValueAt(iCellNum);
                }
                else {
                    ASSERT( bSubTableTotal );

                    iCellNum = pSubTableTotalBorder->GetCellNum( iSubTableI, iSubTableJ, iSubTableK );
                    ASSERT( iCellNum >= 0 );

                    *pdSubTableValue = pSubTableTotalBorder->GetValueAt(iCellNum);

                }
            }
        }
    }
}

void CSubTableExpanded::CalcPercents( CTAB* pCtab, CSubTable& cSubTable, bool bUseRemap, CtStatPercent* pStatPercent ) {
    int     iSubTableOffSet;
    int     iTableI, iTableJ, iTableK;
    int     iSubTableI, iSubTableJ, iSubTableK;
    int     iExpandedNumRows   = m_expandedSubTableAcum.GetNumRows();
    int     iExpandedNumCols   = m_expandedSubTableAcum.GetNumCols();
    int     iExpandedNumLayers = m_expandedSubTableAcum.GetNumLayers();
    int     iNumRows   = iExpandedNumRows   - GetNumTotals( 0 );
    int     iNumCols   = iExpandedNumCols   - GetNumTotals( 1 );
    int     iNumLayers = iExpandedNumLayers - GetNumTotals( 2 );
    double* pdTableValue;
    bool    bOkCoord;

    ASSERT( iNumRows >= 1 );
    ASSERT( iNumCols >= 1 );
    ASSERT( iNumLayers >= 1 );


    int     iPctType=pStatPercent->GetPctType();
    int         iStatDepth;
    //int     iStatDim=cSubTable.GetSymStatDim( &iStatDepth );
    cSubTable.GetSymStatDim( &iStatDepth );
    ASSERT( iStatDepth == 0 || iStatDepth == 1 );

    bool    bPercentCell    = (iPctType & CTSTAT_PERCENT_CELL) != 0;
    bool    bPercentRow     = (iPctType & CTSTAT_PERCENT_ROW) != 0;
    bool    bPercentColumn  = (iPctType & CTSTAT_PERCENT_COLUMN) != 0;
    bool    bPercentTotal   = (iPctType & CTSTAT_PERCENT_TOTAL) != 0;
    bool    bPercentLayer   = (iPctType & CTSTAT_PERCENT_LAYER) != 0;
    bool    bPercentSubClass= (iPctType & CTSTAT_PERCENT_SUBCLASS) != 0;
    bool    bPercentSubTable= (iPctType & CTSTAT_PERCENT_SUBTABLE) != 0;
    bool    bPercentTable   = (iPctType & CTSTAT_PERCENT_TABLE) != 0;

    if( bPercentCell )  bPercentLayer = true; // Always applyed to the layer // RHF Jan 31, 2003

    /*
    ASSERT( iStatDim >= DIM_ROW && iStatDim <= DIM_LAYER );
    bool    bIsMult=(cSubTable.m_iCtTree[iStatDim-1][1] >= 0);

    if( !bIsMult  && bPercentSubClass ) { //iStatDepth == 0
        bPercentSubClass = false;
        bPercentSubTable = true;
    }
    */

    int     iDim=-1;

    if( bPercentRow ) {
        iDim = 0;

        if( bPercentSubClass && m_iClassStep[iDim] == 0 ) {
            bPercentSubClass = false;
            bPercentSubTable = true;
        }

    }
    else if( bPercentColumn ) {
        iDim = 1;

        if( bPercentSubClass && m_iClassStep[iDim] == 0 ) {
            bPercentSubClass = false;
            bPercentSubTable = true;
        }
    }

    int     iPosClassTotal,iClassTotalIndex;
    double  dPct, dCellValue, dValue = 0.0;
    // Calc percents.
    for( iSubTableK=0; iSubTableK < iNumLayers; iSubTableK++ ) {
        for( iSubTableJ=0; iSubTableJ < iNumCols; iSubTableJ++ ) {
            for( iSubTableI=0; iSubTableI < iNumRows; iSubTableI++ ) {
                dCellValue = *(double*) m_expandedSubTableAcum.GetValue( iSubTableI, iSubTableJ, iSubTableK );

                // Cell
                if( bPercentCell ) {
                    ASSERT( bPercentLayer ); // Cell always is applyed to a layer

                    if( bPercentSubClass || bPercentSubTable )
                        dValue = *(double*) m_expandedSubTableAcum.GetValue( iSubTableI, iSubTableJ, iExpandedNumLayers-2 );
                    else if( bPercentTable )
                        dValue = *(double*) m_expandedSubTableAcum.GetValue( iSubTableI, iSubTableJ, iExpandedNumLayers-1 );
                    else
                        ASSERT(0);
                }

                // Row
                else if( bPercentRow ) {
                    if( bPercentLayer ) {
                        if( bPercentSubClass ){
                            ASSERT( m_iClassStep[iDim] > 0 );
                            if( m_iClassStep[iDim] > 0 ) {
                                iPosClassTotal = iNumRows;
                                iClassTotalIndex = iSubTableI / m_iClassStep[iDim];

                                dValue = *(double*) m_expandedSubTableAcum.GetValue( iPosClassTotal+iClassTotalIndex, iSubTableJ, iExpandedNumLayers-2  );
                            }
                            else
                                dValue = NOTAPPL;
                        }
                        else if( bPercentSubTable ){
                            dValue = *(double*) m_expandedSubTableAcum.GetValue( iExpandedNumRows-2, iSubTableJ, iExpandedNumLayers-2 );
                        }
                        else if( bPercentTable ){
                            dValue = *(double*) m_expandedSubTableAcum.GetValue( iExpandedNumRows-1, iSubTableJ, iExpandedNumLayers-1 );
                        }
                        else
                            ASSERT(0);

                    }
                    else { // !bPercentLayer
                        if( bPercentSubClass ){
                            ASSERT( m_iClassStep[iDim] > 0 );
                            if( m_iClassStep[iDim] > 0 ) {
                                iPosClassTotal = iNumRows;
                                iClassTotalIndex = iSubTableI / m_iClassStep[iDim];

                                dValue = *(double*) m_expandedSubTableAcum.GetValue( iPosClassTotal+iClassTotalIndex, iSubTableJ, iSubTableK );
                            }
                            else
                                dValue = NOTAPPL;
                        }
                        else if( bPercentSubTable ){
                            dValue = *(double*) m_expandedSubTableAcum.GetValue( iExpandedNumRows-2, iSubTableJ, iSubTableK );
                        }
                        else if( bPercentTable ){
                            dValue = *(double*) m_expandedSubTableAcum.GetValue( iExpandedNumRows-1, iSubTableJ, iSubTableK );
                        }
                        else
                            ASSERT(0);
                    }
                }

                // Column
                else if( bPercentColumn ) {
                    if( bPercentLayer ) {
                        if( bPercentSubClass ){
                            ASSERT( m_iClassStep[iDim] > 0 );
                            if( m_iClassStep[iDim] > 0 ) {
                                iPosClassTotal = iNumCols;
                                iClassTotalIndex = iSubTableJ / m_iClassStep[iDim];

                                dValue = *(double*) m_expandedSubTableAcum.GetValue( iSubTableI, iPosClassTotal+iClassTotalIndex, iExpandedNumLayers-2  );
                            }
                            else
                                dValue = NOTAPPL;
                        }
                        else if( bPercentSubTable ){
                            dValue = *(double*) m_expandedSubTableAcum.GetValue( iSubTableI, iExpandedNumCols-2, iExpandedNumLayers-2 );
                        }
                        else if( bPercentTable ){
                            dValue = *(double*) m_expandedSubTableAcum.GetValue( iSubTableI, iExpandedNumCols-1, iExpandedNumLayers-1 );
                        }
                        else
                            ASSERT(0);

                    }
                    else { // !bPercentLayer
                        if( bPercentSubClass ){
                            ASSERT( m_iClassStep[iDim] > 0 );
                            if( m_iClassStep[iDim] > 0 ) {
                                iPosClassTotal = iNumCols;
                                iClassTotalIndex = iSubTableJ / m_iClassStep[iDim];

                                dValue = *(double*) m_expandedSubTableAcum.GetValue( iSubTableI, iPosClassTotal+iClassTotalIndex, iSubTableK );
                            }
                            else
                                dValue = NOTAPPL;
                        }
                        else if( bPercentSubTable ){
                            dValue = *(double*) m_expandedSubTableAcum.GetValue( iSubTableI, iExpandedNumCols-2, iSubTableK );
                        }
                        else if( bPercentTable ){
                            dValue = *(double*) m_expandedSubTableAcum.GetValue( iSubTableI, iExpandedNumCols-1, iSubTableK );
                        }
                        else
                            ASSERT(0);
                    }
                }

                // Total
                else if( bPercentTotal ) {
                    if( bPercentLayer ) {
                        if( bPercentSubClass || bPercentSubTable ) {
                            dValue = *(double*) m_expandedSubTableAcum.GetValue( iExpandedNumRows-2, iExpandedNumCols-2, iExpandedNumLayers-2  );
                        }
                        else if( bPercentTable ){
                            dValue = *(double*) m_expandedSubTableAcum.GetValue( iExpandedNumRows-1, iExpandedNumCols-1, iExpandedNumLayers-1 );
                        }
                        else
                            ASSERT(0);

                    }
                    else { // !bPercentLayer
                        if( bPercentSubClass || bPercentSubTable ) {
                            dValue = *(double*) m_expandedSubTableAcum.GetValue( iExpandedNumRows-2, iExpandedNumCols-2, iSubTableK );
                        }
                        else if( bPercentTable ){
                            dValue = *(double*) m_expandedSubTableAcum.GetValue( iExpandedNumRows-1, iExpandedNumCols-1, iSubTableK );
                        }
                        else
                            ASSERT(0);
                    }
                }

                if( dValue == 0 )
                    dPct = NOTAPPL;
                else
                    dPct = dCellValue / dValue * 100;

                // Assign dPct to the correponsing table cell
                iSubTableOffSet = iSubTableK*iNumCols*iNumRows + iSubTableJ*iNumRows + iSubTableI;

                if( bUseRemap )
                    bOkCoord = cSubTable.GetTableCoord( iTableI, iTableJ, iTableK, iSubTableOffSet );
                else
                    bOkCoord = pCtab->GetTableCoordOnLine( iTableI, iTableJ, iTableK, iSubTableOffSet, &cSubTable );

                ASSERT( bOkCoord );

                pdTableValue = (double*) pCtab->m_pAcum.GetValue( iTableI, iTableJ, iTableK );
                ASSERT( pdTableValue != NULL );

                (*pdTableValue) = dPct;
            }
        }
    }
}

bool CSubTableExpanded::AllocExpandedAcum() {
    byte*   pExpandedAcum = m_expandedSubTableAcum.Alloc( sizeof(double), m_iExpandedCells[0], m_iExpandedCells[1], m_iExpandedCells[2], NULL );

    ASSERT( pExpandedAcum != NULL );

    bool    bRet = ( pExpandedAcum != NULL );

    return bRet;
}

void CSubTableExpanded::FreeExpandedAcum() {
    m_expandedSubTableAcum.Free();
}

CAcum& CSubTableExpanded::GetExpandedAcum() {
    return m_expandedSubTableAcum;
}

bool CTAB::MakeAuxCtabs()
{
    int         iStatNumber=0;
    CStatType   eStatType;
    bool        bGrandTotal=false;

    RemoveAllAuxCtabs();

    for( int iSubTable = -1; iSubTable < this->GetNumSubTables(); iSubTable++ ) {
        CSubTable* pSubTable=(iSubTable>=0) ? &this->GetSubTable(iSubTable) : NULL;
//        CSubTableExpanded   cSubTableExpanded;

        if( pSubTable != NULL ) { // RHF Jan 30, 2003
            eStatType = pSubTable->GetStatType();

            if( eStatType == CTSTAT_NONE ) // || eStatType == CTSTAT_OVERLAPPED )
                continue;

            // RHF COM Jan 30, 2003        // Don't need auxiliar tables
            // RHF COM Jan 30, 2003        if( eStatType == CTSTAT_PERCENT )
            // RHF COM Jan 30, 2003             continue;

            if( eStatType == CTSTAT_FREQ )
                continue;
            // RHF INIC Jun 11, 2003
            if( eStatType == CTSTAT_TOTAL )
                continue;
            // RHF END Jun 11, 2003
            bGrandTotal = false;
        }// RHF Jan 30, 2003
        else {
            eStatType = CTSTAT_TOTAL; // Gran table total
            bGrandTotal = true;

            // RHF INIC Apr 17, 2003 Don't use grand total when there is no stat
            if( GetNumSubTables() == 0 ||
                (GetNumSubTables() == 1 && &GetSubTable(0) != NULL && (&GetSubTable(0))->GetStatType() == CTSTAT_NONE ) )
                continue;
            // RHF END Apr 17, 2003
        }


        // Generate Auxiliar table
        auto pAuxCtab = std::make_shared<CTAB>(FormatTextCS2WS(_T("%s_%d"), this->GetName().c_str(), iStatNumber));
        int iAuxCtab = m_engineData->AddSymbol(pAuxCtab, Logic::SymbolTable::NameMapAddition::ToGlobalScope);

        if( pSubTable != NULL ) { // RHF Jan 30, 2003
            // Link to the subtable
            pSubTable->SetStatNumber( iStatNumber );
            pSubTable->SetAuxSymCtab( iAuxCtab );
        }// RHF Jan 30, 2003

        AddAuxCtab(iAuxCtab);

        if( bGrandTotal )
            this->SetCtabGrandTotal( pAuxCtab.get() );

        pAuxCtab->m_uEnviron[0] |= ct_BREAK; // RHF Oct 22, 2002

        pAuxCtab->SetEngineDriver( m_pEngineDriver );
        pAuxCtab->SetRunTimeVersion( 3 );
        pAuxCtab->SetAcumType( sizeof(double) ); // Always double
        pAuxCtab->SetNumDec( GetNumDec() );
        pAuxCtab->SetTableLevel( GetTableLevel() );
        //pAuxCtab->SetNodeExpr(GetNodeExpr(0), 0 ); // Only for avoid that this table is recognized like an array. See CTAB::IsArray
        pAuxCtab->SetAuxiliarTable( true );

        pAuxCtab->SetNumBreaks( GetNumBreaks() ); // RHF Aug 31, 2005


        CTableDef::ETableType     iTbdTableType=CTableDef::Ctab_NoType;
        // RHF INIC Oct 08, 2002
        if( eStatType == CTSTAT_OVERLAPPED )
            iTbdTableType = CTableDef::Ctab_NotAppl;
        // RHF END Oct 08, 2002
        else if( eStatType == CTSTAT_FREQ )
            iTbdTableType = CTableDef::Ctab_Freq;
        else if( eStatType == CTSTAT_TOTAL ) {
            if( bGrandTotal )
                iTbdTableType = CTableDef::Ctab_GrandTotal;
            else
                iTbdTableType = CTableDef::Ctab_Total;
        }
        else if( eStatType == CTSTAT_PERCENT ) {
            iTbdTableType = CTableDef::Ctab_Percent;
            //cSubTableExpanded.Init( this, &cSubTable, true );
        }
        else if( eStatType == CTSTAT_PROP )
            iTbdTableType = CTableDef::Ctab_Prop;
        else if( eStatType == CTSTAT_MIN )
            iTbdTableType = CTableDef::Ctab_MinTable;
        else if( eStatType == CTSTAT_MAX )
            iTbdTableType = CTableDef::Ctab_MaxTable;
        else if( eStatType == CTSTAT_MODE )
            iTbdTableType = CTableDef::Ctab_Mode;
        else if( eStatType == CTSTAT_MEAN )
            iTbdTableType = CTableDef::Ctab_StatMean;
        else if( eStatType == CTSTAT_MEDIAN )
            iTbdTableType = CTableDef::Ctab_Median;
        else if( eStatType == CTSTAT_PTILE )
            iTbdTableType = CTableDef::Ctab_Percentil;
        else if( eStatType == CTSTAT_STDEV )
            iTbdTableType = CTableDef::Ctab_StdDev;
        else if( eStatType == CTSTAT_VARIANCE )
            iTbdTableType = CTableDef::Ctab_Variance;
        else if( eStatType == CTSTAT_STDERR )
            iTbdTableType = CTableDef::Ctab_StdErr;
        else if( eStatType == CTSTAT_VPCT )
            iTbdTableType = CTableDef::Ctab_ValidPct;

        ASSERT( iTbdTableType != CTableDef::Ctab_NoType );


        pAuxCtab->SetTableType( iTbdTableType );

        if( bGrandTotal )
            pAuxCtab->SetNumDim( 3 );
        else
            pAuxCtab->SetNumDim( this->GetNumDim() );

        // Set dimensions
        bool    bStatDimFound=false;

        if( pSubTable != NULL ) { // RHF Jan 30, 2003
            int     iTotDim;

            for( int iDim = 0; iDim < this->GetNumDim(); iDim++ ) {
                ASSERT( pSubTable->GetSymStatDim() == DIM_ROW ||
                    pSubTable->GetSymStatDim() == DIM_COL ||
                    pSubTable->GetSymStatDim() == DIM_LAYER );
                ASSERT( DIM_ROW == 1 && DIM_COL == 2 && DIM_LAYER == 3 );


                int     iStatDepth;
                if( iDim == pSubTable->GetSymStatDim( &iStatDepth ) - 1 ) { // We are in the Stat Dimension
                    int     iCtNode=pSubTable->GetStatCtTree();

                    bStatDimFound = true;

                    ASSERT( iCtNode >= 0 );

                    CTNODE* pCtNode = (CTNODE *) ( CtNodebase + iCtNode );

                    ASSERT( eStatType == CTSTAT_OVERLAPPED || eStatType == pCtNode->m_iStatType );
                    iTotDim = GetStatDimSize( iCtNode, true );

                    // When the stat is in Depth=0 & there is Depth=1
                    // Example: A(min) * B[1:4]
                    if( iStatDepth == 0 && pSubTable->m_iCtTree[iDim][1] >= 0 ) {
                        CTNODE* pCtNodeRight = (CTNODE *) ( CtNodebase + pSubTable->m_iCtTree[iDim][1] );
                        ASSERT( pCtNodeRight->getNumCells() > 0 );
                        iTotDim *= pCtNodeRight->getNumCells();

                        // Search father '*'
                        CTNODE* pFather=pCtNodeRight;

                        bool    bFoundMult=false;
                        while( !bFoundMult && pFather->m_iParentIndex >= 0 ) {
                            pFather = (CTNODE *) ( CtNodebase + pFather->m_iParentIndex );
                            if( pFather->isOperNode(CTNODE_OP_MUL) ) {
                                bFoundMult = true;
                            }
                        }

                        ASSERT( bFoundMult );

                        int     iRightCells = ctcell( CtNodebase, pFather->m_iCtRight );

                        //int     iRightCells=pCtNodeRight->getNumCells();
                        pSubTable->SetRightCells( iRightCells ); // Useful for percents also!
                    }

                    // RHF INIC Sep 11, 2002
                    // Example: A * B[1:4] (min)
                    if( iStatDepth == 1 ) {
                        ASSERT( pSubTable->m_iCtTree[iDim][1] >= 0 );
                        CTNODE* pCtNodeLeft = (CTNODE *) ( CtNodebase + pSubTable->m_iCtTree[iDim][0] );
                        ASSERT( pCtNodeLeft->getNumCells() > 0 );
                        iTotDim *= pCtNodeLeft->getNumCells();
                    }
                    // RHF END Sep 11, 2002
                }
                else {
                    // RHF COM Oct 21, 2002 iTotDim = this->GetTotDim( iDim );

                    // RHF INIC Oct 21, 2002
                    // Calc Cells
                    if( pSubTable->m_iCtTree[iDim][0] < 0 )
                        iTotDim = 1;
                    else {
                        // RHF INIC Jun 09, 2003
                        CTNODE* pCtNode = (CTNODE *) ( CtNodebase + pSubTable->m_iCtTree[iDim][0]  );
                        if( pCtNode->m_iStatType != CTSTAT_NONE )
                            iTotDim = GetStatDimSize( pSubTable->m_iCtTree[iDim][0], false );
                        else
                        // RHF END Jun 09, 2003

                        iTotDim = CtCells(pSubTable->m_iCtTree[iDim][0] );

                        if( pSubTable->m_iCtTree[iDim][1] >= 0 ) {
                            // RHF INIC Jun 09, 2003
                            pCtNode = (CTNODE *) ( CtNodebase + pSubTable->m_iCtTree[iDim][1]  );
                            if( pCtNode->m_iStatType != CTSTAT_NONE )
                                iTotDim *= GetStatDimSize( pSubTable->m_iCtTree[iDim][1], false );
                            else
                                // RHF END Jun 09, 2003

                            iTotDim *= CtCells(pSubTable->m_iCtTree[iDim][1] );
                        }
                    }
                    ASSERT( iTotDim > 0 );
                    // RHF END Oct 21, 2002
                }

                pAuxCtab->SetTotDim( iTotDim, iDim );
            }
        } // RHF Jan 30, 2003
        else // RHF Jan 30, 2003
            bStatDimFound = true;// RHF Jan 30, 2003

        // Special Cases
        // RHF INIC Jan 30, 2003
        int iDimSize[TBD_MAXDIM]; { for( int i = 0; i < TBD_MAXDIM; i++ ) iDimSize[i] = 0; }

        if( eStatType == CTSTAT_PERCENT ) {
            for( int i=0; i < TBD_MAXDIM; i++ ) {
                if( pSubTable->m_iCtTree[i][0] < 0 )
                    iDimSize[i] = 1;
                else {
                    iDimSize[i] = CtCells(pSubTable->m_iCtTree[i][0] );
                    if( pSubTable->m_iCtTree[i][1] >= 0 )
                        iDimSize[i] *= CtCells(pSubTable->m_iCtTree[i][1] );
                }
                ASSERT( iDimSize[i] >= 1 );
            }

        }
        else if( bGrandTotal ) {
            ASSERT( pSubTable == NULL );
            for( int i=0; i < TBD_MAXDIM; i++ ) {
                iDimSize[i] = this->GetTotDim(i);
                if( iDimSize[i] <= 0 )
                    iDimSize[i] = 1;
            }
        }

        if( eStatType == CTSTAT_PERCENT || bGrandTotal ) {
            // The border as auxiliar table. Filled at the of the process!
            // (NumRows*NumCols+1)*(NumLayers+1)+NumRows*NumCols
            int     iTotDim=(iDimSize[0]+iDimSize[1]+1)*(iDimSize[2]+1)+iDimSize[0]*iDimSize[1];
            pAuxCtab->SetTotDim( iTotDim, 0 );
            pAuxCtab->SetTotDim( 1, 1 );
            pAuxCtab->SetTotDim( 1, 2 );

            //iTotDim = cSubTableExpanded.GetDimSize(iDim);

            pAuxCtab->InitBorder( iDimSize[0], iDimSize[1], iDimSize[2] );
        }

        // RHF END Jan 30, 2003

        ASSERT( bStatDimFound );

        iStatNumber++;
    }

    return true;
}



bool    bAllTotals=true;
bool CTAB::DoPercents( CSubTable& cSubTable, CtStatPercent* pStatPercent ) {
    bool    bRet=true;

    CSubTableExpanded*  pSubTableExpanded=new CSubTableExpanded;
    pSubTableExpanded->Init( this, &cSubTable, bAllTotals );

    pSubTableExpanded->AllocExpandedAcum();

    bool     bUseRemap = (GetNumCells() <= MAXCELL_REMAP);


    // RHF INIC Jan 30, 2003
    int     iCtabAux=cSubTable.GetAuxSymCtab();
    CTAB*   pCtabAux=(iCtabAux>0) ? XPT(iCtabAux) : NULL;
    // RHF END Jan 30, 2003

    pSubTableExpanded->CalcTotals( this, cSubTable, bUseRemap, pCtabAux );


    pSubTableExpanded->CalcPercents( this, cSubTable, bUseRemap, pStatPercent );

    pSubTableExpanded->FreeExpandedAcum();

    delete pSubTableExpanded;

    return bRet;
}

void CTAB::CalcHasSomePercent() {
    // Some percent subtable
    bool    bSomePercent=false;
    for( int iSubTable = 0; !bSomePercent && iSubTable < GetNumSubTables(); iSubTable++ ) {
        if( GetSubTable(iSubTable).GetStatType() == CTSTAT_PERCENT )
            bSomePercent = true;
    }

    SetHasSomePercent( bSomePercent );
}


// Calculates statistics & move information from subtables to table
bool CTAB::DoStatistics() {
    bool        bOkCoord;
    int         iTableI, iTableJ, iTableK, iCtNode;
    CStatType   eStatType;
    CTAB*       pCtabAux;
    CTNODE*     pCtNode;
    CtStatBase* pStatBase;
    double*     pdSubTableValue;
    double*     pdTableValue;

    bool    bSomePercent=GetHasSomePercent();

    //if( bSomePercent )
    //    GenTotals();


    // Calculate stats
    for( int iSubTable = 0; iSubTable < GetNumSubTables(); iSubTable++ ) {
        CSubTable& cSubTable=GetSubTable(iSubTable);

        eStatType = cSubTable.GetStatType();

        if( eStatType == CTSTAT_NONE )
            continue;

        // RHF INIC Oct 07, 2002
        if( eStatType == CTSTAT_FREQ )
            continue;

        // RHF END Oct 07, 2002

        // RHF INIC Jun 11, 2003
            if( eStatType == CTSTAT_TOTAL )
                continue;
            // RHF END Jun 11, 2003


        ASSERT( cSubTable.GetSymStatDim() == DIM_ROW ||
            cSubTable.GetSymStatDim() == DIM_COL ||
            cSubTable.GetSymStatDim() == DIM_LAYER );
        ASSERT( DIM_ROW == 1 && DIM_COL == 2 && DIM_LAYER == 3 );


        iCtNode = cSubTable.GetStatCtTree();

        ASSERT( iCtNode >= 0 );

        pCtNode = (CTNODE *) (CtNodebase + iCtNode);

        pStatBase = pCtNode->m_pStatBase;

        if( eStatType == CTSTAT_PERCENT ) {
            CtStatPercent*  pStatPercent=(CtStatPercent*)pStatBase;

            DoPercents( cSubTable, pStatPercent );
            continue;
        }

        // RHF INIT Jul 13, 2005
        else if( eStatType == CTSTAT_OVERLAPPED ) {
            int          iRemapCoordNumber1;
            CRemapCoord  cRemapCoordNumber2;

            POSITION    pos = cSubTable.m_mapSubTableToTable.GetStartPosition();
            while( pos ) {
                cSubTable.m_mapSubTableToTable.GetNextAssoc( pos, iRemapCoordNumber1, cRemapCoordNumber2 );

                // Table cell
                pdTableValue = (double*) this->m_pAcum.GetValue( cRemapCoordNumber2.m_iRow, cRemapCoordNumber2.m_iCol, cRemapCoordNumber2.m_iLay );
                ASSERT( pdTableValue != NULL );

                *pdTableValue = NOTAPPL;
            }
            continue;
        }
        // RHF END Jul 13, 2005

        ASSERT( cSubTable.GetAuxSymCtab() > 0 );
        pCtabAux = XPT(cSubTable.GetAuxSymCtab());

        int iRightCells = cSubTable.GetRightCells();
        int iNumSubCells   = pStatBase->GetSubCells();
        int iNumRows    = ( pCtabAux->GetTotDim(0) <= 0 ) ? 1 : pCtabAux->GetTotDim(0);
        int iNumCols    = ( pCtabAux->GetTotDim(1) <= 0 ) ? 1 : pCtabAux->GetTotDim(1);
        int iNumLayers  = ( pCtabAux->GetTotDim(2) <= 0 ) ? 1 : pCtabAux->GetTotDim(2);

        ASSERT( iNumRows >= 1);
        ASSERT( iNumCols >= 1);
        ASSERT( iNumLayers >= 1);

        ASSERT( iNumSubCells >= 1 );

        int     iDimDesp[TBD_MAXDIM]= {1,1,1};

        if( cSubTable.GetSymStatDim() == DIM_ROW ) {
            iDimDesp[0] = iNumSubCells;
        }
        else if( cSubTable.GetSymStatDim() == DIM_COL ) {
            iDimDesp[1] = iNumSubCells;
        }
        else if( cSubTable.GetSymStatDim() == DIM_LAYER ) {
            iDimDesp[2] = iNumSubCells;
        }
        else
            ASSERT(0);

                // RHF INIT Sep 07, 2004
                int                     iExtraCellDisplacment=0;
                bool        bNeedExtraCellDisplacment=
                        (       eStatType == CTSTAT_MODE ||
                                eStatType == CTSTAT_MEDIAN ||
                                eStatType == CTSTAT_PTILE ||

                                eStatType == CTSTAT_PROP ||
                                eStatType == CTSTAT_MEAN||
                                eStatType == CTSTAT_STDEV||
                                eStatType == CTSTAT_VARIANCE||
                                eStatType == CTSTAT_STDERR
                        );
                if( bNeedExtraCellDisplacment ) {
                        //ASSERT( cStatValue.m_iPos >= 0 && cStatValue.m_iPos < pStatBase->GetSubCells() );
                        if( cSubTable.GetSymStatDim() == DIM_ROW )
                                iExtraCellDisplacment = 1;
                        else if( cSubTable.GetSymStatDim() == DIM_COL )
                                iExtraCellDisplacment = pCtabAux->m_pAcum.GetNumRows();
                        else if( cSubTable.GetSymStatDim() == DIM_LAYER )
                                iExtraCellDisplacment = pCtabAux->m_pAcum.GetNumRows() * pCtabAux->m_pAcum.GetNumCols();
                        else
                                ASSERT(0);
                }
                // RHF END Sep 07, 2004


        //int iSubTableOffSet = -iNumSubCells;
        // RHF COM Jul 08, 2005 int     iSubTableOffSet = -1;

        // RHF INIT Jul 07, 2005
        int     iSubTableOffSetStep=1;
        if( cSubTable.GetSymStatDim() == DIM_ROW ) {
            if( eStatType == CTSTAT_PTILE ) {
                CtStatPTile*  pStatPTile=(CtStatPTile*)pStatBase;
                ASSERT( pStatPTile != NULL && pStatPTile->GetStatType() == CTSTAT_PTILE );

                iSubTableOffSetStep = (pStatPTile->GetNumTiles() -1);
            }
            else if( eStatType == CTSTAT_PROP ) {
                CtStatProp*  pStatProp=(CtStatProp*) pStatBase;
                ASSERT( pStatProp != NULL && pStatProp->GetStatType() == CTSTAT_PROP );

                if( pStatProp->GetPropType() & CTSTAT_PROP_TOTAL )
                    iSubTableOffSetStep = 2;
            }
        }

        int iSubTableOffSet = -iSubTableOffSetStep;
        // RHF END Jul 07, 2005

        for( int iSubTableK=0; iSubTableK < iNumLayers; iSubTableK += iDimDesp[2] ) {
            for( int iSubTableJ=0; iSubTableJ < iNumCols; iSubTableJ += iDimDesp[1] ) {
                for( int iSubTableI=0; iSubTableI < iNumRows; iSubTableI += iDimDesp[0] ) {
                    //iSubTableOffSet += iNumSubCells;

                    //RHF COM Jul 08, 2005 iSubTableOffSet ++;
                    iSubTableOffSet += iSubTableOffSetStep; // RHF Jul 08, 2005

                    // Get iTableI, iTableJ, & iTableK
                    if( GetNumCells() <= MAXCELL_REMAP )
                        bOkCoord = cSubTable.GetTableCoord( iTableI, iTableJ, iTableK, iSubTableOffSet );
                    else
                        bOkCoord = GetTableCoordOnLine( iTableI, iTableJ, iTableK, iSubTableOffSet, &cSubTable );

                    ASSERT( bOkCoord );

                    // Auxiliar table cell
                    pdSubTableValue = (double*) pCtabAux->m_pAcum.GetValue( iSubTableI, iSubTableJ, iSubTableK );
                    ASSERT( pdSubTableValue != NULL );

                    // Table cell
                    pdTableValue = (double*) this->m_pAcum.GetValue( iTableI, iTableJ, iTableK );
                    ASSERT( pdTableValue != NULL );

                    // Process
                    switch( eStatType ) {
                    case CTSTAT_OVERLAPPED:
                        ASSERT(0); // RHF Jul 13, 2005
                        *pdTableValue = NOTAPPL;
                        break;

                    case CTSTAT_FREQ: // Ok
                        ASSERT(0); // No need special behavior
                        //*pdTableValue = *pdSubTableValue;
                        break;

                    case CTSTAT_TOTAL: // Ok
                        ASSERT(0); // RHF Jun 11, 2003
                        *pdTableValue = *pdSubTableValue;
                        break;

                    case CTSTAT_PERCENT: // Ok
                        ASSERT(0); // Implementation in DoPercents
                        break;
                    case CTSTAT_PROP:
                        {
                            CtStatProp*  pStatProp=(CtStatProp*)pStatBase;
                            ASSERT( pStatProp != NULL && pStatProp->GetStatType() == CTSTAT_PROP );
                            int     iPropType=pStatProp->GetPropType();

                            // First SubCell
                            double  dProportion;
                            double  dMatches;
                            double  dN;

                            // First SubCell contains the matches
                            dMatches = *pdSubTableValue;

                            // Second SubCell contains N
                            // RHF COM Sep 07, 2004 pdSubTableValue++;
                            pdSubTableValue += iExtraCellDisplacment; // RHF Sep 07, 2004
                            dN = *pdSubTableValue;

                            if( dN == 0 )
                                dProportion = 0;
                            else
                                dProportion = dMatches / dN;

                            if( iPropType & CTSTAT_PROP_PERCENT )
                                dProportion *= 100;

                            // Fill First table cell
                            *pdTableValue = dProportion;


                            // Fill Second table cell if exists
                            if( iPropType & CTSTAT_PROP_TOTAL ) {
                                if( cSubTable.GetSymStatDim() == DIM_ROW ) {
                                    iTableI += iRightCells;
                                    iTableI++; // RHF Jul 08, 2005
                                }
                                else if( cSubTable.GetSymStatDim() == DIM_COL ) {
                                    iTableJ += iRightCells;
                                    iTableJ++; // RHF Jul 08, 2005
                                }
                                else if( cSubTable.GetSymStatDim() == DIM_LAYER ) {
                                    iTableK += iRightCells;
                                    iTableK++; // RHF Jul 08, 2005
                                }
                                else
                                    ASSERT(0);

                                /* RHF COM Jul 08, 2005
                                if( iRightCells == 0 )
                                    pdTableValue++;
                                else {
                                */
                                    pdTableValue = (double*) this->m_pAcum.GetValue( iTableI, iTableJ, iTableK );
                                    ASSERT( pdTableValue != NULL );
                                // RHF COM Jul 08, 2005}

                                *pdTableValue = dMatches; //JH 8/06 Total should be total in range not overall total
                            }
                        }
                        break;
                    case CTSTAT_MIN:
                    case CTSTAT_MAX:
                        *pdTableValue = *pdSubTableValue;

                        break;
                    case CTSTAT_MODE:
                        {
                            double      dCount;
                            double      dModeCell       = NOTAPPL;
                            double      dModeFreq      = -MAXVALUE;

                            for( int iSubCell=0; iSubCell < iNumSubCells; iSubCell++ ) {
                                dCount = *pdSubTableValue;

                                if( dCount != -NOTAPPL ) // RHF Jul 13, 2005
                                if( dModeFreq < dCount ) {
                                    dModeCell  = iSubCell;
                                    dModeFreq = dCount;
                                }

                                // RHF COM Sep 07, 2004 pdSubTableValue++;
                                pdSubTableValue += iExtraCellDisplacment; // RHF Sep 07, 2004
                            }

                            double  dModeCode;
                            if( dModeCell != NOTAPPL ) {
                                dModeCode = m_pEngineDriver->m_pIntDriver->val_coord( iCtNode, dModeCell );
                                // gsf 17-oct-2005
                                // mode could be negative or 0
//                                if( dModeCode < 0 ) {
//                                    ASSERT(0);
//                                    dModeCode = NOTAPPL;
//                                }
                            }
                            else
                                dModeCode = NOTAPPL;


                            *pdTableValue = dModeCode;
                        }

                        break;
                    case CTSTAT_MEAN:
                        {
                            double      dSumValues;
                            double      dSumValuesX;
                            double      dMean = NOTAPPL;

                            dSumValuesX = *pdSubTableValue;
                            // RHF COM Sep 07, 2004 pdSubTableValue++;
                            pdSubTableValue += iExtraCellDisplacment; // RHF Sep 07, 2004

                            dSumValues = *pdSubTableValue;

                            if( dSumValues )
                                dMean = dSumValuesX / dSumValues;

                            *pdTableValue = dMean;
                        }
                        break;
                    case CTSTAT_MEDIAN:
                        {
                            CtStatMedian*  pStatMedian=(CtStatMedian*)pStatBase;
                            ASSERT( pStatMedian != NULL && pStatMedian->GetStatType() == CTSTAT_MEDIAN );

                            double                  dCount;
                            CArray<double,double>   aValues;
                            for( int iSubCell=0; iSubCell < iNumSubCells; iSubCell++ ) {
                                dCount = *pdSubTableValue;
                                if( dCount != -NOTAPPL ) // RHF Jul 13, 2005
                                    aValues.Add( dCount );
                                else
                                    aValues.Add( 0 );
                                // RHF COM Sep 07, 2004 pdSubTableValue++;
                                pdSubTableValue += iExtraCellDisplacment; // RHF Sep 07, 2004
                            }
                            // New calculation of median    // BMD 20 Oct 2005
                            CArray<double,double> aIntervals;
                            double dInterval = 0.0;
                            for ( int iIntvl = 0 ; iIntvl < aValues.GetSize() ; iIntvl++) {
                                dInterval = m_pEngineDriver->m_pIntDriver->val_coord( iCtNode, (double) iIntvl );
                                aIntervals.Add( dInterval );
                            }
                            dInterval = m_pEngineDriver->m_pIntDriver->val_high();
                            aIntervals.Add( dInterval );

                            double  dMedian;

                            if (pStatMedian->GetMedianType() == 2) {
                                // JH 9/13/06 changed bias to -0.5 to match bureau std for discrete median
                                dMedian = calcPTile(0.5, aValues, aIntervals, -0.5);
                            }
                            else {
                                dMedian = calcPTile(0.5, aValues, aIntervals, 0.0);
                            }
                            *pdTableValue = dMedian;
                        }
                        break;
                    case CTSTAT_PTILE:
                        {
#define CTAB_MAX_TILES      10
                            CtStatPTile*  pStatPTile=(CtStatPTile*)pStatBase;
                            ASSERT( pStatPTile != NULL && pStatPTile->GetStatType() == CTSTAT_PTILE );

                            double      iNumTiles = pStatPTile->GetNumTiles();
                            double      dTileCode[CTAB_MAX_TILES];
                            double      dTileInt[CTAB_MAX_TILES];
                            double      dCount;

                            int         iTile;
                            ASSERT( iNumTiles <= CTAB_MAX_TILES );

                            for( iTile = 0; iTile < CTAB_MAX_TILES; iTile++ )
                                dTileCode[iTile] = dTileInt[iTile] = NOTAPPL;

                            if( pStatPTile->GetInterPol() ) {
                                CArray<double,double>   aValues;
                                for( int iSubCell=0; iSubCell < iNumSubCells; iSubCell++ ) {
                                    dCount = *pdSubTableValue;
                                    if( dCount != -NOTAPPL ) // RHF Jul 13, 2005
                                        aValues.Add( dCount );
                                    else
                                        aValues.Add( 0 );
                                    // RHF COM Sep 07, 2004 pdSubTableValue++;
                                    pdSubTableValue += iExtraCellDisplacment; // RHF Sep 07, 2004
                                }
                                // New calculation of n-tiles   BMD 20 Oct 2005
                                CArray<double,double> aIntervals;
                                for ( int iIntvl = 0 ; iIntvl <= aValues.GetSize() ; iIntvl++) {
                                    double dInterval = m_pEngineDriver->m_pIntDriver->val_coord( iCtNode, (double) iIntvl );
                                    aIntervals.Add( dInterval );
                                }

                                for( iTile = 1; iTile < iNumTiles; iTile++ ) {
                                    if (pStatPTile->GetPTileType() == 2) {
                                        dTileInt[iTile] = calcPTile((double) iTile/iNumTiles, aValues, aIntervals, -1.0);
                                    }
                                    else {
                                        dTileInt[iTile] = calcPTile((double) iTile/iNumTiles, aValues, aIntervals, 0.0);
                                    }
                                }
                            }
                            else { //no interpol
                                double*     pdOldSubTableValue = pdSubTableValue;
                                double      dSumValues      = 0;

                                // Sum Valid Values
                                for( int iSubCell=0; iSubCell < iNumSubCells; iSubCell++ ) {
                                    dCount = *pdSubTableValue;

                                    if( dCount != -NOTAPPL )
                                        dSumValues += dCount;

                                    // RHF COM Sep 07, 2004 pdSubTableValue++;
                                    pdSubTableValue += iExtraCellDisplacment; // RHF Sep 07, 2004
                                }

                                double      dCode;
                                //double      dCountMid = dSumValues * 0.5;   // Median
                                //double      dMedianCode=NOTAPPL;            // Median
                                //double      dMedianInt;                     // Median
                                double      dCountSum = 0;
                                double      dCodeLow  = NOTAPPL;
                                double      dCountLow = 0;

                                // Calculate pTiles
                                pdSubTableValue = pdOldSubTableValue;
                                for( int iSubCell=0; iSubCell < iNumSubCells; iSubCell++ ) {
                                    dCount = *pdSubTableValue;

                                    if( dCount == -NOTAPPL ) { // Empty
                                        // RHF COM Sep 07, 2004 pdSubTableValue++;
                                        pdSubTableValue += iExtraCellDisplacment; // RHF Sep 07, 2004
                                        continue;
                                    }

                                    dCountSum += dCount;
                                    dCode = m_pEngineDriver->m_pIntDriver->val_coord( iCtNode, iSubCell );
                                    if( dCode < 0 ) {
                                        ASSERT(0);
                                        dCode = NOTAPPL;
                                    }

                                    auto Interpolate = [](double dCountMid, double dCodeLow, double dCodeHig, double dCountLow, double dCountHig)
                                    {
                                        double  dCodeInt = ( dCodeLow < NOTAPPL && dCountLow != dCountHig ) ?
                                                           dCodeHig + ( dCountMid - dCountLow ) * ( dCodeHig - dCodeLow ) / ( dCountHig - dCountLow ) : NOTAPPL;  // BMD 20 Oct 2005

                                        return dCodeInt;
                                    };

                                    // ... median
                                    //if( dMedianCode == NOTAPPL && dCountSum >= dCountMid ) {
                                    //    dMedianCode = dCode;
                                    //    dMedianInt  = Interpolate( dCountMid, dCodeLow, dCode, dCountLow, dCountSum );
                                    //}

                                    // ... percentiles
                                    for( iTile = 1; iTile < iNumTiles; iTile++ ) {
                                        double  dTileMid = dSumValues * ( (double) iTile / iNumTiles );

                                        if( dTileCode[iTile] == NOTAPPL && dCountSum >= dTileMid ) {
                                            dTileCode[iTile] = dCode;
                                            dTileInt [iTile] = Interpolate( dTileMid, dCodeLow, dCode, dCountLow, dCountSum );
                                        }
                                    }

                                    dCodeLow  = dCode;
                                    dCountLow = dCountSum;

                                    // RHF COM Sep 07, 2004 pdSubTableValue++;
                                    pdSubTableValue += iExtraCellDisplacment; // RHF Sep 07, 2004
                                }
                            }

                            // Fill table cells
                            for( iTile = 1; iTile < iNumTiles; iTile++ ) {
                                if( pStatPTile->GetInterPol() )
                                    *pdTableValue = dTileInt[iTile]; // interpoled
                                else
                                    *pdTableValue = dTileCode[iTile]; // no interpoled

                                // Fill Second and other cells table cell if exist
                                if( cSubTable.GetSymStatDim() == DIM_ROW ) {
                                    iTableI += iRightCells;
                                    iTableI++; // RHF Jul 05, 2005
                                }
                                else if( cSubTable.GetSymStatDim() == DIM_COL ) {
                                    iTableJ += iRightCells;
                                    iTableJ++; // RHF Jul 05, 2005
                                }
                                else if( cSubTable.GetSymStatDim() == DIM_LAYER ) {
                                    iTableK += iRightCells;
                                    iTableK++; // RHF Jul 05, 2005
                                }
                                else
                                    ASSERT(0);

                                if( iTile < iNumTiles - 1 ) {
                                    pdTableValue = (double*) this->m_pAcum.GetValue( iTableI, iTableJ, iTableK );
                                    ASSERT( pdTableValue != NULL );
                                }
                            }
                        }
                        break;
                    case CTSTAT_STDEV:
                    case CTSTAT_VARIANCE:
                    case CTSTAT_STDERR:
                        {
                            double      dSumValues;
                            double      dSumValuesX;
                            double      dSumValues2;
                            double      dVariance=NOTAPPL;
                            double      dStdDev=NOTAPPL;
                            double      dStdErr=NOTAPPL;

                            dSumValuesX = *pdSubTableValue;
                            // RHF COM Sep 07, 2004 pdSubTableValue++;
                            pdSubTableValue += iExtraCellDisplacment; // RHF Sep 07, 2004

                            dSumValues2 = *pdSubTableValue;
                            // RHF COM Sep 07, 2004 pdSubTableValue++;
                            pdSubTableValue += iExtraCellDisplacment; // RHF Sep 07, 2004

                            dSumValues = *pdSubTableValue;

                            if( dSumValues > 1 )
                                dVariance = ( dSumValues2 - dSumValuesX * dSumValuesX / dSumValues ) / ( dSumValues - 1 ); // Unbaised
                                //dVariance = ( dSumValues2 - dSumValuesX * dSumValuesX / dSumValues ) / ( dSumValues );

                            if( eStatType == CTSTAT_STDEV || eStatType == CTSTAT_STDERR ) {
                                if( dSumValues > 1 )
                                    dStdDev   = sqrt( dVariance );

                                if( eStatType == CTSTAT_STDEV )
                                    *pdTableValue = dStdDev;
                                else if( eStatType == CTSTAT_STDERR ) {
                                    if( dSumValues > 1 )
                                        dStdErr = dStdDev / sqrt(dSumValues);

                                    *pdTableValue = dStdErr;
                                }
                                else
                                    ASSERT(0);
                            }
                            else if( eStatType == CTSTAT_VARIANCE )
                                *pdTableValue = dVariance;
                            else
                                ASSERT(0);
                        }
                        break;
                    case CTSTAT_VPCT: // TODO
                        ASSERT(0);
                        break;
                    default:
                        ASSERT(0);
                        break;
                    }
                }
            }
        }
    }

    //if( bSomePercent ) {
    //    FreeTotalAcum();
    //}

    return false;
}

void CTAB::GetStatList( int iCoordNumber, CArray<CtStatBase*,CtStatBase*>& aStatBase ) {
    for( int iStat=0; iStat < GetNumStats(); iStat++ ) {
        CtStat* pStat=GetStat(iStat);

        for( int iStatVar=0; iStatVar < pStat->GetNumStatVar(); iStatVar++ ) {
            CtStatVar&  ctStatVar=pStat->GetStatVar(iStatVar);

            if( ctStatVar.GetCoordNumber() == iCoordNumber ) {
                for( int iStatNum=0; iStatNum < pStat->GetNumStat(); iStatNum++ ) {
                    CtStatBase*  pStatBase=pStat->GetStat( iStatNum );
                    aStatBase.Add( pStatBase );
                }

                break;
            }
        }
    }
}

bool CTAB::AddStatCoordinates( int* pNodeBase[TBD_MAXDIM], int iCtMaxEnt[TBD_MAXDIM], int iCtNext[TBD_MAXDIM], int iCtRoot[TBD_MAXDIM] ) {
    for( int iDim = 0; iDim < this->GetNumDim(); iDim++ ) {
        int     iRoot    = iCtRoot[iDim];

        ASSERT( iRoot >= 0 );

        // Save
        int*    OldCtNodebase=CtNodebase;
        int     OldCtNodenext=CtNodenext;
        int     OldCtNodemxent=CtNodemxent;


        CtNodebase = pNodeBase[iDim];
        CtNodenext = iCtNext[iDim];
        CtNodemxent = iCtMaxEnt[iDim];


        int     iNewRoot = AddStatCoordinate( iRoot );

        // Could change, so reassign
        iCtNext[iDim] = CtNodenext;
        iCtRoot[iDim] = iNewRoot;


        // Restore
        CtNodebase = OldCtNodebase;
        CtNodenext = OldCtNodenext;
        CtNodemxent = OldCtNodemxent;


        if( iNewRoot < 0 )
            return false;
    }

    return true;
}

int CTAB::AddStatCoordinate( int iCtNode ) {
    int         iLeft, iRight, iThis;

    iThis = iCtNode;
    CTNODE*     pCtNode = (CTNODE *) ( CtNodebase + iCtNode );

    if( pCtNode->isOperNode() ) {
        iLeft  = AddStatCoordinate( pCtNode->m_iCtLeft );         // Left
        iRight = AddStatCoordinate( pCtNode->m_iCtRight );        // Right

        double  dNumCells;
        if( pCtNode->isOperNode(CTNODE_OP_ADD) ) {
            dNumCells = (double) ctcell( CtNodebase, iLeft ) + (double) ctcell( CtNodebase, iRight );
        } else {
            ASSERT( pCtNode->isOperNode(CTNODE_OP_MUL) );
            dNumCells = (double) ctcell( CtNodebase, iLeft ) * (double) ctcell( CtNodebase, iRight );
        }

        if( dNumCells > MAXCELL) {
            m_pEngineDriver->m_pEngineCompFunc->SetSyntErr(640); // too many cells
            return -1;
        }

        pCtNode->setNumCells( (int) dNumCells );
        pCtNode->m_iCtLeft   = iLeft;
        pCtNode->m_iCtRight  = iRight;

        // RHF INIC Jan 24, 2003
        if( iLeft >= 0 ) {
            CTNODE* pLeftNode = (CTNODE*) (CtNodebase + iLeft );
            pLeftNode->m_iParentIndex = iCtNode;
        }

        if( iRight >= 0 ) {
            CTNODE* pRightNode = (CTNODE*) (CtNodebase + iRight );
            pRightNode->m_iParentIndex = iCtNode;
        }
        // RHF END Jan 24, 2003
    }
    else {
        CArray<CtStatBase*,CtStatBase*>  aStatBase;

        GetStatList( pCtNode->m_iCoordNumber, aStatBase );

        // Special case: pct & freq
        bool    bHasFreq = false;
        bool    bHasPct = false;
        for( int iStat=0; /*!bHasFreq &&*/ iStat < aStatBase.GetSize(); iStat++ ) {
            CtStatBase*&     pStatBase=aStatBase.ElementAt(iStat);

            if( pStatBase->GetStatType() == CTSTAT_PERCENT ) {
                bHasPct = true;
            }
        }

        int iNumCells=pCtNode->m_iNumCells;
        if( /*!bHasFreq &&*/ aStatBase.GetSize() > 0 ) { // CTSTAT_FREQ also use an aux table
            pCtNode->m_iNumCells = 0;
        }

        // Add stat
        bool    bEmptyStat=false;
        bool    bFirstPct=true;
        //bHasPct=false;
        for( int iStat=0; iStat < aStatBase.GetSize(); iStat++ ) {
            CtStatBase*&     pStatBase=aStatBase.ElementAt(iStat);

            // RHF INIC Oct 09, 2002

            bEmptyStat = false;
            // Only the first pct generate a node in the tree
            //if( bFirstPct )
            //    continue;
            /* RHF COM INIC Jan 27, 2003
            if( pStatBase->GetStatType() == CTSTAT_FREQ && bHasPct )
                bEmptyStat = true;
            RHF COM END Jan 27, 2003 */

            if( false&&pStatBase->GetStatType() == CTSTAT_PERCENT ) {
                CtStatPercent*  pStatPercent=(CtStatPercent*) pStatBase;
                int             iPctType=pStatPercent->GetPctType();

                 // The first Pct is used as Freq counter
                if( /*!bHasFreq &&*/ bFirstPct) {
                    iPctType |= CTSTAT_PERCENT_USEASFREQ;

                    pStatPercent->SetPctType( iPctType );
                }

                // Only the first pct generate a node in the tree
                /* RHF COM INIC Jan 27, 2003
                if( !bFirstPct )
                    bEmptyStat = true;
                RHF COM END Jan 27, 2003 */

                bFirstPct = false;
            }
            // RHF END Oct 09, 2002

            // New Node for Stat
            int         iNewNode;
            CTNODE*     pNewNode;

            iNewNode = CtNodenext;
            pNewNode = (CTNODE*) ( CtNodebase + iNewNode );

            CtNodenext += sizeof(CTNODE) / sizeof(int);
            if( CtNodenext >= CtNodemxent ) {
                m_pEngineDriver->m_pEngineCompFunc->SetSyntErr(4);
                return -1;
            }

            // Fill New Node
            ASSERT( pCtNode->isVarNode() );
            *pNewNode = *pCtNode; // rcl, Jul 2005

            pNewNode->setNumCells(iNumCells); // Set as original first. GetStatDimSize could use this member
            pNewNode->m_iStatType = pStatBase->GetStatType(); // Must be here because is used by GetStatDimSize
            pNewNode->m_pStatBase = pStatBase; // GetStatDimSize could use this member

            // Sequence Item Number
            CTAB::pCurrentCtab->SetCurrentCoordNumber(CTAB::pCurrentCtab->GetCurrentCoordNumber()+1);
            pNewNode->m_iCoordNumber = CTAB::pCurrentCtab->GetCurrentCoordNumber();

            // RHF INIC Jun 11, 2003
            bool    bTotal=(pNewNode->m_iStatType == CTSTAT_TOTAL);
            int     iRangeCollapsed=1;
            // RHF END Jun 11, 2003

            // RHF INIC Jun 17, 2003
            bool        bPercentCollapsed = false;
            if( pStatBase->GetStatType() == CTSTAT_PERCENT ) {
                CtStatPercent*  pStatPercent=(CtStatPercent*) pStatBase;
                int             iPctType=pStatPercent->GetPctType();

                if( iPctType & CTSTAT_PERCENT_COLLAPSED ) {
                    bPercentCollapsed = true;
                }
            }
            // RHF END Jun 17, 2003

            // Copy Ranges
            int     iStartRange = iCtNode + CTNODE_SLOTS;
            int     iEndRange = iStartRange + CTRANGE_SLOTS * pCtNode->m_iCtNumRanges;

            for( int iRange = iStartRange; iRange < iEndRange; iRange += CTRANGE_SLOTS ) {

                // JH 5/30/06 - use intervals for median/ptile to tally
                if (pStatBase->GetHasIntervals()) {
                    // for median/ptile with intervals, use ranges from intervals

                    std::vector<double>& aIntervals = ((CtStatIntervals*) pStatBase)->GetIntervals();
                    pNewNode->m_iCtNumRanges = (int)aIntervals.size() - 1;

                    int     iStartRange = iCtNode + CTNODE_SLOTS;

                    ASSERT( pNewNode->isVarNode() );
                    ASSERT( pNewNode->m_iSymbol > 0 );
                    Symbol* pSymbol = NPT(pNewNode->m_iSymbol);
                    VART* pVarT = pSymbol->IsA(SymbolType::ValueSet) ? ((ValueSet*)pSymbol)->GetVarT() : (VART*)pSymbol;
                    double dHighOffset = 1;
                    if (pVarT->GetDecimals() > 0) {
                        dHighOffset = 1/pow(10.0, pVarT->GetDecimals()+1);
                    }

                    for (int iInterval = (int)aIntervals.size() - 1; iInterval > 0; --iInterval) {
                        CTRANGE*    pNewRange   = (CTRANGE*) ( CtNodebase + CtNodenext );
                        CtNodenext += sizeof(CTRANGE) / sizeof(int);
                        if( CtNodenext >= CtNodemxent ) {
                            m_pEngineDriver->m_pEngineCompFunc->SetSyntErr(4);
                            return -1;
                        }
                        pNewRange->m_iRangeLow = aIntervals[iInterval];
                        pNewRange->m_iRangeHigh = aIntervals[iInterval-1]-dHighOffset;
                        pNewRange->m_iRangeCollapsed = 1;
                    }
                    // end JH 5/30/06 - use intervals for median/ptile to tally
                }
                else {
                CTRANGE*    pRange      = (CTRANGE*) ( CtNodebase + iRange );
                CTRANGE*    pNewRange   = (CTRANGE*) ( CtNodebase + CtNodenext );

                CtNodenext += sizeof(CTRANGE) / sizeof(int);
                if( CtNodenext >= CtNodemxent ) {
                    m_pEngineDriver->m_pEngineCompFunc->SetSyntErr(4);
                    return -1;
                }

                memmove( pNewRange, pRange, sizeof(CTRANGE) );

                // RHF INIC Jun 11, 2003
                if( bTotal || bPercentCollapsed ) { // RHF Jun 17, 2003 Add bPercentCollapsed
                    pNewRange->m_iRangeCollapsed = iRangeCollapsed;
                    iRangeCollapsed++;
                }
                // RHF END Jun 11, 2003
                }
            }

            pNewNode->setNumCells( GetStatDimSize( iNewNode, false ) ); // RHF Sep 17, 2002 Moved

            // RHF INIC Oct 09, 2002
            if( bEmptyStat )
                pNewNode->setNumCells(0);
            // RHF END Oct 09, 2002

            // New Node for operator +
            iThis = m_pEngineDriver->m_pEngineCompFunc->ctopernode( iThis, iNewNode , CTNODE_OP_ADD );
        }
    }

    if( m_pEngineDriver->m_pEngineCompFunc->GetSyntErr() != 0 )
        return -1;

    return( iThis );
}

// Generate coordinates Maps & bitmaps.
bool CTAB::GenRemapCoord( CSubTable* pSubTable ) {
    int     ctvector[DIM_MAXDIM][MAX_TALLYCELLS+1];
    int     i,j,k;
    bool    bGenMaps=(GetNumCells() <= MAXCELL_REMAP); // If greater than MAXCELL_REMAP use GetTableCoordOnLine & GetSubTableCoordOnLine methods

    // # of elements in row, col, layer
    for( i = 0; i < DIM_MAXDIM; i++ ) {
        ctvector[i][0] = 1;
        ctvector[i][1] = 0;
    }

    for( i = 0; i < GetNumDim(); i++ )
        m_pEngineDriver->m_pIntDriver->CtPos( this, GetNodeExpr(i), ctvector[i], pSubTable, NULL, true );

    CRemapCoord     cTableCoord;
    CRemapCoord     cSubTableCoord;
    int             iTableOffSet;
    int             iSubTableOffSet=-1;
    int             iRow=0, iColumn=0, iLayer=0;


    //Generate the maps for fast run-time processing
    for( k = 1; k <= ctvector[2][0]; k++ ) { // Layer
        if( ctvector[2][k] == -1 )
            continue;
        //cSubTableCoord.m_iLay = k - 1;
        iLayer++;
        iColumn = 0;

        for( j = 1; j <= ctvector[1][0]; j++ ) { // Column
            if( ctvector[1][j] == -1 )
                continue;
            //cSubTableCoord.m_iCol = j - 1;
            iColumn++;
            iRow = 0;


            for( i = 1; i <= ctvector[0][0]; i++ ) { // Row
                if( ctvector[0][i] == -1 )
                    continue;
                //cSubTableCoord.m_iRow = i - 1;
                iRow++;

                iSubTableOffSet++;

                cTableCoord.m_iRow = ctvector[0][i];
                cTableCoord.m_iCol = ctvector[1][j];
                cTableCoord.m_iLay = ctvector[2][k];

                // Maps
                if( bGenMaps ) {
                    cSubTableCoord.m_iRow = iRow-1;
                    cSubTableCoord.m_iCol = iColumn-1;
                    cSubTableCoord.m_iLay = iLayer-1;

                    iTableOffSet = m_pAcum.GetOffSetPos( ctvector[0][i], ctvector[1][j], ctvector[2][k] );

                    iTableOffSet /= sizeof(double);

                    pSubTable->m_mapTableToSubTable.SetAt( iTableOffSet, cSubTableCoord );
                    pSubTable->m_mapSubTableToTable.SetAt( iSubTableOffSet, cTableCoord );
                }
            }
        }
    }


    return true;
}

bool CTAB::GetTableCoordOnLine( int& iTableI, int& iTableJ, int& iTableK, int iSearchSubTableOffSet, CSubTable* pSubTable ) {
    int         ctvector[DIM_MAXDIM][MAX_TALLYCELLS+1];
    int         i,j,k;

    // # of elements in row, col, layer
    for( i = 0; i < DIM_MAXDIM; i++ ) {
        ctvector[i][0] = 1;
        ctvector[i][1] = 0;
    }

    for( i = 0; i < GetNumDim(); i++ )
        m_pEngineDriver->m_pIntDriver->CtPos( this, GetNodeExpr(i), ctvector[i], pSubTable, NULL, true );

    int             iSubTableOffSet=-1;
    //Generate the maps for fast run-time processing
    for( k = 1; k <= ctvector[2][0]; k++ ) { // Layer
        if( ctvector[2][k] == -1 )
            continue;
        for( j = 1; j <= ctvector[1][0]; j++ ) { // Column
            if( ctvector[1][j] == -1 )
                continue;
            for( i = 1; i <= ctvector[0][0]; i++ ) { // Row
                if( ctvector[0][i] == -1 )
                    continue;
                iSubTableOffSet++;

                // Found
                if( iSearchSubTableOffSet == iSubTableOffSet ) {
                    iTableI = ctvector[0][i];
                    iTableJ = ctvector[1][j];
                    iTableK = ctvector[2][k];
                    return true;
                }
            }
        }
    }

    return false;
}


bool CTAB::GetSubTableCoordOnLine(  int& iSubTableI, int& iSubTableJ, int& iSubTableK, int iSearchTableOffSet, CSubTable* pSubTable ) {
    int         ctvector[DIM_MAXDIM][MAX_TALLYCELLS+1];
    int         i,j,k;

    // # of elements in row, col, layer
    for( i = 0; i < DIM_MAXDIM; i++ ) {
        ctvector[i][0] = 1;
        ctvector[i][1] = 0;
    }

    for( i = 0; i < GetNumDim(); i++ ) {
        m_pEngineDriver->m_pIntDriver->CtPos( this, GetNodeExpr(i), ctvector[i], pSubTable, NULL, true );
    }


    int             iTableOffSet;
    int             iRow=0, iColumn=0, iLayer=0;

    //Generate the maps for fast run-time processing
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

                iTableOffSet = m_pAcum.GetOffSetPos( ctvector[0][i], ctvector[1][j], ctvector[2][k] );

                iTableOffSet /= sizeof(double);
                // found
                if( iTableOffSet == iSearchTableOffSet ) {
                    iSubTableI = iRow - 1;
                    iSubTableJ = iColumn - 1;
                    iSubTableK = iLayer - 1;

                    return true;
                }
            }
        }
    }

    return false;
}
// RHF END Jul 30, 2002


bool CTAB::HasOverlappedCat( int* pBase, int iCtNode )
{
    CTNODE*     pCtNode=(CTNODE*) (pBase+iCtNode);
    int         iStartRange = iCtNode + CTNODE_SLOTS;
    int         iEndRange = iStartRange + CTRANGE_SLOTS * pCtNode->m_iCtNumRanges;
    std::vector<Range<double>> aRanges;

    for( int iRange = iStartRange; iRange < iEndRange; iRange += CTRANGE_SLOTS ) {
        CTRANGE*    pRange      = (CTRANGE*) ( pBase + iRange );

        /* that is collapsed and not overlapping. RHF Sep 08, 2004
        if( pRange->m_iRangeCollapsed == 1 && pRange->m_iRangeLow != pRange->m_iRangeHigh ||
            pRange->m_iRangeCollapsed >= 2 )
            return true;
                        */

        aRanges.emplace_back(pRange->m_iRangeLow, pRange->m_iRangeHigh);
    }

    return HasOverlappingRanges(aRanges);
}

// SERPRO_CALC
bool CTAB::GetHasSyntaxError() {
        return m_bHasSyntaxError;
}

void CTAB::SetHasSyntaxError( bool bHasSyntaxError ) {
        m_bHasSyntaxError = bHasSyntaxError;
}

CLinkTable* CTAB::GetLinkTable() {
    ASSERT( m_pLinkTable != NULL );
    return m_pLinkTable;
}

void CTAB::InitLinkTable() {
    if( m_pLinkTable != NULL )
        delete m_pLinkTable;
    m_pLinkTable = new CLinkTable;
}


void CTAB::MakeLinkTableTerms( int* pNodeBase, int iCtNode,
                              CArray<CLinkTerm,CLinkTerm>& aLinkTerms,
                              CArray<CLinkVar,CLinkVar>& aLinkVars, bool bFatherMult, int iDepth ) {
    ASSERT( iCtNode >= 0 );
    int         i;

    CArray<CLinkVar,CLinkVar>   aVector1;
    CArray<CLinkVar,CLinkVar>   aVector2;

    CTNODE*     pCtNode=(CTNODE *) (pNodeBase + iCtNode );

    if( pCtNode->isVarNode() ) // Variable or value-set
    {
        CLinkVar   cLinkVar;
        CString     csName;

        csName = WS2CS(NPT(pCtNode->m_iSymbol)->GetName());

        cLinkVar.SetName( csName );

        CString sOcc;

        bool bHasOcc = CTAB::m_aExtraNodeInfo.Lookup(pCtNode->m_iCoordNumber,sOcc) != 0;
        cLinkVar.SetOcc( bHasOcc ? sOcc : _T("") );

        // Ranges
        int     iStartRange, iEndRange;

        // SubRanges
        iStartRange = iCtNode + CTNODE_SLOTS;
        iEndRange = iStartRange + CTRANGE_SLOTS * pCtNode->m_iCtNumRanges;

        CSubRange cSubRange;
        for( int iRange = iStartRange; iRange < iEndRange; iRange += CTRANGE_SLOTS ) {
            CTRANGE *pSourceRange = (CTRANGE *) ( pNodeBase + iRange );

            bool     bImplicitHigh=(pSourceRange->m_iRangeLow==pSourceRange->m_iRangeHigh);
            cSubRange.Init( pSourceRange->m_iRangeLow, pSourceRange->m_iRangeHigh,
                bImplicitHigh, pSourceRange->m_iRangeCollapsed );

            cLinkVar.AddRange( cSubRange );
        }

        aLinkVars.Add( cLinkVar );

        // No Parent
        if( pCtNode->m_iParentIndex < 0 ) {
            CLinkTerm   cLinkTerm;

            cLinkTerm.AddLinkVar( cLinkVar, 0 );

            aLinkTerms.Add( cLinkTerm );
        }
    }
    else
    {
        ASSERT( pCtNode->isOperNode() );

        switch( pCtNode->getOperator() )
        {
        case CTNODE_OP_ADD:
            {
                MakeLinkTableTerms( pNodeBase, pCtNode->m_iCtLeft, aLinkTerms, aVector1, bFatherMult, iDepth );
                MakeLinkTableTerms( pNodeBase, pCtNode->m_iCtRight, aLinkTerms, aVector2, bFatherMult, iDepth );

                if( bFatherMult ) {
                    for( i=0; i < aVector1.GetSize(); i++ ) {
                        aLinkVars.Add( aVector1.ElementAt(i) );
                    }

                    for( i=0; i < aVector2.GetSize(); i++ ) {
                        aLinkVars.Add( aVector2.ElementAt(i) );
                    }
                }
                else {
                    for( i=0; i < aVector1.GetSize(); i++ ) {
                        CLinkTerm   cLinkTerm1;

                        cLinkTerm1.AddLinkVar( aVector1.ElementAt(i), 0 );
                        aLinkTerms.Add( cLinkTerm1 );
                    }

                    for( i=0; i < aVector2.GetSize(); i++ ) {
                        CLinkTerm   cLinkTerm2;

                        cLinkTerm2.AddLinkVar( aVector2.ElementAt(i), 0 );
                        aLinkTerms.Add( cLinkTerm2 );
                    }
                }
            }
            break;

        case CTNODE_OP_MUL:
            {
                CLinkTerm   cLinkTerm;

                MakeLinkTableTerms( pNodeBase, pCtNode->m_iCtLeft,  aLinkTerms, aVector1, true, 0 );
                MakeLinkTableTerms( pNodeBase, pCtNode->m_iCtRight, aLinkTerms, aVector2, true, 1 );

                for( i=0; i < aVector1.GetSize(); i++ ) {
                    cLinkTerm.AddLinkVar( aVector1.ElementAt(i), 0 );
                }

                for( i=0; i < aVector2.GetSize(); i++ ) {
                    cLinkTerm.AddLinkVar( aVector2.ElementAt(i), 1 );
                }

                aLinkTerms.Add( cLinkTerm );
            }
            break;
        default:
            break;
        }
    }
}

void CTAB::FillLinkTable() {
    ASSERT( m_pLinkTable != NULL );

    //m_pLinkTable->m_bHasNoBreak = !(m_uEnviron[0] & ct_BREAK);
    //m_pLinkTable->m_bHasNoPrint = !(m_uEnviron[0] & ct_PRINT);
    m_pLinkTable->m_bHasSyntaxError = GetHasSyntaxError();
    //m_pLinkTable->m_bApplDeclared = IsApplDeclared();

    bool    bFatherMult=false;
    int     iDepth=0;


    for( int iDim=0; iDim < GetNumDim(); iDim++ ) {
        CArray<CLinkVar,CLinkVar> aLinkVars;
        int     iRoot = GetNodeExpr(iDim);
        int*    pNodeBase=CtNodebase;

        if( iRoot >= 0 ) {
            if( iDim == 0 ) {
                MakeLinkTableTerms( pNodeBase, iRoot, m_pLinkTable->m_RowTermExpr, aLinkVars, bFatherMult, iDepth );

            }
            else if (iDim == 1 )
                MakeLinkTableTerms( pNodeBase, iRoot, m_pLinkTable->m_ColTermExpr, aLinkVars, bFatherMult, iDepth );
            else {
                ASSERT( iDim==2);
                MakeLinkTableTerms( pNodeBase, iRoot, m_pLinkTable->m_LayTermExpr, aLinkVars, bFatherMult, iDepth );
            }
        }
    }
}

// SERPRO_CALC

//RHF INIT Aug 05, 2005
void CTAB::ResetSelectWeightExpr( int& iSelectExpr, int& iWeightExpr ) {
    if( GetNumUnits() == 1 ) {
        CtUnit &ctUnit=GetUnit( 0 );

        if( ctUnit.GetDefaultUnit() ) {

            if( iSelectExpr != 0 && ctUnit.GetSelectExpr() == 0 ) {
                ctUnit.SetSelectExpr( iSelectExpr);
                iSelectExpr = 0;
            }

            if( iWeightExpr != 0 && ctUnit.GetWeightExpr() == 0 ) {
                ctUnit.SetWeightExpr( iWeightExpr);
                iWeightExpr = 0;
            }
        }
    }
}
//RHF END Aug 05, 2005
