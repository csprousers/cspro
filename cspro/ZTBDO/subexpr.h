#pragma once
//---------------------------------------------------------------------------
//  File name: SubExpr.h
//
//  Description:
//          Header for CSubExpresion class
//          This class keep a crostab subtable subexpesion
// Example:
//  a[1:3,5,7,(30:20),56] [recoded by f_a([ par1,..,parn] )]    by
//  b[...] [ recoded by f_b( [ par1,..., parn ] ) ]             by
//  c[...] [ recoded by f_c( [ par1,..., parn ] ) ]

//
//  History:    Date       Author   Comment
//              ---------------------------
//              30 Jul 01   RHF     Created
//
//---------------------------------------------------------------------------
#include <ZTBDO/zTbdO.h>
#include <ZTBDO/ctstat.h>

#ifndef USE_BINARY // IGNORE_CTAB

#include <ZTBDO/cttree.h>

namespace Logic { class SymbolTable; }

class CLASS_DECL_ZTBDO CSubExpresion {
private:
    // Number of dimensions: 0,1,2,3
    int                         m_iNumDim;

    // Symbols table index of the item representing the dimension
    // Could be an ITEM/VALUE-SET
    // 0 if there is no column (or layer)
    int                             m_iSymCoord[TBD_MAXDEPTH][TBD_MAXDIM];

    // Depth can be 0 or 1 or 2 ( 2 if the expresion has an '*'. Example: A*B )
    int                         m_iDepth[TBD_MAXDIM];

    // 1 if there is no dimension.
    int                             m_iNumCells[TBD_MAXDEPTH][TBD_MAXDIM];

    // 0 based
    CArray<int,int>             m_iBaseIndexes[TBD_MAXDIM];

    CArray<CSubRange,CSubRange>         m_aRanges[TBD_MAXDEPTH][TBD_MAXDIM];
    CArray<CtStat,CtStat>               m_aStat;

    // Statistics
    /*
    bool                                        m_bStat;
    bool                                        m_bCount;
    bool                                        m_bPct;
    bool                                        m_bRowPct;
    bool                                        m_bColPct;
    bool                                        m_bLayPct;
    bool                                        m_bTotPct;
    bool                                        m_bMin;
    bool                                        m_bMax;
    bool                                        m_bMode;
    bool                                        m_bMean;
    bool                                        m_bMedian;
    int                                         m_iPercentile; // 0 if none
    bool                                        m_bStdDev;
    bool                                        m_bVariance;
    bool                                        m_bStdErr;
    bool                                        m_bPctValidCases;
    CArray<CSubRange,CSubRange> m_aPropRanges; // No elements if not used.
    bool                                    m_bPropPct;

    // Other parameters not yet defined.
    //CArray<bool,bool>                     m_aOtherParameters; // Sampling error?
    */

    // Function parameters. Only like Cstring because could be a complex expression.
    CArray<CString,CString>     m_aParam[TBD_MAXDEPTH][TBD_MAXDIM];

public:
    CSubExpresion() {
        Init();
    }

    CSubExpresion( CSubExpresion& rSubExpr ) {
        Copy( rSubExpr );
    }

    void Init();

#ifdef _DEBUG
    void Dump( CArray<CString, CString>& aDump, Logic::SymbolTable* pSymbolTable, CString csPrefix=_T("") );
#endif

    void Copy( CSubExpresion& rSubExpr );

    void operator=( CSubExpresion& rSubExpr ) {
        Copy( rSubExpr );
    }

    CString GetLabel( Logic::SymbolTable* pSymbolTable );

    // Number of dimensios
    int   GetNumDim()               { return m_iNumDim; }
    void  SetNumDim( int iNumDim )  {
        if( iNumDim < 0 || iNumDim > TBD_MAXDIM ) {
            ASSERT(0);
            return;
        }
        m_iNumDim = iNumDim;
    }

    // Symbol table indexes of the coordinate
    int  GetSymCoord( int iDim, int iDepth ) {
        if( iDim < 0 || iDim >= TBD_MAXDIM ) {
            ASSERT(0);
            return 0;
        }
        if( iDepth < 0 || iDepth >= TBD_MAXDEPTH ) {
            ASSERT(0);
            return 0;
        }
        return m_iSymCoord[iDepth][iDim];
    }

    void SetSymCoord( int iDim, int iDepth, int iSymCoord ) {
        if( iDim < 0 || iDim >= TBD_MAXDIM ) {
            ASSERT(0);
            return;
        }
        if( iDepth < 0 || iDepth >= TBD_MAXDEPTH ) {
            ASSERT(0);
            return;
        }
        m_iSymCoord[iDepth][iDim] = iSymCoord;
    }

    // Depth
    int  GetDepth( int iDim ) {
        if( iDim < 0 || iDim >= TBD_MAXDIM ) {
            ASSERT(0);
            return 0;
        }
        return m_iDepth[iDim];
    }

    void SetDepth( int iDim, int iDepth ) {
        if( iDim < 0 || iDim >= TBD_MAXDIM ) {
            ASSERT(0);
            return;
        }
        if( iDepth < 0 || iDepth > TBD_MAXDEPTH ) {
            ASSERT(0);
            return;
        }
        m_iDepth[iDim] = iDepth;
    }

    // Number of cells
    int GetNumCells() {
        int     iNumCells=1;

        for( int iDim=0; iDim < GetNumDim(); iDim++ )
            iNumCells *= GetNumCells( iDim );

        return iNumCells;
    }

    int GetNumCells( int iDim ) {
        if( iDim < 0 || iDim >= TBD_MAXDIM ) {
            ASSERT(0);
            return 0;
        }

        int     iNumCells=1;

        for( int iDepth=0; iDepth < GetDepth(iDim); iDepth++ )
            iNumCells *= GetNumCells( iDim, iDepth );

        return iNumCells;
    }

    int GetNumCells( int iDim, int iDepth ){
        if( iDim < 0 || iDim >= TBD_MAXDIM ) {
            ASSERT(0);
            return 0;
        }
        if( iDepth < 0 || iDepth >= TBD_MAXDEPTH ) {
            ASSERT(0);
            return 0;
        }
        return m_iNumCells[iDepth][iDim];
    }

    void SetNumCells( int iDim, int iDepth, int iNumCells ){
        if( iDim < 0 || iDim >= TBD_MAXDIM ) {
            ASSERT(0);
            return;
        }
        if( iDepth < 0 || iDepth >= TBD_MAXDEPTH ) {
            ASSERT(0);
            return;
        }
        m_iNumCells[iDepth][iDim] = iNumCells;
    }

    // Base indexes
    int GetBaseIndex( int iDim, int iSubPart ) {
        if( iDim < 0 || iDim >= TBD_MAXDIM ) {
            ASSERT(0);
            return 0;
        }

        if( iSubPart < 0 || iSubPart >= m_iBaseIndexes[iDim].GetSize() ) {
            //ASSERT(0);
            return 0;
        }
        return m_iBaseIndexes[iDim].ElementAt(iSubPart);
    }

    void AddBaseIndex( int iDim, int iIndex ) {
        if( iDim < 0 || iDim >= TBD_MAXDIM ) {
            ASSERT(0);
            return;
        }
        m_iBaseIndexes[iDim].Add( iIndex );
    }

    int GetNumBaseIndexes( int iDim ) {
        if( iDim < 0 || iDim >= TBD_MAXDIM ) {
            ASSERT(0);
            return 0;
        }

        return m_iBaseIndexes[iDim].GetSize();
    }

    void RemoveBaseIndexes( int iDim ) {
        if( iDim < 0 || iDim >= TBD_MAXDIM ) {
            ASSERT(0);
            return;
        }

        m_iBaseIndexes[iDim].RemoveAll();
    }


    // SubRanges
    void AddSubRange( int iDim, int iDepth, CSubRange& rSubRange ) {
        if( iDim < 0 || iDim >= TBD_MAXDIM ) {
            ASSERT(0);
            return;
        }
        if( iDepth < 0 || iDepth >= TBD_MAXDEPTH ) {
            ASSERT(0);
            return;
        }
        m_aRanges[iDepth][iDim].Add( rSubRange );
    }

    CSubRange*  GetSubRange( int iDim, int iDepth, int i ) {
        if( iDim < 0 || iDim >= TBD_MAXDIM ) {
            ASSERT(0);
            return NULL;
        }
        if( iDepth < 0 || iDepth >= TBD_MAXDEPTH ) {
            ASSERT(0);
            return NULL;
        }
        if( i < 0 || i >= m_aRanges[iDepth][iDim].GetSize() ) {
            ASSERT(0);
            return NULL;
        }
        return &m_aRanges[iDepth][iDim].ElementAt(i);
    }

    int GetNumSubRanges( int iDim, int iDepth ) {
        if( iDim < 0 || iDim >= TBD_MAXDIM ) {
            ASSERT(0);
            return 0;
        }
        if( iDepth < 0 || iDepth >= TBD_MAXDEPTH ) {
            ASSERT(0);
            return 0;
        }
        return m_aRanges[iDepth][iDim].GetSize();
    }

    void RemoveSubRanges( int iDim, int iDepth ) {
        if( iDim < 0 || iDim >= TBD_MAXDIM ) {
            ASSERT(0);
            return;
        }
        if( iDepth < 0 || iDepth >= TBD_MAXDEPTH ) {
            ASSERT(0);
            return;
        }
        m_aRanges[iDepth][iDim].RemoveAll();
    }

    // Statistics
    /*
    void SetStat( bool bStat )                  { m_bStat       = bStat; }
    bool GetStat()                              { return m_bStat; }

    void SetCount( bool bCount )                { m_bCount      = bCount; }
    bool GetCount()                             { return m_bCount; }

    void SetPct( bool bPct )                    { m_bPct        = bPct; }
    bool GetPct()                               { return m_bPct; }

    void SetRowPct( bool bRowPct )              { m_bRowPct     = bRowPct; }
    bool GetRowPct()                            { return m_bRowPct; }

    void SetColPct( bool bColPct )              { m_bColPct     = bColPct; }
    bool GetColPct()                            { return m_bColPct; }

    void SetLayPct( bool bLayPct )              { m_bLayPct     = bLayPct; }
    bool GetLayPct()                            { return m_bLayPct; }

    void SetTotPct( bool bTotPct )              { m_bTotPct     = bTotPct; }
    bool GetTotPct()                            { return m_bTotPct; }

    void SetMin( bool bMin )                    { m_bMin        = bMin; }
    bool GetMin()                               { return m_bMin; }

    void SetMax( bool bMax )                    { m_bMax        = bMax; }
    bool GetMax()                               { return m_bMax; }

    void SetMode( bool bMode )                  { m_bMode       = bMode; }
    bool GetMode()                              { return m_bMode; }

    void SetMean( bool bMean )                  { m_bMean       = bMean; }
    bool GetMean()                              { return m_bMean; }

    void SetMedian( bool bMedian )              { m_bMedian     = bMedian; }
    bool GetMedian()                            { return m_bMedian; }

    void SetPercentile( int iPercentile )       { m_iPercentile = iPercentile; }
    int  GetPercentile()                        { return m_iPercentile; }

    void SetStdDev( bool bStdDev )              { m_bStdDev     = bStdDev; }
    bool GetStdDev()                            { return m_bStdDev; }

    void SetVariance( bool bVariance )          { m_bVariance   = bVariance; }
    bool GetVariance()                          { return m_bVariance; }

    void SetStdErr( bool bStdErr )              { m_bStdErr     = bStdErr; }
    bool GetStdErr()                            { return m_bStdErr; }

    void SetPctValidCases( bool bPctValidCases ){ m_bPctValidCases = bPctValidCases; }
    bool GetPctValidCases()                     { return m_bPctValidCases; }


    // Proportional Ranges
    void AddPropRange( CSubRange& rSubRange ) {
        m_aPropRanges.Add( rSubRange );
    }

    CSubRange* GetPropRange( int i ) {
        if( i < 0 || i >= m_aPropRanges.GetSize() ) {
            ASSERT(0);
            return NULL;
        }
        return &m_aPropRanges.ElementAt(i);
    }

    int GetNumPropRanges() {
        return m_aPropRanges.GetSize();
    }

    void RemovePropRanges() {
        m_aPropRanges.RemoveAll();
    }

    void SetPropPct( bool bPropPct )            { m_bPropPct    = bPropPct; }
    bool GetPropPct()                           { return m_bPropPct; }

    // Other parameters not yet defined.
    // CArray<bool,bool>                    m_aOtherParameters; // Sampling error?
    */


    // Function parameters.
    void AddFunParam( int iDim, int iDepth, CString csParam ) {
        if( iDim < 0 || iDim >= TBD_MAXDIM ) {
            ASSERT(0);
            return;
        }
        if( iDepth < 0 || iDepth >= TBD_MAXDEPTH ) {
            ASSERT(0);
            return;
        }
        m_aParam[iDepth][iDim].Add( csParam );
    }

    CString* GetFunParam( int iDim, int iDepth, int i ) {
        if( iDim < 0 || iDim >= TBD_MAXDIM ) {
            ASSERT(0);
            return NULL;
        }
        if( iDepth < 0 || iDepth >= TBD_MAXDEPTH ) {
            ASSERT(0);
            return NULL;
        }
        if( i < 0 || i >= m_aParam[iDepth][iDim].GetSize() ) {
            ASSERT(0);
            return NULL;
        }
        return &m_aParam[iDepth][iDim].ElementAt(i);
    }

    int GetNumFunParam( int iDim, int iDepth ) {
        if( iDim < 0 || iDim >= TBD_MAXDIM ) {
            ASSERT(0);
            return 0;
        }
        if( iDepth < 0 || iDepth >= TBD_MAXDEPTH ) {
            ASSERT(0);
            return 0;
        }
        return m_aParam[iDepth][iDim].GetSize();
    }

    void RemoveFunParams( int iDim, int iDepth ) {
        if( iDim < 0 || iDim >= TBD_MAXDIM ) {
            ASSERT(0);
            return;
        }
        if( iDepth < 0 || iDepth >= TBD_MAXDEPTH ) {
            ASSERT(0);
            return;
        }

        m_aParam[iDepth][iDim].RemoveAll();
    }
};

#endif
