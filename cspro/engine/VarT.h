#pragma once

//---------------------------------------------------------------------------
//  File name: VarT.h
//
//  Description:
//          Header for engine-Var class
//
//  History:    Date       Author   Comment
//              ---------------------------
//              15 Jul 99   RHF     Basic conversion
//              10 Mar 00   RHF     Adding "utilization" data
//              21 Mar 00   RCH     Delete unused SYMTValueSetOf field
//              05 Jun 00   vc      Adding ptrs (connecting CCASE3, improve execution speed)
//              10 Jul 00   vc      Adding DimSize & DimType
//              20 Jul 00   RHF     Adding overloaded Get/SetDimSize for all dimensions
//              20 Jul 00   vc      Adding GetOwnerFlow, GetMaxOccsInDim
//              28 Jul 00   RHF     Adding GetForm
//              30 Sep 00   vc      Implement searching for previous persistent fields
//              27 Oct 00   vc      Adding 'OccGenerator' attribute & methods
//              27 Dec 00   RHF     Adding Verification flags
//              14 May 01   RHF     Adding specific behavior flags for the Var
//
//---------------------------------------------------------------------------

#include <zDictO/CaptureInfo.h>
#include <engine/ChainedSymbol.h>
#include <engine/Dict.h>
#include <engine/dimens.h>
#include <zEngineO/AllSymbolDeclarations.h>
#include <zEngineO/RunnableSymbol.h>
#include <Zissalib/SecT.h>


class CaseItem;
class CDataDict;
class CDEForm;
class CDictItem;
class DictValueSet;
class CEngineArea;
class CEngineDriver;
class EngineBlock;
struct EngineData;
class NumericValueProcessor;
class ValueProcessor;
class ValueSet;
struct VARX;                                            // victor May 24, 00


// Behavior flags                                       // RHF Sep 23, 2004
#define CANENTER_NOTAPPL                    1
#define CANENTER_OUTOFRANGE                 2
#define CANENTER_NOTAPPL_NOCONFIRM          4
#define CANENTER_OUTOFRANGE_NOCONFIRM       8
#define CANENTER_SET_VIA_VALIDATION_METHOD 16

//---------------------------------------------------------------------------
//
//  class CSymbolVar : public RunnableSymbol, public ChainedSymbol
//
//  Description:
//      Engine-variables management
//
//  Construction/destruction/initialization
//      CSymbolVar                  Constructor
//      ~CSymbolVar                 Destructor
//
//  Related objects and miscellaneous relationship
//      GetVarX                     Return the pointer to the VARX entry matching this Var
//      SetVarX                     Set the pointer to the VARX entry matching this Var
//      GetSPT                      Return the pointer to the owner SECT of this Var
//      SetSPT                      Set the pointer to the owner SECT of this Var
//      GetOwnerGroup               Return the iSymbol of the GROUPT formally owning this Var
//      GetOwnerGPT                 Return the pointer to the GROUPT formally owning this Var
//      SetOwnerGroup               Set both the iSymbol and the pointer to the GROUPT formally owning this Var
//      GetOwnerFlow                Return the Flow pointer of the Flow owning this Var
//      GetOwnerDic                 Return the iSymDic owning this Var
//      GetOwnerSec                 Return the iSymSec owning this Var
//      SetSubItemOf                Set both the symbol and its VarT pointer of the owner Var (reserved to subItems only)
//      GetOwnerSymItem             Return the symbol of the owner Var if subItems, 0 otherwise
//      GetOwnerVarT                Return the VarT pointer of the owner Var if subItems, NULL otherwise
//      GetParentGPT                Return the pointer to the GROUPT holding the occurrences of this Var
//      SetParentGPT                Set the pointer to the GROUPT holding the occurrences of this Var
//      IsInAForm                   True if the Var is associated to a Form
//      GetFormSymbol               The associated Form (0 if none)
//      GetForm                     Return the CDEForm pointer of the Form if any
//
//  Linked IMSA Item
//      SetDictItem                 Set the pointer to the CDictItem defining this Var
//      GetDictItem                 Return the pointer to the CDictItem defining this Var
//
//  Basic data
//      GetFmt                      Return 'format' code (N/A/X)
//      SetFmt                      Set the 'format' code (N/A/X)
//      IsNumeric                   Return true if Numeric
//      GetClass                    Return 'class' code (S/M)
//      SetClass                    Set the 'class' code (S/M)
//      GetLength                   Return the length of field
//      SetLength                   Set the length of field
//      GetDecimals                 Return the number of decimals (0 for non-numeric)
//      SetDecimals                 Set the number of decimals
//      GetLocation                 Get the location in its record (1-based)
//      SetLocation                 Set the location in its record
//
//  Operating conditions
//      GetMaxOccs                  Return the max-occs in its record
//      SetMaxOccs                  Set the max-occs in its record, and set the 'class' code accordingly
//
//  Dimensions management
//      GetNumDim                   Return the # of indexes required by this Var (equiv. the # of dimensions)
//      SetNumDim                   Set the # of indexes required by this Var
//      GetDimType                  Return the dimension-type for one dimension
//      SetDimType                  Set the dimension-type for one dimension to a given type
//      GetDimSize                  Return the dimension-size for one dimension (or all dimensions)
//      SetDimSize                  Set the dimension-size for one dimension (or all dimensions) to (a) given size(s)
//      SetDimFeatures              Sets all the dimension' data related to this Var
//      IsArray                     Return true if this must have a sub-index when mentioned in procedures
//      GetFullNumOccs              Return the full-number-of-occs for this Var (multiplied by owners' occurrences) in its Sect
//                                  ... by default, the max-occs inside its Section only
//                                  ... if bBySecOccs is true, multiplied by Sect' max-occs
//      GetMaxOccsInDim             Return the dimension-size for one dimension, changing 0 by 1
//
//  Associated field features
//  ... added methods for Persistent/Protected condition    // Feb 29, 00
//      IsPersistent                True if the Item is a persistent-field
//      SetPersistent               Set the persistent-field' condition to true/false
//      NeedVerification            True if the Item need verification
//      SetNeedVerification         Set the verification-field' condition to true/false
//      IsProtected                 True if the Item is a currently-protected-field
//      IsPersistentOrProtected     True if the Item is a persistent-field or a currently-protected-field
//      SearchPreviousPersistent    Return pVarT of the previous persistent field if any at same level
//      GetBehavior                 Return the current field-behavior' value
//      SetBehavior                 Set the current field-behavior to a given value
//      ResetBehavior               Set the current field-behavior to the defined field-behavior
//      GetDefinedBehavior          Return the defined field-behavior' value
//      SetDefinedBehavior          Set the defined field-behavior to a given value (and the current field-behavior too)
//
//  Special codes & ranges
//      DoRanges                    Build the list of range-pairs
//      SortRanges
//
//  Utilization in execution
//      IsUsed                      Return true if this is used in procedures
//      SetUsed                     Set this as used in procedures
//      IsMarked                    Return true if marked
//      SetMarked                   Set this as marked
//      SetOccGenerator             Set the "occurrences-genertor" condition to a given value
//      IsOccGenerator              True if the variable must update the occurrences when setting a value
//
//  Other methods
//      GetLevel                    Return the level of the owner Sec
//      dvaltochar                  Build an Ascii image of a given value according to this Var' features (for Numeric Vars only)
//
//---------------------------------------------------------------------------

#include <engine/flddef.h>

// VART : variable
class CSymbolVar : public RunnableSymbol, public ChainedSymbol
{
public:

// --- Data members --------------------------------------------------------
    // --- related objects and miscellaneous relationship
public:
    VARX*   m_pVarX;                    // associated VARX pointer - NULL if none
    SECT*   m_pSecT;                    // associated SECT pointer      // victor Jun 10, 00
    int     m_iSymGroup;                // owner Group - only one, 0 if none
    GROUPT* m_pOwnerGroupT;             // owner Group pointer - NULL if none

    // --- linked IMSA Item
private:
    const CDictItem* m_pDictItem;       // associated CDictItem pointer // RHF Jul 25, 2000

    // --- basic data
private:
    bool                                m_bZeroFill;
    bool                                m_bDecChar;
    csprochar                           m_cFmt;                    // format code
    int                                 m_iNumDec;                 // decimals
    csprochar                           m_cClas;                   // class
    int                                 m_iLength;                 // length
    int                                 m_iLocation;               // location in its record


    // --- operating conditions
public:
    int     m_iMaxOccs;                 // occurrences                  // victor Jul 04, 00
    int     m_iOwnerSymItem;            // owner Item, if this is a subitem (0 if none)
    VART*   m_pOwnerVarT;               // VART pointer to the owner Item, if this is a subitem (NULL if none)
    GROUPT* m_pParentGroupT;            // effective parent Group pointer
    EngineBlock* m_engineBlock;         // the block that this is part of (null if none)

    // --- dimensions management
private:
    int     m_iNumDim;                  // # of dimensions of this Var  // victor Jun 10, 00
    CDimension::VDimType m_aDimType[DIM_MAXDIM];   // dimension type               // victor Jul 10, 00
    int     m_aDimSize[DIM_MAXDIM];    // size of each dimension        // victor Jul 10, 00

    // --- associated field features
public:
    int     SYMTfrm;                    // Form' SYMT index (-1: no assoc Form)
    csprochar m_iBehavior;              // behavior flag                // RHF May 14, 01

private:
    FieldBehavior            m_DefBehavior;              // defined field-behavior
    FieldBehavior            m_CurBehavior;              // current field-behavior // formerly 'fldpause'

    bool                     m_bDefVisible;
    bool                     m_bCurVisible;

    bool    m_bIsPersistent;            // is "persistent" in FormFile
    bool    m_bIsDummyPersistent;       // is "persistent" in FormFile for compiler // BMD 11 Mar 2003
    bool    m_bIsPersistentProtected;
    bool    m_bIsAutoIncrement;
    bool    m_bIsSequential;            // is "sequential" in FormFile //SAVY october 24,2002
    bool    m_alwaysVisualValue;
    bool    m_bNeedVerification;        // need verification?

#ifdef WIN_DESKTOP
    HKL     m_hKL;                      // 20120820 ... is a special keyboard specified for the field?
#endif

    // capture information
    CaptureInfo m_captureInfo;
    mutable std::optional<CaptureInfo> m_evaluatedCaptureInfo;
    bool m_showQuestionText;
    bool m_showExtendedControl;
    bool m_showExtendedControlTitle;
    POINT m_capturePos;

    CString* m_pLogicString;            // 20140325 for variable length strings defined in logic

    // --- utilization in execution                     // RHF Mar 10, 2000
private:
    bool    m_bIsUsed;                  // is used in execution (true/false)
    bool    m_bMarked;                  // flag for other usage (i.e. Frequency)

    bool    m_bOccGenerator;            // true if this var must update the occurrences when setting a value

    int     m_iAbsoluteFlowOrder;  // RHF Dec 10, 2003 Zero base absolute flow order. -1 if field is not in a flow

    std::unique_ptr<EngineItemAccessor> m_engineItemAccessor;

    // --- engine links
public:
    CEngineDriver*  m_pEngineDriver;
    CEngineArea*    m_pEngineArea;
    EngineData*     m_engineData;

// --- Methods -------------------------------------------------------------
    // --- construction/destruction/initialization
public:
    CSymbolVar(std::wstring name, CEngineDriver* pEngineDriver);
    ~CSymbolVar(); // RHF Aug 28, 2002

    // --- related objects and miscellaneous relationship
public:
    VARX*   GetVarX( void ) const           { return m_pVarX; }         // victor May 24, 00
    void    SetVarX( VARX* pVarX )          { m_pVarX = pVarX; }        // victor May 24, 00
    SECT*   GetSPT( void ) const            { return m_pSecT; }         // victor Jun 10, 00
    void    SetSPT( SECT* pSecT )           { m_pSecT = pSecT; }        // victor Jun 10, 00

    DICT*   GetDPT( void ) const            { ASSERT( m_pSecT != NULL );
                                              return m_pSecT->GetDicT(); } // RHF Aug 22, 2002

    int     GetOwnerGroup() const           { return m_iSymGroup; }     // victor May 24, 00
    GROUPT* GetOwnerGPT( void )             { return m_pOwnerGroupT; }  // victor May 24, 00
    const GROUPT* GetOwnerGPT() const       { return m_pOwnerGroupT; }
    void    SetOwnerGroup( int iSymGroup );                             // victor May 24, 00
    FLOW*   GetOwnerFlow( void );                                       // victor Jul 20, 00
    virtual int GetOwnerDic( void ) const; // marked as virtual only so that it is accessible to zEngineO's SymbolCalculator
    int     GetOwnerSec( void ) const;
    bool    IsInAForm() const               { return( SYMTfrm > 0 ); }
    int     GetFormSymbol() const           { return( IsInAForm() ? SYMTfrm : 0 ); }
    CDEForm* GetForm( void ) const;
    void    SetSubItemOf( int iSymItem );                               // victor Jul 04, 00
    int     GetOwnerSymItem( void )         { return m_iOwnerSymItem; } // victor Jul 04, 00
    VART*   GetOwnerVarT() const            { return m_pOwnerVarT; }    // victor Jul 04, 00
    virtual GROUPT* GetParentGPT( int iAncestor = 0 ) const;            // RHF Jul 28, 2000, marked as virtual only so that it is accessible to zEngineO's SymbolCalculator
    int     GetParentGroup( int iAncestor = 0 );                        // RHF Oct 24, 2000
    void    SetParentGPT( GROUPT* pGroupT ) { m_pParentGroupT = pGroupT; } // victor Jun 10, 00
    bool    IsAncestor( int iSymGroup, bool bCountingOccurrences );     // RHF Oct 31, 2000
    VART*   GetNextSubItem() const;                                     // RHF Nov 02, 2000

    EngineBlock* GetEngineBlock() const            { return m_engineBlock; }
    void SetEngineBlock(EngineBlock& engine_block) { m_engineBlock = &engine_block; }

                                                                        // RHF Jul 25, 2000
    // --- linked IMSA Item
public:
    void SetDictItem(const CDictItem* pDictItem) { m_pDictItem = pDictItem; }
    const CDictItem* GetDictItem() const         { return m_pDictItem; }

    const CDataDict* GetDataDict() const
    {
        ASSERT( GetDPT() != NULL );
        return GetDPT()->GetDataDict();
    }

    const CaseItem* GetCaseItem() const;

    // --- basic data
public:
    csprochar GetFmt( void ) const          { return m_cFmt; }
    void    SetFmt( csprochar cFmt )        { m_cFmt = cFmt; }
    bool    IsNumeric( void ) const         { return( m_cFmt == 'N' ); }
    bool    IsAlpha( void ) const           { return !IsNumeric(); }
    DataType GetDataType() const            { return IsNumeric() ? DataType::Numeric : DataType::String; }
    csprochar GetClass( void ) const        { return m_cClas; }
    void    SetClass( csprochar cClas )     { m_cClas = cClas; }
    int     GetLength( void ) const         { return m_iLength; }
    void    SetLength( int iLength )        { m_iLength = iLength; }
    int     GetDecimals( void ) const       { return m_iNumDec; }
    void    SetDecimals( int iNumDec )      { m_iNumDec = iNumDec; }

    bool    GetDecChar(void) const          { return m_bDecChar; }
    void    SetDecChar(bool bDecChar)       { m_bDecChar = bDecChar; }
    bool    GetZeroFill(void) const         { return m_bZeroFill; }
    void    SetZeroFill(bool bZeroFill)     { m_bZeroFill = bZeroFill; }

    int     GetLocation( void ) const       { return m_iLocation; }
    void    SetLocation( int iLoc )         { m_iLocation = iLoc; }

    // --- operating conditions
public:
    int     GetMaxOccs( void ) const        { return m_iMaxOccs; }
    void    SetMaxOccs( int iMax ) {
                if( iMax < 1 )
                    iMax = 1;
                m_iMaxOccs = iMax;
                SetClass( (csprochar) (( m_iMaxOccs > 1 ) ? CL_MULT : CL_SING) );
    }

    // --- dimensions management
public:
    int     GetNumDim( void ) const         { return m_iNumDim; }       // victor Jun 10, 00
    void    SetNumDim( int iNumDim )        { m_iNumDim = iNumDim; }    // victor Jun 10, 00

    int m_aSizeForThisType[DIM_MAXDIM];

    // GetDimType
    //
    // Returns the nature of the  i'th dimension for this variable
    // If this variable, for instance has 2 dimensions, one for record occurrence and
    // the second for subitem occ, then
    //     GetDimType(0) will return CDimension::Record
    //     GetDimType(1) will return CDimension::SubItem
    CDimension::VDimType GetDimType( int iDim ) const;
    void    SetDimType( int iDim, CDimension::VDimType xType );
    int     GetDimSize( int iDim );
    void    SetDimSize( int iDim, int iSize );
    void    SetDimSize( const int* iDimSize );

    bool    SetDimFeatures( void );                                     // victor Jul 19, 00 // formerly 'SetVarIsArray'
    bool    IsArray() const { return ( m_iNumDim > 0 ); }               // victor Jun 10, 00
    int     GetFullNumOccs( bool bBySecOccs = false );                  // victor Jul 10, 00

    // marked as virtual only so that it is accessible to zEngineO
    virtual int GetMaxOccsInDim( CDimension::VDimType dType );          // victor Jul 20, 00
    int GetMaxOccsInDim( int iDim ) const;                              // victor Jul 20, 00

    // GetMaxOccsInFixedDim( int iDim )
    // if variable uses 2 dimensions, this method uses the correct index, like
    // if a record is index 0, item is index 1 and subitem is index 2
    // if you meant discover dimensions,
    // use GetMaxOccsInDim method instead.
    //
    int     GetMaxOccsInFixedDim( int iDim ) const;


    // --- associated field features
public:
    bool    IsPersistent() const              { return m_bIsPersistent; }
    void    SetPersistent(bool bPersistent)   { m_bIsPersistent = bPersistent; }

    bool    IsDummyPersistent() const                 { return m_bIsDummyPersistent; }  // BMD 11 Mar 2003
    void    SetDummyPersistent(bool bDummyPersistent) { m_bIsDummyPersistent = bDummyPersistent; }  // BMD 11 Mar 2003

    bool    IsPersistentProtected() const                       { return m_bIsPersistentProtected; }
    void    SetPersistentProtected(bool bIsPersistentProtected) { m_bIsPersistentProtected = bIsPersistentProtected; }

    bool    IsAutoIncrement() const               { return m_bIsAutoIncrement; }
    void    SetAutoIncrement(bool bAutoIncrement) { m_bIsAutoIncrement = bAutoIncrement; }

    bool    IsSequential() const                { return m_bIsSequential; }//SAVY october 24,2002
    void    SetSequential( bool bIsSequential ) { m_bIsSequential = bIsSequential; }//SAVY october 24,2002

    bool    IsAlwaysVisualValue() const     { return m_alwaysVisualValue; }
    void    SetAlwaysVisualValue(bool flag) { m_alwaysVisualValue = flag; }

    bool    NeedVerification() const                    { return m_bNeedVerification; }
    void    SetNeedVerification(bool bNeedVerification) { m_bNeedVerification = bNeedVerification; }

    bool    IsProtected() const             { return ( m_CurBehavior == AsProtected ); }
    bool    IsPersistentOrProtected() const { return ( IsPersistent() || IsProtected() ); }
    bool    IsProtectedOrNoNeedVerif();
    VART*   SearchPreviousPersistent();

    int     GetBehavior() const                  { return m_CurBehavior; }
    void    SetBehavior(FieldBehavior eBehavior) { m_CurBehavior = eBehavior; }
    void    ResetBehavior()                      { m_CurBehavior = m_DefBehavior; }
    FieldBehavior  GetDefinedBehavior()          { return m_DefBehavior; }
    void    SetDefinedBehavior( FieldBehavior eBehavior) {
                m_DefBehavior = eBehavior;
                ResetBehavior();
    }

    bool    AddNotApplToValueSet() const { return ( ( ( m_iBehavior & CANENTER_NOTAPPL ) != 0 ) &&
                                                    ( ( m_iBehavior & CANENTER_SET_VIA_VALIDATION_METHOD ) == 0 ) ); }

    bool    GetVisible() const        { return m_bCurVisible; }
    void    SetVisible(bool bVisible) { m_bCurVisible = bVisible; }

    void    ResetVisible()            { m_bCurVisible = m_bDefVisible; }

    bool    GetDefinedVisible() const { return m_bDefVisible; }

    void    SetDefinedVisible( bool bVisible ) {
            m_bDefVisible = bVisible;
            ResetVisible();
    }

#ifdef WIN_DESKTOP
    HKL     GetHKL() const { return m_hKL; }
    void    SetHKL(HKL hKL) { m_hKL = hKL; }
#endif

    // capture information
    const CaptureInfo& GetCaptureInfo() const { return m_captureInfo; }
    void SetCaptureInfo(const CaptureInfo& capture_info);
    const CaptureInfo& GetEvaluatedCaptureInfo() const;

    bool GetShowQuestionText() const    { return m_showQuestionText; }
    void SetShowQuestionText(bool flag) { m_showQuestionText = flag; }

    bool GetShowExtendedControl() const     { return m_showExtendedControl; }
    void SetShowExtendedControl(bool flag);

    bool GetShowExtendedControlTitle() const    { return m_showExtendedControlTitle; }
    void SetShowExtendedControlTitle(bool flag) { m_showExtendedControlTitle = flag; }

    const POINT& GetCapturePos() const           { return m_capturePos; }
    void SetCapturePos(const POINT& capture_pos) { m_capturePos = capture_pos; }


    CString*        GetLogicStringPtr()         { return m_pLogicString; } // 20140325 these functions are for variable length strings
    const CString*  GetLogicStringPtr() const   { return m_pLogicString; }
    void            AllocateLogicStringMemory() { m_pLogicString = new CString(); }

    // --- utilization in execution
public:
    bool    IsUsed() const              { return m_bIsUsed; }
    virtual void SetUsed(bool used); // marked as virtual only so that it is accessible to zEngineO
    bool    IsMarked() const        { return( m_bMarked ); }        // RHF Mar 21, 2000
    void    SetMarked(bool bMarked) { m_bMarked = bMarked; }        // RHF Mar 21, 2000

    void    SetOccGenerator( bool bX = true ) { m_bOccGenerator = bX; }   // victor Oct 27, 00
    bool    IsOccGenerator() const            { return m_bOccGenerator; } // victor Oct 27, 00

    void    LoadPersistentValue(CString csValue);

    // --- other methods
public:
    virtual int GetLevel() const; // marked as virtual only so that it is accessible to zEngineO's SymbolCalculator
    void    dvaltochar( const double dValue, csprochar* pBuf ) const;

private:
    bool m_bNeedConvertSomeSubItem;
public:
    void DoNeedConvertSomeSubItem();
    bool NeedConvertSomeSubItem() const { return m_bNeedConvertSomeSubItem; }

private:
    std::vector<double> m_aVectorValue; 
    std::vector<double> m_aVectorOcc;
    int m_iVectorIndex;
public:
    void VectorAdd( double dValue, double dOcc ) {
        m_iVectorIndex++;
        if( m_iVectorIndex >= (int)m_aVectorValue.size() ) {
            ASSERT(m_aVectorValue.size() == m_aVectorOcc.size());
            m_aVectorValue.resize(m_iVectorIndex + 1);
            m_aVectorOcc.resize(m_iVectorIndex + 1);
        }
        m_aVectorValue[m_iVectorIndex] = dValue;
        m_aVectorOcc[m_iVectorIndex] = dOcc;
    }
    double VectorGet( int i, double& dOcc ) {
        dOcc = m_aVectorOcc[i];
        return m_aVectorValue[i];
    }
    int     VectorSize() const { /*return m_aVectorValue.GetSize(); */ return m_iVectorIndex+1; }
    void    VectorClean()      {/* m_aVectorValue.RemoveAll(); m_aVectorOcc.RemoveAll(); */ m_iVectorIndex =-1;}

    CString GetAsciiValue(int iOccur);
    CString GetBlankValue() const;

private:
    bool    m_bSkipStructImpute;

public:
    void    SetSkipStrucImpute( bool bSkipStructImpute )    { m_bSkipStructImpute = bSkipStructImpute; }
    bool    IsSkipStrucImpute()                             { return m_bSkipStructImpute; }

private:
    std::shared_ptr<const ValueSet> m_baseValueSet;
    std::shared_ptr<const ValueSet> m_currentValueSet;
    mutable std::shared_ptr<const ValueProcessor> m_currentValueProcessor;

public:
    const ValueSet* GetBaseValueSet() const { return m_baseValueSet.get(); }
    void SetBaseValueSet(std::shared_ptr<const ValueSet> value_set);
    void DeleteCurrentValueSet();
    void ResetCurrentValueSet() { SetCurrentValueSet(m_baseValueSet); }
    const ValueSet* GetCurrentValueSet() const { return m_currentValueSet.get(); }
    const DictValueSet* GetCurrentDictValueSet() const;
    void SetCurrentValueSet(std::shared_ptr<const ValueSet> value_set);

    const ValueProcessor& GetCurrentValueProcessor() const;
    const NumericValueProcessor& GetCurrentNumericValueProcessor() const;

public:
    void SetAbsoluteFlowOrder(int iAbsoluteFlowOrder) { m_iAbsoluteFlowOrder = iAbsoluteFlowOrder; }
    int GetAbsoluteFlowOrder() const                  { return m_iAbsoluteFlowOrder; }

    // Symbol overrides
    EngineItemAccessor* GetEngineItemAccessor() const override;

    Symbol* FindChildSymbol(const std::wstring& symbol_name) const override;

    void serialize_subclass(Serializer& ar) override;

protected:
    void WriteJsonMetadata_subclass(JsonWriter& json_writer) const override;

private:
    const Logic::SymbolTable& GetSymbolTable() const;

private:
    int m_containerIndex = 0; // the container table index

public:
    int GetContainerIndex() const  			    { return m_containerIndex; }
    void SetContainerIndex(int container_index) { m_containerIndex = container_index; }
};
