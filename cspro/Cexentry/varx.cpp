//---------------------------------------------------------------------------
//  File name: VarX.cpp
//
//  Description:
//          Implementation for executor-Var parallel table
//
//  History:    Date       Author   Comment
//              ---------------------------
//              20 Jul 99   RHF     Basic conversion
//              24 May 00   vc      Basic customization
//              04 Jul 00   vc      New approach for data management by Sect-occ
//              19 Jul 00   vc      See TRANSITION (make it dissapear!)
//              21 Jul 00   vc      Adding occurrences' management: array remapping, getting parent' indexes
//              24 Jul 00   RHF     Adding occurrences' management: array remapping for value sets and other functions
//              28 Jul 00   RHF     Adding PassTheCurrentIndex
//              04 Apr 01   vc      brack_... suppressed
//              21 May 01   vc      Adding index-translation methods PassIndexFrom3DToEngine & PassIndexFromEngineTo3D
//              10 Apr 04   rcl     Modifications to handle more than 1 dimension
//
//---------------------------------------------------------------------------
#include "STDAFX.H"
#include <engine/Tables.h>
#include <engine/Engine.h>
#include <engine/RELATION.H>
#include <engine/VARX.h>
#include <zEngineF/TraceHandler.h>
#include <zToolsO/VarFuncs.h>
#include <zUtilO/AppLdr.h>
#include <zUtilO/TraceMsg.h>
#include <zAppO/Application.h>
#include <zDictO/ValueProcessor.h>


//////////////////////////////////////////////////////////////////////////////
//
// --- Initialization
//
//////////////////////////////////////////////////////////////////////////////

void VARX::Init()
{
    // --- related objects
    m_pVarT = NULL;                     // VART entry
    m_pSecT = NULL;                     // SECT owner' entry

    // --- data-access information
    SetIndToAscii( -1 );
    SetIndToFloat( -1 );
    SetIndToFlags( -1 );

    // --- related items
    iRelatedSlot = 0;                   // slot of related items // RHF Aug 6, 1999

    // --- engine links
    m_pEngineDriver = NULL;
    m_pEngineArea   = NULL;
}


//////////////////////////////////////////////////////////////////////////////
//
// --- Managing related data
//
//////////////////////////////////////////////////////////////////////////////

void VARX::VarxRefreshRelatedData( int* aIndex )
{
    // aIndex[0] relative to Record, aIndex[1] relative to item, aIndex[2] relative to sub-item
    // If aIndex[2] is used then VARX is a sub-item else iSymVar is an item.
    ASSERT( aIndex[0] >= 0 );
    ASSERT( aIndex[1] >= 0 );
    ASSERT( aIndex[2] >= 0 );

    CNDIndexes theIndex( ZERO_BASED, aIndex );

    VARX*   pVarX = this;
    VART*   pVarT = GetVarT();
    int     iSymDic = m_pEngineArea->ownerdic( pVarT->GetSymbolIndex() );
    DICX*   pDicx = DPX(iSymDic);
    CRelations* pRelations = pDicx->pRelations;
    int     iSlotNum = pVarX->iRelatedSlot;
    ASSERT( iSlotNum >= 0 );
    int     iSourceOcc;

    bool    bItem = ( !pVarT->GetOwnerSymItem() );

    iSourceOcc = bItem ? 1 : theIndex.getIndexValue(2)+1; // RHF Nov 03, 2000 Always item use the same template. Example: Item is multiple with sub-items singles
    // RHF COM Nov 03, 2000 iSourceOcc = bItem ? theIndex.getIndexValue(1)+1 : theIndex.getIndexValue(2)+1;

    CRelatedTable* pRelTable = pRelations->GetRelatedTable( iSlotNum, iSourceOcc );
    ASSERT( pRelTable );

    for( const std::shared_ptr<CItemBase>& pItemBase : pRelTable->GetRelated() ) {
        int     iRelatedSymVar = pItemBase->GetSymbolIdx();
        ASSERT( iRelatedSymVar >= 0 );

        // Calculating more valuesets to refresh
        // [This object's related's related]
        //
        VARX* pVarXRelated = VPX(iRelatedSymVar);
        VART* pVarTRelated = VPT(iRelatedSymVar);

        if( !pVarTRelated->IsNumeric() ) // RHF Aug 04, 2000
            continue;// RHF Aug 04, 2000

        bItem = ( !pVarTRelated->GetOwnerSymItem() );
        if( bItem ) {
            theIndex.setIndexValue(2,0);
            // TODO SetFlotValid( pVarXRelated, false );
            m_pEngineDriver->varatof( pVarXRelated, theIndex );
        }
        else {
            int    iOccFrom, iOccTo, iFrom, iTo;

            iOccFrom = pItemBase->GetFrom();    // 1 based
            iOccTo = pItemBase->GetTo();        // 1 based

            if( iOccFrom == -1 ) {
                int  iNocc = pVarTRelated->GetMaxOccsInDim(2); // Obtain occurrences of pVarX. TODO

                iFrom = 1;
                iTo = iNocc;
            }
            else {
                iFrom = iOccFrom;
                iTo = iOccTo;
            }

            ASSERT( iFrom <= iTo );

            for( int i = iFrom; i <= iTo; i++ ) {
                theIndex.setIndexValue(2,i-1);
                // TODO SetVarFloatValid( pVarXRelated, false );
                m_pEngineDriver->varatof( pVarXRelated, theIndex );
            }
        }
    }
}

void VARX::VarxRefreshRelatedData( const CNDIndexes& theIndex )
{
    int aIndex[DIM_MAXDIM];

    for( int i = 0; i < DIM_MAXDIM; i++ )
        aIndex[i] = theIndex.getIndexValue(i);

    VarxRefreshRelatedData( aIndex );
}


//////////////////////////////////////////////////////////////////////////////
//
// --- Engine links
//
//////////////////////////////////////////////////////////////////////////////

void VARX::SetEngineDriver( CEngineDriver* pEngineDriver ) {
    m_pEngineDriver = pEngineDriver;
    ASSERT( pEngineDriver != 0 );
    m_pEngineArea = pEngineDriver->getEngineAreaPtr();
}


const Logic::SymbolTable& VARX::GetSymbolTable() const
{
    return m_pEngineArea->GetSymbolTable();
}


/////////////////////////////////////////////////////////////////////////////
//
// --- occurrences' management: array remapping, getting parent' indexes  // victor Jul 21, 00
//
/////////////////////////////////////////////////////////////////////////////
void VARX::PassIndexFrom3DToEngine( int* aIndex, CNDIndexes& anDIndex ) { // victor May 21, 01
    VART*   pVarT = GetVarT();

    for( int iDim = 0; iDim < DIM_MAXDIM; iDim++ ) {
        int     iAxis = iDim;

        if( !pVarT->IsArray() || anDIndex.getIndexValue(iDim) <= 0 )
            aIndex[iAxis] = 0;
        else
            aIndex[iAxis] = anDIndex.getIndexValue(iDim) - 1;
    }
}

CNDIndexes VARX::PassIndexFrom3DToEngine( C3DIndexes& anDIndex ) { // rcl, Jun 21, 2004
    VART*   pVarT = GetVarT();
    CNDIndexes theIndex( ZERO_BASED );

    if( pVarT->IsArray() )
    {
        anDIndex.decrease(theIndex);
    }
    else
    {
        theIndex.setHomePosition();
    }

    return theIndex;
}

void VARX::PassIndexFromEngineTo3D( int* aIndex, int* a3DIndex ) { // victor May 21, 01
    VART*   pVarT = GetVarT();
    int     iAxis;

    for( int iDim = 0; iDim < DIM_MAXDIM; iDim++ ) {
        if( !pVarT->IsArray() )
            iAxis = -1;
        else {
            CDimension::VDimType    xType = pVarT->GetDimType( iDim );

            switch( xType ) {
                case CDimension::Record:
                case CDimension::Item:
                case CDimension::SubItem:
                    iAxis = (int) xType;
                    break;
                default:
                    iAxis = -1;
                    break;
            }
        }
        if( iAxis < 0 )
            a3DIndex[iDim] = 0;
        else
            a3DIndex[iDim] = aIndex[iAxis] + 1;
    }
}

#define SHOW_PROBLEM(x)

static void checkIndexUsed( int iThisOcc, int iMaxOccs, int iTotOccs ) /*throw( IndexUsedException ) */
{
    if( iThisOcc > 0 && iThisOcc <= iTotOccs )
        return;

    if( iThisOcc < INT_MIN || iThisOcc > INT_MAX )
    {
        SHOW_PROBLEM( _T("Occurrence is lower than INT_MIN or higher than INT_MAX") );
        throw IndexUsedException( IndexUsedException::ERR_OVERFLOW_INDEX, iThisOcc );
    }

    if( iThisOcc > iTotOccs && iThisOcc <= iMaxOccs )
    {
        SHOW_PROBLEM( _T("Occurrence is bigger than TotalOcc, but lower than MaxOccs") );
        throw IndexUsedException( IndexUsedException::ERR_BAD_LIMITS_ALMOST_GOOD, iThisOcc );
    }

    SHOW_PROBLEM( _T("Occurrence is bigger than TotOcc and MaxOcc") );
    throw IndexUsedException( IndexUsedException::ERR_BAD_LIMITS_INDEX, iThisOcc );
}

void VARX::checkIndexUsed( int* aIndex, bool bCheckTotal ) /*throw( IndexUsedException )*/
{
    VART* pVarT = GetVarT();


    int iDimNo = 0;
    for( int i = 0; i < DIM_MAXDIM; i++ )
    {
        if( aIndex[i] >= 0 )
        {
            int iIndexValue = aIndex[i] + 1;

            int iMaxOccs = pVarT->GetMaxOccsInDim( iDimNo );
            int iTotOccs = iMaxOccs;
            // Notice:
            // To keep backward compatibility, we will only make checks (total
            // check) over the first multiple dimension, when it happens to be
            // a record
            if( bCheckTotal && i == 0 && pVarT->GetDimType(i) == CDimension::Record )
            {
                GROUPT* pGroupT = pVarT->GetOwnerGPT();
                while( pGroupT != 0 && pGroupT->GetDimType() != CDimension::Record )
                    pGroupT = pGroupT->GetOwnerGPT();

                if( pGroupT != 0 )
                    iTotOccs = pGroupT->GetTotalOccurrences();
            }

             ::checkIndexUsed( iIndexValue, iMaxOccs, iTotOccs );

            iDimNo++;
        }
    }
}

bool AreEqual( double d1, double d2 )
{
    bool bRet = false;
    if( d1 > d2 )
        bRet = (d1-d2) < 1e-5;
    else
        bRet = (d2-d1) < 1e-5;

    return bRet;
}

// used to be:
// bool VARX::RemapDoubleIndexes( double* aIndex, double* aOccur ) { // victor Jul 21, 00

// RemapIndexes
//   aOccur,  the input,  is 1-based
//   paIndex, the output, is 0-based
bool VARX::RemapIndexes( int* paIndex, double* aOccur, bool bCheckTotal, bool bGenerateExceptions ) { // rcl May 17, 04
    ASSERT( paIndex != 0 );
    // RemapDoubleIndexes: remap from a source' compact-dimensions occurs-array (with iNumDim occurrencess)
    //                     to a target' full-dimensions indexes-array (exactly DIM_MAXDIM indexes)
    //    - returns false if any problem (and one or more indexes in the target-array are -1)
    //    - problem 1: when the DimType internal array is not set
    //    - problem 2: an occur-value is below 1 (1-based expected)
    //    - problem 3: an occur-value exceeds the dimension-size
    //    - problem 4: one of the dimensions is referred to more than once
    VART*   pVarT=GetVarT();
    bool    bOkey   = true;
    int aIndex[DIM_MAXDIM];

    try
    {
        int iNumDim = pVarT->GetNumDim();   // the # of dimensions of this Var

        // preset the target-indexes to 'not loaded' condition if multi-dimension
        aIndex[0] = aIndex[1] = aIndex[2] = ( iNumDim ? -1 : 1 );

        // for each source-dimension, looks for the target-dimension
        for( int iSourceDim = 0; iSourceDim < iNumDim; iSourceDim++ ) {
            // RHF INIC Aug 15, 2000
            double dCurrIndex = aOccur[iSourceDim];

            if( dCurrIndex <= 0 )
                throw IndexUsedException( IndexUsedException::ERR_OVERFLOW_INDEX, dCurrIndex );

            // Sometimes comparing a double against a double does not work
            // as one may expect.. sometimes a != b even if we assigned
            // a = 1.3 and b = 1.3 just because any of the two 1.3's is
            // stored as a 1.30000000001 and then a != b
            //
            #define TEST_EQ(y) AreEqual(dCurrIndex,y)
            if( TEST_EQ(DEFAULT) || TEST_EQ(NOTAPPL) || TEST_EQ(MISSING) || TEST_EQ(REFUSED) )
                throw IndexUsedException( IndexUsedException::ERR_SPECIAL_VALUE_USED, dCurrIndex );

            if( TEST_EQ(INVALIDINDEX) )
                throw IndexUsedException( IndexUsedException::ERR_SPECIAL_VALUE_USED, dCurrIndex );

            CDimension::VDimType    xType = pVarT->GetDimType( iSourceDim );
            bool    bValidDimRef = (
                xType == CDimension::Record  ||
                xType == CDimension::Item    ||
                xType == CDimension::SubItem
                );

            int     iTargetDim = xType;
            ASSERT( bValidDimRef );     // 1) invalid DimType' internal array

            int iIndexValue = (int)dCurrIndex - 1;
            ASSERT( iIndexValue >= 0 ); // 2) invalid index-value (1-based expected)

            aIndex[iTargetDim] = iIndexValue;
        }

#ifdef BUCEN // BUCEN checks total
        if( bCheckTotal )
        {
            bool bIsInputDict = m_pEngineDriver->m_pApplication->GetDictionaryType(*pVarT->GetDataDict()) == DictionaryType::Input;
            bCheckTotal = bIsInputDict && Appl.ApplicationType == ModuleType::Batch;
        }
#endif

        if( iNumDim > 0 )
            checkIndexUsed( aIndex, bCheckTotal ); // check other conditions
    }
    catch( IndexUsedException& e )
    {
        if( bGenerateExceptions )
            throw e;

        bOkey = false;
        double dBadSubscript = e.getIndexUsed();

#ifdef BUCEN
        // logic is applied when exception is ERR_BAD_LIMITS_ALMOST_GOOD
        // Chirag inic Nov 20 2002
        // Recoded by rcl, May 20, 2004
        if( e.getCode() == IndexUsedException::ERR_BAD_LIMITS_ALMOST_GOOD )
        {
            bOkey = true; // Notice that discovering this problem and
                          // generating this warning does not convert this
                          // operation into a banned one (bOkey becomes true),
                          // just to keep backward compatibility with v2.5
                          // rcl, Sept 21, 2004
            issaerror( MessageType::Warning, 34089, pVarT->GetName().c_str(), long(dBadSubscript) );   // GSF 10-jan-03
        }
        else
#endif
        issaerror( MessageType::Warning, 34088, pVarT->GetName().c_str(), long(dBadSubscript) );   // GSF 10-jan-03

        // 20100601 added on tom's request
        if( m_pEngineDriver->m_pIntDriver->m_traceHandler != nullptr )
        {
            m_pEngineDriver->m_pIntDriver->m_traceHandler->Output(FormatTextCS2WS(_T("Invalid subscript: %s(%.0f)"), pVarT->GetName().c_str(), dBadSubscript),
                                                                  TraceHandler::OutputType::SystemText);
        }
    }

    for( int i=0; i < DIM_MAXDIM; i++ )
    {
        int iValue = (paIndex[i] = aIndex[i]);

        if( iValue < 0 )
        {
            paIndex[i] = 0;
        }
    }

    return bOkey;
}

// bool VARX::BuildIntParentIndexes( CNDIndexes& theIndex, int iOccur )
//      For "focus" VARXs only, completes the whole set of indexes
//      (integer' type arguments)
//
bool VARX::BuildIntParentIndexes( CNDIndexes& theIndex, int iOccur ) {
    VART*   pVarT = GetVarT();

    ASSERT( iOccur >= 1 );

    theIndex.setHomePosition();

    if( !pVarT->IsArray() )
        return true;

    // IsArray: equiv. 'pVarT->GetNumDim() > 0'
    // get the collection of parent-indexes
    GROUPT* pParentGroupT = pVarT->GetParentGPT();
    ASSERT( pParentGroupT );

    bool    bFirst=true;

    for( int iDim = pVarT->GetNumDim()-1; iDim >= 0; iDim-- ) {
        ASSERT( pParentGroupT != NULL );    // dimensions must have parent-group

        // Calculate iDimIndex
        CDimension::VDimType theDim =  pVarT->GetDimType(iDim);
        ASSERT( theDim == CDimension::Record ||
                theDim == CDimension::Item ||
                theDim == CDimension::SubItem );

        int iFinalValue = 0;
        int iGroupCurrentOcc = pParentGroupT->GetCurrentOccurrences();

        // Checking 1: Target ocurrrence must agree with current ocurrences of the first group
        if( bFirst )
        {
            iFinalValue = iOccur;
            bFirst = false;
        }
        else
        {
            iFinalValue = iGroupCurrentOcc;
        }

        if( iFinalValue <= 0 )
            iFinalValue = 1;

        if( theIndex.isZeroBased() )
            theIndex.setIndexValue( theDim, iFinalValue - 1 );
        else
            theIndex.setIndexValue( theDim, iFinalValue );

        pParentGroupT = pParentGroupT->GetParentGPT();
    }

    // Checking 2: Last parent must be a level
    // past the leftmost dimension, there should be no more parents
    if( pParentGroupT != NULL && pParentGroupT->GetGroupType() != GROUPT::Level) {
        ASSERT( 0 );
        return false;
    }

    return true;
}
// RHF END Jul 28, 2000

/////////////////////////////////////////////////////////////////////////////
//
// ==> TRANSITION: set THE proper index (working with 1-dimension only) // victor Jul 19, 00
//
/////////////////////////////////////////////////////////////////////////////

// decrease(int iValue):
//   substracts 1 to iValue, if the result is < 0, it is changed to 0.
//
static int decrease( const int iValue )
{
    int iRet = 0;
    if( iValue > 0 )
        iRet = iValue - 1;

    return iRet;
}

static void setOneDim( VART* pVarT, CNDIndexes& theIndex, int iOccValue )
{
    ASSERT( pVarT->GetNumDim() == 1 );
    int iNewOccValue = decrease(iOccValue);

    CDimension::VDimType whichDim = pVarT->GetDimType(0);

    switch( whichDim )
    {
        case CDimension::Record:
            theIndex.setIndexValue( 0, iNewOccValue );
            break;
        case CDimension::Item:
            theIndex.setIndexValue( 1, iNewOccValue );
            break;
        case CDimension::SubItem:
            theIndex.setIndexValue( 2, iNewOccValue );
            break;
            // RHF COM Jul 26, 2000    default:
            // RHF COM Jul 26, 2000        bDone = true;
            // RHF COM Jul 26, 2000        ASSERT( 0 );
            // RHF COM Jul 26, 2000        break;

        case CDimension::VoidDim:
            break;
    }
}

static void setTwoDim( VART* pVarT, CNDIndexes& theIndex, int iOccValue )
{
    ASSERT( pVarT->GetNumDim() == 2 );
    int iNewOccValue = decrease(iOccValue);

    GROUPT* pGroupT = pVarT->GetOwnerGPT();

    switch( pVarT->GetDimType(1) )
    {
    case CDimension::Record:
        ASSERT(0);
        // Es muy raro que la segunda dimension sea un record... o no??
        // theIndex.setIndexValue( 0, iNewOccValue );
        break;
    case CDimension::Item:
        if( pGroupT != 0 )
        {
            ASSERT( pGroupT != 0 );
            ASSERT( pGroupT->GetDimType() == CDimension::Item );
            pGroupT = pGroupT->GetOwnerGPT();
            ASSERT( pGroupT != 0 );
            if( pGroupT->GetDimType() != CDimension::VoidDim )
            {
                ASSERT( pGroupT->GetDimType() == CDimension::Record );
                theIndex.setIndexValue( 0, decrease(pGroupT->GetCurrentExOccurrence()) );
            }
            theIndex.setIndexValue( 1, iNewOccValue );
        }
        break;
    case CDimension::SubItem:
        if( pGroupT != 0 )
        {
            ASSERT( pGroupT != 0 );
            ASSERT( pGroupT->GetDimType() == CDimension::SubItem );
            pGroupT = pGroupT->GetOwnerGPT();
            ASSERT( pGroupT != 0 );
            if( pGroupT->GetDimType() != CDimension::VoidDim )
            {
                ASSERT( pGroupT->GetDimType() == CDimension::Record ||
                    pGroupT->GetDimType() == CDimension::Item );

                int iOcc = decrease(pGroupT->GetCurrentExOccurrence());
                if( pVarT->GetDimType(0) == CDimension::Record )
                {
                    theIndex.setIndexValue( 0, iOcc );
                }
                else
                if( pVarT->GetDimType(0) == CDimension::Item )
                {
                    theIndex.setIndexValue( 1, iOcc );
                }
            }
            theIndex.setIndexValue( 2, iNewOccValue );
        }
        break;
    case CDimension::VoidDim:
        break;
    }
}

static void setThreeDim( VART* pVarT, CNDIndexes& theIndex, int iOccValue )
{
    ASSERT( pVarT->GetNumDim() == 3 );

    GROUPT* pGroupT = pVarT->GetParentGPT();

    ASSERT(
        pVarT->GetDimType(0) == CDimension::Record &&
        pVarT->GetDimType(1) == CDimension::Item &&
        pVarT->GetDimType(2) == CDimension::SubItem );

    int iNewOccValue = decrease(iOccValue);

    theIndex.setIndexValue( 2, iNewOccValue );

    ASSERT( pGroupT != 0 );
    CString csGroupName = WS2CS(pGroupT->GetName());

    theIndex.setIndexValue( 1, decrease(pGroupT->GetCurrentExOccurrence()) );

    pGroupT = pGroupT->GetParentGPT();

    ASSERT( pGroupT != 0 );
    csGroupName = WS2CS(pGroupT->GetName());

    theIndex.setIndexValue( 0, decrease(pGroupT->GetCurrentExOccurrence()) );
}
// RCL Apr 10, 2004, Modifications to handle more than 1 dimension

// theIndex can be only partially initialized (only base specified)
bool VARX::PassTheOnlyIndex( CNDIndexes& theIndex, int iOccur ) {
    //  ASSERT( iOccur > 0 );                               // victor Mar 26, 01
    ASSERT( theIndex.isZeroBased() );

    VART*   pVarT = GetVarT();

    bool    bDone = true;

    // given that theIndex object could be a not-initialized object
    // we first call the setHomePosition() method to initialize it
    theIndex.setHomePosition();
    // if we do not do call this method it will be important to
    // call at least the setAsInitialized() method

    // allows for no iOccur for Single vars     <begin> // victor Mar 26, 01
    // ... consistent with suppressing 'case default' below!
    if( pVarT->IsArray() )  // ... no need to check for iOccur==0 for !isArray() vars
    {
        ASSERT( iOccur > 0 );

        switch( pVarT->GetNumDim() )
        {
        case 1:
            // set THE proper index (working with 1-dimension only)
            setOneDim( pVarT, theIndex, iOccur );
            break;
        case 2:
            setTwoDim( pVarT, theIndex, iOccur );
            break;
        case 3:
            setThreeDim( pVarT, theIndex, iOccur );
            break;
        default:
            ASSERT(0); // Not handling more than 3 dimensions, sorry.

        }

        ASSERT( theIndex.isValid() );
    }

    // theIndex.setAsInitialized(); // superfluous if setHomePosition() used before.

    return bDone;
}


//////////////////////////////////////////////////////////////////////////////
//
// --- CEngineArea::methods
//
//////////////////////////////////////////////////////////////////////////////

int CEngineArea::VarxStart()
{
    for( VART* pVarT : m_engineData->variables )
    {
        VARX* pVarX = pVarT->GetVarX();
        pVarX->iRelatedSlot = -1;                       // Item relations
        pVarX->SetEngineDriver( m_pEngineDriver );
    }

    return TRUE;
}

// returns true if the value specified in the index is in the variable's current value set
bool VARX::InRange(const CNDIndexes* pTheIndex) const
{
    CIntDriver* pIntDriver = m_pEngineDriver->m_pIntDriver.get();
    const VART* pVarT = GetVarT();
    const ValueProcessor& value_processor = pVarT->GetCurrentValueProcessor();
    double numeric_value = 0;
    CString string_value;
    bool in_range = false;

    switch( pVarT->GetEvaluatedCaptureInfo().GetCaptureType() )
    {
        case CaptureType::Photo:
        case CaptureType::Signature:
        case CaptureType::Audio:
            return true; // BINARY_TYPES_TO_ENGINE_TODO temporarily confirming entry of these capture types
    }

    if( pVarT->IsNumeric() )
    {
        numeric_value = ( pTheIndex == nullptr ) ?
            pIntDriver->GetVarFloatValue(const_cast<VARX*>(this)) :
            pIntDriver->GetVarFloatValue(const_cast<VARX*>(this), *pTheIndex);

        in_range = value_processor.IsValid(numeric_value);
    }

    else
    {
        ASSERT(pVarT->IsAlpha());
        string_value = GetValue(pTheIndex);
        in_range = value_processor.IsValid(string_value);
    }

    // starting with CSPro 7.3, in range checks will also account for the capture type
    const auto& evaluated_capture_info = pVarT->GetEvaluatedCaptureInfo();

    // process dates
    if( evaluated_capture_info.GetCaptureType() == CaptureType::Date )
    {
        // we only need to check if the value was in range
        if( in_range )
        {
            if( pVarT->IsNumeric() )
            {
                CString numeric_formatter;
                numeric_formatter.Format(_T("%%%dd"), pVarT->GetLength());
                string_value.Format(numeric_formatter, (int)numeric_value);
            }

            in_range = evaluated_capture_info.GetExtended<DateCaptureInfo>().IsResponseValid(string_value);
        }
    }


    // process checkboxes
    else if( evaluated_capture_info.GetCaptureType() == CaptureType::CheckBox )
    {
        // we only need to check if the value was out of range
        if( !in_range )
            in_range = CheckBoxCaptureInfo::IsResponseValid(string_value, value_processor);
    }


    // process toggle buttons
    else if( evaluated_capture_info.GetCaptureType() == CaptureType::ToggleButton )
    {
        if( !in_range )
        {
            // blank values are allowed for toggle buttons
            in_range = pVarT->IsNumeric() ? ( numeric_value == NOTAPPL ) :
                                            SO::IsBlank(string_value);
        }
    }

    return in_range;
}

//////////////////////////////////////////////////////////////////////////

CString VARX::GetValue(const CNDIndexes* pTheIndex) const
{
    VART* pVarT = GetVarT();
    CString csValue;

    if( pVarT->IsNumeric() )
    {
        double dValue = ( pTheIndex == nullptr ) ?
            varoutval() :
            varoutval(*pTheIndex);

        const ValueProcessor& value_processor = pVarT->GetCurrentValueProcessor();
        csValue = value_processor.GetOutput(dValue);
    }

    else
    {
        TCHAR* pszAsciiAddr = ( pTheIndex == nullptr ) ?
            m_pEngineDriver->m_pIntDriver->GetVarAsciiAddr(pVarT) :
            m_pEngineDriver->m_pIntDriver->GetVarAsciiAddr(pVarT, *pTheIndex);

        int iVarLen = pVarT->GetLength();
        TCHAR* pszVarValue = csValue.GetBufferSetLength(iVarLen);

        if( pszAsciiAddr != nullptr )
            _tmemcpy(pszVarValue, pszAsciiAddr, iVarLen);
    }

    return csValue;
}

void VARX::SetValue(const CNDIndexes& theIndex, CString csValue, bool* value_has_been_modified/* = nullptr*/)
{
    VARX* pVarX = this;
    VART* pVarT = pVarX->GetVarT();
    ASSERT(pVarT->GetLength() == csValue.GetLength());

    // for numerics, first convert the double value from input to an engine format
    if( pVarT->IsNumeric() )
    {
        const ValueProcessor& value_processor = pVarT->GetCurrentValueProcessor();
        double numeric_value = value_processor.GetNumericFromInput(csValue);

        numeric_value = pVarX->varinval(numeric_value, theIndex);

        csValue = value_processor.GetOutput(numeric_value);
    }

    // see if the value has been modified
    CIntDriver* pIntDriver = m_pEngineDriver->m_pIntDriver.get();
    TCHAR* pszAsciiAddr = pIntDriver->GetVarAsciiAddr(pVarX, theIndex);

    if( value_has_been_modified != nullptr )
    {
        ASSERT(!*value_has_been_modified);
        *value_has_been_modified = ( _tmemcmp(pszAsciiAddr, csValue.GetString(), csValue.GetLength()) != 0 );
    }

    // copy the new value to the buffer
    _tmemcpy(pszAsciiAddr, csValue, csValue.GetLength());

    if( pVarT->IsNumeric() && pVarX->iRelatedSlot >= 0 )
        pVarX->VarxRefreshRelatedData(theIndex);
}
