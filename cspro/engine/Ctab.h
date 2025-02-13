#pragma once

//---------------------------------------------------------------------------
//  File name: Ctab.h
//
//  Description:
//          Header for Crosstab classes
//
//  History:    Date       Author   Comment
//              ---------------------------
//              15 Jul 99   RHF     Basic conversion
//              17 Jul 01   RHF     Redo for multy-tally support
//              09 Aug 01   RHF     Add makesubexpresions
//              20 Jun 01   RHF     Add unit support
//
//---------------------------------------------------------------------------

#include <zLogicO/Symbol.h>
#include <zEngineO/RunnableSymbol.h>

#ifdef WIN_DESKTOP

#include <engine/citer.h>
#include <engine/ctterm.h>
#include <engine/ttype.h>
#include <engine/CtDef.h>

#include <zToolsO/Range.h>
#include <zToolsO/Tools.h>

#include <ZTBDO/cttree.h>
#include <ZTBDO/ccross.h>
#include <ZTBDO/TbdSlice.h>
#include <ZTBDO/cLinkTab.h>


#ifndef _DEF_MEANACUM_
#define _DEF_MEANACUM_

typedef struct {                // Acumulator structure for MEAN table
    double  m_dSumX;            // Sum of dependent variable
    double  m_dSumW;            // Frequency
    double  m_dSumXW;           // Weighted sum of dependent variable
    double  m_dSumXW2;          // Sum of dependent variable raised to 2
} MEANACUM;

typedef struct {
    double  m_dNumWeight;
    double  m_dNumSquare;
    double  m_dDenWeight;
    double  m_dDenSquare;
    double  m_dDenUnWeight;
    double  m_dCumWeight;
    double  m_dCrosProd;
    double  m_dCumCases;       // Guillermo - RH/VC May 3, 95
} SMEANACUM;

// Nodes for old-tbd (tbd Until Jul, 2001 )
typedef struct {
    int     m_iNodeType;             // node type: Variable or Operator
    int     m_iNumCells;             // # of cells in associated subtree
    struct {
        int m_iInfo1;
        int m_iInfo2;
    } m_NodeInfo;
} OLD_CTNODE;


typedef struct {
    int     m_iRangeLow;
    int     m_iRangeHigh;
} OLD_CTRANGE;

#endif // _DEF_MEANACUM

#include <engine/CtUnit.h>
#include <engine/subtable.h>

class CBorder;

const int MAX_TALLYCELLS = 10100;

class VTSTRUCT;

class CSymbolCtab : public Symbol, public RunnableSymbol, public CCrossTable
{
    // --- engine links
public:
    CEngineDriver*  m_pEngineDriver;
    CEngineArea*    m_pEngineArea;
    EngineData*     m_engineData;

private:
    Logic::SymbolTable& GetSymbolTable() const;

public:
    static CTAB*    pCurrentCtab;

    //SAVY for Speed
    std::vector<std::shared_ptr<VTSTRUCT>> m_arrVectors;
private:
    CTbdSlice*  m_pTbdSlice;
    CTableDef::ETableType  m_eTableType;   // Table type: 1:Crosstab, 2:MEAN, 3:SMEAN, 4: STABLE, 5:HOTDECK

    int     m_iNumDim;          // Number of dimensions (max. 3)
    int     m_iTotDim[3];       // Number of cells in each dimension
    int     m_iNodeExpr[3];     // CTNODE index for dimension expressions
    CTTERM  *m_pCtTerm[3];      // CTTERM index for each dimension
    short   m_siNumDec;         // Number of decimals to be printed
    short   m_siAcumType;       // Acumulator basic lenght

    TCHAR*  m_pAcumArea;        // Address of acumulation area


    int     m_iDepSymVar[2];    // For MEAN & SMEAN: dependent variables
    int     m_iDepVarOcc[2];    //                   occurrence indexes


    int     m_iTableNumber;
    int     m_iRunTimeVersion;

    // RHF INIC Jun 14, 2002
    CtUnit* m_pCurrentUnit;

    int     m_iCurrentCoordNumber;

    CArray  <CtUnit,CtUnit>         m_aUnits;
    CArray  <CSubTable,CSubTable>   m_aSubTables;
    CArray  <CtStat*,CtStat*>       m_aStats;

    int     m_iSelectExpr;
    int     m_iWeightExpr;
    int     m_iTabLogicExpr;

    CArray<int,int>                 m_aAuxCtabs;
    bool                            m_bAuxiliarTable; // RHF Aug 09, 2002

    // Other stats members
    bool    m_bHasSomePercent;    // true when the table has some PERCENT stat

private:
    bool        DoPercents(CSubTable& cSubTable, CtStatPercent* pStatPercent ); // RHF Sep 17, 2002

public:
    // Used when m_bHasSomePercent=true
    void       MakeStatBitMaps();

public:
    // --- engine links
    void        SetEngineDriver( CEngineDriver* pEngineDriver );

    // Units
    int         GetNumUnits()                       { return m_aUnits.GetSize(); }
    CtUnit&     GetUnit(int  iUnit )                { return m_aUnits.ElementAt(iUnit); }

    void        AddUnit(CtUnit& ctUnit )            { m_aUnits.Add( ctUnit ); }
    void        RemoveAllUnits()                    { m_aUnits.RemoveAll(); }

    void        SetCurrentUnit( CtUnit* pCurrentUnit )  { m_pCurrentUnit = pCurrentUnit; }
    CtUnit*     GetCurrentUnit()                          { return m_pCurrentUnit; }
    // RHF END Jun 14, 2002

    //SubTables
    void        MakeSubTables( int* pNodeBase[TBD_MAXDIM], int iRoot[TBD_MAXDIM] );
    void        RemoveAllSubTables()                    { m_aSubTables.RemoveAll(); }

#ifdef _DEBUG
    void        DumpSubTables( int* pNodeBase[TBD_MAXDIM] );
#endif

    void        MakeSubTableDim( int* pNodeBase, int iCtNode, std::vector<Range<int>>& aVector );

    int         GetNumSubTables()            { return m_aSubTables.GetSize(); }
    CSubTable&  GetSubTable( int iSubTable ) { return m_aSubTables.ElementAt(iSubTable); }

    int         SearchSubTables( int* pNodeBase[TBD_MAXDIM], int iDimType[TBD_MAXDIM], int iVar[TBD_MAXDIM][TBD_MAXDEPTH], int iSeq[TBD_MAXDIM][TBD_MAXDEPTH],
                                 std::vector<int>& aSubTables, int iTheDimType );

    // Stats
    int         GetNumStats()                       { return m_aStats.GetSize(); }
    CtStat*     GetStat(int  iStat )                { return m_aStats.ElementAt(iStat); }
    void        AddStat(CtStat* pStat )            { m_aStats.Add( pStat ); }
    void        RemoveAllStats()                    {
        for( int i=0; i < GetNumStats(); i++ ) {
             CtStat*   pStat=GetStat(i);
             delete pStat;
        }
        m_aStats.RemoveAll();
    }


    int         GetStatVar( CArray<CtStatVar,CtStatVar>& aStatVar, int* pNodeBase[TBD_MAXDIM], int iVar, int iSeq, std::vector<int>* pSubTables=NULL );


    void        GetStatList( int iCoordNumber, CArray<CtStatBase*,CtStatBase*>& aStatBase );
    bool        AddStatCoordinates( int* pNodeBase[TBD_MAXDIM], int iCtMaxEnt[TBD_MAXDIM], int iCtNext[TBD_MAXDIM], int iCtRoot[TBD_MAXDIM] );
    int         AddStatCoordinate( int iCtNode );
    bool        DoStatistics();


    static double calcPTile(double dProportion, CArray<double,double>& aValues, CArray<double,double>&      aIntervals, double dBias){

    ASSERT(dProportion >= 0.1 && dProportion < 1.0);
    ASSERT(dBias >= -1.0 && dBias <= 0.0);
    int iNumValues = aValues.GetSize();
    int iNumIntervals = aIntervals.GetSize();
    ASSERT(iNumValues > 0);
    ASSERT(iNumValues + 1 == iNumIntervals);

    double dValue = 0;          // Current value being used
    double dSumValues = 0;      // Sum of number of values
    double dTile = 0;           // Number of values in requested nTile
    double dSumPrev = 0;
    double dSumNow = 0;
    double dPart = 0;           // The proportion through the interval
    double dMedian = 0;

    for (int i = 0 ; i < iNumValues ; i++) {
        dValue = aValues.GetAt(i);
        if (!IsSpecial(dValue)) {
            dSumValues += dValue;
        }
    }
    if (dSumValues == 0.0) {
        return DEFAULT;
    }
    dTile = dSumValues * dProportion;
    int k;
    for( int i = 0; i < iNumValues && dSumNow < dTile; i++ ) {
        dValue = aValues.GetAt(i);
        if( !IsSpecial(dValue) ) { // ignore special values
            dSumPrev = dSumNow;    // previously cumulated
            dSumNow += dValue;     // cumulated up to this point
            k = i;
        }
    }
    dPart = (dTile - dSumPrev) / dValue;
    if (dTile - dSumNow != 0.0) {
        dMedian = aIntervals.GetAt(k) + dBias + dPart * (aIntervals.GetAt(k+1) - aIntervals.GetAt(k));
    }
    else {
        int l;
        for ( l = k + 1; l < aValues.GetSize() && aValues.GetAt(l) == 0.0; l++)
            ;
        dMedian = aIntervals.GetAt(k+1) + dBias + (aIntervals.GetAt(l) - aIntervals.GetAt(k+1)) / 2.0;
    }
    return dMedian;
}

    // Aux Ctabs
    // RHF INIC Jul 30, 2002
    int         GetStatDimSize( int iCtNode, bool bAuxCtab );
    bool        MakeAuxCtabs();
    int         GetNumAuxCtabs()                    { return m_aAuxCtabs.GetSize(); }
    int         GetAuxCtab( int iAuxCtab )          { return m_aAuxCtabs.ElementAt(iAuxCtab); }
    void        AddAuxCtab( int iAuxCtab )          { m_aAuxCtabs.Add( iAuxCtab ); }
    void        RemoveAllAuxCtabs()                 { m_aAuxCtabs.RemoveAll(); }
    // RHF END Jul 30, 2002

    //Coordinates
    void        SetCurrentCoordNumber( int iCurrentCoordNumber )  { m_iCurrentCoordNumber = iCurrentCoordNumber; }
    int         GetCurrentCoordNumber()                    { return m_iCurrentCoordNumber; }

public:
    CAcum       m_pAcum;// RHF Jul 31, 2001
    CBorder*    m_pBorder; // RHF Jan 29, 2003

    void        InitBorder( int iNumRows, int iNumCols, int iNumLayers ); // RHF Jan 29, 2003
    bool        AllocBorder(); // RHF Jan 30, 2003

    csprochar   m_uEnviron[4];      // ... see ct_xxx for environment
// m_iOptions privatized, Jul 2005
private:
    int         m_iOptions;         // ... see ct_xxx for m_iOptions
public:
    void resetOptions() { m_iOptions = 0;  }
    void addOption( int iFlagOption ) { m_iOptions |= iFlagOption; }
    void removeOption( int iFlagOption )
    {
        ASSERT( iFlagOption != 0 );
        m_iOptions &= ~iFlagOption;
    }
    int getOptions() { return m_iOptions; }
    bool OptionActivated( int iFlag ) { return (iFlag > 0) && ((getOptions() & iFlag) == iFlag); }

    CTbdSlice*  GetTbdSlice()                       { return m_pTbdSlice; }
    void        SetTbdSlice( CTbdSlice* pTbdSlice ) { m_pTbdSlice = pTbdSlice; }

    CTableDef::ETableType   GetTableType()                          { return m_eTableType; }
    void        SetTableType( CTableDef::ETableType eTableType )   { m_eTableType = eTableType; }


    int GetNumDim() const                               { return m_iNumDim; }
    void SetNumDim( int iNumDim )                       { ASSERT( iNumDim >= 0 && iNumDim <= TBD_MAXDIM );
                                                            m_iNumDim = iNumDim; }


    int GetTotDim( int iDim ) const                     { ASSERT( iDim >= 0 && iDim < TBD_MAXDIM );
                                                          return m_iTotDim[iDim];}

    int SetTotDim( int iTotDim, int iDim )             { ASSERT( iDim >= 0 && iDim < TBD_MAXDIM );
                                                           m_iTotDim[iDim] = iTotDim;
                                                           return iTotDim; }


    int GetNodeExpr( int iDim )                         { ASSERT( iDim >= 0 && iDim < TBD_MAXDIM );
                                                            return m_iNodeExpr[iDim]; }

    void SetNodeExpr( int iNodeExpr, int iDim )         { ASSERT( iDim >= 0 && iDim < TBD_MAXDIM );
                                                            m_iNodeExpr[iDim] = iNodeExpr; }


    CTTERM* GetTerm( int iTerm )                        { ASSERT( iTerm >= 0 && iTerm < TBD_MAXDIM );
                                                            return m_pCtTerm[iTerm]; }

    void SetTerm( CTTERM* pTerm, int iTerm )            { ASSERT( iTerm >= 0 && iTerm < TBD_MAXDIM );
                                                            m_pCtTerm[iTerm] = pTerm; }

    short GetNumDec() const                             { return m_siNumDec; }
    void  SetNumDec( short siNumDec )                   { m_siNumDec = siNumDec; }

    short GetAcumType() const                           { return m_siAcumType; }
    void  SetAcumType( short siAcumType )               { m_siAcumType = siAcumType; }

    TCHAR* GetAcumArea()                                { return m_pAcumArea; }
    void  SetAcumArea( TCHAR* pAcumArea )               { m_pAcumArea = pAcumArea; }


    int  GetDepSymVar( int iDepVarNum )                 { ASSERT( iDepVarNum >= 0 && iDepVarNum <= 1 );
                                                            return m_iDepSymVar[iDepVarNum]; }

    void SetDepSymVar( int iSym, int iDepVarNum )       { ASSERT( iDepVarNum >= 0 && iDepVarNum <= 1 );
                                                            m_iDepSymVar[iDepVarNum] = iSym; }


    int  GetDepVarOcc( int iDepVarNum )                 { ASSERT( iDepVarNum >= 0 && iDepVarNum <= 1 );
                                                            return m_iDepVarOcc[iDepVarNum]; }

    void SetDepVarOcc( int iOcc, int iDepVarNum )       { ASSERT( iDepVarNum >= 0 && iDepVarNum <= 1 );
                                                            m_iDepVarOcc[iDepVarNum] = iOcc; }


    void SetTableNumber( int iTableNumber )             { m_iTableNumber = iTableNumber; }
    int  GetTableNumber()                               { return m_iTableNumber; }

    void SetRunTimeVersion( int iVersion )              { m_iRunTimeVersion = iVersion; }
    int  GetRunTimeVersion()                            { return m_iRunTimeVersion; }

public:
    CSymbolCtab(std::wstring name);
    virtual ~CSymbolCtab();

    CCrossTable* MakeSubExpresions(); // RHF Aug 09, 2001

    UINT GetAcumSize();
    virtual UINT GetCellSize();
    UINT GetNumCells();
    UINT GetNumRows();
    UINT GetNumCols();
    UINT GetNumLayers();

//    int  GetNumSubTables( int* pNodeBase[TBD_MAXDIM], int* iDim, int* iVar1, int* iVar2, int* iSeq1, int* iSeq2, CSubTable &cSubTable );
    bool HasItem( int* pNodeBase[TBD_MAXDIM], int iDimType, int iDepthType,
                  int iSymbol, int iSeq, int iSubTable, int* iCtVarNode=NULL, int* iCtDim=NULL ); // RHF Jul 02, 2002

public:
    //////////////////////////////////////////////////////////////////////////
    // 3d Modifications                                    rcl, November 2004

    // Sometimes we need to get MVAR_NODE data, but it is not always available
    // [GENCODE compilation only]
    // so we define a method to obtain the data, independently of compilation mode
    // Currently it is being used by SolveSymbolFor2Dims()
    MVAR_NODE* getMVarNode( int iMVarNodeIndex );
private:
    // getOwnerGrpIdxForVarOrRecord() is used inside CompleteDimensionsUsingThisUnit() method
    int getOwnerGrpIdxForVarOrRecord( int iVar );

public:
    // CompleteDimensionsUsingThisUnit() is used inside
    //    - [commented out] AddDefaultUnits() method
    //    - void CIntDriver::CtPos_FillIndexArray() method
    void CompleteDimensionsUsingThisUnit( MVAR_NODE* pMVarNode, int iNumDim, int iUnitSymbol );

private:
    int SolveSymbolFor1Dim( VART* pVarT );
    int SolveSymbolFor2Dims( CTNODE* pNode, VART* pVarT, int* piExtraInfo );
    int SolveSymbolFor3Dims( CTNODE* pNode, VART* pVarT );
    //////////////////////////////////////////////////////////////////////////

public:
    int AddDefaultUnits(int* pNodeBase[TBD_MAXDIM]);

private:
    int  SearchRelation( CSubTable::CStMultVarSpecArray& aMultItemSymbols );

public:
    void ResetSelectWeightExpr( int& iSelectExpr, int& iWeightExpr );

    void SetSelectExpr( int iSelectExpr )       { m_iSelectExpr = iSelectExpr; }
    void SetWeightExpr( int iWeightExpr )       { m_iWeightExpr = iWeightExpr; }
    void SetTabLogicExpr( int iTabLogicExpr )   { m_iTabLogicExpr = iTabLogicExpr; }

    int GetSelectExpr()                         { return m_iSelectExpr; }
    int GetWeightExpr()                         { return m_iWeightExpr; }
    int GetTabLogicExpr()                       { return m_iTabLogicExpr; }

    void CleanVectorItems();

    bool UseCoordNumber( LIST_NODE* pListNode, int iCoordNumber );

    bool GenRemapCoord( CSubTable* pSubTable );

    bool GetTableCoordOnLine( int& iSubTableI, int& iSubTableJ, int& iSubTableK, int iSearchSubTableOffSet, CSubTable* pSubTable );
    bool GetSubTableCoordOnLine(  int& iSubTableI, int& iSubTableJ, int& iSubTableK, int iSearchTableOffSet, CSubTable* pSubTable );

    void SetAuxiliarTable( bool bAuxiliarTable )                { m_bAuxiliarTable = bAuxiliarTable; }
    bool IsAuxiliarTable()                                      { return m_bAuxiliarTable; }

    int     CtCells( int iCtNode ); // Give real number of cells based on ranges // RHF Sep 10, 2002

    void    SetHasSomePercent( bool bHasSomePercent ) { m_bHasSomePercent = bHasSomePercent; }
    bool    GetHasSomePercent()                       { return m_bHasSomePercent; }

    void    CalcHasSomePercent();

private:
    bool    m_bPreDeclared;
public:
    void    SetPreDeclared( bool bPreDeclared ) { m_bPreDeclared = bPreDeclared; }
    bool    GetPreDeclared()                    { return m_bPreDeclared; }

private:
    CTAB*   m_pCtabGrandTotal;

public:
    CTAB*   GetCtabGrandTotal();
    void    SetCtabGrandTotal( CTAB* pCtabGrandTotal );

    bool    HasOverlappedCat( int* pBase, int iCtNode );
    void    FillTotals( double dWeight, int iSubTableI, int iSubTableJ, int iSubTableK );

// SERPRO_CALC
private:
    bool            m_bHasSyntaxError;

    CLinkTable*     m_pLinkTable;

    int             m_iNumBreaks; // RHF Apr 16, 2003

public:

    // RHF INIC Apr 16, 2003
    void        SetNumBreaks(int iNumBreaks) { m_iNumBreaks = iNumBreaks; }
    int         GetNumBreaks() const         { return m_iNumBreaks; }
    // RHF END Apr 16, 2003

    void        InitLinkTable();
    CLinkTable* GetLinkTable();
    void        FillLinkTable();

    void        SetHasSyntaxError( bool bHasSyntaxError );
    bool        GetHasSyntaxError();

    void        MakeLinkTableTerms( int* pNodeBase, int iCtNode,
                              CArray<CLinkTerm,CLinkTerm>& aLinkTerms,
                              CArray<CLinkVar,CLinkVar>& aLinkVars, bool bFatherMult, int iDepth );

    static CMap<int,int,CString,CString> m_aExtraNodeInfo;

// SERPRO_CALC

private:
    int m_containerIndex = 0; // the container table index

public:
    int GetContainerIndex() const  			    { return m_containerIndex; }
    void SetContainerIndex(int container_index) { m_containerIndex = container_index; }
};

// Ctnode access macro
#ifndef CTN
#define CTN(i)  ((CTNODE *) (m_pEngineArea->m_CtNodebase + i))
#endif

class CCoordValue {
public:
    int     m_iCtNodeLeft;
    int     m_iCtNodeRight;
    double  m_dValueLeft;
    double  m_dValueRight;
    int     m_iSeqLeft;
    int     m_iSeqRight;
};


#else // not WIN_DESKTOP

// crosstabs aren't used in entry applications so just stub out the class

class CSymbolCtab : public Symbol, public RunnableSymbol
{
public:
    CSymbolCtab(std::wstring name)
        :   Symbol(std::move(name), SymbolType::Crosstab)
    {
    }
};

#endif

typedef CSymbolCtab CTAB;
