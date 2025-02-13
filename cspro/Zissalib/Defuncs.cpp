//-------------------------------------------------------------------------//
//                                                                         //
//  deFUNCS : data entry functions                                         //
//                                                                         //
//-------------------------------------------------------------------------//
#include "StdAfx.h"
#include "CsDriver.h"
#include <engine/EXENTRY.H>
#include <engine/Engine.h>
#include <engine/INTERPRE.H>
#include <zCaseO/Case.h>
#include <zMessageO/MessageManager.h>
#include <zMessageO/Messages.h>
#include <zParadataO/Logger.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[]= __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
//                                                                         //
//  main public functions                                                  //
//                                                                         //
/////////////////////////////////////////////////////////////////////////////


///////////////////////// executor support? ////////////////////////////////

void CEntryDriver::DeInitLevel( int iLevel )
{
    // this loop is no longer meaningful, REVIEW        // victor Jul 27, 99
    for( int iSymSec = DIP(0)->SYMTfsec; iSymSec >= 0; iSymSec = SPT(iSymSec)->SYMTfwd )
        initsect(iSymSec);

    for( int iAtLevel = iLevel; iAtLevel < (int)MaxNumberLevels; iAtLevel++ )
        m_pEngineArea->InitLevelGroups( iAtLevel );    // added!       // victor Jul 27, 99

    InitializePersistentFields(iLevel);
    InitializeAutoIncrementFields();
    PrefillKeyFromPff();
    PrefillNonPersistentFields();
}


void CEntryDriver::WipeGroupFieldColors( int iSymGroup ) {
    // wipe-out color flags in a given Group (up to max-occs, for all members)
    GROUPT* pGroupT  = GPT(iSymGroup);
    int     iUpToOcc = pGroupT->GetMaxOccs();

    for( int iItem = 0; iItem < pGroupT->GetNumItems(); iItem++ ) {
        int     iSymItem = pGroupT->GetItemSymbol( iItem );

        if( m_pEngineArea->IsSymbolTypeGR( iSymItem ) )
            WipeGroupFieldColors( iSymItem );
        else if( m_pEngineArea->IsSymbolTypeVA( iSymItem ) ) {
            for( int iOccur = 1; iOccur <= iUpToOcc; iOccur++ )
                m_pIntDriver->SetFieldColor( iSymItem, iOccur, FLAG_NOLIGHT );
        }
    }
}


int CEntryDriver::dedemode()
{
    if( Issademode == ADD && !IsModification() )
        return 1;

    DICT* pDicT = DIP(0);
    DICX* pDicX = pDicT->GetDicX();
    const Case& data_case = pDicX->GetCase();

    bool bAfterPartialPos = false;

    if( m_pCsDriver != nullptr && data_case.GetPartialSaveCaseItemReference() != nullptr )
    {
        auto o3DTarget = m_pIntDriver->ConvertIndex(*data_case.GetPartialSaveCaseItemReference());
        int iLocation = m_pCsDriver->SearchTargetLocation(o3DTarget.get());

        if( iLocation > 0 ) // target after current
            bAfterPartialPos = false;

        else if( iLocation == 0 )
        {
            // check events because the partial save can only be done by interface so PreProc & OnFocus were executed before partial save
            if( m_pIntDriver->m_iProgType == PROCTYPE_PRE || m_pIntDriver->m_iProgType == PROCTYPE_ONFOCUS )
                bAfterPartialPos = false;

            else
                bAfterPartialPos = true;
        }

        else // target before current
            bAfterPartialPos = true;
    }

    int iMode = 1; // default to add mode

    if( bAfterPartialPos )
    {
        if( m_ePartialMode == MODIFY_MODE )
            iMode = 2;

        else if( m_ePartialMode == VERIFY_MODE )
            iMode = 3;
    }

    else if( IsVerify() )
        iMode = 3;

    else
        iMode = 2;

    return iMode;
}


///////////////////////// basic public functions /////////////////////////////
bool CEntryDriver::MakeField3( DEFLD3* pszFld3, const DEFLD* pszFld ) {
    bool    bDone = true;
    VARX*   pVarX = VPX( ((DEFLD*) pszFld)->GetSymbol());

    ASSERT( pszFld->isUsingOnlyOneDimension() );
    int     iOccur = ( (DEFLD*) pszFld )->getIndexValue(0);

    if( iOccur < 1 )
        iOccur = 1;

    CNDIndexes theIndex( ZERO_BASED );
    pVarX->BuildIntParentIndexes( theIndex, iOccur );       // TRANSITION ?

    pszFld3->SetSymbol( ((DEFLD*) pszFld)->GetSymbol() );
    pszFld3->setIndexes( theIndex );

    return bDone;
}

/////////////////////////////// deset_xxx ///////////////////////////////////

void CEntryDriver::deset_low( DEFLD* fld ) {
    bool    bPathOn = m_pEngineSettings->IsPathOn();

    if( fld->getIndexValue(0) < 1 )
        fld->setIndexValue(0,1 );

    // uninstall highlight for this field
    if( bPathOn )                               // always becomes no-light
        m_pIntDriver->SetFieldColor( fld->GetSymbol(), fld->getIndexValue(0), FLAG_NOLIGHT );
    else {
        int     iFieldColor = m_pIntDriver->GetFieldColor( fld->GetSymbol(), fld->getIndexValue(0) );

        if( iFieldColor != FLAG_NOLIGHT )       // had color - becomes mid
            m_pIntDriver->SetFieldColor( fld->GetSymbol(), fld->getIndexValue(0), FLAG_MIDLIGHT );
    }
}


///////////////////////////////////////////////////////////////////////////
//                                                                      ///
//                                                                      ///
//  elementary functions added                          // victor       ///
//                                                                      ///
//                                                                      ///
///////////////////////////////////////////////////////////////////////////

bool CEntryDriver::ConfirmValue(bool value_is_notappl, VART* pVarT, CNDIndexes& theIndex)
{
    const bool can_accept_invalid_value = value_is_notappl ? ( ( pVarT->m_iBehavior & CANENTER_NOTAPPL ) != 0 ) :
                                                             ( ( pVarT->m_iBehavior & CANENTER_OUTOFRANGE) != 0 );

    constexpr MessageType message_type = MessageType::Error;
    const int message_number = can_accept_invalid_value ? MGF::OutOfRangeConfirm :
                                                          MGF::OutOfRange;
    std::wstring occurrence_text;

    if( pVarT->IsArray() )
        occurrence_text = theIndex.toStringBare(); // TRANSITION.

    // if they can't accept the value, display the error and return
    if( !can_accept_invalid_value )
    {
        if( m_pIntDriver->IsUsing3D_Driver() || Decurmode != ADVMODE )
            issaerror(message_type, message_number, pVarT->GetName().c_str(), occurrence_text.c_str());

        return false;
    }

    // otherwise, show a message with yes/no buttons

    const std::wstring message_text = FormatTextCS2WS(MGF::GetMessageText(message_number).c_str(), pVarT->GetName().c_str(), occurrence_text.c_str());
    m_pEngineDriver->GetSystemMessageManager().IncrementMessageCount(message_number);

    std::unique_ptr<Paradata::MessageEvent> message_event;

    if( Paradata::Logger::IsOpen() )
        message_event = m_pIntDriver->m_pParadataDriver->CreateMessageEvent(MessageType::Error, message_number, message_text);

    const std::tuple<std::vector<std::wstring>, int> button_text_and_default_button_number({ MGF::GetMessageText(MGF::Yes), MGF::GetMessageText(MGF::No) },
                                                                                           1); // default to selecting No

    const int selected_button_number = DisplayMessage(message_type, message_number, message_text, &button_text_and_default_button_number);

    if( message_event != nullptr )
    {
        message_event->SetPostDisplayReturnValue(selected_button_number);
        m_pIntDriver->m_pParadataDriver->RegisterAndLogEvent(std::move(message_event));
    }

    return ( selected_button_number == 1 );
}


/////////////////////////////////////////////////////////////////////////////
//                                                                         //
//  private functions                                                      //
//                                                                         //
/////////////////////////////////////////////////////////////////////////////

///////////////////// new functions to support PATH OFF //////////////////////

void CEntryDriver::ResetLevelCurOccs( int iLevel, bool bResetItAlso ) {
    ASSERT( iLevel > 0 );

    // checking level existence         <begin>         // victor Jan 20, 00
    GROUPT* pGroupTRoot = m_pEngineDriver->GetGroupTRootInProcess();

    if( iLevel > pGroupTRoot->GetNumItems() )
        return;

    GROUPT* pGroupTLevel = pGroupTRoot->GetLevelGPT( iLevel );
    // checking level existence         <end>           // victor Jan 20, 00
    int     iSymLevelGroup = pGroupTLevel->GetSymbol();
    ASSERT( iSymLevelGroup );

    ResetGroupCurOccs( iSymLevelGroup, bResetItAlso );
}

void CEntryDriver::ResetGroupCurOccs( int iSymGroup, bool bResetItAlso ) {
    // reset the occurrences of any children group of the target group and,
    // if requested, also reset the occurrences of the target group
    GROUPT* pGroupT = GPT(iSymGroup);
    int     iMaxOccs = pGroupT->GetMaxOccs();
    int     iNewCurOcc = ( bResetItAlso ) ? 0 : pGroupT->GetCurrentOccurrences(); // victor Jun 02, 00

    // for each item of group:
    for( int iItem = 0; iItem < pGroupT->GetNumItems(); iItem++ ) {
        DEFLD   ItemField;
        int     iSymItem = pGroupT->GetItemSymbol( iItem );

        switch( m_pEngineArea->GetTypeGRorVA( iSymItem ) ) {
            case SymbolType::Group:
                // ... reset any child group
                ResetGroupCurOccs( iSymItem );
                break;
            case SymbolType::Variable:
                // ... reset field highlighting at each possible occurrence
                for( int iOccur = 1; iOccur <= iMaxOccs; iOccur++ ) {
                    ItemField.SetSymbol( iSymItem );
                    ItemField.setIndexValue(0, iOccur );
                    deset_low( &ItemField );
                }
        }
    }

    // reset the occurrences of the target group        // victor Jun 02, 00
    pGroupT->SetCurrentOccurrences( iNewCurOcc );       // victor Jun 02, 00
}
