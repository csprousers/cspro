#include "StandardSystemIncludes.h"
#include "Comp.h"
#include <strstream>

//////////////////////////////////////////////////////////////////////////
// VarAnalysis Helper class

CEngineCompFunc::VarAnalysis::VarAnalysis( int iNumDim )
    : m_iNumDim(iNumDim)
{
    ASSERT( m_iNumDim > 0 ); // only useful for multiple variables

    memset( m_aSubindexExpr, 0, DIM_MAXDIM * sizeof(int) );// RHF Aug 04, 2000

    for( int i = 0; i < DIM_MAXDIM; i++ )
    {
        m_abIsExprConstant[i] = false;
        m_abIsImplicit[i] = false;
    }

    m_iSubIndexCount = 0;
    m_iExplicitIndexes = 0;
}

// private:
void CEngineCompFunc::VarAnalysis::setIndexValue( int iIndex, int iValue )
{
    ASSERT( iIndex >= 0 && iIndex < DIM_MAXDIM );
    m_aSubindexExpr[iIndex] = iValue;
}

void CEngineCompFunc::VarAnalysis::addIndexValue( int iValue )
{
    int iIndex = m_iSubIndexCount;
    ASSERT( iIndex >= 0 && iIndex < DIM_MAXDIM );
    setIndexValue( iIndex, iValue );
    m_iSubIndexCount++;
    m_iExplicitIndexes++;
}

void CEngineCompFunc::VarAnalysis::addImplicitIndex()
{
    addIndexValue( -1 );   // increments SubIndexCount and ExplicitIndexes
    m_iExplicitIndexes--;  // but now.. only SubIndexCount has been increment
    m_abIsImplicit[m_iSubIndexCount-1] = true;
}

void CEngineCompFunc::VarAnalysis::setPreviousAsConstant() // do not change previous inserted value
{
    int iIndex = m_iSubIndexCount - 1;
    ASSERT( iIndex >= 0 && iIndex < DIM_MAXDIM );
    // have to undo the count of explicit index if we
    // are going to set another value
    if( m_abIsImplicit[iIndex] )
    {
        m_abIsImplicit[iIndex] = false;
        m_iExplicitIndexes++;
    }

    m_abIsExprConstant[iIndex] = true;
}

void CEngineCompFunc::VarAnalysis::setPreviousAsConstant( int iValue ) // change previous inserted value to specified
{
    int iIndex = m_iSubIndexCount - 1;
    setPreviousAsConstant();
    setIndexValue( iIndex, iValue );
}

bool CEngineCompFunc::VarAnalysis::isConstant( int iIndex )
{
    ASSERT( iIndex >= 0 && iIndex < DIM_MAXDIM );
    return m_abIsExprConstant[iIndex];
}

bool CEngineCompFunc::VarAnalysis::canAddAnotherIndex( bool bCompleteCompilation )
{
    // the number of subindexes must be lower thant the total number
    // of indexes
    bool bCanAddOneMore = ( m_iSubIndexCount < m_iNumDim );

    // and if user has specified no complete compilation, then
    // the total number of explicit indexes that can be specified is one
    // less than the total number of indexes the current variable has.
    if( bCanAddOneMore && !bCompleteCompilation )
    {
        bCanAddOneMore = m_iExplicitIndexes < (m_iNumDim - 1);
    }

    return bCanAddOneMore;
}

bool CEngineCompFunc::VarAnalysis::hasTooManyIndexes()
{
    bool bHasTooMany = m_iSubIndexCount > m_iNumDim;

    return bHasTooMany;
}

bool CEngineCompFunc::VarAnalysis::thisIndexIsSpecified( int iIndex )
{
    return iIndex < m_iSubIndexCount && (m_abIsImplicit[iIndex] == false);
}

bool CEngineCompFunc::VarAnalysis::allIndexesAreSpecified()
{
    bool bAllSpecified = (m_iSubIndexCount == m_iNumDim);

    return bAllSpecified;
}

bool CEngineCompFunc::VarAnalysis::allIndexesAreExplicit()
{
    bool bAllExplicit = (m_iExplicitIndexes == m_iNumDim);

    return bAllExplicit;
}

int CEngineCompFunc::VarAnalysis::howManyIndexesSpecified() const
{
    return m_iSubIndexCount;
}

// checks if index limits are correct.
// it is incorrect to have
//  m_iSubIndexCount > m_iNumDim, i.e. more indexes than needed or
//  m_iExplicitIndexes < m_iSubIndexCount    [some implicit indces]
//     and !allIndexesSpecified()
//   [when an implicit index is used, all indexes must be specified]
bool CEngineCompFunc::VarAnalysis::indexLimitsAreCorrect( int* pError )
{
    bool bCorrect = true;

    if( m_iSubIndexCount > m_iNumDim )
    {
        bCorrect = false;
        if( pError != 0 )
            *pError = ERROR_TOO_MANY_SUBINDEXES;
    }

    // if user ommits a dimension, he/she is commanded to complete all dimensions
    // rcl, Aug 04, 2004
    if( bCorrect )
    {
        if( m_iExplicitIndexes < m_iSubIndexCount )
        {
            if( !allIndexesAreSpecified() )
            {
                bCorrect = false;
                if( pError != 0 )
                    *pError = ERROR_NOT_ENOUGH_SUBINDEXES;
            }
        }
    }

    return bCorrect;
}

void CEngineCompFunc::VarAnalysis::copyTo( int* aIndexes, int *aTypes )
{
    ASSERT( aIndexes != 0 );
    ASSERT( aTypes != 0 );

    for( int i = 0; i < m_iNumDim; i++ )
        copyTo_OnlyOneIndexDirect( aIndexes, aTypes, i );
}

void CEngineCompFunc::VarAnalysis::copyTo_OnlyOneIndexDirect( int* aIndexes, int *aTypes, int iIndex )
{
    copyTo_OnlyOneIndex( aIndexes, aTypes, iIndex, iIndex );
}

void CEngineCompFunc::VarAnalysis::copyTo_OnlyOneIndex( int* aIndexes, int *aTypes,
                                                       int iToIndex, int iFromIndex )
{
    ASSERT( aIndexes != 0 );
    ASSERT( aTypes != 0 );
    ASSERT( iToIndex >= 0 && iToIndex < m_iNumDim );
    ASSERT( iFromIndex >= 0 && iFromIndex < m_iSubIndexCount );

    // Interpreter speed up, part 2
    // specify integer CONSTANTS where they have been used
    // rcl, Jul 24, 2004
    if( isConstant(iFromIndex) )
        aTypes[iToIndex] = MVAR_CONSTANT;
    else
        aTypes[iToIndex] = MVAR_EXPR;
    aIndexes[iToIndex] = m_aSubindexExpr[iFromIndex];
}

bool CEngineCompFunc::VarAnalysis::changeImplicitToConstOk( int iIndex, int iValue )
{
    ASSERT( iIndex >= 0 && iIndex < m_iNumDim );
    if( iIndex < 0 || iIndex >= m_iNumDim )
        return false;

    if( !m_abIsImplicit[iIndex] )
    {
        if( iIndex == m_iSubIndexCount )
        {
            addIndexValue(iValue);
            setPreviousAsConstant();
            return true;
        }
        return false;
    }

    m_abIsImplicit[iIndex] = false;
    m_abIsExprConstant[iIndex] = true;
    m_aSubindexExpr[iIndex] = iValue;
    m_iExplicitIndexes++;

    return true;
}
