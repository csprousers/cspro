#include "StandardSystemIncludes.h"
#include "INTERPRE.H"
#include "citer.h"

//---------------------------------------------------------------------------
//  File name: citer.cpp
//
//  Description:
//          Manipulate a 3-d iterator
//
//  History:    Date       Author   Comment
//              ---------------------------
//              16 Jul 2001 RHF     Creation
//              24 Jul 2004 rcl     Adds MVAR_CONSTANT handling for CIterator::Make
//              27 Oct 2004 rhf     Adds MakeExportIterator method
//              03 Nov 2004 rcl     Adds MakeTableIterator method
//
//---------------------------------------------------------------------------

//////////////////////////////////////////////////////////////////////////
// CBasicIterator methods
void CBasicIterator::Add3DIndex(const threeDim& the3Ddim )
{
    m_aIndexes.push_back( the3Ddim );
}

// internal m_aIndexes array is 1 based
// aIndex will be filled with 0-based indexes
void CBasicIterator::getIndexes( int iWhichOne, int aIndex[DIM_MAXDIM] )
{
    if( iWhichOne < 0 || (size_t) iWhichOne >= m_aIndexes.size() )
        throw CIteratorException();

    ASSERT( DIM_MAXDIM == 3 );
    // if DIM_MAXDIM changes to something different from 3
    // the code below needs to be updated
    // now it is very DIM_MAXDIM == 3 dependent

    threeDim s3d = m_aIndexes[iWhichOne];

    aIndex[0] = s3d[0] - 1;
    aIndex[1] = s3d[1] - 1;
    aIndex[2] = s3d[2] - 1;
}

// internal m_aIndexes array is 1 based
// dOccur will be filled with 1-based indexes
void CBasicIterator::getIndexes( int iWhichOne, double dOccur[DIM_MAXDIM] )
{
    if( iWhichOne < 0 || (size_t) iWhichOne >= m_aIndexes.size() )
        throw CIteratorException();

    ASSERT( DIM_MAXDIM == 3 );
    // if DIM_MAXDIM changes to something different from 3
    // the code below needs to be updated
    // now it is very DIM_MAXDIM == 3 dependent

    threeDim s3d = m_aIndexes[iWhichOne];

    dOccur[0] = s3d[0];
    dOccur[1] = s3d[1];
    dOccur[2] = s3d[2];
}

//////////////////////////////////////////////////////////////////////////

CIterator::CIterator() {
    Clean();
}

CIterator::~CIterator() {
}

void  CIterator::SetEngineDriver( CEngineDriver* pEngineDriver ) {
    m_pEngineDriver = pEngineDriver;
    ASSERT( pEngineDriver != 0 );
    m_pEngineArea = pEngineDriver->getEngineAreaPtr();
    m_engineData = &m_pEngineArea->GetEngineData();
    m_pIntDriver = m_pEngineDriver->m_pIntDriver.get();
}

void CIterator::Clean() {
    for( int i = 0; i < DIM_MAXDIM; i++ ) {
        m_iLow[i] = 0;
        m_iHigh[i] = 0;
        m_pGroupT[i] = NULL;
        m_iDimFlag[i] = CITERATOR_FLAG_NONE;
    }
}

bool CIterator::Make( MVAR_NODE* pMultVarNode, bool bStackMode ) {
    bool  bRet  = true;
    VART* pVarT = VPT(pMultVarNode->m_iVarIndex);

    //    Clean();

    //mvarGetSubindexes( pMultVarNode, dIndex );
    double dIndexLow[DIM_MAXDIM];
    double dIndexHigh[DIM_MAXDIM];

    int iDim;
    for( iDim = 0; iDim < pVarT->GetNumDim(); iDim++ ) {
        GROUPT* pGroupTAux = NULL;
        CDimension::VDimType xType = pVarT->GetDimType( iDim );
        bool    bValidDimRef = (
            xType == CDimension::Record  ||
            xType == CDimension::Item    ||
            xType == CDimension::SubItem
            );

        ASSERT( bValidDimRef );

        int iExprType  = pMultVarNode->m_iVarSubindexType[iDim];
        int iExprValue = pMultVarNode->m_iVarSubindexExpr[iDim];

        if( iExprType == MVAR_CONSTANT || iExprType == MVAR_EXPR ) { // rcl, Jul 24 2004
            m_iDimFlag[xType] = CITERATOR_FLAG_EXPLICIT;

            int iExprFinalValue = GetExpressionValue( iExprValue, iExprType, CITERATOR_ONE_BASED_RESULT );
            dIndexLow[iDim] = dIndexHigh[iDim] = iExprFinalValue;

            // Could be optimized if we keep the precalculated dimension in the GROUP
            GROUPT*  pParentGroupT = pVarT->GetParentGPT();
            while( pParentGroupT != NULL  ) {
                if( pParentGroupT->GetDimType() == xType ) {
                    pGroupTAux = pParentGroupT;
                    if( iExprFinalValue > pGroupTAux->GetTotalOccurrences() )
                        bRet = false;
                    break;
                }
                pParentGroupT = pParentGroupT->GetParentGPT();
            }
        }
        else
        if( iExprType == MVAR_GROUP ) {
            m_iDimFlag[xType] = CITERATOR_FLAG_IMPLICIT;

            int iSymGroup = iExprValue;

            pGroupTAux = GPT(iSymGroup);

            // Set as maximum
            dIndexLow[iDim]  = 1;
            dIndexHigh[iDim] = pGroupTAux->GetTotalOccurrences();

            // RHF INIC Oct 29, 2001
            // Check stack for a better approach
            if( bStackMode ) {
                double dIndex[DIM_MAXDIM];
                int    iIndexType[DIM_MAXDIM];
                m_pIntDriver->mvarGetSubindexes( pMultVarNode, dIndex, iIndexType );

                // dIndexType can be 'E' (Expresion) 'G' (Group), 'M' (Multiple Relation), 'R' (Relation) or ' ' (None)
                if( iIndexType[iDim] == 'M' )
                    issaerror( MessageType::Abort, 33108 );
                else  if( iIndexType[iDim] != ' ' ) // Was resolved by the stack
                    dIndexLow[iDim] = dIndexHigh[iDim] = dIndex[iDim];
            }
            // RHF END Oct 29, 2001

            if( dIndexHigh[iDim] <= 0 ) {
                bRet = false;
                break;
            }
        }
        else if( iExprType == '?' ) {
            m_iDimFlag[xType] = CITERATOR_FLAG_EXPLICIT; // rcl, Mar 11, 2005

            dIndexLow[iDim] = dIndexHigh[iDim] = iExprValue; // RHF Oct 29, 2001

            if( dIndexLow[iDim] > dIndexHigh[iDim] )
                bRet = false;
        }
        else {
            ASSERT(0);
            m_iDimFlag[xType] = CITERATOR_FLAG_NONE;
            bRet = false;
        }

        if( bRet && bValidDimRef )
            m_pGroupT[xType] = pGroupTAux;
    }

    for( ; iDim < DIM_MAXDIM; iDim++ ) {
        dIndexLow[iDim]  = 0;
        dIndexHigh[iDim] = 0;
    }

    VARX* pVarX = pVarT->GetVarX();

    if( bRet )
    {
        if( pVarX->RemapIndexes( m_iLow, dIndexLow ) == false ||
            pVarX->RemapIndexes( m_iHigh, dIndexHigh ) == false )
            bRet = false;
    }

    return bRet;
}

void CIterator::GetLow( int* iLow ) const {
    ASSERT( iLow != 0 );
    for( int iDim = 0; iDim < DIM_MAXDIM; iDim++ )
        iLow[iDim] = m_iLow[iDim];
}

void CIterator::GetHigh( int* iHigh ) const {
    ASSERT( iHigh != 0 );
    for( int iDim = 0; iDim < DIM_MAXDIM; iDim++ )
        iHigh[iDim] = m_iHigh[iDim];
}

void CIterator::GetGroupT( GROUPT* pGroupT[DIM_MAXDIM] ) {
    for( int iDim=0; iDim < DIM_MAXDIM; iDim++ )
        pGroupT[iDim] = m_pGroupT[iDim];
}

void CIterator::GetExplicit( bool* bExplicit ) {
    ASSERT( bExplicit != 0 );
    for( int iDim = 0; iDim < DIM_MAXDIM; iDim++ )
        bExplicit[iDim] = ( m_iDimFlag[iDim] & CITERATOR_FLAG_EXPLICIT ) != 0;
}

void CIterator::GetLessTotal( bool* bLessTotal ) {
    ASSERT( bLessTotal != 0 );
    for( int iDim = 0; iDim < DIM_MAXDIM; iDim++ )
        bLessTotal[iDim] = ( m_iDimFlag[iDim] & CITERATOR_FLAG_LESS_TOTAL ) != 0;
}

void CIterator::GetConstant( bool* bConstant ) {
    ASSERT( bConstant != 0 );
    for( int iDim=0; iDim < DIM_MAXDIM; iDim++ )
        bConstant[iDim] = (m_iDimFlag[iDim] & CITERATOR_FLAG_CONSTANT) != 0;
}

void CIterator::GetDimFlag( int* iDimFlag ) {
    ASSERT( iDimFlag != 0 );
    for( int iDim = 0; iDim < DIM_MAXDIM; iDim++ )
        iDimFlag[iDim] = m_iDimFlag[iDim];
}

void CIterator::Copy( CIterator* pIterator ) {
    ASSERT( pIterator != 0 );
    pIterator->GetLow( m_iLow );
    pIterator->GetHigh( m_iHigh );
    pIterator->GetGroupT( m_pGroupT );
}

bool CIterator::Compare( CIterator*  pIterator, bool bIgnoreExplicit ) {
    ASSERT( pIterator != 0 );
    for( int iDim = 0; iDim < DIM_MAXDIM; iDim++ ) {
        if( GetLow(iDim) != pIterator->GetLow(iDim) ||
            GetHigh(iDim) != pIterator->GetHigh(iDim) ||
            GetGroupT(iDim) != pIterator->GetGroupT(iDim) ||
            ( !bIgnoreExplicit && isExplicit(iDim) != pIterator->isExplicit(iDim) )
            )
            return false;
    }

    return true;
}

bool CIterator::MakeExportIterator( MVAR_NODE*  pMultVarNode, bool bEvaluateItem, bool bUseTotalRecord ) {
    bool bRet = true;

    ASSERT( pMultVarNode != 0 );
    ASSERT( NPT(pMultVarNode->m_iVarIndex)->IsA(SymbolType::Variable) );

    VART* pVarT = VPT(pMultVarNode->m_iVarIndex);

    ASSERT( pVarT->IsArray() );

    double dIndexLow[DIM_MAXDIM];
    double dIndexHigh[DIM_MAXDIM];

    CNDIndexes theIndex( ZERO_BASED, DEFAULT_INDEXES );
    switch( pVarT->GetNumDim() )
    {
    case 1: theIndex.specifyIndexesUsed( USE_DIM_1 ); break;
    case 2: theIndex.specifyIndexesUsed( USE_DIM_1 | USE_DIM_2 ); break;
    case 3: theIndex.specifyIndexesUsed( USE_ALL_DIM ); break;
    default: ASSERT(0); break;
    }

    double  dIndex;
    int iDim;
    for( iDim = 0; bRet && iDim < pVarT->GetNumDim(); iDim++ ) {
        GROUPT*     pGroupTAux=NULL;
        CDimension::VDimType xType = pVarT->GetDimType( iDim );
        bool    bValidDimRef = (
            xType == CDimension::Record  ||
            xType == CDimension::Item    ||
            xType == CDimension::SubItem
            );

        ASSERT( bValidDimRef );

        int iExprType = pMultVarNode->m_iVarSubindexType[iDim];
        if( iExprType == MVAR_CONSTANT || iExprType == MVAR_EXPR ) {
            m_iDimFlag[xType] = CITERATOR_FLAG_EXPLICIT;

            // RHF INIC Nov 09, 2004
            bool bIsConstant = ( iExprType == MVAR_CONSTANT );
            if( !bIsConstant ) {
                int iPossibleConstExpr = pMultVarNode->m_iVarSubindexExpr[iDim];
                const CONST_NODE* pNode = reinterpret_cast<const CONST_NODE*>(m_pEngineArea->GetLogicByteCode().GetCodeAtPosition(iPossibleConstExpr));
                if( pNode->const_type == CONST_CODE )
                    bIsConstant = true;
            }

            // always evaluate the constant because the value is going to be
            // used to generate the varname
            // (example: export repeat(1), repeat(2) are generated as
            //                  repeat_1 & repeat_2 )
            if( bIsConstant ) {
                bEvaluateItem = true;
                m_iDimFlag[xType] |= CITERATOR_FLAG_CONSTANT;
            }
            // RHF END Nov 09, 2004

            if( bEvaluateItem )
            {
                if( bIsConstant )
                    dIndex = pMultVarNode->m_iVarSubindexExpr[iDim];
                else
                    dIndex = m_pIntDriver->evalexpr( pMultVarNode->m_iVarSubindexExpr[iDim] );
            }
            else
                dIndex = 1;

            dIndexLow[iDim] = dIndexHigh[iDim] = (int) dIndex;
            theIndex.setIndexValue( iDim, (int) dIndex - 1 );

            // Could be optimized if we keep the precalculated dimension in the GROUP
            GROUPT*  pParentGroupT = pVarT->GetParentGPT();
            while( pParentGroupT != NULL  ) {
                if( pParentGroupT->GetDimType() == xType ) {
                    pGroupTAux = pParentGroupT;

                    int iLimitTotal = pGroupTAux->GetTotalOccurrences( theIndex );
                    int iLimit = (xType == CDimension::Record && bUseTotalRecord) ?
                                iLimitTotal :
                                pVarT->GetDimSize(iDim);

                    if( dIndex >= 1 && dIndex <= iLimitTotal )
                        m_iDimFlag[xType] |= CITERATOR_FLAG_LESS_TOTAL;

                    if( dIndex <= 0 || dIndex > iLimit ) {
                        // RHF INIC Feb 21, 2005
                        m_iDimFlag[xType] |= CITERATOR_FLAG_INVALID_DIM;
                        dIndexLow[iDim] = dIndexHigh[iDim] = 1;
                        // RHF END Feb 21, 2005
                        // RHF COM Feb 21, 2005 bRet = false;
                    }
                    break;
                }
                pParentGroupT = pParentGroupT->GetParentGPT();
            }
        }
        else {
            m_iDimFlag[xType] = CITERATOR_FLAG_IMPLICIT;

            int iSymGroup = pMultVarNode->m_iVarSubindexExpr[iDim];

            pGroupTAux = GPT(iSymGroup);

            ASSERT( pGroupTAux != 0 );
            int iLimit= (xType == CDimension::Record && bUseTotalRecord) ?
                        pGroupTAux->GetTotalOccurrences( theIndex ) :
                        pVarT->GetDimSize(iDim);

            dIndexLow[iDim]  = 1;
            dIndexHigh[iDim] = iLimit;

            if( dIndexHigh[iDim] <= 0 )
                bRet = false;
        }

        if( bRet && bValidDimRef )
            m_pGroupT[xType] = pGroupTAux;
    }

    for( ; iDim < DIM_MAXDIM; iDim++ ) {
        dIndexLow[iDim] = 0;
        dIndexHigh[iDim] = 0;
    }

    VARX* pVarX = pVarT->GetVarX();

    if( bRet )
    {
        if( pVarX->RemapIndexes( m_iLow, dIndexLow, false ) == false ||
            pVarX->RemapIndexes( m_iHigh, dIndexHigh, false ) == false )
            bRet = false;
    }

    return bRet;
}

// GetExpressionValue returns a 0 based or 1 based index value depending on
// the value of bZeroBasedResult parameter
// bZeroBasedResult can be CITERATOR_ZERO_BASED_RESULT or CITERATOR_ONE_BASED_RESULT
int CIterator::GetExpressionValue( int iExpression, int iType, bool bZeroBasedResult )
{
    int iRet = 0;
    int iSub = 0; // amount to substract to result value

    // original values are 1 based, so if user wants 0 based we have to substract 1
    if( bZeroBasedResult == CITERATOR_ZERO_BASED_RESULT )
        iSub = 1;

    ASSERT( iType == MVAR_CONSTANT || iType == MVAR_EXPR || iType == '?' );
    if( iType == MVAR_CONSTANT || iType == '?' )
    {
        ASSERT( iExpression > 0 );
        // iExpression is 1 based and dRet is 0 based
        iRet = iExpression - iSub;
    }
    else
    {
        ASSERT( iType == MVAR_EXPR );
        double dExpr = m_pIntDriver->evalexpr( iExpression );
        iRet = (int) dExpr - iSub;
    }

    return iRet;
}

bool CIterator::MakeTableIterator( MVAR_NODE* pMultVarNode, bool bEvaluateItem, bool bUseTotalRecord )
{
    TRACE( _T(".............................................\n") );
    ASSERT( pMultVarNode != 0 );

    ASSERT( NPT(pMultVarNode->m_iVarIndex)->IsA(SymbolType::Variable) );
    VART* pVarT = VPT(pMultVarNode->m_iVarIndex);
    ASSERT( pVarT->IsArray() );

    // i, ti [ti = "type of i"] is first dimension
    int i  = pMultVarNode->m_iVarSubindexExpr[0];
    int ti = pMultVarNode->m_iVarSubindexType[0];
    // j, tj [tj = "type of j"] is second dimension
    int j  = pMultVarNode->m_iVarSubindexExpr[1];
    int tj = pMultVarNode->m_iVarSubindexType[1];

    CDimension::VDimType xType = CDimension::VoidDim;
    // pMultVarNode->m_iVarSubindexExpr[2]; is not being used this time

    switch( pVarT->GetNumDim() )
    {
    case 1:
    {
        ASSERT( pMultVarNode->m_iVarSubindexType[0] == MVAR_CONSTANT );
        Add3DIndex( threeDim(i,0,0) );
        xType = pVarT->GetDimType(0);
        m_iDimFlag[xType] = CITERATOR_FLAG_EXPLICIT;
    }
        break;
    case 2:
        {
            ASSERT( ti == MVAR_CONSTANT || ti == MVAR_EXPR || ti == '?' );

            int iValueForI = GetExpressionValue( i, ti, CITERATOR_ZERO_BASED_RESULT );
            if( tj == MVAR_CONSTANT || tj == MVAR_EXPR || tj == '?' )
            {
                int iValueForJ = GetExpressionValue( j, tj, CITERATOR_ZERO_BASED_RESULT );
                Add3DIndex( threeDim( iValueForI + 1, iValueForJ + 1, 0 ) );
            }
            else
            {
                CNDIndexes theIndex( ZERO_BASED, DEFAULT_INDEXES );
                theIndex.specifyIndexesUsed( USE_DIM_1 );
                theIndex.setIndexValue( 0, iValueForI );

                GROUPT* pGroupT = pVarT->GetOwnerGPT();
                int iNewjTotal = pGroupT->GetDataOccurrences( theIndex );
                for( int iNewj = 1; iNewj <= iNewjTotal; iNewj++ )
                    Add3DIndex( threeDim( iValueForI + 1, iNewj, 0 ) );
            }
        }
        break;
    case 3:
        {
            // notice that the 1st dimension will always be a constant or
            // an expression, because we have a restriction where there are
            // more than 1 multiple dimension: it always has to iterate over
            // the leftmost one, so it has to get here with a constant or
            // an expression and never with a group.
            // rcl, Nov 02 2004
            CNDIndexes theIndex( ZERO_BASED, DEFAULT_INDEXES );
            theIndex.specifyIndexesUsed( USE_DIM_1 | USE_DIM_2 );

            ASSERT( ti == MVAR_CONSTANT || ti == MVAR_EXPR || ti == '?' );

            int iValueForI = GetExpressionValue( i, ti, CITERATOR_ZERO_BASED_RESULT );
            theIndex.setIndexValue( 0, iValueForI );

            GROUPT* p2dGroupt = pVarT->GetOwnerGPT();
            ASSERT( p2dGroupt != 0 );
            GROUPT* p3dGroupt = p2dGroupt->GetOwnerGPT();
            ASSERT( p3dGroupt != 0 );

            ASSERT( p3dGroupt->GetDimType() == CDimension::Record );

            if( tj == MVAR_GROUP )
            {
                // iterate in 2 loops
                int iNewjTotal = p3dGroupt->GetDataOccurrences( theIndex );
                for( int iNewJ = 1; iNewJ <= iNewjTotal; iNewJ++ )
                {
                    ASSERT( p2dGroupt->GetDimType() == CDimension::Item );

                    theIndex.setIndexValue( 1, iNewJ );

                    int iNewkTotal = p2dGroupt->GetDataOccurrences( theIndex );
                    for( int iNewK = 1; iNewK <= iNewkTotal; iNewK++ )
                        Add3DIndex( threeDim( iValueForI + 1, iNewJ, iNewK ) );
                }
            }
            else
            {
                // iterate only in 1 loop
                ASSERT( tj == MVAR_CONSTANT || tj == MVAR_EXPR || tj == '?' );
                int iValueForJ = GetExpressionValue( j, tj, CITERATOR_ZERO_BASED_RESULT );
                theIndex.setIndexValue( 1, iValueForJ );

                int iNewkTotal = p3dGroupt->GetDataOccurrences( theIndex );
                for( int iNewK = 1; iNewK <= iNewkTotal; iNewK++ )
                    Add3DIndex( threeDim( iValueForI + 1, iValueForJ + 1, iNewK ) );
            }
        }
        break;
    default: ASSERT(0); break;
    }

    TRACE( _T("................................ [%d cases] ...\n"), size() );
    return true;
}


const Logic::SymbolTable& CIterator::GetSymbolTable() const
{
    return m_engineData->symbol_table;
}
