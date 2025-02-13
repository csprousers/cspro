#pragma once
//---------------------------------------------------------------------------
//  File name: VarX.h
//
//  Description:
//          Header for executor-Var parallel table
//
//  History:    Date       Author   Comment
//              ---------------------------
//              20 Jul 99   RHF     Basic conversion
//              24 May 00   vc      Basic customization
//              04 Jul 00   vc      New approach for data management by Sect-occ
//              19 Jul 00   vc      See TRANSITION (make it dissapear!)
//              21 Jul 00   vc      Adding occurrences' management methods
//              15 Jan 01   vc      Link to procedures for 3-D implementation
//              04 Apr 01   vc      brack_... suppressed
//              21 May 01   vc      Adding index-translation methods PassIndexFrom3DToEngine & PassIndexFromEngineTo3D
//              12 May 04   rcl     RemapDoubleIndexes changed to int index array.and renamed to RemapIndexes
//                                  IndexUsedException class added.
//---------------------------------------------------------------------------
#include <Zissalib/CsKernel.h>
#include <engine/Exappl.h>

class CEngineArea;
class CEngineDriver;
class CDEField;
class CDEForm;

//////////////////////////////////////////////////////////////////////////

class IndexUsedException
{
public:
    enum IndexUsedExceptionCodes
    {
        ERR_OVERFLOW_INDEX         = 1, // lower than MININT or bigger than MAXINT
        ERR_BAD_LIMITS_INDEX       = 2, // bigger than MaxOccs, lower than or equal to 0
        ERR_BAD_LIMITS_ALMOST_GOOD = 3, // bigger than TotOccs, lower than or equal to TotOccs
        ERR_SPECIAL_VALUE_USED     = 4  // NOTAPPL used as index
    };

private:
    IndexUsedExceptionCodes m_iCode;
    double m_dIndexUsed;
public:
    IndexUsedException( IndexUsedExceptionCodes iCode, double dIndexUsed )
        : m_iCode( iCode ), m_dIndexUsed( dIndexUsed )
    {}
    IndexUsedExceptionCodes getCode() const { return m_iCode; }
    double getIndexUsed() const             { return m_dIndexUsed; }
};

//////////////////////////////////////////////////////////////////////////

//---------------------------------------------------------------------------
//
//  struct VARX
//
//  Description:
//      Parallel Vars' table management
//
//  Construction/Destruction/Initialization
//      Init                        Initialization
//
//  Related objects
//      GetVarT                     Return the pointer to the VART parallel to this VARX
//      SetVarT                     Set the pointer to the VART parallel to this VARX
//      GetSecT                     Return the pointer to the SECT owning this Var
//      SetSecT                     Set the pointer to the SECT owning this Var
//
//  Data-access information             <new version>   // victor Jul 04, 00
//      SetIndToAscii               Set the index to the Ascii displacement in the Sect' ascii-area
//      SetIndToFloat               Set the index to the Float displacement in the Sect' float-area
//      SetIndToFlags               Set the index to the Flags displacement in the Sect' flags-area
//      GetIndToAscii               Return the index to the Ascii displacement in the Sect' ascii-area
//      GetIndToFloat               Return the index to the Float displacement in the Sect' float-area
//      GetIndToFlags               Return the index to the Flags displacement in the Sect' flags-area
//
//  Converting values to ascii and reverse
//      varinval
//      varoutval
//
//  Managing related data
//      RefreshValueSet
//      VarxRefreshRelatedData
//      VarxRefreshRelatedData
//
//  Engine links
//      SetEngineDriver
//
//  Occurrences' management: array remapping, getting parent' indexes
//      PassIndexFrom3DToEngine     Pass full-dimension indexes from 3D-objects(1-based) to engine (0-based)
//      PassIndexFromEngineTo3D     Pass full-dimension indexes from engine (0-based) to 3D-objects(1-based)
//      RemapIndexes                Remap from a source' compact-dimensions occurs-array (with iNumDim occurrencess)
//                                  to a target' full-dimensions indexes-array (exactly DIM_MAXDIM indexes)
//      RemapIntIndexes             Similar to 'RemapDoubleIndexes' (integer' type arguments)
//      BuildIntParentIndexes       For "focus" VARXs only, completes the whole set of indexes
//                                  (integer' type arguments)
//
//---------------------------------------------------------------------------

struct VARX {
// --- Data members --------------------------------------------------------
    // --- related objects
private:
    VART*           m_pVarT;            // VART entry
    SECT*           m_pSecT;            // SECT owner' entry

    // --- data-access information <new version>        // victor Jul 04, 00
    // ... these 3 members are "indexes to xxx-area in the record-occurrence"
public:
    int             m_iAscii;           // index to Ascii-area
    int             m_iFloat;           // index to Float-area
    int             m_iFlags;           // index to Flags-area

    // --- related items
public:
    int             iRelatedSlot;       // slot of related items // RHF Aug 6, 1999

    // --- engine links
public:
    CEngineDriver*  m_pEngineDriver;
    CEngineArea*    m_pEngineArea;

// --- Methods -------------------------------------------------------------
    // --- initialization
public:
    void            Init( void );

    // --- related objects
public:
    VART*           GetVarT( void ) const       { return m_pVarT; }     // victor May 24, 00
    void            SetVarT( VART* pVarT )      { m_pVarT = pVarT; }    // victor May 24, 00
    SECT*           GetSecT( void ) const       { return m_pSecT; }     // victor May 24, 00
    void            SetSecT( SECT* pSecT )      { m_pSecT = pSecT; }    // victor May 24, 00

    // --- data-access information      <new version>   // victor Jul 04, 00
    void            SetIndToAscii( int iInd )   { m_iAscii = iInd; }
    void            SetIndToFloat( int iInd )   { m_iFloat = iInd; }
    void            SetIndToFlags( int iInd )   { m_iFlags = iInd; }
    int             GetIndToAscii( void )       {
                        ASSERT( m_iAscii >= 0 );
                        return m_iAscii;
    }
    int             GetIndToFloat( void )       { return m_iFloat; }
    int             GetIndToFlags( void )       { return m_iFlags; }

    // --- converting values to ascii and reverse
private:
    double          varinval(double value, const CNDIndexes* pTheIndex);
public:
    double          varinval(double value) { return varinval(value, nullptr); }
    double          varinval(double value, const CNDIndexes& theIndex) { return varinval(value, &theIndex); }

private:
    double          varoutval(const CNDIndexes* pTheIndex) const;
public:
    double          varoutval(double value) const;
    double          varoutval() const { return varoutval(nullptr); }
    double          varoutval(const CNDIndexes& theIndex) const  { return varoutval(&theIndex); }

private:
    bool            InRange(const CNDIndexes* pTheIndex) const;
public:
    bool            InRange() const { return InRange(nullptr); }
    bool            InRange(const CNDIndexes& theIndex) const { return InRange(&theIndex); }

public:
    // --- managing related data
    void            VarxRefreshRelatedData( int* aIndex );// RHF Jul 21, 2000
    void            VarxRefreshRelatedData( const CNDIndexes& theIndex );// rcl, Jun 17, 2004

    // --- engine links
public:
    void    SetEngineDriver( CEngineDriver* pEngineDriver );

    // --- occurrences' management: array remapping, getting parent' indexes    // victor Jul 21, 00
public:
    void    PassIndexFrom3DToEngine( int* aIndex, CNDIndexes& a3DIndex );

    // PassIndexFrom3DToEngine( CNDIndexes& aNDIndex )
    //   Decreases (by 1) indexes  e,g ( 4, 3, 1 ) -> (3, 2, 0)
    CNDIndexes PassIndexFrom3DToEngine( CNDIndexes& aNDIndex ); // rcl, Jun 21, 2004
    void    PassIndexFromEngineTo3D( int* aIndex, int* a3DIndex );      // victor May 21, 01

    void    checkIndexUsed( int* aIndex, bool bCheckTotal ) /*throw( IndexUsedException ) */;

    // aOccur 1 based
    bool    RemapIndexes( int* aIndex, double* aOccur,
                          bool bCheckTotal = false,
                          bool bGenerateExceptions = false );           // rcl, May 12, 04

    //      For "focus" VARXs only, completes the whole set of indexes
    //      (integer' type arguments)
    bool    BuildIntParentIndexes( CNDIndexes& theIndex, int dOccur );  // rcl, Jun 15 2004

    // ==> TRANSITION: set THE proper index (working with 1-dimension only) // victor Jul 19, 00
    //      theIndex can be partially initialized (only base specified)
    bool    PassTheOnlyIndex( CNDIndexes& theIndex, int iOccur ); // updated rcl Jun 21, 2004

private:
    CString GetValue(const CNDIndexes* pTheIndex) const;

public:
    CString GetValue() const { return GetValue(nullptr); }
    CString GetValue(const CNDIndexes& theIndex) const { return GetValue(&theIndex); }
    void    SetValue(const CNDIndexes& theIndex, CString csValue, bool* value_has_been_modified = nullptr);

private:
    const Logic::SymbolTable& GetSymbolTable() const;
};

const bool CHECK_TOTAL = true;
const bool GENERATE_EXCEPTIONS = true;
