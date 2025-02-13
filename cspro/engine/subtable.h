#pragma once

#ifdef USE_BINARY // IGNORE_CTAB

#else

//---------------------------------------------------------------------------
//  File name: SubTable.h
//
//  Description:
//          Header for Subtable Class
//
//  History:    Date       Author   Comment
//              ---------------------------
//              10 Jun 02  RHF     Creation
//
//---------------------------------------------------------------------------

#include <engine/Defines.h>
#include <ZTBDO/cttree.h>
class CtStat;

#include <ZTBDO/ctstadef.h>

class CRemapCoord {
public:
    CRemapCoord() {
        CRemapCoord( 0, 0, 0 );
    }

    CRemapCoord( int iRow, int iCol, int iLay ) {
        m_iRow = iRow;
        m_iCol = iCol;
        m_iLay = iLay;
    }

    CRemapCoord( CRemapCoord& rRemapCoord ) {
        Copy( rRemapCoord );
    }

    void Copy( CRemapCoord& rRemapCoord ) {
        m_iRow = rRemapCoord.m_iRow;
        m_iCol = rRemapCoord.m_iCol;
        m_iLay = rRemapCoord.m_iLay;
    }

    void operator=( CRemapCoord& rRemapCoord) {
        Copy( rRemapCoord );
    }

    bool operator==( CRemapCoord& rRemapCoord) {
        return  m_iRow == rRemapCoord.m_iRow &&
                m_iCol == rRemapCoord.m_iCol &&
                m_iLay == rRemapCoord.m_iLay;
    }

    int     m_iRow;
    int     m_iCol;
    int     m_iLay;
};

class CStatValue {
public:
    double      m_dValue;
    int         m_iPos;
};


class CSubTable {
private:
    int                         m_iSymCtab;
    bool                        m_bIsUsed;
    CString                     m_csName;
    CArray<int,int>             m_aItemSymbols;
    CArray<CStatValue,CStatValue>       m_aStatValue;
    CMap<int,int,int,int>       m_aCtCoordNumber;

    // Stat
    int                         m_iAuxSymCtab;
    CStatType                   m_cStatType;
    int                         m_iStatNumber;
    int                         m_iStatDim;
    int                         m_iStatDepth;
    CtStatBase*                 m_pStatBase;
    int                         m_iRightCells;   // Example:  A* (B[1:5]+C[1:2]) --> 7, if no right expresion is 0.


public:
    int                         m_iCtTree[TBD_MAXDIM][TBD_MAXDEPTH];
    CMap<int,int,CRemapCoord,CRemapCoord&> m_mapSubTableToTable;
    CMap<int,int,CRemapCoord,CRemapCoord&> m_mapTableToSubTable;

public:
    CSubTable() {
        Init();
    }

    CSubTable( CSubTable& cSubTable ) {
        Copy( cSubTable );
    }


    void operator=( CSubTable& cSubTable ) {
        Copy( cSubTable );
    }

    void Copy( CSubTable& cSubTable );

    void  Init();

    void        SetSymCtab( int iSymCtab )      { m_iSymCtab = iSymCtab; }
    int         GetSymCtab()                     { return m_iSymCtab; }


    void SetUsed( bool bIsUsed )    { m_bIsUsed = bIsUsed; }
    bool IsUsed()                   { return m_bIsUsed; }

    void        GenName( CEngineArea* m_pEngineArea, int* pNodeBase[TBD_MAXDIM], CString* csSubTableName=NULL );
    CString&    GetName() { return m_csName; }

    // m_aItemSymbols
    void        MakeItemSymbols( CEngineArea* m_pEngineArea, int* pNodeBase[TBD_MAXDIM] );
    int         GetNumItemSymbols() { return m_aItemSymbols.GetSize(); }
    int         GetItemSymbol( int i ) { return m_aItemSymbols.ElementAt(i); }

    //////////////////////////////////////////////////////////////////////////
    // hand information between subtable and crostab's AddDefaultUnits() method
    // rcl, Nov 2004
    struct CStMultVarSpec
    {
        int m_iDim;
        int m_iOffset;
        int m_iVarSymbol; // even though it is redundant it is already calculated
                          // specially when CtNode talks about a ValueSet
    public:
        CStMultVarSpec()  { m_iDim = m_iOffset = m_iVarSymbol = 0; }
        CStMultVarSpec( int iDim, int iOffset, int iVarSymbol )
            : m_iDim(iDim), m_iOffset(iOffset), m_iVarSymbol(iVarSymbol ) {}
    };

    typedef CArray<CStMultVarSpec> CStMultVarSpecArray;
    void    GetMultItemSymbols( CEngineArea* m_pEngineArea, int* pNodeBase[TBD_MAXDIM],
                                CStMultVarSpecArray& aMultItemSymbols );
    //////////////////////////////////////////////////////////////////////////

    // m_aStatValue
    void        AddStatValue( double dStatValue, int iPos )   {
                                                    CStatValue  cStatValue;

                                                    cStatValue.m_dValue = dStatValue;
                                                    cStatValue.m_iPos = iPos;

                                                    m_aStatValue.Add( cStatValue );
                                                    }

    void        AddStatValue(CStatValue&  cStatValue )   {
                                                    m_aStatValue.Add( cStatValue );
                                                    }
    void        RemoveAllStatValues()               { m_aStatValue.RemoveAll(); }
    CStatValue& GetStatValue( int i )               { return  m_aStatValue.ElementAt(i); }
    int         GetNumStatValues()                  { return  m_aStatValue.GetSize(); }

    // m_aCtCoordNumber
    void        MakeCoordNumberMap( CTAB* pCTab, int* pNodeBase[TBD_MAXDIM] );
    bool        UseCoordNumber( int iCoordNumber );

    // m_mapSubTableToTable, m_mapTableToSubTable
    void        GenRemapCoord( CEngineArea* m_pEngineArea );

    bool        GetSubTableCoord( int &iSubTableI, int &iSubTableJ, int &iSubTableK, const int iTableOffset );
    bool        GetTableCoord   ( int &iTableI, int &iTableJ, int &iTableK, const int iSubTableOffset );

    // Stats
    void        SetAuxSymCtab( int iAuxSymCtab )    { m_iAuxSymCtab = iAuxSymCtab; }
    int         GetAuxSymCtab()                     { return m_iAuxSymCtab; }

    void        SetStatType( CStatType cStatType )  { m_cStatType = cStatType; }
    CStatType   GetStatType()                       { return m_cStatType; }

    void        SetStatNumber( int iStatNumber )    { m_iStatNumber = iStatNumber; }
    int         GetStatNumber()                     { return m_iStatNumber; }

    void        SetSymStatDim( int iStatDim, int iStatDepth )    { ASSERT( iStatDim == DIM_NODIM || iStatDim == DIM_ROW || iStatDim == DIM_COL || iStatDim == DIM_LAYER );
                                                                  ASSERT( iStatDepth == -1 || iStatDepth == 0 || iStatDepth == 1 );
                                                                  m_iStatDim = iStatDim;
                                                                  m_iStatDepth = iStatDepth;
                                                                 }

    int         GetSymStatDim( int* iStatDepth=NULL )            {
                                                            if( iStatDepth != NULL )
                                                                *iStatDepth = m_iStatDepth;
                                                            return m_iStatDim; }

    void        SetStatBase( CtStatBase* pStatBase)  { m_pStatBase = pStatBase; }
    CtStatBase*  GetStatBase()                       { return m_pStatBase; }

    void        SetRightCells( int iRightCells )    { m_iRightCells = iRightCells; }
    int         GetRightCells()                     { return m_iRightCells; }

    int         GetStatCtTree()                               {
           ASSERT( m_iStatDim == DIM_ROW || m_iStatDim == DIM_COL || m_iStatDim == DIM_LAYER );
           ASSERT( m_iStatDepth == 0 || m_iStatDepth == 1 );

           return m_iCtTree[m_iStatDim-1][m_iStatDepth];
    }

    // Miscelaneous
    void        CleanVectorItems(CEngineArea* m_pEngineArea);
    void        GetAuxCtabDims( CEngineArea* m_pEngineArea, int iDim[TBD_MAXDIM] );
};


typedef struct {
    double  m_dRowTotal;
    double  m_dColTotal;
    double  m_dTotTotal;
} SUBTABLE_TOTALS;


#endif
