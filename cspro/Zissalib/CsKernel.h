#pragma once
//-----------------------------------------------------------------------------
//  File name: CsKernel.h
//
//  Description:
//          Header for the kernel of the 3-D driver and all his clients
//
//  History:  Date       Author   Comment
//            ---------------------------
//            23 Jan 01   vc      Created for final expansion to 3 dimensions
//                                (initially placed at CsDriver.h)
//            14 Jun 01   vc      Adding overloaded, no-parm GetIndex just returning the address of the indexes' array
//            13 Jun 04   rcl     Add C3DIndexes class
//            01 Jul 04   rcl     create C3DObject_Base template,
//                                C3DObject is a specialization of that template
//            15 Jul 04   rcl     const correctness pass
//
//-----------------------------------------------------------------------------
#undef new

#include <engine/dimens.h>


const int ZERO_BASED = 0;
const int ONE_BASED  = 1;

#define DEFAULT_INDEXES 0
const csprochar NO_PARENTHESIS = 0;

const int USE_DIM_1 = 1;
const int USE_DIM_2 = 2;
const int USE_DIM_3 = 4;
const int USE_DIM_1_2 = USE_DIM_1 | USE_DIM_2;
const int USE_DIM_1_3 = USE_DIM_1 | USE_DIM_3;
const int USE_DIM_2_3 = USE_DIM_2 | USE_DIM_3;
const int USE_DIM_1_2_3 = USE_DIM_1 | USE_DIM_2 | USE_DIM_3;
const int USE_ALL_DIM = USE_DIM_1_2_3;

class C3DIndexes
{
private:
    int m_iEmptyValue;
    int m_iBase;
    int m_aIndex[DIM_MAXDIM];
    int m_iIndexNumber;
    bool m_bHasBeenInitialized;
    int  m_iUseThisDimension;
public:
    C3DIndexes();
    C3DIndexes( int iBase );
    C3DIndexes( int iBase, const int *pIndexArray );

    int getIndexNumber() const
    { return m_iIndexNumber; }
    bool isZeroBased() const
    { return m_iBase == ZERO_BASED; }
    void setAsInitialized() { m_bHasBeenInitialized = true; }
private:
    void getIndexes( int* pIndexArray, int iSize = -1 ) const;
public:
    // iIndexesUsed is a mask to specify which values are valid inside
    // the index, default is USE_ALL_DIM
    void specifyIndexesUsed( int iIndexesUsed );
    // iMaskDimension is one of the constants defined above
    bool isUsingThisDimension( int iMaskDimension ) const;

    void setEmpty();
    void setAtOrigin();
    void setIndexes( const int* pIndexArray = 0 );
    bool isEmpty() const;
    void setIndexValue( int iWhichDim, int iWhichValue );
    void setIndexValue( CDimension::VDimType xType, int iWhichValue );
    int getIndexValue( int iWhichDim ) const;
    int getIndexValue( CDimension::VDimType xType ) const;

    // Given a string like "3,4" or "3.4"
    // setIndexValuesFromString() can be used to set the individual
    // indexes if user states what separator will be used.
    // If user says separator is ',' then "3,4" will perform
    //  index.setIndexValue( 0, 3 ) + index.setIndexValue( 1, 4 )
    // if user says separator is '.' then "3.4" will have the same
    // meaning.
    // If the number of indexes is bigger than 3, this method
    // does not crash, but does not tell the user neither... yet.
    // rcl, Sept 2004
    void setIndexValuesFromString( std::wstring strOcc, csprochar cSeparator = ',' );

    // searchNotEmptyValue()
    //  gets the not empty value that has the highest index
    int searchNotEmptyValue() const;

    //////////////////////////////////////////////////////////////////////////
    // operators
    C3DIndexes& operator=( const C3DIndexes& other );
    bool operator==( const C3DIndexes& other ) const;
    int Precedence( C3DIndexes& other ) const;

    std::wstring toStringBare() const;
    // std::wstring toString( int iNumDim,
    //                       csprochar cLeft = '(', csprochar cRight = ')', csprochar cSep = ',' )
    //
    // show the first iNumDim values, using delimiters indicated
    std::wstring toString( int iNumDim,
        csprochar cLeft  = _T('('),
        csprochar cRight = _T(')'),
        csprochar cSep   = ',' ) const;

    /*
    std::wstring toString( csprochar cLeft  = '(',
                          csprochar cRight = ')',
                          csprochar cSep   = ',' ) const;
                          */

    // conversions
    // decrease( (4,3,1) ) -> (3,2,0)
    void decrease(C3DIndexes& theNewIndex) const;

    // setHomePosition()
    // puts coordinates in least possible valid value
    // ONE_BASE would be (1,1,1)
    // ZERO_BASE would be (0,0,0)
    void setHomePosition();

    bool isValid() const;

private:
    bool m_bIsUsing1DimensionOnly;
    void willUseOnly1Dimension()   { m_bIsUsing1DimensionOnly = true;  }
    void willUseManyDimensions()   { m_bIsUsing1DimensionOnly = false; }
public:
    bool isUsingOnlyOneDimension() const
                                   { return m_bIsUsing1DimensionOnly;  }

    void useOnlyOneDimension( int iDimValue );
};

typedef C3DIndexes CNDIndexes; // ND stands for N-dimension

template <int iBaseUsed>
class C3DObject_Base {
// --- Data members --------------------------------------------------------
private:
    int     m_iSymbol;                  // Group or Var
    C3DIndexes m_theIndexes;
    int     m_iAtomIndex; //Savy for optimizing skip to during runtime
// --- Methods -------------------------------------------------------------
public:
    // --- construction/destruction/initialization
    C3DObject_Base( void ) : m_theIndexes( iBaseUsed, DEFAULT_INDEXES )
    { Init( 0 ); }
    C3DObject_Base( int iSymbol ) : m_theIndexes( iBaseUsed, DEFAULT_INDEXES )
    { Init( iSymbol ); }
    C3DObject_Base( int iSymbol, const CNDIndexes& theIndex ) : m_theIndexes(theIndex)
    { Init( iSymbol); };
    virtual ~C3DObject_Base( void )
    {}
private:
    void    Init( void )                { Init( 0 ); }
    void    Init( int iSymbol ) {
            m_iSymbol = CheckSymbol( iSymbol );
            m_iAtomIndex = -1;
    }
    int     CheckSymbol( int iSymbol )
    {
        int     iSymChecked = iSymbol;

        /* TODO
        // this thing should check that the symbol is either GR or VA,
        // and set to zero if not
        CEngineArea*    m_pEngineArea =

          if( NPT(iSymbol)->GetType() != GR && NPT(iSymbol)->GetType() != VA )
          iSymChecked = 0;
        TODO */

        return iSymChecked;

    }

    void Copy(const C3DObject_Base& the3DObject )
    {
         SetSymbol( the3DObject.GetSymbol() );
         m_theIndexes = the3DObject.GetIndexes();
    }

    bool IsSameSymbol( const C3DObject_Base* p3DObject ) const
    {
        ASSERT( p3DObject != 0 );
        return IsSameSymbol( *p3DObject );
    }

    bool IsSameSymbol( const C3DObject_Base& the3DObject ) const
    { return( m_iSymbol == the3DObject.GetSymbol() ); }

    bool IsEqualTo( const C3DObject_Base* p3DObject ) const {

        ASSERT( p3DObject != 0 );
        return IsEqualTo( *p3DObject );
    }

    bool IsEqualTo( const C3DObject_Base& the3DObject ) const {
        bool    bEqual = IsSameSymbol( the3DObject );

        if( bEqual ) {
            bEqual = (m_theIndexes == ((C3DObject_Base&)the3DObject).getIndexes());
        }

        return bEqual;
    }

    int Precedence( const C3DObject_Base* p3DObject ) {

        ASSERT( p3DObject != 0 );
                // remark - only for objects of the same symbol
                //          0: identical, -1: low-index-of-this, +1: high-index-of-this
                ASSERT( IsSameSymbol( p3DObject ) );

        return m_theIndexes.Precedence( p3DObject->GetIndexes() );
    }
public:
    // --- basic info
    void SetEmpty( void )
    {
        m_iSymbol = 0;
        m_theIndexes.setEmpty();
    }

    bool    IsEmpty( void ) const {
        bool    bEmpty = ( GetSymbol() == 0 );

        if( bEmpty )
            bEmpty = m_theIndexes.isEmpty();

        return bEmpty;
    }
    void SetAtomIndex(int iAtomIndex) { m_iAtomIndex = iAtomIndex; } //used for storing the index while skip to to avoid recomupte
    int GetAtomIndex() { return m_iAtomIndex; } //used for storing the index while skip to to avoid recomupte

    void    SetSymbol( int iSymbol )    { m_iSymbol = CheckSymbol( iSymbol ); }
    int     GetSymbol( void ) const     { return m_iSymbol; }

    void setIndexes( C3DIndexes& theIndexes )
    {
        m_theIndexes = theIndexes;
    }

    void setIndexes( int* pIndex )
    {
        m_theIndexes.setIndexes( pIndex );
    }

    void setIndexValue( int iDim, int iValue )
    {
        m_theIndexes.setIndexValue( iDim, iValue );
    }

    C3DIndexes& GetIndexes() { return m_theIndexes; }
    const C3DIndexes& GetIndexes() const { return m_theIndexes; }
    C3DIndexes& getIndexes() { return m_theIndexes; }
    void getIndexes( CNDIndexes& theIndex );

    int getIndexValue( CDimension::VDimType xType ) const { return m_theIndexes.getIndexValue(xType); }
    int getIndexValue( int iDim ) const { return m_theIndexes.getIndexValue(iDim); }

    // See comment in C3DIndexes for setIndexValuesFromString() method documentation
    void setIndexValuesFromString( std::wstring strOcc, csprochar cSeparator = ',' );

    // --- operator helpers
    void    operator=  (const C3DObject_Base& r3DObject ) { Copy( r3DObject ); }

    bool    operator== ( const C3DObject_Base& r3DObject ) const { return( IsEqualTo( r3DObject ) ); }
    bool    operator== ( C3DObject_Base* p3DObject ) const { return( IsEqualTo( p3DObject ) );  }

    bool    operator!= ( C3DObject_Base& r3DObject ) const { return( !IsEqualTo( &r3DObject ) ); }
    bool    operator!= ( C3DObject_Base* p3DObject ) const { return( !IsEqualTo( p3DObject ) );  }

    bool    operator<  ( C3DObject_Base& r3DObject ) { return( Precedence( &r3DObject ) < 0 ); }
    bool    operator<  ( C3DObject_Base* p3DObject ) { return( Precedence( p3DObject )  < 0 ); }

    bool    operator>  ( C3DObject_Base& r3DObject ) { return( Precedence( &r3DObject ) > 0 ); }
    bool    operator>  ( C3DObject_Base* p3DObject ) { return( Precedence( p3DObject )  > 0 ); }

public:
    bool isUsingOnlyOneDimension() const { return m_theIndexes.isUsingOnlyOneDimension(); }
    void useOnlyOneDimension( int iDimValue ) { m_theIndexes.useOnlyOneDimension(iDimValue);}
};

typedef C3DObject_Base<ONE_BASED>  C3DObject;

// if user says x( i, j ) he might mean x( i, 0, j )
// because of CSPro language omition features.
// In some moment we are carrying UserIndexes array
// and sometimes we are using correct internal indexes.
// For user indexes we define UserIndexesArray type,
// just to make .code clearer
// typedef double* UserIndexes;
typedef double UserIndexesArray[DIM_MAXDIM];
#define LOOP(var) for( int var = 0; var < DIM_MAXDIM; var++ )
static void initUserIndexArray( UserIndexesArray arr )
{  LOOP(i) { arr[i] = 0; }   }

static bool areUserIndexesValid( UserIndexesArray specifiedIndexes, int iMaxIndex )
{
    bool bRet = true;

    if( iMaxIndex > DIM_MAXDIM )
        iMaxIndex = DIM_MAXDIM;

    for( int i = 0; i < iMaxIndex; i++ )
    {
        if( specifiedIndexes[i] <= 0 )
        {
            bRet = false;
            break;
        }
    }

    return bRet;
}
