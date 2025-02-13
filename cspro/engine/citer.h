#pragma once

//---------------------------------------------------------------------------
//  File name: citer.h
//
//  Description:
//          Header for iterator class
//
//  History:    Date       Author   Comment
//              ---------------------------
//              16 Jul 2001 RHF     Creation
//
//---------------------------------------------------------------------------

#include <Zissalib/GroupT.h>
#include <engine/NODES.H>

class CIntDriver;
class CIteratorException;
struct EngineData;

const int CITERATOR_FLAG_NONE        = 0;
const int CITERATOR_FLAG_EXPLICIT    = 1;
const int CITERATOR_FLAG_IMPLICIT    = 2;
const int CITERATOR_FLAG_LESS_TOTAL  = 4; // Use when explicit and Export iterator
const int CITERATOR_FLAG_CONSTANT    = 8;
const int CITERATOR_FLAG_INVALID_DIM = 16;

const int CITERATOR_ZERO_BASED_RESULT = true;
const int CITERATOR_ONE_BASED_RESULT = false;


class CBasicIterator // rcl, Aug 2005
{
public:
    //////////////////////////////////////////////////////////////////////////
    // class threeDim and m_aIndexes vector is filled one by one using
    // Add3dIndex method and user can get its values one by one using
    // getIndexes() method.
    // ----- NOTICE that values are supposed to be 1 based -----
    // rcl, Aug 2005
    //
    class threeDim
    {
    private:
        int m_aDims[3];
    public:
        threeDim()
        { m_aDims[0] = m_aDims[1] = m_aDims[2] = 0; }
        threeDim( int a, int b, int c )
        { m_aDims[0] = a; m_aDims[1] = b; m_aDims[2] = c;
        }
        int getDim(int iWhich) const // iWhich 0 based
        {
            ASSERT( iWhich >= 0 && iWhich <= 2 );
            return m_aDims[iWhich];
        }
        void setDim( int iWhich, int iValue )
        {
            ASSERT( iWhich >= 0 && iWhich <= 2 );
            m_aDims[iWhich] = iValue;
        }
        void setDims( int new_i, int new_j, int new_k )
        {
            setDim( 0, new_i ); setDim( 1, new_j ); setDim( 2, new_k );
        }
        void setDims( int newValues[] ) // hopefully the array has enough valid values
        {
            setDims( newValues[0], newValues[1], newValues[2] );
        }
        bool allSpecified( int iNumToCheck )
        {
            bool bAllSpecified = true;
            switch( iNumToCheck )
            {
            case 3: bAllSpecified = bAllSpecified && (getDim(2) > 0);
            case 2: bAllSpecified = bAllSpecified && (getDim(1) > 0);
            case 1: bAllSpecified = bAllSpecified && (getDim(0) > 0);
            }
            return bAllSpecified;
        }
        bool anyDimSpecified()
        {
            bool bAnySpecified =
                getDim(0) > 0 || getDim(1) > 0 || getDim(2) > 0;
            return bAnySpecified;
        }
        int operator[]( int iDim )
        {
            return getDim( iDim );
        }
        bool operator==( const threeDim& other ) const
        {
            #define Other(x) other.getDim(x)
            #define sameVal(x) (getDim(x) == Other(x))
            return sameVal(0) && sameVal(1) && sameVal(2);
        }
        bool operator<( const threeDim& other ) const
        {
            if( getDim(0) > Other(0) )
                return false;
            if( sameVal(0) && getDim(1) > Other(1) )
                return false;
            if( sameVal(0) && sameVal(1) && getDim(2) >= Other(2) )
                return false;
            return true;
        }
        threeDim& operator=( const threeDim& other )
        {
            if( &other != this )
            {
                setDim( 0, Other(0) );
                setDim( 1, Other(1) );
                setDim( 2, Other(2) );
            }
            return *this;
        }
    };

private:
    std::vector<threeDim>   m_aIndexes;
public:
    virtual int size() { return m_aIndexes.size(); }
    virtual void Add3DIndex(const threeDim& the3Ddim );

    // void getIndexes( int iWhichOne, int aIndex[DIM_MAXDIM] ) throw(CIteratorException);
    // iWhichOne is zero based, and must be between 0 and m_aIndexes's size
    // aIndex is a 0-based-int-index collection for 1 based double index collection use
    //        same method different signature
    virtual void getIndexes( int iWhichOne, int aIndex[DIM_MAXDIM] ) /*throw(CIteratorException)*/;

    // void getIndexes( int iWhichOne, double dOccur[DIM_MAXDIM] ) throw(CIteratorException);
    // iWhichOne is zero based, and must be between 0 and m_aIndexes's size
    // dOccur is a 1-based-double-index collection for 0-based-int-index collection use
    //        same method different signature
    virtual void getIndexes( int iWhichOne, double dOccur[DIM_MAXDIM] ) /*throw(CIteratorException)*/;
};


class CIterator : public CBasicIterator
{
    //////////////////////////////////////////////////////////////////////////
    // class threeDim and m_aIndexes vector  {from CBasicIterator} is filled in
    // MakeExportIterator method
    // *********   NOTE: values are 1 based  **************
    // rcl, Oct 28, 2004

private:
    int         m_iLow[DIM_MAXDIM];
    int         m_iHigh[DIM_MAXDIM];
    GROUPT*     m_pGroupT[DIM_MAXDIM];
    int         m_iDimFlag[DIM_MAXDIM];

    // Engine Links
    CEngineDriver*          m_pEngineDriver;
    CEngineArea*            m_pEngineArea;
    EngineData*             m_engineData;
    CIntDriver*             m_pIntDriver;

    void                    Copy( CIterator*  pIterator );

public:
    CIterator();
    ~CIterator();

    void        SetEngineDriver( CEngineDriver* pEngineDriver );
    void        Clean();

    bool        Make( MVAR_NODE* pMVar, bool bStackMode );

    bool        MakeExportIterator( MVAR_NODE* pMultVarNode, bool bEvaluateItem, bool bUseTotalRecord );

private:
    // GetExpressionValue returns a 0 based or 1 based index value
    // it is used in MakeTableIterator, refactoring reason
    // bZeroBasedResult can be CITERATOR_ZERO_BASED_RESULT or CITERATOR_ONE_BASED_RESULT
    int         GetExpressionValue( int iExpression, int iType, bool bZeroBasedResult );

public:
    bool        MakeTableIterator( MVAR_NODE* pMultVarNode, bool bEvaluateItem, bool bUseTotalRecord );

public:
    int         GetLow( int iDim ) const  { return m_iLow[iDim];    }
    int         GetHigh( int iDim ) const { return m_iHigh[iDim];   }
    GROUPT*     GetGroupT( int iDim )     { return m_pGroupT[iDim]; }
    bool        GetExplicit( int iDim )   { return (m_iDimFlag[iDim] & CITERATOR_FLAG_EXPLICIT) !=0; }
    bool        isExplicit( int iDim )    { return GetExplicit(iDim); }
    bool        GetLessTotal( int iDim )  { return (m_iDimFlag[iDim] & CITERATOR_FLAG_LESS_TOTAL) !=0; }
    bool        GetConstant( int iDim )   { return (m_iDimFlag[iDim] & CITERATOR_FLAG_CONSTANT)!=0; }
    int         GetDimFlag( int iDim )    { return m_iDimFlag[iDim]; }

    void        GetLow( int* iLow ) const;
    void        GetHigh( int* iHigh ) const;
    void        GetGroupT( GROUPT* pGroupT[DIM_MAXDIM] );
    void        GetExplicit( bool* bExplicit );
    void        GetLessTotal( bool* bLessTotal );
    void        GetDimFlag( int* iDimFlag );
    void        GetConstant( bool* bConstant );

    void        operator=( CIterator& rIterator ) { Copy( &rIterator ); }
    void        operator=( CIterator* pIterator ) { Copy(  pIterator ); }

    bool        Compare( CIterator*  pIterator, bool bIgnoreExplicit );

    bool        operator==( CIterator& rIterator ) { return Compare( &rIterator, false ); }
    bool        operator==( CIterator* pIterator ) { return Compare( pIterator, false ); }

private:
    const Logic::SymbolTable& GetSymbolTable() const;
};
