//---------------------------------------------------------------------------
//  File name: VarT.cpp
//
//  Description:
//          Implementation for engine-Var class
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
//              30 Sep 00   vc      Implement searching for adjacent persistent fields
//              27 Oct 00   vc      Adding 'OccGenerator' attribute & methods
//              27 Dec 00   RHF     Adding Verification flags
//
//---------------------------------------------------------------------------
#include "StdAfx.h"
#include <zToolsO/Serializer.h>
#include <zToolsO/VarFuncs.h>
#include <zJson/Json.h>
#include <zDictO/ValueProcessor.h>
#include <engine/Engarea.h>
#include <engine/Engdrv.h>
#include <engine/Entdrv.h>
#include <engine/IntDrive.h>
#include <engine/Tables.h>
#include <zEngineO/EngineItem.h>
#include <zEngineO/ValueSet.h>


const Logic::SymbolTable& CSymbolVar::GetSymbolTable() const
{
    return m_engineData->symbol_table;
}


/////////////////////////////////////////////////////////////////////////////
//
// --- construction/destruction/initialization
//
/////////////////////////////////////////////////////////////////////////////

CSymbolVar::CSymbolVar(std::wstring name, CEngineDriver* pEngineDriver)
    :   ChainedSymbol(std::move(name), SymbolType::Variable),
        m_engineBlock(nullptr)
{
    m_pEngineDriver = pEngineDriver;
    m_pEngineArea = pEngineDriver->getEngineAreaPtr();
    m_engineData = m_pEngineDriver->m_engineData;

    // --- related objects and miscellaneous relationship
    m_pVarX          = NULL;            // associated VARX pointer
    m_pSecT          = NULL;            // associated SECT pointer
    SetOwnerGroup( 0 );                 // owner Group
    m_bNeedConvertSomeSubItem = false;
    m_iVectorIndex = -1;

    // --- linked IMSA Item
    SetDictItem( NULL );                                // RHF Jul 25, 2000

    // --- basic data
    SetFmt( 'A' );                      // format code
    SetClass( CL_SING );                // class
    SetLength( 0 );                     // length
    SetDecimals( 0 );                   // decimals
    SetLocation( 0 );                   // location in its record

    m_bZeroFill = false;
    m_bDecChar = true;


    // --- operating conditions
    m_iMaxOccs       = 0;               // occurrences
    SetSubItemOf( 0 );                  // owner Item (for sub-items only)
    SetParentGPT( NULL );               // effective parent Group pointer

    // --- dimensions management
    SetNumDim( 0 );                     // # of dimensions of this Var
    for( int iDim = 0; iDim < DIM_MAXDIM; iDim++ ) {
        SetDimType( iDim, CDimension::VoidDim );
        SetDimSize( iDim, 1 );          // size of each dimension
    }

    // --- associated field features
    SYMTfrm          = 0;               // associated Form
    m_iBehavior      = 0;
    SetDefinedBehavior( AsAutoSkip );
    ResetBehavior();                    // current field-behavior

    SetDefinedVisible( true );
    ResetVisible();  // current field-visibility

    SetPersistent( false );             // is "persistent" in FormFile
    SetPersistentProtected(false);
    SetNeedVerification( true );// RHF Dec 27, 2000
    SetAutoIncrement(false);
    SetSequential(false); //SAVY Oct 24 ,2002
    SetAlwaysVisualValue(false);

    // --- utilization in execution                     // RHF Mar 10, 2000
    SetUsed( false );
    SetMarked( false );
    SetOccGenerator( false );                           // victor Oct 27, 00

    m_bSkipStructImpute = true; // RHF Nov 09, 2001

    SetAbsoluteFlowOrder( -1 ); // RHF Dec 10, 2003

    #define IS_INSIDE_LIMITS(x)  (((int)x)>=0 && ((int)x) < DIM_MAXDIM)
    ASSERT( CDimension::Record != CDimension::Item &&
            CDimension::Record != CDimension::SubItem &&
            CDimension::Item != CDimension::SubItem &&
            IS_INSIDE_LIMITS( CDimension::Record ) &&
            IS_INSIDE_LIMITS( CDimension::Item ) &&
            IS_INSIDE_LIMITS( CDimension::SubItem ) );
    m_aSizeForThisType[CDimension::Record]  = 1;
    m_aSizeForThisType[CDimension::Item]    = 1;
    m_aSizeForThisType[CDimension::SubItem] = 1;

#ifdef WIN_DESKTOP
    SetHKL(NULL); // GHM 20120820
#endif

	m_showQuestionText = true;
    m_showExtendedControl = true;
    m_showExtendedControlTitle = true;
    m_capturePos = POINT { -1, -1 };

    m_pLogicString = nullptr;

    SetDummyPersistent(false); // GHM 20121120 for boost serialization all bools must be initialized
}

CSymbolVar::~CSymbolVar()
{
    DeleteCurrentValueSet();
    delete m_pLogicString;
}

//////////////////////////////////////////////////////////////////////////////
//
// --- related objects and miscellaneous relationship
//
//////////////////////////////////////////////////////////////////////////////

void CSymbolVar::SetOwnerGroup( int iSymGroup ) {             // victor May 24, 00
    // SetOwnerGroup: setup owner group (both the iSymbol and the GPT' pointer)
    if( iSymGroup > 0 ) {
        m_iSymGroup    = iSymGroup;
        m_pOwnerGroupT = GPT(m_iSymGroup);
    }
    else {
        m_iSymGroup    = 0;
        m_pOwnerGroupT = NULL;
    }
}

FLOW* CSymbolVar::GetOwnerFlow( void ) {                      // victor Jul 20, 00
    // GetOwnerFlow: return the Flow pointer of the Flow owning this Var
    return GetOwnerGPT()->GetFlow();
}

int CSymbolVar::GetOwnerDic( void ) const {
    // GetOwnerDic: return the iSymDic owning this Var
    SECT*   pSecT   = GetSPT();
    DICT*   pDicT   = ( pSecT != NULL ) ? pSecT->GetDicT() : NULL;
    int     iSymDic = ( pDicT != NULL ) ? pDicT->GetSymbolIndex() : 0;

    return iSymDic;
}

int CSymbolVar::GetOwnerSec() const
{
    // GetOwnerSec: return the iSymSec owning this Var
    const SECT* pSecT = GetSPT();
    int iSymSec = ( pSecT != NULL ) ? pSecT->GetSymbolIndex() : 0;
    return iSymSec;
}

CDEForm* CSymbolVar::GetForm( void ) const {
    if( SYMTfrm <= 0 )
        return NULL;

    FORM*   pForm = FPT(SYMTfrm);

    return( pForm->GetForm() );
}

const CaseItem* CSymbolVar::GetCaseItem() const
{
    const auto& case_access = GetDPT()->GetCaseAccess();
    const CaseItem* case_item = ( case_access != nullptr ) ? case_access->LookupCaseItem(*GetDictItem()) : nullptr;
    ASSERT(case_item != nullptr);
    return case_item;
}

void CSymbolVar::SetSubItemOf( int iSymItem ) {               // victor Jul 04, 00
    // SetSubItemOf: set both the symbol and its VarT pointer of the owner Var
    //             - reserved to subItems only
    if( iSymItem > 0 ) {                // is a subItem
        m_iOwnerSymItem = iSymItem;
        m_pOwnerVarT    = VPT(iSymItem);
    }
    else {                              // is a true Item
        m_iOwnerSymItem = 0;
        m_pOwnerVarT    = NULL;
    }
}

GROUPT* CSymbolVar::GetParentGPT( int iAncestor /*=0*/ ) const { // RHF Jul 28, 2000
    if( iAncestor == 0 )
        return m_pParentGroupT;
    else {
        GROUPT* pParentGroupT=m_pParentGroupT;

        for(; pParentGroupT != NULL && iAncestor > 0; iAncestor-- )
            pParentGroupT = pParentGroupT->GetParentGPT();

        return pParentGroupT;
    }
}

int CSymbolVar::GetParentGroup( int iAncestor /*=0*/ ) {      // RHF Oct 24, 2000
    GROUPT* pGroupT=GetParentGPT( iAncestor );

    return pGroupT ? pGroupT->GetSymbolIndex() : 0;
}


bool CSymbolVar::IsAncestor( int iSymGroup, bool bCountingOcurrences ) { // RHF Oct 31, 2000
    GROUPT* pGroupT;
    bool    bFound=false;

    pGroupT= bCountingOcurrences ? this->GetParentGPT() : this->GetOwnerGPT();
    while( pGroupT != NULL && !bFound ) {
        if( pGroupT->GetSymbolIndex() == iSymGroup ) {
            bFound = true;
            continue;
        }
        pGroupT = bCountingOcurrences ? pGroupT->GetParentGPT() : pGroupT->GetOwnerGPT();
    }

    return bFound;
}

VART* CSymbolVar::GetNextSubItem() const
{
    // RHF Nov 02, 2000
    int     iSymNext = this->SYMTfwd;
    VART*   pVarTSubItem = NULL;
    bool    bItem = ( GetOwnerVarT() == NULL );

    if( iSymNext > 0 ) {
        pVarTSubItem = VPT(iSymNext);

        if( bItem ) {
            if( pVarTSubItem->GetOwnerVarT() != this )
                pVarTSubItem = NULL;    // This is not sub-item of this
        }
        else {
            if( pVarTSubItem->GetOwnerVarT() != GetOwnerVarT() )
                pVarTSubItem = NULL;    // This is not sub-item of this
        }
    }

    return pVarTSubItem;
}


/////////////////////////////////////////////////////////////////////////////
//
// --- dimensions management
//
/////////////////////////////////////////////////////////////////////////////

bool CSymbolVar::SetDimFeatures( void ) {
    // SetDimFeatures: sets all the dimension' data related to this Var
    // UP TO 3 INDEXES ALLOWED:         // full remake  // victor Jul 04, 00
    SECT*   pSecT          = GetSPT();
    VART*   pThisVarT      = this;
    bool    bIsSubItem     = ( pThisVarT->GetOwnerSymItem() > 0 );
    VART*   pOwnerVarT     = ( bIsSubItem ) ? pThisVarT->GetOwnerVarT() : NULL;

    bool    bIsMultSection = ( pSecT->GetMaxOccs() > 1 );
    bool    bIsMultItem    = false;
    bool    bIsMultSubItem = false;

    if( !bIsSubItem )                   // is an Item
        bIsMultItem    = ( pThisVarT->GetClass() == CL_MULT );
    else {                              // is a subItem
        bIsMultItem    = ( pOwnerVarT->GetClass() == CL_MULT );
        bIsMultSubItem = ( pThisVarT->GetClass() == CL_MULT );
    }

    // setting dimension' related features
    int         iDim  = 0;

    if( bIsMultSection ) {
        // dimension referring to the Record
        pThisVarT->SetDimType( iDim, CDimension::Record );
        pThisVarT->SetDimSize( iDim, pSecT->GetMaxOccs() );

        iDim += 1;
    }
    if( !bIsSubItem ) {                 // is an Item
        if( bIsMultItem ) {
            // dimension referring to the Item
            pThisVarT->SetDimType( iDim, CDimension::Item );
            pThisVarT->SetDimSize( iDim, pThisVarT->GetMaxOccs() );

            iDim += 1;
        }
    }
    else {                              // is a subItem
        if( bIsMultItem ) {
            // dimension referring to the owner-Item
            pThisVarT->SetDimType( iDim, CDimension::Item );
            pThisVarT->SetDimSize( iDim, pOwnerVarT->GetMaxOccs() );

            iDim += 1;
        }

        if( bIsMultSubItem ) {
            // dimension referring to the subItem
            pThisVarT->SetDimType( iDim, CDimension::SubItem );
            pThisVarT->SetDimSize( iDim, pThisVarT->GetMaxOccs() );

            iDim += 1;
        }
    }

    // the final number of dimensions is equal to the number of dimensions computed
    int     iNumIndexes = iDim;

    bool    bDimensionsOK = ( iNumIndexes <= 3 );
    ASSERT( bDimensionsOK );            // shouldn't exceed DIM_MAXDIM!!!

    SetNumDim( iNumIndexes );

    DICT*   pDicT=GetDPT();
    if( pDicT )
        pDicT->SetNumDim(std::max(pDicT->GetNumDim(), iNumIndexes) );


    return bDimensionsOK;
}

int CSymbolVar::GetFullNumOccs( bool bBySecOccs ) {           // victor Jul 10, 00
    // GetFullNumOccs: return the full-number-of-occs for this Var (multiplied by owners' occurrences) in its Sect
    //                 ... by default, the max-occs inside its Section only
    //                 ... if bBySecOccs is true, multiplied by Sect' max-occs
    int     iVarOccs;
    VART*   pThisVarT  = this;
    VART*   pOwnerVarT = pThisVarT->GetOwnerVarT();

    // a) the max-occs inside its Section (for one Sect-occ only)
    if( pOwnerVarT == NULL ) {      // this Var is an Item
        iVarOccs = pThisVarT->GetMaxOccs();
    }
    else {                          // this Var is a SubItem
        int     iItemOccs  = pOwnerVarT->GetMaxOccs();
        ASSERT( iItemOccs >= 1 );
        int     iSubItemOccs = pThisVarT->GetMaxOccs();
        ASSERT( iSubItemOccs >= 1 );

        iVarOccs = iItemOccs * iSubItemOccs;
    }

    // b) if requested, mutiplied by its Sect'max-occs
    if( bBySecOccs ) {
        SECT*   pSecT = GetSPT();

        iVarOccs *= pSecT->GetMaxOccs();
    }

    return iVarOccs;
}

CDimension::VDimType CSymbolVar::GetDimType( int iDim ) const
{
    ASSERT( iDim >= 0 && iDim < DIM_MAXDIM );
    return m_aDimType[iDim];
}

void CSymbolVar::SetDimType( int iDim, CDimension::VDimType xType )
{
    ASSERT( iDim >= 0 && iDim < DIM_MAXDIM );
    m_aDimType[iDim] = xType;
}

int CSymbolVar::GetDimSize( int iDim )
{
    ASSERT( iDim >= 0 && iDim < DIM_MAXDIM );
    return m_aDimSize[iDim];
}

void CSymbolVar::SetDimSize( int iDim, int iSize )
{
    ASSERT( iDim >= 0 && iDim < DIM_MAXDIM );
    ASSERT( iSize >= 0 );

    CDimension::VDimType xType = m_aDimType[iDim];

    // First establish type, then size, okay?
    if( xType != CDimension::VoidDim )
    {
        // then, please use something we already know ..
        ASSERT( xType == CDimension::Record ||
                xType == CDimension::Item ||
                xType == CDimension::SubItem );

        m_aSizeForThisType[xType] = iSize;
    }

    m_aDimSize[iDim] = iSize;
}

void CSymbolVar::SetDimSize( const int* iDimSize )
{
    ASSERT(0);
    /* rcl, sep 2004
    for( int iDim = 0; iDim < DIM_MAXDIM; iDim++ ); // <-- ; ?????
    SetDimSize( iDim, iDimSize[iDim] );
    */
}

int CSymbolVar::GetMaxOccsInDim( CDimension::VDimType xType ) {// victor Jul 10, 00
    // GetMaxOccsInDim: return the dimension-size for one dimension
    //                - if dimension-size is zero, changes by 1
    //                - version for dimension-type argument
    int             iSize = 1;

    if( xType != CDimension::VoidDim )
    {
        iSize = GetDimSize( xType );
    }

    return( iSize > 0 ? iSize : 1 );
}

// GetMaxOccsInFixedDim( int iDim )
//
// m_aDimSize saves maximum size for each valid dimension
// be careful, because :
// if variable has 3 dims, then
//     m_aDimSize[0], m_aDimSize[1] and m_aDimSize[2] are valid
// if variable has 2 dims, then
//     m_aDimSize[0] and m_aDimSize[1] are valid, but m_aDimSize[2] is not valid
// if variable has 1 dim, then
//     m_aDimSize[0] is valid and m_aDimSize[1] and m_aDimSize[2] are not valid
//
// many times a CNDIndex object has to check if it has the correct information
// inside, so we need a method that can check every dimension
//
// GetMaxOccsInFixedDim() was created to solve this kind of need.
//
//   GetMaxOccsInFixedDim(0) returns the maximum occurrence for the first dimension
//                           of a CNDIndex object (a Record index)
//   GetMaxOccsInFixedDim(1) returns the maximum occurrence for the 2nd dimension
//                           of a CNDIndex object (an Item index)
//   GetMaxOccsInFixedDim(2) returns the maximum occurrence for the 3rd dimension
//                           of a CNDIndex object (a Subitem index)

int CSymbolVar::GetMaxOccsInFixedDim( int iDim ) const
{
    ASSERT( iDim >= 0 && iDim < DIM_MAXDIM );

    const CDimension::VDimType USE_1st_INDEX = CDimension::Record;
    const CDimension::VDimType USE_2nd_INDEX = CDimension::Item;
    const CDimension::VDimType USE_3rd_INDEX = CDimension::SubItem;

    CDimension::VDimType iIndexToUse = USE_3rd_INDEX;

    if( iDim == GetDimType(0) )
        iIndexToUse = USE_1st_INDEX;
    else
    if( iDim == GetDimType(1) )
        iIndexToUse = USE_2nd_INDEX;

    //  else
    //    iIndexToUse = USE_3rd_INDEX;

    int iOriginalValue = GetMaxOccsInDim( iIndexToUse );

    return iOriginalValue;
}

int CSymbolVar::GetMaxOccsInDim( int iDim ) const
{
    // GetMaxOccsInDim: return the dimension-size for one dimension
    //                - if a void dimension or dimension-size is zero, changes by 1
    //                - version for direct, expected dimension (translated via DimType' array)
    ASSERT( iDim >= 0 && iDim < DIM_MAXDIM );

    int iNewValue = 1;

    if( GetDimType(iDim) != CDimension::VoidDim )
        iNewValue = m_aSizeForThisType[GetDimType(iDim)];

    return iNewValue;
}

/* El metodo original

int CSymbolVar::GetMaxOccsInDim( int iDim ) {                 // victor Jul 20, 00
    // GetMaxOccsInDim: return the dimension-size for one dimension
    //                - if a void dimension or dimension-size is zero, changes by 1
    //                - version for direct, expected dimension (translated via DimType' array)
    ASSERT( iDim >= 0 && iDim < DIM_MAXDIM );
    //// RHF COM Jul 26, 2000CDimension::VDimType    xType = GetDimType( iDim );
    CDimension::VDimType    xType = (iDim==GetDimType(0))? CDimension::Record :
    (iDim==GetDimType(1))? CDimension::Item : CDimension::SubItem;

    return GetMaxOccsInDim( xType );
}
*/


/////////////////////////////////////////////////////////////////////////////
//
// --- associated field features
//
/////////////////////////////////////////////////////////////////////////////

void CSymbolVar::SetUsed(bool used)
{
    m_bIsUsed = used;

    if( m_bIsUsed )
    {
        DICT* pDicT = GetDPT();

        if( pDicT != nullptr )
        {
            CaseAccess* case_access = pDicT->GetCaseAccess();

            if( case_access != nullptr && !case_access->IsInitialized() )
                case_access->SetUseDictionaryItem(*GetDictItem());
        }
    }
}


bool CSymbolVar::IsProtectedOrNoNeedVerif( void ) {           // RHF Dec 28, 2000
    bool    bRet = IsProtected();

    if( Issamod == ModuleType::Entry ) {
        CEntryDriver*   pEntryDriver = (CEntryDriver*) (m_pEngineArea->m_pEngineDriver);

        if( pEntryDriver->IsVerify() ) {
            bool    bNeedVerification = NeedVerification();
            bRet |= !bNeedVerification;
        }
    }

    return bRet;
}

VART* CSymbolVar::SearchPreviousPersistent()
{
    // SearchPreviousPersistent: looks for the previous persistent field at the same level;
    // when nothing found, a NULL is returned
    int     iSymVar = GetSymbolIndex();
    GROUPT* pGroupT = GetOwnerGPT();
    int     iRefLevel = GetLevel();
    int     iRefSlot = pGroupT->GetContainerIndex();
    int     iRefFlowOrder;              // the flow-order of this var
    int     iItem = pGroupT->GetItemIndex( iSymVar, &iRefFlowOrder );

    // bounds of the visible set
    FLOW*   pFlow = pGroupT->GetFlow();
    int     iBaseSlot = pFlow->GetGroupTRoot()->GetContainerIndex();
    int     iHighSlot = iBaseSlot + pFlow->GetNumberOfVisibleGroups() - 1;

    // check that this var is in the visible set
    if( iRefSlot < iBaseSlot || iRefSlot > iHighSlot )
        return NULL;

    // searching accross every groups at the visible set (at same level only)
    int     iSymNearest = 0;
    int     iFlowOrderNearest = 0;

    for( int iGroupT = iBaseSlot; iGroupT <= iHighSlot; iGroupT++ )
    {
        pGroupT = GIP(iGroupT);
        if( pGroupT->GetLevel() != iRefLevel )
            continue;

        for( iItem = 0; pGroupT->IsAnItem( iItem ); iItem++ )
        {
            int iSymbol = pGroupT->GetItemSymbol( iItem );

            if( m_pEngineArea->IsSymbolTypeVA(iSymbol) )
            {
                VART* pVarT = VPT(iSymbol);

                if( pVarT->IsPersistent() && !pVarT->IsPersistentProtected() )
                {
                    int     iFlowOrder = pGroupT->GetFlowOrder( iItem );
                    bool    bChoose = false;

                    if( iFlowOrder < iRefFlowOrder ) {
                        // backwards: the highest flow-order lower than the reference
                        bChoose = ( !iFlowOrderNearest || iFlowOrderNearest < iFlowOrder );
                    }
                    if( bChoose ) {
                        iFlowOrderNearest = iFlowOrder;
                        iSymNearest = iSymbol;
                    }
                }
            }
        }
    }

    return ( iSymNearest ) ? VPT(iSymNearest) : NULL;
}


void CSymbolVar::LoadPersistentValue(CString csValue)
{
    SetBehavior(AsProtected);

    if( csValue.IsEmpty() )
    {
        if( !IsPersistentProtected() )
            SetBehavior(AsAutoSkip); // open the field to allow for its entry
    }

    else
    {
        TCHAR* pszVarText = m_pEngineDriver->m_pIntDriver->GetVarAsciiAddr(this);

        _tmemcpy(pszVarText,csValue,GetLength());

        if( IsNumeric() )
        {
            // persistent fields are always singly occurring
            VARX* pVarX = GetVarX();
            double* pFlotAddr = m_pEngineDriver->m_pIntDriver->GetVarFloatAddr(pVarX);
            *pFlotAddr = chartodval(pszVarText,GetLength(),GetDecimals());
        }
    }
}


//////////////////////////////////////////////////////////////////////////////
//
// --- other methods
//
//////////////////////////////////////////////////////////////////////////////

int CSymbolVar::GetLevel() const {
    // GetLevel: return the level of the owner Sec
    SECT*   pSecT = GetSPT();

    return( ( pSecT != NULL ) ? pSecT->GetLevel() : 0 );
}

// RHF INIC Aug 23, 2002 Better approach should be a little faster than the previous one
void CSymbolVar::dvaltochar( const double dValue, csprochar* pBuf ) const
{
    // dvaltochar: build an Ascii image of a given value according to this Var' features (for Numeric Vars only)

#ifdef _DEBUG
    const CDictItem*  pItem = GetDictItem();
    if( pItem != NULL ) {
        ASSERT( m_iLength == (int)pItem->GetLen() );
        ASSERT( m_iNumDec == (int)pItem->GetDecimal() );
        ASSERT( m_bZeroFill == pItem->GetZeroFill() );
        ASSERT( m_bDecChar == pItem->GetDecChar() );
    }
#endif

    ::dvaltochar( dValue, pBuf, m_iLength, m_iNumDec, m_bZeroFill, m_bDecChar );
}
// RHF END Aug 23, 2002


// RHF INIC May 2, 2001
void CSymbolVar::DoNeedConvertSomeSubItem() {
    m_bNeedConvertSomeSubItem = false;

    VART*   pVarTSubItem=GetNextSubItem();

    // RHF INIC Mar 04, 2004
    bool    bCheckRanges = ( Issamod != ModuleType::Entry && m_pEngineArea->m_pEngineSettings->HasCheckRanges() );
    bool    bSkipStruc   = ( Issamod != ModuleType::Entry && m_pEngineArea->m_pEngineSettings->HasSkipStruc() );
    if( (bCheckRanges || bSkipStruc) && pVarTSubItem != NULL ) {
        m_bNeedConvertSomeSubItem = true;
    }
    else
    // RHF END Mar 04, 2004

    while( pVarTSubItem != NULL ) {
        if( pVarTSubItem->IsUsed() && pVarTSubItem->IsNumeric() ) {
            m_bNeedConvertSomeSubItem = true;
            break;
        }

        pVarTSubItem = pVarTSubItem->GetNextSubItem();
    }
}
// RHF END May 2, 2001


CString CSymbolVar::GetAsciiValue(int iOccur)
{
    // GetAsciiValue: for a given couple (iSymbol,iOccur) retrieves the ascii value from the engine and return it on a given CString
    if( m_pEngineDriver->m_pIntDriver == NULL )
        return GetBlankValue();

    // otherwise, retrieve the actual value
    CIntDriver* pIntDriver = m_pEngineDriver->m_pIntDriver.get();
    VART* pVarT = this;
    int iLength = pVarT->GetLength();
    TCHAR* pVarAsciiAddr = NULL;

    // remark - this code is adapted from
    // ........ CIntDriver::GetVarAsciiValue (VAs only, stack memory instead of allocated)
    // ........ and from CSymbolVar::dvaltocharexec
    if( iOccur <= 0 )                   // get current occurrence if iOccur <= 0
        iOccur = pVarT->GetOwnerGPT()->GetCurrentOccurrences();

    // retrieve the ascii address, no matter if highlighted or not
    VARX*       pVarX   = pVarT->GetVarX();

    csprochar cOldFmt=pVarT->GetFmt();
    if( pVarT->IsNumeric() )
        pVarT->SetFmt( 'X' );

    pVarAsciiAddr = ( pVarT->IsArray() ) ? (csprochar*) pIntDriver->mvaraddr( pVarX, (double) iOccur ) :
                                           (csprochar*) pIntDriver->svaraddr( pVarX );

    pVarT->SetFmt( cOldFmt );

    if( pVarAsciiAddr == NULL ) {
        return GetBlankValue();
    }
    else {
        return CString(pVarAsciiAddr, iLength);
    }
}


CString CSymbolVar::GetBlankValue() const {
    // GetBlankValue: for a given iSymbol, format an empty ascii value into a given CString
    const VART* pVarT     = this;
    int         iLength   = pVarT->GetLength();
    int         iDecimals = pVarT->GetDecimals();

    CString csValue(_T(' '), iLength);

    if( iDecimals )
        csValue.SetAt(iLength - iDecimals - 1, _T('.'));

    return csValue;
}


void CSymbolVar::SetBaseValueSet(std::shared_ptr<const ValueSet> value_set)
{
    m_baseValueSet = std::move(value_set);
    ResetCurrentValueSet();
}


void CSymbolVar::DeleteCurrentValueSet()
{
    m_currentValueProcessor.reset();
    m_currentValueSet.reset();

    m_evaluatedCaptureInfo.reset();
}


const DictValueSet* CSymbolVar::GetCurrentDictValueSet() const
{
    return ( m_currentValueSet != nullptr ) ? &m_currentValueSet->GetDictValueSet() : nullptr;
}


void CSymbolVar::SetCurrentValueSet(std::shared_ptr<const ValueSet> value_set)
{
    if( m_currentValueSet != value_set )
    {
        DeleteCurrentValueSet();
        m_currentValueSet = std::move(value_set);
    }
}


const ValueProcessor& CSymbolVar::GetCurrentValueProcessor() const
{
    if( m_currentValueProcessor == nullptr )
    {
        if( m_currentValueSet == nullptr )
        {
            m_currentValueProcessor = ValueProcessor::CreateValueProcessor(*m_pDictItem);
        }

        else
        {
            m_currentValueProcessor = m_currentValueSet->GetSharedValueProcessor();
        }
    }

    return *m_currentValueProcessor;
}


const NumericValueProcessor& CSymbolVar::GetCurrentNumericValueProcessor() const
{
    return assert_cast<const NumericValueProcessor&>(GetCurrentValueProcessor());
}


void CSymbolVar::SetCaptureInfo(const CaptureInfo& capture_info)
{
    ASSERT(!capture_info.IsSpecified() || CaptureInfo::IsCaptureTypePossible(*m_pDictItem, capture_info.GetCaptureType()));

    m_captureInfo = capture_info;
    m_evaluatedCaptureInfo.reset();
}


const CaptureInfo& CSymbolVar::GetEvaluatedCaptureInfo() const
{
    if( !m_evaluatedCaptureInfo.has_value() )
    {
        m_evaluatedCaptureInfo = m_showExtendedControl ? m_captureInfo.MakeValid(*m_pDictItem, GetCurrentDictValueSet()) :
                                                         CaptureInfo::GetBaseCaptureType(*m_pDictItem);
    }

    return *m_evaluatedCaptureInfo;
}


void CSymbolVar::SetShowExtendedControl(bool flag)
{
    m_showExtendedControl = flag;
    m_evaluatedCaptureInfo.reset();
}


Symbol* CSymbolVar::FindChildSymbol(const std::wstring& symbol_name) const
{
    for( Symbol* symbol : GetSymbolTable().FindSymbols(symbol_name) )
    {
        // only value sets can be children of the variable
        if( symbol->IsA(SymbolType::ValueSet) && assert_cast<const ValueSet*>(symbol)->GetVarT() == this )
            return symbol;
    }

    return nullptr;
}


void CSymbolVar::serialize_subclass(Serializer& ar)
{
    RunnableSymbol::serialize(ar);

    if( ar.IsSaving() )
    {
        int iVal = GetSymbolIndex();
        ar << iVal;

        ar << m_bZeroFill << m_bDecChar << m_cFmt << m_iNumDec
           << m_cClas << m_iLength << m_iLocation
           << m_iMaxOccs << m_iNumDim;

        for( int i = 0; i < m_iNumDim; i++ )
        {
            iVal = m_aDimType[i]; ar << iVal; // dimension type
            iVal = m_aDimSize[i]; ar << iVal; // size of each dimension
        }

        ar << SYMTfrm                    // Form' SYMT index (-1: no assoc Form)
           << m_iBehavior;

        ar.SerializeEnum(m_DefBehavior);
        ar.SerializeEnum(m_CurBehavior);

        ar << m_bDefVisible << m_bCurVisible

           << m_bIsPersistent << m_bIsDummyPersistent
           << m_bIsSequential << m_bNeedVerification

           << m_bIsUsed << m_bMarked
           << m_bOccGenerator << m_iAbsoluteFlowOrder
           << m_bIsPersistentProtected << m_bIsAutoIncrement;

        for( int i = 0; i < m_iNumDim; i++ )
            ar << m_aSizeForThisType[i];

        ar << m_bNeedConvertSomeSubItem
           << m_iSymGroup << m_iOwnerSymItem
           << m_bSkipStructImpute;
    }

    else
    {
        int iIndex;
        ar >> iIndex;
        ASSERT( iIndex == GetSymbolIndex() ); // if problem -> file corrupted
        ar >> m_bZeroFill >> m_bDecChar;

        unsigned short wideCharVal = 0; //Android NDK uses 4 bytes for the wide char so using unsigned short to mimic two byte read - Savy
//      ar >> m_cFmt;
        ar >> wideCharVal;
        m_cFmt = (csprochar)wideCharVal;

        ar >> m_iNumDec;

//      ar >> m_cClas;
        wideCharVal =0;
        ar >> wideCharVal;
        m_cClas = (csprochar)wideCharVal;

        ar >> m_iLength >> m_iLocation
           >> m_iMaxOccs >> m_iNumDim;

        for( int i = 0; i < m_iNumDim; i++ )
        {
            int iType;
            ar >> iType;

            m_aDimType[i] = (CDimension::VDimType) iType;  // dimension type
            ar >> m_aDimSize[i];         // size of each dimension
        }

        ar >> SYMTfrm;

        wideCharVal =0; ar >> wideCharVal;
        m_iBehavior = (csprochar)wideCharVal;
//      ar >> m_iBehavior;

        ar.SerializeEnum(m_DefBehavior);
        ar.SerializeEnum(m_CurBehavior);

        ar >> m_bDefVisible >> m_bCurVisible

           >> m_bIsPersistent >> m_bIsDummyPersistent
           >> m_bIsSequential >> m_bNeedVerification

           >> m_bIsUsed >> m_bMarked
           >> m_bOccGenerator >> m_iAbsoluteFlowOrder
           >> m_bIsPersistentProtected
           >> m_bIsAutoIncrement;

        for( int i = 0; i < m_iNumDim; i++ )
            ar >> m_aSizeForThisType[i];

        ar >> m_bNeedConvertSomeSubItem
           >> m_iSymGroup >> m_iOwnerSymItem
           >> m_bSkipStructImpute;

        SetOwnerGroup(m_iSymGroup);
        SetSubItemOf(m_iOwnerSymItem);

        if( m_cFmt == _T('A') && m_iLength == 0 ) // GHM 20140326 a variable length string, so initialize the memory
            AllocateLogicStringMemory();
    }
}


void CSymbolVar::WriteJsonMetadata_subclass(JsonWriter& json_writer) const
{
    ASSERT(m_pDictItem != nullptr);

    json_writer.Write(JK::item, *m_pDictItem);
}


// --------------------------------------------------------------------------
// VART_EngineItemAccessor: a temporary (for CSPro 8.0) object to
// allow EngineItem-style access to a VART;
// when removing this, look at EIA_TODO_REMOVE
// --------------------------------------------------------------------------

class VART_EngineItemAccessor : public EngineItemAccessor
{
public:
    VART_EngineItemAccessor(const VART& variable);

    const CDictItem& GetDictItem() const override { return *m_variable.GetDictItem(); }

    const CSymbolVar& GetVarT() const override { return m_variable; }

    bool HasValue() override;
    bool IsValid() override;
    std::wstring GetValueLabel(const std::optional<std::wstring>& language) override;

private:
    const VART& m_variable;
};


VART_EngineItemAccessor::VART_EngineItemAccessor(const VART& variable)
    :   EngineItemAccessor(nullptr, ItemIndex()),
        m_variable(variable)
{
    ASSERT(m_variable.GetDictItem() != nullptr);
}


bool VART_EngineItemAccessor::HasValue()
{
    return ReturnProgrammingError(false); // ENGINECR_TODO
}


bool VART_EngineItemAccessor::IsValid()
{
    return ReturnProgrammingError(false); // ENGINECR_TODO
}


std::wstring VART_EngineItemAccessor::GetValueLabel(const std::optional<std::wstring>& /*language*/)
{
    return ReturnProgrammingError(std::wstring()); // ENGINECR_TODO
}


EngineItemAccessor* CSymbolVar::GetEngineItemAccessor() const
{
    if( m_engineItemAccessor == nullptr )
        const_cast<VART*>(this)->m_engineItemAccessor = std::make_unique<VART_EngineItemAccessor>(*this);

    return m_engineItemAccessor.get();
}
