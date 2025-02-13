//---------------------------------------------------------------------------
//  File name: Exapplfu.cpp
//
//  Description:
//          application' executor functions (batch and data entry processors)
//
//  History:    Date       Author   Comment
//              ---------------------------
//              10 Nov 99   RHF     Basic conversion
//              01 Apr 00   vc      Add some groups functions
//              24 Jul 00   RHF     Convert to 3 dimension
//              04 Apr 01   vc      Tailoring for RepMgr compatibility
//              15 Oct 04   rcl     Changes to notes 3d handling
//
//---------------------------------------------------------------------------
#include "StdAfx.h"
#include <zPlatformO/PlatformInterface.h>
#include <zToolsO/VarFuncs.h>
#include <engine/BinaryStorageFor80.h>
#include <engine/Exappl.h>
#include <engine/COMMONIN.H>
#include <engine/Engine.h>
#include <engine/Comp.h>
#include <zUtilO/TraceMsg.h>
#include <zCaseO/BinaryCaseItem.h>
#include <zCaseO/Case.h>


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[]= __FILE__;
#endif


void CEngineDriver::initsect( int iSymSec ) {
    initsect( SPT(iSymSec) );
}

void CEngineDriver::initsect( SECT* pSecT ) {
    // initsect: cleans up internal memory for a given Section
    SECX*   pSecX = pSecT->GetSecX();
    int     iSecMaxOcc = pSecT->GetMaxOccs();
    bool    bInputDic  = ( pSecT->GetSubType() == SymbolSubType::Input );
    bool    bWorkDic   = ( pSecT->GetSubType() == SymbolSubType::Work );
    bool    bLikeBatch = ( Issamod != ModuleType::Entry );


    // RHF INIC Oct 23, 2002 Fix problem of loadcase. soccurs was returning the max sect occurrences!

    bool    bExternalNoFlow=false;
    if( pSecT->GetSubType() == SymbolSubType::External ) {
        FLOW*   pFlow = pSecT->GetDicT()->GetFlow();
        if( pFlow != NULL && pFlow->GetFormFile() == NULL )
            bExternalNoFlow = true;
    }
    // RHF END Oct 23, 2002

    // initializes section occurrences
    pSecX->SetOccurrences( ( !bWorkDic || bExternalNoFlow ) ? 0 : iSecMaxOcc );

    // initializes data areas for all occurrences
    double  dInitValue = ( !bWorkDic ) ? NOTAPPL : 0.0;
    TCHAR cInitLight = ( bLikeBatch || !bInputDic ) ? FLAG_HIGHLIGHT : FLAG_NOLIGHT;

    // RHF INIC May 14, 2003
    // BMD BEGIN 14 Aug 2003
    if( m_bUsePrevLimits && iSecMaxOcc >= 2 ) {
        iSecMaxOcc = m_pIntDriver->exsoccurs( pSecT );
        GROUPT* pGroupT;
        int iGroupNum = 0;
        while( (pGroupT = pSecT->GetGroup(iGroupNum)) != NULL ) {
            iSecMaxOcc = std::max(iSecMaxOcc, pGroupT->GetHighOccurrences());
            pGroupT->SetHighOccurrences(0);
            iGroupNum++;
        }
    }
    // BMD END 14 Aug 2003


    // RHF END May 14, 2003

    pSecX->InitSecOccArray( dInitValue, cInitLight, iSecMaxOcc );
}

void CEngineDriver::initwsect()
{
    // initwsect: cleans up internal memory for all Sections in all workDicts
    for( DICT* pDicT : m_engineData->dictionaries_pre80 )
    {
        if( pDicT->GetSubType() != SymbolSubType::Work )
        {
            int iSymSec = pDicT->SYMTfsec;

            while( iSymSec > 0 )
            {
                SECT* pSecT = SPT(iSymSec);
                initsect(iSymSec);
                iSymSec = pSecT->SYMTfwd;
            }
        }
    }
}

void CEngineDriver::ClearLevelNode( DICT* pDicT, int iNodeLevel ) {
    // ClearLevelNode: clean all dict' sections and attached flow-Groups (for a given level)
    // ... formerly 'clearlevel', now 'ClearLevelNode'  // victor May 16, 00
    if( iNodeLevel < 1 )
        return;

    // a) cleaning dict' sections (at given level)
    int     iSymSec = pDicT->SYMTfsec;

    SECT*   pSecT = (iSymSec > 0) ? SPT(iSymSec) : NULL;
    while( pSecT != NULL ) {
        if( pSecT->GetLevel() == iNodeLevel )
            initsect( pSecT );

        pSecT = (SECT*)pSecT->next_symbol;
    }

    // b) cleaning Groups (at given level) of associated Flow
    FLOW*   pFlow = pDicT->GetFlow();

    if( pFlow != NULL ) {           // workDicts have no Flow
        GROUPT* pGroupTRoot = pFlow->GetGroupTRoot();
        int     iBaseSlot;
        int     iNumSlots;

        if( pFlow->GetSubType() == SymbolSubType::Primary ||
            pFlow->GetSubType() == SymbolSubType::Secondary ) {
                // Primary & Secondary flows: fully clean-up of Level-group (descendants included)
                GROUPT* pGroupTLevel = pGroupTRoot->GetLevelGPT( iNodeLevel );

                // ... initialize the whole level in Flow
                if( pGroupTLevel != NULL )
                    pGroupTLevel->InitOneGroup();

                // ... initialize every dict-Groups of this Flow
                iBaseSlot = pGroupTRoot->GetContainerIndex() +
                    pFlow->GetNumberOfVisibleGroups();
                iNumSlots = pFlow->GetNumberOfInvisibleGroups();

                for( int iGroupT = iBaseSlot; iGroupT < iBaseSlot + iNumSlots; iGroupT++ ) {
                    GROUPT* pGroupT = GIP(iGroupT);

                    // remark - must check level
                    if( pGroupT->GetLevel() == iNodeLevel )
                        pGroupT->InitOneGroup();
                }
            }
        else if( pFlow->GetSubType() == SymbolSubType::External ) {
            // External flows: cleaning-up each group in Flow
            iBaseSlot = pGroupTRoot->GetContainerIndex();
            iNumSlots = pFlow->GetNumberOfGroups();

            for( int iGroupT = iBaseSlot; iGroupT < iBaseSlot + iNumSlots; iGroupT++ ) {
                GROUPT* pGroupT = GIP(iGroupT);

                // remark - no need to check level, externals are Level-1 only
                if( pGroupT->GetLevel() == iNodeLevel )
                    pGroupT->InitOneGroup();
            }
        }
        else {
            ASSERT( 0 ); // shouldn't happen, only Primary/Secondary/External are defined
        }
    }

}

#ifdef USE_BINARY
#else
bool CEngineDriver::EvaluateNode( DICT* pDicT, int iNodeLevel ) {
    // EvaluateNode: check minimum section occurrences for a node at a given level
    // ... formerly 'evalcase', now 'EvaluateNode'      // victor May 16, 00
    bool    bOK = true;
    int     iSymSec = pDicT->SYMTfsec;


    SECT*   pSecT = (iSymSec > 0) ? SPT(iSymSec) : NULL;
    while( pSecT != NULL && bOK ) {       // remark - stops at 1st error found
        if( pSecT->GetLevel() == iNodeLevel ) {
            SECX*   pSecX = pSecT->GetSecX();

            bOK = ( pSecX->GetOccurrences() >= pSecT->GetMinOccs() );

            if( !bOK ) {
                m_processSummary->IncrementBadCaseLevelStructures(iNodeLevel - 1);
                issaerror( MessageType::Error, 10008, pSecT->GetName().c_str() ); // RHF Jul 13, 2001
            }
        }

        pSecT = (SECT*) pSecT->next_symbol;
    }

    return bOK;
}
#endif // USE_BINARY


// return code:  true=reached the max # messages, false=did not reach
bool CEngineDriver::sectadd(SECT* pSecT, const CaseRecord* case_record_with_binary_data)
{
    // sectadd: add a section record to current case
    // ... tailored to get data from record added       // victor Jul 25, 00
    SECX*   pSecX = pSecT->GetSecX();
    int     iSecOcc = pSecX->GetOccurrences() + 1;

    // 1.- discard the record if the section already has the structural maximum
    if( iSecOcc > pSecT->GetMaxOccs() ) {
        return false;
    }

    // 2.- copy the record to the sect-occ ascii-area
    CNDIndexes theIndex( ZERO_BASED, DEFAULT_INDEXES );
    DICX*   pDicX = pSecX->GetDicX();
    TCHAR*  pRecArea = pDicX->GetRecordArea();
    int     iLenAscii = pSecX->GetSecDataSize();
    int     iDataLen = _tcslen( pRecArea );
    int     iRightFiller = iLenAscii - iDataLen;

    if( iRightFiller > 0 )
        _tmemset( pRecArea + iDataLen, BLANK, iRightFiller );

    // ... copy source-record, update section' occs
    TCHAR* pAsciiArea = pSecX->GetAsciiAreaAtOccur( iSecOcc );

    _tmemcpy( pAsciiArea, pRecArea, iLenAscii );
    pSecX->SetOccurrences( iSecOcc );

    //return; // SPEED RHF Mar 14, 2001 Se ganan 18 segundos en 2 minutos


    // 3.- scanning Item/subItems: occurrences' recognition and converting
    theIndex.setIndexValue(0,iSecOcc-1); // 1st dimension is the Record' occurrence

    int     iSymItem = pSecT->SYMTfvar;

    VART*   pVarTItem;
    VARX*   pVarXItem;
    VART*   pVarTSubItem;
    VARX*   pVarXSubItem;
    bool    bConvertItem;
    bool    bConvertSubItem;
    int     iSymNextItem;
    int     iItemNumOccs;           // to save the # of occs of the Item
    int     iItemOcc;               // to loop on Item occs
    int     iTopOcc;                // top-occ to start occs' recognition

    int     iSubItemNumOccs;    // to save the # of occs of the subItem
    int     iSubItemOcc;        // to loop on subItem occs

    int     iVarLen;
    TCHAR*  pAsciiAddr;


    while( iSymItem > 0 ) {
        pVarTItem = VPT(iSymItem);
        pVarXItem = pVarTItem->GetVarX();

        bConvertItem = ( pVarTItem->IsUsed() && pVarTItem->IsNumeric() );
        iSymNextItem = pVarTItem->SYMTfwd;

        ASSERT(pVarTItem->GetDictItem() != nullptr);

        // handle (used) binary dictionary items
        if( IsBinary(*pVarTItem->GetDictItem()) )
        {
            const BinaryCaseItem* binary_case_item = ( case_record_with_binary_data != nullptr ) ? assert_nullable_cast<const BinaryCaseItem*>(case_record_with_binary_data->GetCaseRecordMetadata().GetCaseLevelMetadata().GetCaseMetadata().FindCaseItem(pVarTItem->GetDictItem()->GetName())) :
                                                                                                   nullptr;

            if( binary_case_item != nullptr )
            {
                ASSERT(!pVarTItem->GetDictItem()->IsSubitem());
                theIndex.setIndexValue(CDimension::VDimType::SubItem, 0);

                for( CaseItemIndex index = case_record_with_binary_data->GetCaseItemIndex(iSecOcc - 1);
                     index.GetItemOccurrence() < static_cast<size_t>(pVarTItem->GetMaxOccs());
                     index.IncrementItemOccurrence() )
                {
                    BinaryDataAccessor& binary_data_accessor = binary_case_item->GetBinaryDataAccessor(index);

                    if( binary_data_accessor.IsDefined() )
                    {
                        theIndex.setIndexValue(CDimension::VDimType::Item, index.GetItemOccurrence());
                        
                        // set the value of the fake VART to be the index of this binary data
                        m_pIntDriver->SetVarFloatValue(pVarTItem->GetDPT()->m_binaryStorageFor80.size(), pVarXItem, theIndex);

                        pVarTItem->GetDPT()->m_binaryStorageFor80.emplace_back(std::make_shared<BinaryStorageFor80>(BinaryStorageFor80 { binary_data_accessor, nullptr }));
                    }
                }
            }
        }

        // handle numeric dictionary items as well as items with subitems
        else if( bConvertItem || pVarTItem->NeedConvertSomeSubItem() ) {// SPEED RHF May 1, 2001

            // insure no subItem is found here (are skipped below)
            ASSERT( pVarTItem->GetOwnerVarT() == NULL );


            // 3a. recognition of Item' occurrences
            iVarLen = pVarTItem->GetLength();
            iTopOcc = pVarTItem->GetMaxOccs() - 1;
            theIndex.setIndexValue(2,0);    // 3rd dimension is always 0 for Items

            for( iItemOcc = iTopOcc; iItemOcc >= 0; iItemOcc-- ) {
                theIndex.setIndexValue(1,iItemOcc);
                // 2nd dimension is the Item' occurrence

                pAsciiAddr = m_pIntDriver->GetVarAsciiAddr( pVarXItem, theIndex );

                // stops at first non-blank occurrence found
                ASSERT( iVarLen <= ENG_BLANKSIZE ); // re blank-area for comparisons
                if( _tmemcmp( pAsciiAddr, pEngBlank, iVarLen ) != 0 )
                    break;
            }

            // 3.b save the # of occs of the Item
            iItemNumOccs = iItemOcc + 1;
            // TODO: set aIndex[1] = 0 and store iItemNumOccs

            // process all Item' occurrences
            for( iItemOcc = 0; iItemOcc < iItemNumOccs; iItemOcc++ ) {
                // 3c. converting the Item' occurrence
                theIndex.setIndexValue(1,iItemOcc); // 2nd dimension is the Item' occurrence
                theIndex.setIndexValue(2,0);        // 3rd dimension is always 0 for Items
                if( bConvertItem )
                {
                    theIndex.specifyIndexesUsed( USE_DIM_1_2 );
                    varatof( pVarXItem, theIndex );
                }

                // 3d. installing the VSets of this Item-occurrence?
                // TODO?

                // 4. analyzing subItems belonging to the Item
                int     iSymSubItem = iSymNextItem;

                while( iSymSubItem > 0 ) {
                    pVarTSubItem = VPT(iSymSubItem);

                    // stops at any non-subItem of the Item being processed
                    if( pVarTSubItem->GetOwnerVarT() != pVarTItem )
                        break;

                    bConvertSubItem = ( pVarTSubItem->IsUsed() && pVarTSubItem->IsNumeric() );

                    if( bConvertSubItem ) {  // SPEED RHF May 1, 2001
                        pVarXSubItem = pVarTSubItem->GetVarX();

                        // 4a. recognition of subItem' occurrences
                        iVarLen = pVarTSubItem->GetLength();
                        iTopOcc = pVarTSubItem->GetMaxOccs() - 1;

                        for( iSubItemOcc = iTopOcc; iSubItemOcc >= 0; iSubItemOcc-- ) {
                            // 3rd dimension is the subItem' occurrence
                            theIndex.specifyIndexesUsed( USE_ALL_DIM );
                            theIndex.setIndexValue( 2,iSubItemOcc );

                            pAsciiAddr = m_pIntDriver->GetVarAsciiAddr( pVarXSubItem, theIndex );

                            // stops at first non-blank occurrence found
                            ASSERT( iVarLen <= ENG_BLANKSIZE ); // re blank-area for comparisons
                            if( _tmemcmp( pAsciiAddr, pEngBlank, iVarLen ) != 0 )
                                break;
                        }

                        // 4b. save the # of occs of the subItem
                        iSubItemNumOccs = iSubItemOcc + 1;
                        // TODO: set aIndex[2] = 0 and store iSubItemNumOccs

                        // 4c. converting all subItem occurrences

                        if( bConvertSubItem ) {
                            theIndex.specifyIndexesUsed( USE_ALL_DIM );
                            for( iSubItemOcc = 0; iSubItemOcc < iSubItemNumOccs; iSubItemOcc++ ) {
                                // 3rd dimension is the subItem' occurrence
                                theIndex.setIndexValue(2,iSubItemOcc);

                                varatof( pVarXSubItem, theIndex );
                            }
                        }
                    }

                    // 4d. installing the VSets of this subItem (every occurrences)?
                    // TODO?

                    // set next subItem' candidate to be analyzed
                    iSymSubItem = pVarTSubItem->SYMTfwd;
                }
            }
        } // SPEED RHF May 1, 2001

        // searching for the next true Item (skips any subItem found)
        while( iSymNextItem > 0 && VPT(iSymNextItem)->GetOwnerVarT() != NULL )
            iSymNextItem = VPT(iSymNextItem)->SYMTfwd;

        // set next Item to be analyzed
        iSymItem = iSymNextItem;
    }

    return true;
}

void CEngineDriver::sectcommonadd( DICT* pDicT, int iLevel ) {
    int     iSymSec = pDicT->Common_GetCommonSection( iLevel );
    ASSERT( iSymSec > 0 );

    SECT*   pSecT = SPT(iSymSec);
    SECX*   pSecX = pSecT->GetSecX();

    pSecX->SetOccurrences( 1 );


    // RHF INIC Jul 27, 2000
    int     iSymVar = pSecT->SYMTfvar;

    while( iSymVar > 0 ) {
        VART*   pVarT = VPT(iSymVar);

        if( pVarT->IsUsed() && pVarT->IsNumeric() )
            varatof( pVarT->GetVarX() );

        iSymVar = pVarT->SYMTfwd;
    }
    // RHF END Jul 27, 2000

}


// RHF INIC Jul 24, 2000
// rcl signature and method modifications, Jun 25 2004
void CEngineDriver::prepvar( int iSymVar, const CNDIndexes& theIndex, bool bVisualValue/*=false*/ )
{
    prepvar( VPT(iSymVar), theIndex, bVisualValue );
}

void CEngineDriver::prepvar( VART* pVarT, const CNDIndexes& theIndex, bool bVisualValue/*=false*/ ) {

    ASSERT( theIndex.isZeroBased() );
    ASSERT( theIndex.isValid() );

    bool    bIsField = ( Issamod == ModuleType::Entry && pVarT->SYMTfrm > 0 );
    VARX*   pVarX = pVarT->GetVarX();
    int     iColor=0;
    bool    bNotappl=false;

    // A specific occurrence
    TCHAR* pAsciiAddr = m_pIntDriver->GetVarAsciiAddr( pVarT, theIndex );

    if( !bVisualValue ) { // RHF Feb 28, 2001
        iColor = m_pIntDriver->GetFieldColor( pVarT, theIndex );
        bNotappl = ( m_pEngineSettings->IsPathOff() ) ? // RHF+vc Oct 13, 00
            ( iColor == FLAG_NOLIGHT ) :         // RHF+vc Oct 13, 00
            ( iColor != FLAG_HIGHLIGHT );        // RHF+vc Oct 13, 00
    } // RHF Feb 28, 2001

    if( bIsField && bNotappl )
        _tmemset( pAsciiAddr, BLANK, pVarT->GetLength() );
    else if( pVarT->IsUsed() && pVarT->IsNumeric() ) {
        double dValue = pVarX->varoutval( theIndex );
        pVarT->dvaltochar( dValue, pAsciiAddr );
    }
}

// Refresh ascii buffer fo all occurrences
void CEngineDriver::prepvar( VART* pVarT, bool bVisualValue ) // rcl Jun 25 2004
{
    // previously used with optional parameters aIndex=0 and bVisualValue = false;

    bool    bIsField = ( Issamod == ModuleType::Entry && pVarT->SYMTfrm > 0 );
    TCHAR*  pAsciiAddr;
    VARX*   pVarX = pVarT->GetVarX();
    double  dValue;
    int     iColor=0;
    bool    bNotappl=false;

    CNDIndexes theIndexAux( ZERO_BASED, DEFAULT_INDEXES );

    int iMaxOccsInDim0  = pVarT->GetMaxOccsInFixedDim(0);

#ifdef BUCEN
    // begin SAVY 28-May-2003  SPEED
    if( Appl.ApplicationType != ModuleType::Entry ) { // RHF Sep 05, 2003
        int iMyDim = 0;
        CDimension::VDimType    xType = (iMyDim==pVarT->GetDimType(0))? CDimension::Record :
        (iMyDim==pVarT->GetDimType(1))? CDimension::Item : CDimension::SubItem;
        if(xType == CDimension::Record){
            iMaxOccsInDim0 = m_pIntDriver->exsoccurs( pVarT->GetSPT());
        }
    }
    // end SAVY 28-May-2003  SPEED
#endif

#define SET_IDX( x, y ) theIndexAux.setIndexValue(x,y)
#define GET_IDX(x) theIndexAux.getIndexValue(x)
#define INCR_IDX(x) SET_IDX(x,GET_IDX(x)+1)

    for( SET_IDX(0,0); GET_IDX(0) < iMaxOccsInDim0; INCR_IDX(0) ) {
        for( SET_IDX(1,0); GET_IDX(1) < pVarT->GetMaxOccsInFixedDim(1); INCR_IDX(1) ) {
            for( SET_IDX(2,0); GET_IDX(2) < pVarT->GetMaxOccsInFixedDim(2); INCR_IDX(2) ) {
                pAsciiAddr = m_pIntDriver->GetVarAsciiAddr( pVarX, theIndexAux );

                if( !bVisualValue ) { // RHF Feb 28, 2001
                    iColor = m_pIntDriver->GetFieldColor( pVarX, theIndexAux );
                    bNotappl = ( m_pEngineSettings->IsPathOff() ) ? // RHF+vc Oct 13, 00
                        ( iColor == FLAG_NOLIGHT ) :         // RHF+vc Oct 13, 00
                        ( iColor != FLAG_HIGHLIGHT );        // RHF+vc Oct 13, 00
                } // RHF Feb 28, 2001

                if( bIsField && bNotappl ) {
                    _tmemset( pAsciiAddr, BLANK, pVarT->GetLength() );
                }

                else if( IsBinary(*pVarT->GetDictItem()) ) {
                    ASSERT(pVarT->GetDictItem()->GetLen() == 1);

                    // the value of binary data will be equal to its index in m_binaryStorageFor80 (if not blank)
                    dValue = m_pIntDriver->GetVarFloatValue(pVarT, theIndexAux);

                    if( dValue == NOTAPPL ) {
                        *pAsciiAddr = ' ';
                    }
                    else {
                        ASSERT(static_cast<size_t>(dValue) < pVarT->GetDPT()->m_binaryStorageFor80.size());
                        const double value_with_offset = dValue + BinaryStorageFor80::BinaryCaseItemCharacterOffset;
                        ASSERT(static_cast<wchar_t>(value_with_offset) == value_with_offset);
                        *pAsciiAddr = static_cast<wchar_t>(value_with_offset);
                    }
                }

                else if( pVarT->IsUsed() && pVarT->IsNumeric() ) {
                    dValue = pVarX->varoutval( theIndexAux );
                    pVarT->dvaltochar( dValue, pAsciiAddr );
                }
            }
        }
    }
}

// RHF END Jul 24, 2000

void CEngineDriver::ShowOutOfRange( VART* pVarT, CNDIndexes* pTheIndex, TCHAR* pAsciiAddr )
{
    CString csLocalBuf(pAsciiAddr, pVarT->GetLength());

    CString csOcc;
    if( pVarT->IsArray() ) {
        if( pTheIndex == NULL )
            csOcc.Format(_T("(<unavailable>,<unavailable>,unavailable)") );
        else
            csOcc = pTheIndex->toString(pVarT->GetNumDim()).c_str();
    }

    issaerror( MessageType::Warning, 88870, csLocalBuf.GetString(), pVarT->GetName().c_str(), csOcc.GetString() );
}

void CEngineDriver::varatof( VARX* pVarX ) { // similar to varatof( p, NULL )

    VART*   pVarT = pVarX->GetVarT();
    ASSERT( pVarT != 0 );

#ifdef _DEBUG
    //////////////////////////////////////////////////////////////////////////
    // for debug purposes only, delete it later please RCL TODO
    //////////////////////////////////////////////////////////////////////////

    ASSERT( pVarT->IsNumeric() );

    for( int i = 0; i < DIM_MAXDIM; i++ ) {
        ASSERT( pVarT->GetMaxOccsInDim(i) >= 1 );
    }
#endif

    double  dValue;
    TCHAR*  pAsciiAddr = m_pIntDriver->GetVarAsciiAddr( pVarT );
    double* pFlotAddr  = m_pIntDriver->GetVarFloatAddr( pVarT ); // Is not necessary refresh.

    bool    bCheckRanges = ( Issamod != ModuleType::Entry && m_pEngineSettings->IsCheckingRanges() ); // victor Dec 07, 00

    if( pAsciiAddr != NULL && pFlotAddr != NULL ) {
        dValue = chartodval( pAsciiAddr, pVarT->GetLength(), pVarT->GetDecimals() );
        *pFlotAddr = pVarX->varinval( dValue );

        if( bCheckRanges ) {                            // victor Dec 07, 00
            bool    bInRange = ( dValue == MISSING || dValue == NOTAPPL || pVarX->InRange() );

            if( !bInRange )
                ShowOutOfRange( pVarT, NULL, pAsciiAddr );
        }
    }
    else
        ASSERT(0);

}

// RHF INIC Jul 22, 2000
void CEngineDriver::varatof( VARX* pVarX, CNDIndexes& theIndex ) {
    VART*   pVarT = pVarX->GetVarT();
#ifdef _DEBUG
    ASSERT( theIndex.isZeroBased() );

    ASSERT( pVarT->IsNumeric() );
    int     i;

    for( i = 0; i < DIM_MAXDIM; i++ ) {
        ASSERT( pVarT->GetMaxOccsInDim(i) >= 1 );
    }

    for( i = 0; i < DIM_MAXDIM; i++ ) {
        ASSERT( theIndex.getIndexValue(i) >= 0 );
        ASSERT( theIndex.getIndexValue(i) < pVarT->GetMaxOccsInFixedDim(i) );
    }
#endif
    double  dValue;
    TCHAR*  pAsciiAddr = m_pIntDriver->GetVarAsciiAddr( pVarX, theIndex );
    double* pFlotAddr  = m_pIntDriver->GetVarFloatAddr( pVarX, theIndex ); // Is not necessary refresh.
    bool    bCheckRanges = ( Issamod != ModuleType::Entry && m_pEngineSettings->IsCheckingRanges() ); // victor Dec 07, 00

    if( pAsciiAddr != NULL && pFlotAddr != NULL ) {
        dValue = chartodval( pAsciiAddr, pVarT->GetLength(), pVarT->GetDecimals() );
        *pFlotAddr = pVarX->varinval( dValue, theIndex );

        if( bCheckRanges ) {                            // victor Dec 07, 00
            bool    bInRange = ( dValue == MISSING || dValue == NOTAPPL || pVarX->InRange( theIndex ) );

            if( !bInRange )
                ShowOutOfRange( pVarT, &theIndex, pAsciiAddr );
        }
    }
    else
        ASSERT(0);
}
// RHF END Jul 22, 2000

bool CEngineDriver::IsBlankField( int iSymVar, int iOccur ) { // victor Mar 20, 00
    // IsAnEmptyField: test the ascii-buffer (not the float-image) to see if blank
    return( IsBlankField( VPT(iSymVar), iOccur ) );
}


bool CEngineDriver::IsBlankField( VART* pVarT, int iOccur ) {
    ASSERT( iOccur >= 1 );
    ASSERT( pVarT != 0 );

    VARX*   pVarX = pVarT->GetVarX();

    ASSERT( pVarX != 0 );

    // set THE proper index (working with 1-dimension only) // TRANSITION
    CNDIndexes theIndex( ZERO_BASED );
    pVarX->PassTheOnlyIndex( theIndex, iOccur );            // TRANSITION

    return IsBlankField( pVarT, theIndex );
}

bool CEngineDriver::IsBlankField( VART* pVarT, CNDIndexes& theIndex ) {
    ASSERT( theIndex.isZeroBased() );
    // IsAnEmptyField: test the ascii-buffer (not the float-image) to see if blank
    int     iVarLen = pVarT->GetLength();

    TCHAR* pVarBuff = m_pIntDriver->GetVarAsciiAddr( pVarT, theIndex ); // RHF Jul 24, 2000

    ASSERT( iVarLen <= ENG_BLANKSIZE ); // re blank-area for comparisons
    bool    bEmptyField = ( _tmemcmp( pVarBuff, pEngBlank, iVarLen ) == 0 );

    return bEmptyField;
}


// returns a CString containing the first level key using the values of the items in memory
CString CEngineDriver::key_string(const DICT* pDicT)
{
    int iExpectedKeyLength = pDicT->GetLevelsIdLen();
    int iKeyLength = 0;

    CString csKey;
    TCHAR* pszKeyBuffer = csKey.GetBufferSetLength(iExpectedKeyLength);
    _tmemset(pszKeyBuffer, BLANK, iExpectedKeyLength);

    const CDataDict* pDataDict = pDicT->GetDataDict();
    const DictLevel& dict_level = pDataDict->GetLevel(0);
    const CDictRecord* pRecord = dict_level.GetIdItemsRec();

    for( int iItem = 0; iItem < pRecord->GetNumItems(); ++iItem )
    {
        const CDictItem* pItem = pRecord->GetItem(iItem);
        int iSymVar = pItem->GetSymbol();

        // check that this is a valid var
        if( iSymVar <= 0 || !NPT(iSymVar)->IsA(SymbolType::Variable) )
            continue;

        VART* pVarT = VPT(iSymVar);
        VARX* pVarX = pVarT->GetVarX();

        // check this var has a valid length and fits into max length
        if( pVarT->GetLength() <= 0 || iKeyLength + pVarT->GetLength() > MAX_KEY_SIZE )
            break;

        // add its ascii text to pszKeyText
        TCHAR* pszVarBuffer = pszKeyBuffer + iKeyLength;

        if( pVarT->IsNumeric() )
        {
            double dValue = pVarX->varoutval();
            pVarT->dvaltochar(dValue, pszVarBuffer);
        }

        else
        {
            TCHAR* pFrom = (TCHAR*)m_pIntDriver->svaraddr(pVarX);
            _tmemcpy(pszVarBuffer, pFrom, pVarT->GetLength());
        }

        iKeyLength += pVarT->GetLength();
    }

    ASSERT(iKeyLength == iExpectedKeyLength);

    return csKey;
}


// --- coordinating occurrences of Sections and Groups
// RHF Mar 16, 2000: normalization - occs' members of GroupT were set directly into members,
// RHF Mar 16, 2000: normalization - are now set by means of public methods SetxxxOccurrences

// ------ setting up Sect occurrences from related Groups
int CEngineDriver::SetupSectOccsFromGroups( int iSymSec, bool bPartialSave/* = false*/ ) {     // victor Sep 09, 99
    // SetupSectOccsFromGroup: setup this Sect' occurrences extracting from related Groups
    SECT*       pSecT = SPT(iSymSec);
    SECX*       pSecX = pSecT->GetSecX();
    bool        bLikeBatch = ( Issamod != ModuleType::Entry );

    pSecX->SetOccurrences( 0 );         // reset section' occurrences
    pSecX->SetPartialOccurrences( 0 );   // RHF May 07, 2004

    // looks for vars which "produce" section occurrences
    int         iSymVar = pSecT->SYMTfvar;

    while( iSymVar > 0 ) {
        VART* pVarT      = VPT(iSymVar);
        int   iOwnerItem = pVarT->GetOwnerSymItem();
        bool  bIsSubItem = (iOwnerItem > 0);
        bool  bIsItem    = (iOwnerItem == 0); // RHF Aug 25, 2000 // rcl, Jul 8, 2004

        // in Entry, ignore vars not in forms           // victor Mar 23, 00
        bool    bUseThisVar = ( bLikeBatch || pVarT->SYMTfrm > 0 || pVarT->IsOccGenerator() ); // RHF Nov 15, 2002 Add IsOccGenerator for fixing problem in writecase mono-record, not required.

        if( bUseThisVar ) {

            // Fix for single and multiple subitem appearing in a form / logic
            // and sometimes multiple or owner item not appearing in a form
            // An item and a subitem cannot both appear in the same form
            if( bIsSubItem )
            {
                if( pVarT->GetClass() == CL_SING )
                {
                    // lets check if item is multiple
                    VART* pItem = VPT( iOwnerItem );
                    //subitem's parent item can be multiple or the subitem's record
                    //could be multiple, if the subitem's record is multiple, it should bUseRecordLogic to set the occurrences in the logic that sets the occs below
                    //Savy to fix the bug related data getting deleted in modify mode on a subitem that is rostered when the record occurs and the subitem and item are single
                    //and the only items on the repeating record is an item with subitems and neither the item or the subitem occurs
                    // in this case the data occs are getting reset to 1 and deleting the existing subitem occs.
                    if( pItem->GetClass() == CL_MULT || pVarT->GetOwnerGPT()->GetMaxOccs() > 1)
                        bIsItem = true;
                }
                else // if( pVarT->GetClass() == CL_MULT ) // superfluous?
                {
                    ASSERT( pVarT->GetClass() == CL_MULT );
                    bIsItem = true;
                }
            }

            GROUPT* pGroupT = pVarT->GetOwnerGPT();
            ASSERT( pGroupT != 0 );

            GROUPT* pRecordCounter = pGroupT;
            while( pRecordCounter != 0 )
            {
                if( pRecordCounter->GetDimType() == CDimension::Record )
                {
                    // found!, let's use it
                    break;
                }
                pRecordCounter = pRecordCounter->GetOwnerGPT();
            }

            if( pRecordCounter == 0 )
            {
                // oops! could not get to Record?
                // let's try using original GroupT then
                pRecordCounter = pGroupT;
            }

            int iVarNumOccs = 0;
            int iVarNumPartialOccs = 0; // RHF May 07, 2004
            if( bPartialSave )
            {
                iVarNumOccs = pRecordCounter->GetDataOccurrences();
                iVarNumPartialOccs = pRecordCounter->GetPartialOccurrences(); // RHF May 07, 2004
            }
            else
            {
                iVarNumOccs = pRecordCounter->GetTotalOccurrences();
                iVarNumPartialOccs = iVarNumOccs; // RHF May 07, 2004
            }

            int     iSecNumOccs = 0;
            int     iSecNumPartialOccs = 0; // RHF May 07, 2004
            int     iSecMaxOccs = pSecT->GetMaxOccs();

            bool bDefineSectionExistence = true;
            if( bIsItem ) {                               // RHF Aug 25, 2000
                bool bUseRecordLogic = false;
                // sing-vars: produce a number of occs
                if( pVarT->GetClass() == CL_SING ) {
                    bUseRecordLogic = true;
                }
                // mult-vars with occs: force section existence
                else if( iVarNumOccs > 0 ) {
                    if( pSecT->GetMaxOccs() > 1 )
                        bUseRecordLogic = true;
                }

                if( bUseRecordLogic )
                {
                    if( pRecordCounter->GetDimType() == CDimension::Record )
                    {
                        iSecNumOccs = iVarNumOccs;
                        iSecNumPartialOccs = iVarNumPartialOccs; // RHF May 07, 2004, rcl Dec 2005
                        bDefineSectionExistence = false;
                    }
                }
            }                                           // RHF Aug 25, 2000

            if( bDefineSectionExistence )
            {
                iSecNumOccs = ( iVarNumOccs > 0 );      // RHF Aug 25, 2000
                iSecNumPartialOccs = ( iVarNumPartialOccs > 0 ); // RHF May 07, 2004
            }

            ASSERT( iSecNumOccs <= iSecMaxOccs );
            // update section' occurrences
            if( pSecX->GetOccurrences() < iSecNumOccs )
            {
                pSecX->SetOccurrences( iSecNumOccs );
            }

            // RHF INIC May 07, 2004
            if( !bLikeBatch ) { // Not necessary in batch but iSecNumPartialOccs is assigned anyway
                iSecNumPartialOccs = std::max( iSecNumPartialOccs, iSecNumOccs );
                ASSERT(iSecNumPartialOccs <= iSecMaxOccs);
                if( pSecX->GetPartialOccurrences() < iSecNumPartialOccs )
                    pSecX->SetPartialOccurrences( iSecNumPartialOccs );
            }
            // RHF END May 07, 2004
        }

        iSymVar = pVarT->SYMTfwd;
    }

    return pSecX->GetOccurrences();     // return section' occurrences
}

// ------ setting up Group occurrences from related Sects

#ifdef NEW_VERSION
// RHF INIC Sep 16, 2000
static int iNewCurrentOccurrences=0; // TODO eliminates when void CEngineDriver::SetupGroupOccsFromVar( VART* pVarT ) accepts 2 parameters
// SetupGroupOccsFromSect: transfer Sect' occurrences to related Groups
void CEngineDriver::SetupGroupOccsFromSect( SECT* pSecT ) {
    VART*       pVarT;
    GROUPT*     pGroupT = NULL;
    int         iSymVar = pSecT->SYMTfvar;
    int         iSecNumOccs = pSecT->GetSecX()->GetOccurrences();
    int         iMaxGroup;
    bool        bEndGroup;

    iNewCurrentOccurrences = 0;

    while( iSymVar> 0 ) {
        pVarT = VPT(iSymVar);
        ASSERT( pVarT );
        pGroupT = pVarT->GetOwnerGPT();
        ASSERT( pGroupT );
        ASSERT( pGroupT->GetSymbolIndex() > 0 );

        iMaxGroup = pGroupT->GetMaxOccs(); // fast, so no problem.

        // sing-vars: increase the Group' number of occs up to the occurrences of the Sect
        if( iMaxGroup <= 1 )
            iNewCurrentOccurrences = iSecNumOccs;
        else {
            for( int iOccur = iMaxGroup; iOccur >= 1; iOccur-- ) {
                if( !IsBlankField( pVarT, iOccur ) ) {
                    if( iOccur > iNewCurrentOccurrences )
                        iNewCurrentOccurrences = iOccur;
                    break;
                }
            }
        }

        // Next var
        iSymVar = pVarT->SYMTfwd;
        bEndGroup = ( iSymVar <= 0 || VPT(iSymVar)->GetOwnerGPT() != pGroupT );

        if( bEndGroup ) {
            SetupGroupOccsFromVar( pVarT );
            iNewCurrentOccurrences = 0;
        }
    }
}


//TODO: Change the name to this function. Parameters will be: GROUPT and iNewCurrentOccurrences
void CEngineDriver::SetupGroupOccsFromVar( VART* pVarT ) {
    GROUPT* pGroupT = pVarT->GetOwnerGPT();
    bool    bNeedRefresh = ( pGroupT->GetCurrentOccurrences() < iNewCurrentOccurrences );

    if( bNeedRefresh ) {
        pGroupT->SetCurrentOccurrences( iNewCurrentOccurrences );
        pGroupT->SetTotalOccurrences( iNewCurrentOccurrences );
    }

    if( bNeedRefresh && iNewCurrentOccurrences > 0 ) {
        int     iSymOwner = pGroupT->GetOwnerSymbol();

        // climb up to owner groups up to the level...
        while( iSymOwner ) {
            if( !m_pEngineArea->IsSymbolTypeGR( iSymOwner ) )
                break;

            // get the owner group and...
            GROUPT* pGroupTOwner  = GPT(iSymOwner);
            bool    bIsLevelGroup = ( pGroupTOwner->GetGroupType() == 1 );

            // ... force its existence
            if( pGroupTOwner->GetDataOccurrences() < 1 ) {
                pGroupTOwner->SetCurrentOccurrences( 1 );
                pGroupTOwner->SetTotalOccurrences( 1 );
            }

            // don't climb up beyond a Level-group
            iSymOwner = ( bIsLevelGroup ) ? 0 : pGroupTOwner->GetOwnerSymbol();
        }
    }
}

// RHF END Sep 16, 2000
#else

/* RHF COM INIC Mar 08, 2001
void CEngineDriver::SetupGroupOccsFromSect( SECT* pSecT ) {     // victor Mar 23, 00
// SetupGroupOccsFromSect: transfer Sect' occurrences to related Groups
int         iSymVar = pSecT->SYMTfvar;

// each Var in the Sect affect separatedly its Group' occurrences
int         iSymLastGroup = 0;

while( iSymVar > 0 ) {
VART*   pVarT = VPT(iSymVar);
int     iSymGroup = pVarT->GetOwnerGroup();

//      if( iSymGroup && iSymGroup != iSymLastGroup ) { // victor Sep 15, 00
//          SetupGroupOccsFromVar( pVarT );             // victor Sep 15, 00
//          iSymLastGroup = iSymGroup;                  // victor Sep 15, 00
//      }                                               // victor Sep 15, 00
SetupGroupOccsFromVar( pVarT ); // VERY SLOW!!  // victor Sep 15, 00

iSymVar = pVarT->SYMTfwd;
}
}
RHF COM END Mar 08, 2001 */

void CEngineDriver::SetupGroupOccsFromVar( VART* pVarT ) { // victor Mar 23, 00
    // SetupGroupOccsFromVar: transfer Var' occs from its Sect to the Group hosting that Var
    int     iSymGroup = pVarT->GetOwnerGroup();
    int     iSymSec   = pVarT->GetOwnerSec();
    bool    bLikeBatch = ( Issamod != ModuleType::Entry );
    int     iSecNumOccs = 0;
    bool    bAffectGroup = ( iSymGroup && ( bLikeBatch || iSymGroup > 0 ) ); // victor May 10, 00

    //  if( bAffectGroup && iSymSec > 0 && ( bLikeBatch || iSymFrm > 0 ) ) // victor May 10, 00
    if( bAffectGroup && iSymSec > 0 )                                  // victor Sep 13, 00
        iSecNumOccs = SPX(iSymSec)->GetOccurrences();

    if( iSecNumOccs ) {
        // the Var in the Sect affects its Group' occurrences:
        GROUPT* pGroupT   = pVarT->GetOwnerGPT();       // victor May 25, 00
        int     iSymOwner = pGroupT->GetOwnerSymbol();

        //      if( pVarT->GetClass() == CL_SING ) {                           // victor Sep 13, 00
        if( m_pEngineArea->GroupMaxNumOccs( iSymGroup ) <= 1 ) { // victor Sep 13, 00
            // sing-vars: increase the Group' number of occs up to the occurrences of the Sect
            if( pGroupT->GetCurrentOccurrences() < iSecNumOccs ) {
                pGroupT->SetCurrentOccurrences( iSecNumOccs );
                pGroupT->SetTotalOccurrences( iSecNumOccs );
            }
        }
        else {
            // mult-vars: get the highest occurrence, scanning up to the maximum Group occs:
            int     iMultMaxOccs = m_pEngineArea->GroupMaxNumOccs( iSymGroup );
            int     iMultNumOccs = 0;

            for( int iOccur = 1; iOccur <= iMultMaxOccs; iOccur++ ) {
                if( !IsBlankField( pVarT, iOccur ) ) {
                    iMultNumOccs = iOccur;
                }
            }

            if( !iMultNumOccs ) {
                // no occurrences detected: don't touch owner groups
                iSymOwner = 0;
            }
            else {
                // install the number of occurrences...
                if( pGroupT->GetCurrentOccurrences() < iMultNumOccs ) {
                    pGroupT->SetCurrentOccurrences( iMultNumOccs );
                    pGroupT->SetTotalOccurrences( iMultNumOccs );
                }
            }
        }

        // climb up to owner groups up to the level...
        while( iSymOwner ) {
            if( !m_pEngineArea->IsSymbolTypeGR( iSymOwner ) )
                break;

            // get the owner group and...
            GROUPT* pGroupTOwner  = GPT(iSymOwner);
            bool    bIsLevelGroup = ( pGroupTOwner->GetGroupType() == 1 );

            // ... force its existence
            if( pGroupTOwner->GetDataOccurrences() < 1 ) {
                pGroupTOwner->SetCurrentOccurrences( 1 );
                pGroupTOwner->SetTotalOccurrences( 1 );
            }

            // don't climb up beyond a Level-group
            iSymOwner = ( bIsLevelGroup ) ? 0 : pGroupTOwner->GetOwnerSymbol();
        }
    }
}
#endif


static bool bOldMode=false;
// RHF INIC Mar 03, 2001
// SetupGroupOccsFromSect: transfer Sect' occurrences to related Groups
void CEngineDriver::SetupGroupOccsFromSect( SECT* pSecT ) {
    if( bOldMode ) {
        //////////////////////////////////////////////////////////////////////////////////
        // Old mode
        //////////////////////////////////////////////////////////////////////////////////
        // SetupGroupOccsFromSect: transfer Sect' occurrences to related Groups
        int         iSymVar = pSecT->SYMTfvar;

        // each Var in the Sect affect separatedly its Group' occurrences
        // int         iSymLastGroup = 0;

        while( iSymVar > 0 ) {
            VART*   pVarT = VPT(iSymVar);
            // int     iSymGroup = pVarT->GetOwnerGroup();

            //      if( iSymGroup && iSymGroup != iSymLastGroup ) { // victor Sep 15, 00
            //          SetupGroupOccsFromVar( pVarT );             // victor Sep 15, 00
            //          iSymLastGroup = iSymGroup;                  // victor Sep 15, 00
            //      }                                               // victor Sep 15, 00
            SetupGroupOccsFromVar( pVarT ); // VERY SLOW!!  // victor Sep 15, 00

            iSymVar = pVarT->SYMTfwd;
        }

        return;
    }

    //////////////////////////////////////////////////////////////////////////////////
    // New mode
    //////////////////////////////////////////////////////////////////////////////////
    int         iSymVar = pSecT->SYMTfvar;

    // each Var in the Sect affect separatedly its Group' occurrences
    GROUPT*     pGroupT;
    VART*       pVarT;
    std::set<GROUPT*> aGroupSet;

    while( iSymVar > 0 ) {
        pVarT = VPT(iSymVar);
        ASSERT( pVarT );


        pGroupT = pVarT->GetOwnerGPT();
        if( pGroupT != 0 && aGroupSet.find(pGroupT) == aGroupSet.end() )
        {
            pGroupT->CalculateOccurrence();
            aGroupSet.insert( pGroupT );
        }

        iSymVar = pVarT->SYMTfwd;
    }
}
// RHF END Mar 03, 2001


void CEngineDriver::CheckSectionOccurrences(int iSymSec, bool bPartialSave)
{
    // CheckSectionOccurrences: verifies structural limits for a given section and, if needed, adjusts occurrences
    ASSERT(Issamod == ModuleType::Entry);
    SECT* pSecT = SPT(iSymSec);
    SECX* pSecX = SPX(iSymSec);
    CNDIndexes theIndex(ZERO_BASED, DEFAULT_INDEXES);

    theIndex.setHomePosition(); // theIndex should be:  (0,0,0)

    // trapping typos in single records
    if( pSecX->GetOccurrences() == 0 && pSecT->GetMaxOccs() <= 1 )
    {
        int iSymVar = pSecT->SYMTfvar;
        int iFieldColor = 0;

        while( iSymVar > 0 )
        {
            VART* pVarT = VPT(iSymVar);

            // RHF COM Dec 03, 2001 iFieldColor = bPartialSave ? FLAG_HIGHLIGHT : m_pIntDriver->GetFieldColor( pVarT, aIndex ); // RHF Jan 23, 2001 ADD aIndex
            iFieldColor = ( bPartialSave && !IsBlankField( pVarT, 1 ) ) ? FLAG_HIGHLIGHT : m_pIntDriver->GetFieldColor( pVarT, theIndex );// RHF Dec 03, 2001

            if( pVarT->GetClass() == CL_SING && iFieldColor != FLAG_NOLIGHT )
            {
                pSecX->SetOccurrences(1);
                break;
            }

            iSymVar = pVarT->SYMTfwd;
        }
    }

    // verifying if min/max occs are respected
    int iOccError = ( pSecX->GetOccurrences() < pSecT->GetMinOccs() ) ? -1 :
                    ( pSecX->GetOccurrences() > pSecT->GetMaxOccs() ) ?  1 : 0;

    if( iOccError )
        pSecX->SetOccurrences( ( iOccError < 0 ) ? pSecT->GetMinOccs() : pSecT->GetMaxOccs() );
}


//Returns  -1,0,1 indicating relation between iSourceSym & iTargetSym
//Returns -2,-3,-4,-5 when an error is produced
// RHF INIC Dec 10, 2003
// BUCEN_2003 Changes Init
int CEngineDriver::CompareAbsoluteFlowOrder( int iSourceSym, int iSourceOcc, int iTargetSym, int iTargetOcc ) {
    int         iRet=0;

    GROUPT*     pOwnerGroupSource=NULL;
    GROUPT*     pOwnerGroupTarget=NULL;


    if( iSourceOcc < 1 ) iSourceOcc = 1;
    if( iTargetOcc < 1 ) iTargetOcc = 1;

    int     iMaxSourceOcc=0, iMaxTargetOcc=0;
    int     iSourceFlowOrder=-1, iTargetFlowOrder=-1;

    if( NPT(iSourceSym)->GetType() == SymbolType::Variable ) {
        VART*   pSourceVar = VPT(iSourceSym);

        iMaxSourceOcc=pSourceVar->GetFullNumOccs( true );
        iSourceFlowOrder = pSourceVar->GetAbsoluteFlowOrder();

        pOwnerGroupSource=pSourceVar->GetOwnerGPT();
    }
    else if( NPT(iSourceSym)->GetType() == SymbolType::Group ) {
        GROUPT*     pSourceGroup=GPT(iSourceSym);

        iMaxSourceOcc = 1;
        iSourceFlowOrder = pSourceGroup->GetAbsoluteFlowOrder();
        pOwnerGroupSource = pSourceGroup;
    }
    else {
        ASSERT(0);
        return -4;
    }

    if( NPT(iTargetSym)->GetType() == SymbolType::Variable ) {
        VART*       pTargetVar=VPT(iTargetSym);

        iMaxTargetOcc = pTargetVar->GetFullNumOccs( true );
        iTargetFlowOrder = pTargetVar->GetAbsoluteFlowOrder();
        pOwnerGroupTarget = pTargetVar->GetOwnerGPT();
    }

    else if( NPT(iTargetSym)->GetType() == SymbolType::Group ) {
        GROUPT*     pTargetGroup=GPT(iTargetSym);

        iMaxTargetOcc = 1;
        iTargetFlowOrder = pTargetGroup->GetAbsoluteFlowOrder();
        pOwnerGroupTarget = pTargetGroup;
    }
    else {
        ASSERT(0);
        return -5;
    }

    if( iSourceOcc > iMaxSourceOcc )
        iRet = -2;
    else if( iTargetOcc > iMaxTargetOcc )
        iRet = -3;
    else {
        if( iSourceOcc == 1 || iTargetOcc == 1 || iSourceOcc == iTargetOcc ) {// One single or the same Occ
            if( iSourceFlowOrder > iTargetFlowOrder)
                iRet = 1;
            if( iSourceFlowOrder < iTargetFlowOrder )
                iRet = -1;
            else
                iRet = 0;
        }
        else { // Both multiple & iSourceOcc != iTargetOcc
            if( pOwnerGroupSource == pOwnerGroupTarget ) {
                if( iSourceOcc > iTargetOcc )
                    iRet = 1;
                else if( iSourceOcc < iTargetOcc )
                    iRet = -1;
                else
                    ASSERT(0);
            }
            else {
                if( pOwnerGroupSource->GetAbsoluteFlowOrder() > pOwnerGroupTarget->GetAbsoluteFlowOrder() )
                    iRet = 1;
                if( pOwnerGroupSource->GetAbsoluteFlowOrder() < pOwnerGroupTarget->GetAbsoluteFlowOrder() )
                    iRet = -1;
                else
                    ASSERT(0);
            }
        }
    }

    return iRet;
}
// BUCEN_2003 Changes End
// RHF END Dec 10, 2003


int CEngineDriver::commonlvl( DICT* pDicT, LPCTSTR lastkey, int iLastLevel, LPCTSTR key, int iLevel ) {
    // commonlvl: return "maximum" shared level for 'lastkey' and 'key' keys
    LPCTSTR p = lastkey;
    LPCTSTR q = key;
    int     iSharedLevel;
    int     iSharedLen;

    for( iSharedLevel = 0; iSharedLevel < iLastLevel && iSharedLevel < iLevel; iSharedLevel++ ) {
        int     iLevelKeyLen = pDicT->qlen[iSharedLevel];

        for( iSharedLen = 0; iSharedLen < iLevelKeyLen; iSharedLen++ )
            if( *p++ != *q++ )
                break;
        if( iSharedLen < iLevelKeyLen )
            break;
    }

    return iSharedLevel;
}


void CEngineDriver::UpdateLoadedLevels(Pre74_CaseLevel* pCaseLevel,bool bIsNew)
{
    int iLevel = pCaseLevel->GetLevelNum();

    m_aLoadedLevels[iLevel - 1].m_caseLevel = pCaseLevel;
    m_aLoadedLevels[iLevel - 1].m_isNew = bIsNew;

    // clear any loaded levels at a higher level
    ClearLoadedLevels(iLevel + 1);
}

void CEngineDriver::ClearLoadedLevels(int iLevel)
{
    int iElementsToSet = ( (int)MaxNumberLevels - iLevel + 1 );

    if( iElementsToSet > 0 )
        memset(m_aLoadedLevels + iLevel - 1,0,sizeof(m_aLoadedLevels[0]) * iElementsToSet);
}


void CEngineDriver::ParseCaseLevel(Case* pCasetainer, Pre74_CaseLevel* pCaseLevel, const CaseLevel* case_level_with_binary_data, DICT* pDicT)
{
    if( &pCasetainer->GetRootCaseLevel() == case_level_with_binary_data )
        pDicT->m_binaryStorageFor80.clear();

    DICX* pDicX = pDicT->GetDicX();

    // remember the information about the last key
    pDicX->lastlevel = pDicX->level;
    _tcscpy(pDicX->last_key, pDicX->current_key);

    // store the new key information
    _tcscpy(pDicX->current_key, pCaseLevel->GetKey());
    pDicX->level = pCaseLevel->GetLevelNum();

    // set the node-level and clear every intermediate level
    int iNodeLevel = pDicX->level;

    int iSharedLevel = commonlvl(pDicT, pDicX->last_key, pDicX->lastlevel, pDicX->current_key, iNodeLevel);

    for( int iLevel = iSharedLevel + 1; iLevel <= iNodeLevel; iLevel++ )
        ClearLevelNode(pDicT,iLevel);

    if( iNodeLevel <= iSharedLevel )
        ClearLevelNode(pDicT,iNodeLevel);

    ResetDynamicAttributes(pDicX);

    const DictLevel& dict_level = pCaseLevel->GetDictLevel();
    bool bFirstRecord = true;

    for( int iRecord = 0; iRecord < dict_level.GetNumRecords(); iRecord++ )
    {
        const CDictRecord* pDictRecord = dict_level.GetRecord(iRecord);
        Pre74_CaseRecord* pCaseRecord = pCaseLevel->GetRecord(iRecord);

        const CaseRecord* case_record_with_binary_data = ( case_level_with_binary_data != nullptr ) ? &case_level_with_binary_data->GetCaseRecord(iRecord) : nullptr;
        ASSERT(case_record_with_binary_data == nullptr || case_record_with_binary_data->GetNumberOccurrences() == static_cast<size_t>(pCaseRecord->GetNumRecordOccs()));

        int iSymSec = pDictRecord->GetSymbol();
        SECT* pSecT = SPT(iSymSec);

        for( int iOcc = 0; iOcc < pCaseRecord->GetNumRecordOccs(); iOcc++ )
        {
            int iRecordLength = pCaseRecord->GetRecordLength();
            TCHAR* pRecordArea = pDicX->GetRecordArea();

            _tmemcpy(pRecordArea,pCaseRecord->GetRecordBuffer(iOcc),iRecordLength); // REPO_TODO: in the future, can pass sectadd the record buffer directly
            pRecordArea[iRecordLength] = 0; // REPO_TEMP: sectadd now checks the record length; perhaps it doesn't have to in the future

            bool bAdded = sectadd(pSecT, case_record_with_binary_data);
            ASSERT(bAdded);

            if( bFirstRecord )
            {
                pDicT->Common_GetData(iNodeLevel,iNodeLevel);
                sectcommonadd(pDicT,iNodeLevel - 1);
                bFirstRecord = false;
            }
        }
    }

    // transfer all Sect' occurrences in node-level to related Groups
    BuildGroupOccsFromNodeRecords(pDicT,iNodeLevel); // victor May 16, 00

    if( pDicT == DIP(0) )
        UpdateLoadedLevels(pCaseLevel,false); // if on the primary dictionary

    // load the notes if on the first level
    if( pCaseLevel->GetLevelNum() == 1 )
        ParseCaseNotes_pre80(pDicX);
}


void CEngineDriver::CopyLevelToRepository(DICT* pDicT,Case* pCasetainer,Pre74_CaseLevel* pCaseLevel,bool bPartialSave/* = false*/)
{
    int iLevel = pCaseLevel->GetLevelNum();
    bool bNoRecordsOutput = false;

    // process each section in the given level
    int iSymSec = pDicT->SYMTfsec;
    int iCommonSymSec = -1;

    while( iSymSec >= 0 )
    {
        SECX* pSecX = SPX(iSymSec);
        SECT* pSecT = pSecX->GetSecT();

        bool bIsCommonSect = pSecT->IsCommon();

        if( pSecT->GetLevel() == iLevel || bIsCommonSect )
        {
            SetupSectOccsFromGroups(iSymSec, bPartialSave); // fix problem of generating new records

            if( Issamod == ModuleType::Entry )
                CheckSectionOccurrences(iSymSec, bPartialSave);

            Pre74_CaseRecord* pCaseRecord = pCaseLevel->GetRecord(pSecT->GetDictRecord());

            if( pCaseRecord != nullptr )
                pCaseRecord->Reset();

            int section_occurrences = pSecX->GetOccurrences();

            if( bPartialSave )
                section_occurrences = std::max(section_occurrences, pSecX->GetPartialOccurrences());

            if( section_occurrences > 0 )
            {
                int iSymVar = pSecT->SYMTfvar;

                while( iSymVar >= 0 )
                {
                    VARX* pVarX = VPX(iSymVar);
                    VART* pVarT = pVarX->GetVarT();
                    bool bIsTrueItem = ( pVarT->GetOwnerSymItem() == 0 );

                    if( IsBinary(*pVarT->GetDictItem()) )
                    {
                        // prepare all binary data as if it were not partially saved
                        m_pEngineDriver->prepvar(pVarT, false);
                        pCaseLevel->m_binaryStorageFor80 = std::make_unique<std::vector<std::shared_ptr<BinaryStorageFor80>>>(pDicT->m_binaryStorageFor80);
                    }

                    else if( Issamod == ModuleType::Entry )
                    {
                        if( bIsTrueItem )
                        {
                            // Fix problem of skip in sub-items. Gray sub-item was been written!!
                            VART* pSubItem = pVarT->GetNextSubItem();
                            bool bSomeSubItemRefreshed = false;

                            while( pSubItem != nullptr )
                            {
                                if( pSubItem->IsInAForm() )
                                {
                                    m_pEngineDriver->prepvar(pSubItem,bPartialSave);
                                    bSomeSubItemRefreshed = true;
                                }

                                pSubItem = pSubItem->GetNextSubItem();
                            }

                            // Only refresh the item if there is no sub-item in form
                            if( !bSomeSubItemRefreshed )
                                prepvar(pVarT,bPartialSave);
                        }
                    }

                    else // batch mode
                    {
                        // converts all variables in this section (only items are prepared)
                        if( pVarT->IsUsed() && bIsTrueItem )
                            m_pEngineDriver->prepvar(pVarT,bPartialSave);
                    }

                    iSymVar = pVarT->SYMTfwd;
                }

                if( !bIsCommonSect )
                {
                    bNoRecordsOutput = false;
                    CopySectionToRepository(pDicT,pCaseRecord,iSymSec,bPartialSave);
                }

                else if( iCommonSymSec < 0 )
                {
                    iCommonSymSec = iSymSec;
                }
            }
        }

        iSymSec = SPT(iSymSec)->SYMTfwd;
    }

    // in dictionaries where the ID makes up the whole case (and there is only one record), the single record will always be empty, but we still want to write out the case (comprised of only the ID fields)
    if( bNoRecordsOutput && iCommonSymSec >= 0 )
    {
        int iFirstSec = SPT(iCommonSymSec)->SYMTfwd;
        SECT* pSecT = SPT(iFirstSec);
        SECX* pSecX = SPX(iFirstSec);

        if( pSecT->SYMTfwd == -1 && !pSecX->GetOccurrences() ) // only do this when there is only one record
        {
            pSecX->SetOccurrences(1);
            CopySectionToRepository(pDicT,pCaseLevel->GetRecord(pSecT->GetDictRecord()),iFirstSec,bPartialSave);
            pSecX->SetOccurrences(0);
        }
    }
}


void CEngineDriver::CopySectionToRepository(DICT* pDicT,Pre74_CaseRecord* pCaseRecord,int iSymSec,bool bPartialSave)
{
    SECT* pSecT = SPT(iSymSec);
    SECX* pSecX = SPX(iSymSec);

    int section_length = std::min(pSecX->GetSecDataSize(), pCaseRecord->GetRecordLength());
    ASSERT(section_length > 0);

    int section_occurrences = pSecX->GetOccurrences();

    if( bPartialSave )
        section_occurrences = std::max(section_occurrences, pSecX->GetPartialOccurrences());

    for( int iOcc = 0; iOcc < section_occurrences; iOcc++ )
    {
        TCHAR* pSecBuffer = pSecX->GetAsciiAreaAtOccur(iOcc + 1);
        ASSERT(pSecBuffer != NULL);

        TCHAR* pCaseRecordBuffer = pCaseRecord->GetOrCreateRecordBuffer(iOcc);

        // copy the record contents
        _tmemcpy(pCaseRecordBuffer, pSecBuffer, section_length);

        // copy the IDs (unless it is a special output dictionary)
        if( pDicT->GetSubType() != SymbolSubType::Output )
            pDicT->Common_DoBuffer(pCaseRecordBuffer,1,pSecT->GetLevel());
    }
}


void CEngineDriver::PrepareCaseFromEngineForQuestionnaireViewer(DICT* pDicT, Case& data_case)
{
    // REPO_TODO ... this method will not be necessary once the Case object is updated directly
    ASSERT(pDicT->GetDataDict() == &data_case.GetCaseMetadata().GetDictionary());

    ASSERT(data_case.m_pre74Case == nullptr);
    Pre74_Case* pCase = data_case.GetPre74_Case();
    Pre74_CaseLevel* pRootLevel = pCase->GetRootLevel();

    // copy the case values that are currently updated in the Case object (e.g., notes)
    DICX* pDicX = pDicT->GetDicX();
    UpdateCaseNotesLevelKeys_pre80(pDicX);
    data_case = pDicX->GetCase();

    // clear the records
    data_case.GetRootCaseLevel().Reset();

    // copy the root level...
    CopyLevelToRepository(pDicT, &data_case, pRootLevel, true);

    try
    {
        // ...and finalize it        
        pCase->FinalizeLevel(pRootLevel, true, true, nullptr);
    }

    catch( const CaseHasNoValidRecordsException& )
    {
        CString csWriteCaseKey = m_pEngineDriver->key_string(pDicT);
        pCase->ApplySpecialOutputKey(pCase, pRootLevel, csWriteCaseKey);
    }

    data_case.ApplyPre74_Case(pCase);
}
