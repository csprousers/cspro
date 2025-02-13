//---------------------------------------------------------------------------
//  File name: CsKernel.cpp
//
//  Description:
//          Implementation for the kernel of the 3-D driver and all his clients
//
//  History:    Date       Author   Comment
//              ---------------------------
//              23 Jan 01   vc      Created for final expansion to 3 dimensions
//                                  (initially placed at CsDriver.h)
//              13 Jun 04   rcl     Adds C3DIndexes class
//              01 Jul 04   rcl     create C3DObject_Base template,
//                                  C3DObject is a specialization of that template
//
//---------------------------------------------------------------------------
#include "StdAfx.h"

#include "CsKernel.h"

//////////////////////////////////////////////////////////////////////////
#include <sstream>

C3DIndexes::C3DIndexes()
    : m_iIndexNumber(DIM_MAXDIM),
      m_bHasBeenInitialized(false)
{
    m_iBase = -1;
    willUseManyDimensions();
    m_iUseThisDimension = USE_ALL_DIM;
}

C3DIndexes::C3DIndexes( int iBase )
    : m_iBase(iBase),
      m_iIndexNumber(DIM_MAXDIM),
      m_bHasBeenInitialized(false)
{
    // only accepting ZERO_BASED and ONE_BASED objects
    ASSERT( iBase == ZERO_BASED || iBase == ONE_BASED );
    m_iEmptyValue = m_iBase - 1;
    willUseManyDimensions();
    m_iUseThisDimension = USE_ALL_DIM;
}

C3DIndexes::C3DIndexes( int iBase, const int *pIndexArray  )
    : m_iBase( iBase ),
      m_iIndexNumber(DIM_MAXDIM),
      m_bHasBeenInitialized(true)
{
    // only accepting ZERO_BASED and ONE_BASED objects
    ASSERT( iBase == ZERO_BASED || iBase == ONE_BASED );
    m_iEmptyValue = m_iBase - 1;
    setIndexes( pIndexArray );
    willUseManyDimensions();
    m_iUseThisDimension = USE_ALL_DIM;
}

// iMaskDimension is one of the constants defined in cskernel.h
bool C3DIndexes::isUsingThisDimension( int iMaskDimension ) const
{
    return ((m_iUseThisDimension & iMaskDimension) == iMaskDimension);
}

void C3DIndexes::specifyIndexesUsed( int iIndexesUsed )
{
    m_iUseThisDimension = iIndexesUsed;
}

void C3DIndexes::getIndexes( int* pIndexArray, int iSize ) const
{
    ASSERT( m_bHasBeenInitialized );
    if( pIndexArray != 0 )
    {
        if( iSize != -1 && iSize != DIM_MAXDIM )
            ASSERT( 0 );

        for( int i = 0; i < DIM_MAXDIM; i++ )
            pIndexArray[i] = m_aIndex[i];
    }
}

void C3DIndexes::setEmpty()
{
    for( int i = 0; i < DIM_MAXDIM; i++ )
        m_aIndex[i] = m_iEmptyValue;
}

void C3DIndexes::setAtOrigin()
{
    ASSERT(m_iBase == ( m_iEmptyValue + 1 ));

    for( int i = 0; i < DIM_MAXDIM; i++ )
        m_aIndex[i] = m_iEmptyValue + 1;

    setAsInitialized();
}

bool C3DIndexes::isEmpty() const
{
    ASSERT( m_bHasBeenInitialized );
    bool bEmpty = true;

    for( int i = 0; i < DIM_MAXDIM; i++ )
    {
        if( m_aIndex[i] != m_iEmptyValue )
        {
            bEmpty = false;
            break;
        }
    }

    return bEmpty;
}

void C3DIndexes::setIndexes( const int* pIndexArray )
{
    if( pIndexArray == DEFAULT_INDEXES )
    {
        setEmpty();
    }
    else
    {
        for( int i = 0; i < DIM_MAXDIM; i++ )
            m_aIndex[i] = pIndexArray[i];
    }
    setAsInitialized();
}

void C3DIndexes::setIndexValue( int iWhichDim, int iWhichValue )
{
    // GHM 20131223 removed the below assert because it was getting triggered from GROUPT::DoMoveOcc
    // ASSERT( iWhichDim >= 0 && iWhichDim < DIM_MAXDIM );
    /*
    if( !isZeroBased() && iWhichValue == 0 )
        ASSERT(0);
    */
    if( iWhichDim >= 0 && iWhichDim < DIM_MAXDIM ) // BUG! Fixed
    m_aIndex[iWhichDim] = iWhichValue;
}

void C3DIndexes::setIndexValue( CDimension::VDimType xType, int iWhichValue )
{
    if( !isZeroBased() && iWhichValue == 0 )
        ASSERT(0);

    m_aIndex[xType] = iWhichValue;
}


static std::wstring skipNoNumbers(std::wstring strText)
{
    std::wstring strResult = strText;

    while( strText.size() > 0 && !_istdigit(strText[0]) )
    {
        strResult = strText.erase(0,1);
        strText = strResult;
    }

    return strResult;
}

void C3DIndexes::setIndexValuesFromString( std::wstring strOcc, csprochar cSeparator )
{
    std::wstring strNewOcc = strOcc;
    bool bContinue = true;
    int iIndex = 0;
    while( bContinue )
    {
        strNewOcc = skipNoNumbers(strNewOcc);

        if( strNewOcc.size() <= 0 )
            break;

        int iIndexValue = 0;
        int iDotPosition = strNewOcc.find(cSeparator);
        if( iDotPosition != std::wstring::npos ) // i.e. = some separator present
        {
            iIndexValue = _tstof( strNewOcc.substr( 0, iDotPosition ).c_str() );
            std::wstring strDeleted = strNewOcc.erase( 0, iDotPosition + 1 );
            strNewOcc = strDeleted;
        }
        else // no separator, but data found
        {
            iIndexValue = _tstof( strNewOcc.c_str() );
            bContinue = false;
        }
        setIndexValue( iIndex, iIndexValue );
        iIndex++;
        if( iIndex >= DIM_MAXDIM )
            bContinue = false;
    }
}

int C3DIndexes::getIndexValue( int iWhichDim ) const
{
    ASSERT(m_bHasBeenInitialized);
    ASSERT( iWhichDim >= 0 && iWhichDim < DIM_MAXDIM );
    ASSERT( iWhichDim == 0 || (iWhichDim > 0 && !isUsingOnlyOneDimension()) );
    return m_aIndex[iWhichDim];
}

int C3DIndexes::getIndexValue( CDimension::VDimType xType ) const
{
    ASSERT( m_bHasBeenInitialized );
    return m_aIndex[xType];
}

// Consider "lowest" available index
// rcl, Jun 15, 2004
int C3DIndexes::searchNotEmptyValue() const
{
    ASSERT( m_bHasBeenInitialized );
    int iRetValue = m_iEmptyValue;
    for( int i = DIM_MAXDIM - 1; i >= 0; i-- )
    {
        int iValue = getIndexValue(i);
        if( iValue != m_iEmptyValue )
        {
            iRetValue = iValue;
            break;
        }
    }

    return iRetValue;
}

//////////////////////////////////////////////////////////////////////////
// operators

C3DIndexes& C3DIndexes::operator=( const C3DIndexes& other )
{
    ASSERT( other.m_bHasBeenInitialized );
    if( &other != this )
    {
        m_bHasBeenInitialized = true;
        ASSERT( other.m_iBase == ZERO_BASED || other.m_iBase == ONE_BASED );
        m_iBase = other.m_iBase;
        m_iEmptyValue = other.m_iEmptyValue;
        m_iIndexNumber = other.getIndexNumber();
        other.getIndexes( m_aIndex, getIndexNumber() );
        m_iUseThisDimension = other.m_iUseThisDimension;
    }

    return *this;
}

bool C3DIndexes::operator==( const C3DIndexes& other ) const
{
    ASSERT( this->m_bHasBeenInitialized );
    ASSERT( other.m_bHasBeenInitialized );

    bool bResult = (m_iBase == other.m_iBase );
    if( bResult )
    {
        for( int i = 0; i < DIM_MAXDIM; i++ )
        {
            if( getIndexValue(i) != other.getIndexValue(i) )
            {
                bResult = false;
                break;
            }
        }
    }

    return bResult;
}

int C3DIndexes::Precedence( C3DIndexes& other ) const
{
    ASSERT( other.m_bHasBeenInitialized );
    ASSERT( this->m_bHasBeenInitialized );

    // remark - only for same 0 based or 1 based information
    //          0: identical, -1: low-index-of-this, +1: high-index-of-this
    ASSERT( other.m_iBase == m_iBase );

    int     iPrecedence = 0;    // assuming identical

    for( int iDim = 0; iPrecedence == 0 && iDim < DIM_MAXDIM; iDim++ )
    {
        if( getIndexValue( iDim ) < other.getIndexValue( iDim ))
            iPrecedence = -1;   // this has a lower index
        else if( getIndexValue(iDim) > other.getIndexValue(iDim) )
            iPrecedence = 1;    // this has a higher index
    }

    return iPrecedence;
}


std::wstring C3DIndexes::toStringBare() const
{
    ASSERT( m_bHasBeenInitialized );
    std::wstring strResult;

    // if this index is zero based, when converting to string
    // we will show it as 1 based
    int iSum = 0;
    if( isZeroBased() )
        iSum = 1;

#ifdef UNICODE
    std::basic_ostringstream<wchar_t> strBuf;
#else
    ostrstream strBuf;
#endif

    strBuf << _T("(") << (m_aIndex[0]+iSum)
           << _T(",") << (m_aIndex[1]+iSum)
           << _T(",") << (m_aIndex[2]+iSum)
           << _T(")") << std::ends;

    strResult = strBuf.str();

    return strResult;
}

std::wstring C3DIndexes::toString(int iNumDim,
                                  csprochar cLeft ,       // defaults to '('
                                  csprochar cRight,       // defaults to ')'
                                  csprochar cSep ) const  // defaults to ','
{
    // if this index is zero based, when converting to string
    // we will show it as 1 based
    int iSum = 0;
    if( isZeroBased() )
        iSum = 1;

    if( iNumDim <= 0 ) {
        return _T("");
    }

    ASSERT( m_bHasBeenInitialized );

#ifdef UNICODE
    std::basic_ostringstream<wchar_t> strBuf;
#else
    ostrstream strBuf;
#endif

    if( cLeft != NO_PARENTHESIS )
        strBuf << cLeft;

    bool bSomeThingWritten = false;
    #define SH(x,y)  \
    if( isUsingThisDimension(x) ) { \
        if( bSomeThingWritten ) \
            strBuf << cSep; \
        strBuf << m_aIndex[y] + iSum; \
        bSomeThingWritten = true; \
        iNumDim--; \
    }

    SH( USE_DIM_1, 0 );
    SH( USE_DIM_2, 1 );
    SH( USE_DIM_3, 2 );

    if( cRight != NO_PARENTHESIS )
        strBuf << cRight;

    strBuf << std::ends;

    std::wstring strResult = strBuf.str();

    return strResult;
}

// int decrease( int iValue )
//     substracts 1 for numbers greater than 1
//     returns 0 for all the other cases
static
int internal_decrease( int iValue )
{
    int iRet = 0;

    if( iValue > 0 )
        iRet = iValue - 1;

    return iRet;
}

void C3DIndexes::decrease( C3DIndexes& theNewIndex ) const // rcl Jun 2004
{
    // ASSERT( theNewIndex.m_bHasBeenInitialized );
    theNewIndex.m_iBase = internal_decrease(m_iBase);
    ASSERT( theNewIndex.m_iBase == ZERO_BASED ||
            theNewIndex.m_iBase == ONE_BASED );
    theNewIndex.m_iEmptyValue = theNewIndex.m_iBase - 1;
    theNewIndex.m_iIndexNumber = this->getIndexNumber();
    theNewIndex.setAsInitialized();
    ASSERT( theNewIndex.m_iIndexNumber <= DIM_MAXDIM );
    for( int i = 0; i < m_iIndexNumber; i++ )
        theNewIndex.setIndexValue( i, internal_decrease( getIndexValue(i) ) );
}

void C3DIndexes::setHomePosition() // rcl Jun 25, 2004
{
    // valid base?
    if( m_iBase != ZERO_BASED && m_iBase != ONE_BASED )
        // no -> do not change initialized state then
        return;

    // faster version than loop one (shown below)
    ASSERT( getIndexNumber() == 3 );
    setIndexValue( 0, m_iBase );
    setIndexValue( 1, m_iBase );
    setIndexValue( 2, m_iBase );

    setAsInitialized();

/* Should have been done more generally like this
    for( int i = 0; i < this->getIndexNumber(); i++ )
        setIndexValue( i, m_iBase );
*/
}

bool C3DIndexes::isValid() const
{
    return
        getIndexValue( 0 ) >= 0 &&
        getIndexValue( 1 ) >= 0 &&
        getIndexValue( 2 ) >= 0;
}

void C3DIndexes::useOnlyOneDimension( int iDimValue )
{
    willUseOnly1Dimension();
    setIndexValue( 0, iDimValue );
    specifyIndexesUsed( USE_DIM_1 );
}

//////////////////////////////////////////////////////////////////////////

// C3DObject updated methods

/*
C3DObject::C3DObject( int iSymbol, int* pIndexArray )
    : m_theIndexes( ONE_BASED, pIndexArray )
{
    Init( iSymbol );
}
*/
template <>
void C3DObject::getIndexes( CNDIndexes& theIndex )
{
    theIndex = m_theIndexes;
}

template <>
void C3DObject::setIndexValuesFromString( std::wstring strOcc, csprochar cSeparator )
{
    m_theIndexes.setIndexValuesFromString( strOcc, cSeparator );
}
