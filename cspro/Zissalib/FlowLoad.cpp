#include "StdAfx.h"
#include "CFlAdmin.h"                                   // victor Jul 25, 01
#include "FlowCore.h"                                   // victor Jan 08, 01
#include <engine/Tables.h>
#include <engine/Engine.h>
#include <ZBRIDGEO/npff.h>


#if defined(_DEBUG) && defined(WIN_DESKTOP)
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[]= __FILE__;
#endif


bool CEngineDriver::LoadApplFlows()
{
    int         iNumberOfFlows = Appl.GetNumFlows();
    bool        bDone = true;
    bool        bIsEntryRun = ( Issamod == ModuleType::Entry );
    CFlAdmin*   pFlAdmin = ( bIsEntryRun ) ? ((CEntryDriver*) m_pEngineDriver)->GetFlAdmin() : NULL;
#ifdef _DEBUG
    TRACE( _T("\n\n* LoadApplFlows: loading %d Flows:\n"), iNumberOfFlows );
#endif

    m_iAbsoluteFlowOrder = 0; // BUCEN_2003 Changes
    AttachForms();                                // RHF Feb 22, 2000

    for( int iNumFlow = 0; bDone && iNumFlow < iNumberOfFlows; iNumFlow++ ) {
        ASSERT( Appl.GetFlowAt( iNumFlow ) );
        SetFlowInProcess( Appl.GetFlowAt( iNumFlow ) );
        FLOW*       pFlow = GetFlowInProcess();
        int         iSymFlow = pFlow->GetSymbolIndex();
        ASSERT( iSymFlow > 0 );

        io_Var.Empty();
        io_Err  = 0;

#ifdef _DEBUG
        TRACE( _T("\n\n--Loading Flow %s (%d) {io_Err=%d} ------------\n"), m_pEngineArea->DumpGroupTName(iSymFlow).GetString(), iSymFlow, io_Err );
#endif
        int     iNumGroupsBefore;
        int     iNumGroupsAfter;

        // 1. loading Flow' visible Groups
        m_iGlobalFlowOrder = 0;         // reset flow order
        iNumGroupsBefore = m_engineData->groups.size();
        if( !AddGroupTForOneFlow() ) {
            CString csMsgText;
            csMsgText.Format( _T("Unable to load Visible groups of Flow '%s'"), NPT(iSymFlow)->GetName().c_str() );
            issaerror( MessageType::Abort, MGF::OpenMessage, csMsgText.GetString() );
        }
        iNumGroupsAfter = m_engineData->groups.size();
        GetFlowInProcess()->SetVisibleGroups( iNumGroupsAfter - iNumGroupsBefore );

        // 2. loading invisible Groups from Dictionaries
        iNumGroupsBefore = m_engineData->groups.size();
        if( !AddGroupTForFlowDics() ) {
            CString csMsgText;
            csMsgText.Format( _T("Unable to load Invisible groups of Flow '%s'"), NPT(iSymFlow)->GetName().c_str() );
            issaerror( MessageType::Abort, MGF::OpenMessage, csMsgText.GetString() );
        }
        iNumGroupsAfter = m_engineData->groups.size();
        GetFlowInProcess()->SetInvisibleGroups( iNumGroupsAfter - iNumGroupsBefore );

        // 3. dimension and effective parents           // victor Jun 10, 00
        GROUPT*     pGroupTRoot = pFlow->GetGroupTRoot();
        int         iBaseSlot = pGroupTRoot->GetContainerIndex();
        int         iNumSlots = pFlow->GetNumberOfVisibleGroups() +
                                pFlow->GetNumberOfInvisibleGroups();

        for( int iGroupT = iBaseSlot; iGroupT < iBaseSlot + iNumSlots; iGroupT++ ) {
            GROUPT* pGroupT = GIP(iGroupT);

            pGroupT->SetDimAndParentGPT();

            // set effective parent group for each Var
            for( int iItem = 0; iItem < pGroupT->GetNumItems(); iItem++ ) {
                int     iSymItem = pGroupT->GetItemSymbol( iItem );

                if( m_pEngineArea->IsSymbolTypeVA( iSymItem ) ) {
                    VART*       pVarT        = VPT(iSymItem);
                    GROUPT*     pOwnerGroupT = pVarT->GetOwnerGPT();
                    bool        bFound       = false;

                    while( pOwnerGroupT != NULL && !bFound ) {
                        int     iOwnerType = pOwnerGroupT->GetGroupType();

                        // RHF INIC Aug 10, 2000
                        if( iOwnerType == GROUPT::Level ) {
                            pOwnerGroupT = NULL;
                            continue;
                        }
                        // RHF END Aug 10, 2000
                        ASSERT( iOwnerType != GROUPT::Level ); // an item can't be in a Level-group

                        bool        bIsEffectiveParent = false;

                        if( pOwnerGroupT->GetSource() == GROUPT::Source::DcfFile ) {
                            // groups coming from DCF are always effective parents
                            bIsEffectiveParent = true;
                        }
                        else {
                            CDEGroup*   pGroup = pOwnerGroupT->GetCDEGroup();
                            ASSERT( pGroup != NULL );   // should always have "IMSA group"
                            bIsEffectiveParent = ( pGroup->GetMaxLoopOccs() > 1 ); // RHF Aug 10, 2000
                        }

                        if( bIsEffectiveParent )
                            bFound = true;
                        else            // must climb-up to ascendant
                            pOwnerGroupT = pOwnerGroupT->GetOwnerGPT();
                    }

                    pVarT->SetParentGPT( pOwnerGroupT );

                    // RHF INIC Aug 10, 2000
                    // Assign dimension to groups
                    // (only not yet having any dimension assigned) // victor Jan 08, 00
                    for( int iDim = pVarT->GetNumDim() - 1; iDim >= 0; iDim-- ) {
                        // re. BUG115/Glenn     <begin> // victor Jan 08, 01
                        // take care: invisible groups never have a parent,
                        // they are isolated in order to let the engine access
                        // its values up to the maximum of defined occurrences.
                        // The ASSERT below was commented out and replaced by
                        // a 'break' when next parent reached does not exist.
//                      ASSERT( pOwnerGroupT == NULL );
                        if( pOwnerGroupT == NULL )
                            break;
                        // re. BUG115/Glenn     <end>   // victor Jan 08, 01

                        if( pOwnerGroupT->GetDimType() == CDimension::VoidDim ) // victor Jan 08, 00
                            pOwnerGroupT->SetDimType( pVarT->GetDimType(iDim) );
                        pOwnerGroupT = pOwnerGroupT->GetParentGPT();
                    }
                    // RHF END Aug 10, 2000
                }
            }

        }

        // RHF INIC Aug 16, 2000
        for( int iGroupT = iBaseSlot; iGroupT < iBaseSlot + iNumSlots; iGroupT++ ) {
            GROUPT* pGroupT = GIP(iGroupT);

            // Posiblemente sea un buen lugar para conocer
            // los valores para poder inicializar el groupT

            pGroupT->CreateRecordArray();// RHF Mar 05, 2001
        }
        // RHF END Aug 16, 2000


        bool    bFlowLoadedOK = LoadApplMessage();  // evaluate & prepare message

#ifdef _DEBUG
        TRACE( _T("\n--Loading Flow %s: ending with status=%d ------------\n"), NPT(iSymFlow)->GetName().c_str(), bFlowLoadedOK );
#endif

        if( !bFlowLoadedOK ) {              // failure
        }

        bDone &= bFlowLoadedOK;

        // building flow-strip for this flow    <begin> // victor Jan 08, 01
        CFlowCore*  pFlowCore = pFlow->GetFlowCore();

        pFlowCore->FlowStripBuild();
        // building flow-strip for this flow    <end>   // victor Jan 08, 01

        // --- flow-administrator: add the flow <begin> // victor Jul 25, 01
        if( pFlAdmin != NULL ) {
            pFlAdmin->CreateFlowUnit( pFlow );

            // ... insure the Primary flow gets activated
            if( pFlow->IsPrimary() )
                pFlAdmin->EnableFlow( iSymFlow );
        }
        // --- flow-administrator: add the flow <end>   // victor Jul 25, 01

        GenerateAbsoluteFlowOrder( pFlowCore ); // RHF Dec 10, 2003 BUCEN_2003 Changes
    }

    ResetFlowInProcess();

    // RHF INIC Feb 19, 2002
    // Create array of GROUPT asociated to SECT
    for( SECT* pSecT : m_engineData->sections )
        pSecT->CreateGroupArray();
    // RHF END Feb 19, 2002

    return bDone;
}

void CEngineDriver::AttachForms() {                     // RHF Feb 22, 2000
    int     iNumberOfFlows = Appl.GetNumFlows();

    for( int iNumFlow = 0; iNumFlow < iNumberOfFlows; iNumFlow++ ) {
        FLOW*   pFlow = Appl.GetFlowAt( iNumFlow );
        ASSERT( pFlow );

        CDEFormFile*    pFormFile = pFlow->GetFormFile();

        if( pFormFile != NULL ) {
            SetFlowInProcess( pFlow );

            // get PATH ON/OFF from primary Flow only   // RHF Apr 14, 2000
            if( pFlow->IsPrimary() )                    // victor Jul 25, 01
                pFormFile->IsPathOn() ? m_pEngineSettings->SetPathOn() :
                                        m_pEngineSettings->SetPathOff();

            // traversing each Form in this FormFile
            for( int iForm = 0; iForm < pFormFile->GetNumForms(); iForm++ ) {
                CDEForm*    pForm = pFormFile->GetForm( iForm );

                if( pForm != NULL )
                    AttachOneForm( pForm );
            }

            ResetFlowInProcess();
        }
    }
}

void CEngineDriver::AttachOneForm( CDEForm* pForm ) {   // RHF Feb 22, 2000
    for( int iMember = 0; iMember < pForm->GetNumItems(); iMember++ ) {
        CDEItemBase*    pMember = pForm->GetItem( iMember );

        AttachFormItem( pForm, pMember );
    }
}

void CEngineDriver::AttachFormItem( CDEForm* pForm, CDEItemBase* pMember ) { // RHF Feb 22, 2000
    FLOW*       pCurrentFlow = GetFlowInProcess();
    bool        bIsEntryRun  = ( Issamod == ModuleType::Entry );
    CDEFormBase::eItemType   eType        = pMember->GetItemType();
    int         iMember;

    if( eType == CDEFormBase::Roster ) {//SAVY 29 Aug 2000
        CDEGroup* pRoster = (CDEGroup*) pMember;

        for( iMember = 0; iMember < pRoster->GetNumItems(); iMember++ ) {
            CDEItemBase*    pRosterMember = pRoster->GetItem( iMember );
            AttachFormItem( pForm, pRosterMember);
        }
    }
    else if( eType == CDEFormBase::Group ) {                         // victor Apr 26, 00
        CDEGroup*   pGroup = (CDEGroup*) pMember;

        for( iMember = 0; iMember < pGroup->GetNumItems(); iMember++ ) {
            CDEItemBase*    pGroupMember = pGroup->GetItem( iMember );

            AttachFormItem( pForm, pGroupMember );
        }
    }
    else if( eType == CDEFormBase::Field ) {
        CDEField* pField = (CDEField*) pMember;

        if( pField->IsMirror() )
            return;

        if( !pField->GetDictItem()->AddToTreeFor80() )
        {
            return; // BINARY_TYPES_TO_ENGINE_TODO skipping loading non-VART objects for now
            // BINARY_TYPES_TO_ENGINE_TODO ignore above and continue, adding the item as a VART
        }

        CString csFullName;
        csFullName.Format( _T("%s.%s"), pField->GetItemDict().GetString(), pField->GetItemName().GetString());
        int iSymItem = m_pEngineArea->SymbolTableSearch(csFullName, { SymbolType::Variable });

        if( iSymItem == 0 )
        {
            issaerror(MessageType::Abort, 502, csFullName.GetString());
            return;
        }

        VART* pVarT = VPT(iSymItem);

        pVarT->SYMTfrm = pForm->GetSymbol();
        pField->SetSymbol( iSymItem ); // RHF COM Jul 28, 2000 Before was in VarxAddItemOcc

        // get field-behavior from pField
        FieldBehavior eBehavior = pField->IsProtected()        ? AsProtected :
                                  pField->IsEnterKeyRequired() ? AsEnter :
                                                                 AsAutoSkip;

        pVarT->SetDefinedBehavior( eBehavior );

        // set condition for Persistent fields...
        // ... only for ENTRY runs and primary flow
        pVarT->SetDummyPersistent(pField->IsPersistent());

        bool bAsPersistent = ( bIsEntryRun && pCurrentFlow->IsPrimary() && pField->IsPersistent() );

        pVarT->SetPersistent( bAsPersistent );
        pVarT->SetPersistentProtected(bAsPersistent && pField->IsProtected());

        pVarT->SetAutoIncrement( bIsEntryRun && pCurrentFlow->IsPrimary() && pField->IsAutoIncrement() );

        pVarT->SetSequential(pField->IsSequential());

        pVarT->SetAlwaysVisualValue(pField->IsAlwaysVisualValue());

        const int behavior_flags = CANENTER_NOTAPPL | CANENTER_OUTOFRANGE | CANENTER_SET_VIA_VALIDATION_METHOD;
        const int noconfirm_flags = CANENTER_NOTAPPL_NOCONFIRM | CANENTER_OUTOFRANGE_NOCONFIRM;

        if( pField->GetValidationMethod() == ValidationMethod::ValidateWithoutConfirmation )
            pVarT->m_iBehavior = pVarT->m_iBehavior | behavior_flags | noconfirm_flags;

        else if( pField->GetValidationMethod() == ValidationMethod::ValidateWithConfirmation  )
            pVarT->m_iBehavior = ( pVarT->m_iBehavior | behavior_flags ) & ~noconfirm_flags;

        else
            pVarT->m_iBehavior = pVarT->m_iBehavior & ~( behavior_flags | noconfirm_flags );

        pVarT->SetNeedVerification(pField->GetVerifyFlag()); // Nov 11 , 2001

        // persistent fields behave as protected by default
        if( bAsPersistent ) {
            pVarT->SetDefinedBehavior( AsProtected );
            pVarT->SetBehavior( AsProtected );
        }

        if( !pVarT->IsNumeric() ) {
            if( pField->IsUpperCase() )
                pVarT->SetFmt( 'A' );
            else
                pVarT->SetFmt( 'X' );
        }

        pVarT->SetCaptureInfo(pField->GetEvaluatedCaptureInfo());

        // inherit the parent form's capture position
        pVarT->SetCapturePos(pForm->GetCapturePos());

#ifdef WIN_DESKTOP
        if( bIsEntryRun )
            pVarT->SetHKL(LoadKLID(pField->GetKLID()));
#endif
    }
}

/////////////////////////////////////////////////////////////////////
//
//  GROUPT loading & maintenance for one Flow
//
/////////////////////////////////////////////////////////////////////

bool CEngineDriver::AddGroupTForOneFlow( void ) {
    // AddGroupTForOneFlow: building GROUPT chain for visible groups
    FLOW*   pCurrentFlow = GetFlowInProcess();

#ifdef _DEBUG
    TRACE(_T("\n--- AddGroupTForOneFlow: Flow %s, symbol #%d\n"), m_pEngineArea->DumpGroupTName(pCurrentFlow->GetSymbolIndex()).GetString(),
                                                                  pCurrentFlow->GetSymbolIndex());
#endif

    // looks for higher level
    CDEFormFile*    pFormFile = pCurrentFlow->GetFormFile();
    int             iNumLevels = ( pFormFile != NULL ) ? pFormFile->GetNumLevels() : 0;
    int             iMaxLevel = 0;      // the true number of levels found
    int             iLevel;

    for( iLevel = 0; iLevel < iNumLevels; iLevel++ ) {
        CDELevel*   pLevel = pFormFile->GetLevel(iLevel);

        if( pLevel != NULL ) {
            if( iMaxLevel < pLevel->GetHierarchy() + 1 )
                iMaxLevel = pLevel->GetHierarchy() + 1;
        }
    }

    // checking levels <begin>                          // victor Mar 02, 00
    if( iNumLevels != iMaxLevel ) {
        issaerror( MessageType::Error, 10081, pFormFile->GetName().GetString(), iNumLevels, iMaxLevel );
        return false;
    }

    if( pCurrentFlow->IsPrimary() ) {
        // check the number of levels of Flow and input dict
        if( iNumLevels < 1 ) {
            issaerror( MessageType::Error, 10082, iNumLevels );
            return false;
        }

        if( iNumLevels > (int)MaxNumberLevels ) {
            issaerror( MessageType::Error, 10083, iNumLevels, (int)MaxNumberLevels );
            return false;
        }

        int     iSymDic = GetFlowInProcess()->GetSymDicAt( 0 );
        int     iDicMaxLevel = DPT(iSymDic)->GetMaxLevel();

        if( iNumLevels > iDicMaxLevel ) {
            issaerror( MessageType::Error, 10084, iNumLevels, iDicMaxLevel );
            return false;
        }

        if( iNumLevels < iDicMaxLevel )
            issaerror( MessageType::Warning, 10085, iNumLevels, iDicMaxLevel );
    }
    // checking levels <end>                            // victor Mar 02, 00

    // Level 0 - only one for 1st Flow??? one for each Flow???
    GROUPT*     pGroupTRoot = AddGroupTForLevelZero( iMaxLevel );

    ASSERT( pGroupTRoot != NULL );

    if( pGroupTRoot == NULL )
        return false;

    pCurrentFlow->SetGroupTRoot( pGroupTRoot );

    m_iGlobalFlowOrder = 1; // flow order: will be 1+ for remaining elements

    // collect levels for each FormFile (TODO: decide of Input and Output???)
    for( iLevel = 0; iLevel < iNumLevels; iLevel++ ) {

        CDELevel* pLevel = SearchLevelInFormFile( pFormFile, iLevel );
        if( pLevel != NULL ) {

            if( !AddGroupTForLevel( pFormFile, pLevel, pGroupTRoot ) )
                return false;
        }
    }

    pCurrentFlow->BuildIdCheckPoints();                  // victor Feb 21, 00

    return true;
}

GROUPT* CEngineDriver::AddGroupTForLevelZero( int iMaxLevel ) {
    FLOW*   pFlow    = GetFlowInProcess();
    int     iSymFlow = pFlow->GetSymbolIndex();
    CString csLevelName;

    // "LEVEL_0" WAS a reserved name, now from ApplName // victor Apr 04, 00
    CString csLevelZeroName = m_pEngineSettings->GetLevelZeroName();
    if( pFlow->IsPrimary() )
        csLevelName = csLevelZeroName;
    else
        csLevelName.Format( _T("__%s_%s"), NPT(iSymFlow)->GetName().c_str(), csLevelZeroName.GetString() );

    auto pGroupTLevel = std::make_shared<GROUPT>(CS2WS(csLevelName), this);
    int iSymLevel = m_engineData->AddSymbol(pGroupTLevel);

    pGroupTLevel->SYMTfwd = 0; // SEE W/RHF victor Jan 09, 00
    pGroupTLevel->SetFlow( GetFlowInProcess() );
    pGroupTLevel->SetSubType( GetFlowInProcess()->GetSubType() ); // RHF Dec 30, 2003
    // source of groups' structure is either 1: FormFile; 2: Dicts
    if( !pFlow->IsExternal() )                          // victor Jul 25, 01
        pGroupTLevel->SetSource( GROUPT::Source::FrmFile );
    else
        pGroupTLevel->SetSource( GROUPT::Source::DcfFile );

    pGroupTLevel->SetGroupType( GROUPT::Level );
    pGroupTLevel->SetLevel( 0 );
    pGroupTLevel->SYMTowner = iSymFlow;// Level owner is the Flow itself
    pGroupTLevel->SetNumItems( iMaxLevel );
    pGroupTLevel->SetMinOccs( 0 );
    pGroupTLevel->SetMaxOccs( 1 );
    pGroupTLevel->SetCDEGroup( NULL );  // no linked IMSA Group

#ifdef _DEBUG
    TRACE( _T("... AddGroupTForLevelZero %s, level %d: %d Levels\n"), m_pEngineArea->DumpGroupTName(iSymLevel).GetString(), pGroupTLevel->GetLevel(), iMaxLevel );
#endif

    return pGroupTLevel.get();
}

bool CEngineDriver::AddGroupTForLevel( CDEFormFile* pFormFile, CDELevel* pLevel, GROUPT* pGroupTOwner )
{
    int     iLevel = pLevel->GetHierarchy();
    int     iNumLevelItems = pLevel->GetNumItems();

    // insert given Level name                          // victor Dec 16, 99
    CString level_name = CString(pLevel->GetName()).MakeUpper();
    auto pGroupTLevel = std::make_shared<GROUPT>(CS2WS(level_name), this);
    int iSymLevel = m_engineData->AddSymbol(pGroupTLevel);

    for( const CString& alias : pFormFile->GetDictionary()->LookupName(level_name)->GetAliases() )
        GetSymbolTable().AddAlias(CS2WS(alias), *pGroupTLevel);

    pGroupTLevel->SYMTfwd = 0; // SEE W/RHF victor Jan 09, 00
    pGroupTLevel->SetFlow( GetFlowInProcess() );
    pGroupTLevel->SetSubType( GetFlowInProcess()->GetSubType() ); // RHF Dec 30, 2003

    // links this level to root-GROUPT internal chain
    if( pGroupTOwner->SYMTfirst > 0 ) {
        GROUPT* pGroupTBwd = GPT(pGroupTOwner->SYMTlast);
        pGroupTBwd->SYMTfwd    = iSymLevel;
    }
    pGroupTLevel->SetSource( GROUPT::Source::FrmFile );
    pGroupTLevel->SetGroupType( GROUPT::Level );
    pGroupTLevel->SetLevel( iLevel + 1 );       // interface is 0-based
    pGroupTLevel->SYMTowner   = pGroupTOwner->GetSymbol();
    pGroupTLevel->SetOwnerGPT();                        // victor Jun 10, 00
    pGroupTLevel->SetNumItems( iNumLevelItems );
    pGroupTLevel->SetMinOccs( 0 );
    pGroupTLevel->SetMaxOccs( pGroupTLevel->GetLevel() == 1 ?    1 :
                              pGroupTLevel->GetLevel() >  1 ? 9999 : 0 );
    pGroupTLevel->SetCDEGroup( pLevel );

#ifdef  _DEBUG
    TRACE( _T("... AddGroupTForLevel %s, level %d: %d Items\n"), m_pEngineArea->DumpGroupTName(iSymLevel).GetString(), pGroupTLevel->GetLevel(), iNumLevelItems );
#endif

    // attach to owner group
    InstallItemIntoGroupT( pGroupTOwner, iSymLevel, iLevel, m_iGlobalFlowOrder );
    m_iGlobalFlowOrder++;

    int     iNumMirror = 0;                             // RHF Feb 23, 2000
    int     iItemAux   = 0;                             // RHF Feb 23, 2000

    // process each item in Level
    for( int iItem = 0; iItem < iNumLevelItems; iItem++ ) {
        CDEItemBase*    pItem = pLevel->GetItem(iItem);

        // RHF INIC Feb 23, 2000
        if( pItem->GetItemType() == CDEFormBase::Field ) {
            CDEField*   pField = (CDEField*) pItem;

            if( pField->IsMirror() ) {
                iNumMirror++;
                continue;
            }
        }
        // RHF END Feb 23, 2000

        if( !AddItemToOwner( pItem, pGroupTLevel.get(), iItemAux ) )
            return false;// RHF Apr 07, 2000

        iItemAux++; // RHF Feb 23, 2000
    }

    pGroupTLevel->SetNumItems( iNumLevelItems - iNumMirror); // RHF Feb 23, 2000

    return true;
}

bool CEngineDriver::AddItemToOwner( CDEItemBase* pItem, GROUPT* pGroupTOwner, int iItemInOwner )
{
    int iSymItem = 0;
    CDEFormBase::eItemType eType = pItem->GetItemType();

    // Groups & Rosters: insert in symbol table (doesn't exist previously)
    if( eType == CDEFormBase::Group || eType == CDEFormBase::Roster )
    {
        auto pGroupT = std::make_shared<GROUPT>(CS2WS(pItem->GetName()), this);
        iSymItem = m_engineData->AddSymbol(pGroupT);

        pItem->SetSymbol(iSymItem);

        pGroupT->SYMTfwd = 0; // SEE W/RHF victor Jan 09, 00
        pGroupT->SetFlow( GetFlowInProcess() );
        pGroupT->SetSubType( GetFlowInProcess()->GetSubType() ); // RHF Dec 30, 2003

        // links this entry to its owner-group' internal chain
        int iSymLastInOwner = pGroupTOwner->SYMTlast;
        if( iSymLastInOwner ) {
            // last-item-in-owner is a group:
            if( m_pEngineArea->IsSymbolTypeGR( iSymLastInOwner ) ) {

                // ... set next symbol of last-item-in-owner: this entry
                GPT(iSymLastInOwner)->SYMTfwd = iSymItem;
            }
        }
    }

    else if( eType == CDEFormBase::Block )
    {
        CDEBlock& form_block = assert_cast<CDEBlock&>(*pItem);
        auto engine_block = std::make_shared<EngineBlock>(form_block, GetSymbolTable());
        iSymItem = m_EngineArea.m_engineData->AddSymbol(engine_block);

        form_block.SetSymbol(iSymItem);

        // links this entry to its owner-group' internal chain
        int iSymLastInOwner = pGroupTOwner->SYMTlast;
        if (iSymLastInOwner) {
            // last-item-in-owner is a group:
            if (m_pEngineArea->IsSymbolTypeGR(iSymLastInOwner)) {

                // ... set next symbol of last-item-in-owner: this entry
                GPT(iSymLastInOwner)->SYMTfwd = iSymItem;
            }
        }
    }

    else if (eType == CDEFormBase::Field)
    {
        CDEField* pField = (CDEField*)pItem;
        iSymItem = pField->GetSymbol();

        /*???*/ if( VPT(iSymItem)->GetOwnerGroup() ) {
        /*???*/     // ATTN: true item already attached - this is a nth occ
        /*???*/
        /*???*/     pGroupTOwner->DecreaseNumItems();
#ifdef  _DEBUG
        TRACE( _T("      ***** %s (%d) already attached to Group %d {iItemInOwner=%d was reduced to %d Items}\n"), pField->GetItemName().GetString(), iSymItem, VPT(iSymItem)->GetOwnerGroup(), iItemInOwner, pGroupTOwner->GetNumItems() );
#endif
        /*???*/     return true; //  RHF Apr 07, 2000 Was false;
        /*???*/ }

        // links this entry to its owner-group' internal chain
        int iSymLastInOwner = pGroupTOwner->SYMTlast;
        if( iSymLastInOwner ) {
            // last-item-in-owner is a group:
            if( m_pEngineArea->IsSymbolTypeGR( iSymLastInOwner ) ) {

                // ... set next symbol of last-item-in-owner: this entry
                GPT(iSymLastInOwner)->SYMTfwd = iSymItem;
            }
        }
    }

    // only for allowed items (Groups, Rosters & Fields)
    if( eType == CDEFormBase::Group || eType == CDEFormBase::Roster || eType == CDEFormBase::Block )
    {
        if( !AttachItemToOwner( pItem, pGroupTOwner, iItemInOwner ) )
            return false;
    }
    else if (eType == CDEFormBase::Field) {  //
        // checking Mult-var against owner group <begin>// victor Feb 25, 00
        VART*   pVarT = VPT(iSymItem);
        bool    bContainerReady = true;

        if( pVarT->GetClass() != CL_SING ) {
            CDEGroup*   pOwnerGroup    = pGroupTOwner->GetCDEGroup();
            CDEFormBase::eItemType   eOwnerType     = pOwnerGroup->GetItemType();
            int         iOwnerNumItems = pOwnerGroup->GetNumItems();
            int         iOwnerMaxOccs  = pGroupTOwner->GetMaxOccs();
            const CDictItem* pMultItem = pVarT->GetDictItem();
            int         iMultVarOccs   = pMultItem->GetOccurs();

            // TODO: get a version of GetNumItems returning only the number of real-vars-in-flow,
            // TODO: ... currently include mirror-fields, which aren't real-vars-in-flow!!!
            //SAVY&& 29 Aug 2000
            bContainerReady = (
                                ( eOwnerType == CDEFormBase::Roster || eOwnerType == CDEFormBase::Group ) &&
                                iOwnerNumItems == 1  &&
                                iOwnerMaxOccs == iMultVarOccs
                              );
        }

        if( bContainerReady ) {
            // checking Mult-var against owner group <end>  // victor Feb 25, 00
            if( !AttachItemToOwner( pItem, pGroupTOwner, iItemInOwner ) )
                return false;

            if( !SetOwnerIntoItem( iSymItem, pGroupTOwner->GetSymbol() ) )
                return false;
        }
        else {
            GROUPT::Source eGroupSource = GROUPT::Source::FrmFile;
            int     iSymOwner = pGroupTOwner->GetSymbol();      // victor Jan 28, 00

            iSymItem = AddGroupTForMultVar( iSymItem, eGroupSource, iSymOwner ); // new approach // victor Jan 28, 00

            if( iSymItem <= 0 )
                return false;           // unable to add group for Mult var

            InstallItemIntoGroupT( pGroupTOwner, iSymItem, iItemInOwner, -1 );
            m_iGlobalFlowOrder++;
        }
    }

    return true;
}

bool CEngineDriver::AttachItemToOwner(CDEItemBase* pItem, GROUPT* pGroupTOwner, int iItemInOwner)
{
    CDEFormBase::eItemType eType = pItem->GetItemType();
    int iSymItem = pItem->GetSymbol();
    ASSERT(iSymItem > 0);

    // links item to its owner group
    InstallItemIntoGroupT(pGroupTOwner, iSymItem, iItemInOwner, m_iGlobalFlowOrder);
    m_iGlobalFlowOrder++;

    if( eType == CDEFormBase::Group || eType == CDEFormBase::Roster )
        return AddGroupTForGroup((CDEGroup*)pItem, pGroupTOwner);

    return true;
}

bool CEngineDriver::SetOwnerIntoItem( int iSymItem, int iSymGroup ) {
    ASSERT( iSymItem  );
    ASSERT( iSymGroup );
    int     iSymOwner = VPT(iSymItem)->GetOwnerGroup();

    if( iSymOwner ) {
        CString csOwnerText;

        if( iSymOwner > 0 )
            csOwnerText.Format(_T("Group %s"), NPT(iSymOwner)->GetName().c_str());
        else
            csOwnerText = _T("a Record");

        CString csMsgText;
        csMsgText.Format(_T("Cannot set Group %s as owner of Item %s (already owned by %s)"),
                         NPT(iSymGroup)->GetName().c_str(), NPT(iSymItem)->GetName().c_str(), csOwnerText.GetString() );
        issaerror( MessageType::Abort, MGF::OpenMessage, csMsgText.GetString() );

        return false;
    }

    // successful installation
    VPT(iSymItem)->SetOwnerGroup( iSymGroup );

    return true;
}

bool CEngineDriver::AddGroupTForGroup( CDEGroup* pGroup, GROUPT* pGroupTOwner )
{
    ASSERT(pGroupTOwner != NULL);
    int iNumGroupItems = pGroup->GetNumItems();

    GROUPT* pGroupT = GPT(pGroup->GetSymbol());
    pGroupT->SetFlow( GetFlowInProcess() );
    pGroupT->SetSource( GROUPT::Source::FrmFile );
    pGroupT->SetGroupType( GROUPT::GroupOrRecord );
    pGroupT->SetLevel( pGroupTOwner->GetLevel() );
    pGroupT->SYMTowner   = pGroupTOwner->GetSymbol();
    pGroupT->SetOwnerGPT();                             // victor Jun 10, 00
    pGroupT->SetNumItems( iNumGroupItems );
    pGroupT->SetMinOccs( pGroup->GetRequired() ? 1 : 0 );
    pGroupT->SetMaxOccs( pGroup->GetMaxLoopOccs() ); // physical limit
    pGroupT->SetCDEGroup( pGroup );

#ifdef _DEBUG
    TRACE( _T("\n... AddGroupTForGroup %s: %d Items\n"), pGroup->GetName().GetString(), pGroupT->GetNumItems() );
#endif

    int     iNumMirror = 0;                             // RHF Feb 23, 2000
    int     iItemAux   = 0;                             // RHF Feb 23, 2000

    // process each item in Group
    for( int iItem = 0; iItem < iNumGroupItems; iItem++ ) {
        CDEItemBase*    pItem = pGroup->GetItem(iItem);

        // RHF INIC Feb 23, 2000
        if( pItem->GetItemType() == CDEFormBase::Field ) {
            CDEField*   pField = (CDEField*) pItem;

            if( pField->IsMirror() ) {
                iNumMirror++;
                continue;
            }

            if( !pField->GetDictItem()->AddToTreeFor80() )
            {
                continue; // BINARY_TYPES_TO_ENGINE_TODO skipping loading non-VART objects for now
                // BINARY_TYPES_TO_ENGINE_TODO ignore above and continue, adding the item as a VART
            }
        }
        // RHF END Feb 23, 2000

        if( !AddItemToOwner( pItem, pGroupT, iItemAux ) )
            return false; // RHF Apr 07, 2000
        iItemAux++;                                     // RHF Feb 23, 2000
    }

    pGroupT->SetNumItems( iNumGroupItems - iNumMirror); // RHF Feb 23, 2000

    return true;
}

CDELevel* CEngineDriver::SearchLevelInFormFile( CDEFormFile* pFormFile, int iLevelNum ) {
    CDELevel*   pLevelFound = NULL;
    int         iNumLevels = pFormFile->GetNumLevels();

    // looks for searched level into all available levels
    for( int iLevel = 0; iLevel < iNumLevels && pLevelFound == NULL; iLevel++ ) {
        CDELevel*   pLevel = pFormFile->GetLevel(iLevel);

        if( pLevel->GetHierarchy() == iLevelNum )
            pLevelFound = pLevel;
    }

    return  pLevelFound;
}

//---------- building GROUPT chain for remaining items in Dict --------------
bool CEngineDriver::AddGroupTForFlowDics() {
    int     iNumberOfDics = GetFlowInProcess()->GetNumberOfDics();

    for( int iNumDic = 0; iNumDic < iNumberOfDics; iNumDic++ ) {
        int     iSymDic = GetFlowInProcess()->GetSymDicAt( iNumDic );
        ASSERT( iSymDic > 0 );
        DICT*   pDicT = DPT(iSymDic);

        // for each section in this dict
        int         iSymSec = pDicT->SYMTfsec;
        while( iSymSec > 0 ) {

            if( !AddGroupTForOneSec( iSymSec ) )
                return false;

            iSymSec = SPT(iSymSec)->SYMTfwd;
        }
    }

    return true;
}

bool CEngineDriver::AddGroupTForOneSec( int iSymSec ) {
    bool        bWithForm    = false;   // to collect no-form Mult items only
    GROUPT::Source eGroupSource = GROUPT::Source::DcfFile;
    SECT*       pSecT = SPT(iSymSec);
    bool        bOutput=(pSecT->GetSubType() == SymbolSubType::Output); // RHF Aug 23, 2002
    int         iSymVar;
    int         iItem = 0;
#ifdef _DEBUG
    TRACE( _T("\n... AddGroupTForOneSec %s: first Var %s\n"), m_pEngineArea->DumpGroupTName(iSymSec).GetString(),
                                                              m_pEngineArea->DumpGroupTName(pSecT->SYMTfvar).GetString() );
#endif

    // counts no-form vars in this section
    int         iNumItems = 0;
    bool        bAllMatching = true;    // all items match 'bWithForm' // victor Jun 15, 00


    iSymVar = pSecT->SYMTfvar;          // first Var in section

    while( iSymVar > 0 ) {
        VART*   pVarT = VPT(iSymVar);

        if( pVarT->GetClass() == CL_SING ) {

            if( pVarT->IsInAForm() == bWithForm )
                iNumItems += 1;
            else
                bAllMatching = false;   // this item doesn't match 'bWithForm' // victor Jun 15, 00
        }
        else {
            // count items & subitems without form in Mult-set
            int     iNumItemsInSet = NumItemsInMultSet( iSymVar, bWithForm, &bAllMatching ); // 'bAllMatching' added // victor Jun 15, 00

            if( iNumItemsInSet )        // will add 1 member for the Mult set
                iNumItems += 1;
        }

        // advance to next item to be scanned
        iSymVar = ( pVarT->GetClass() == CL_SING ) ? pVarT->SYMTfwd : PassOverMultSet( iSymVar );
    }

    // couldn't find no-form elements - no further proccess needed
    if( !iNumItems )
        return true;

    // add a GroupT for this section (type GroupOrRecord)
    CString csNewName;

    // naming depending on "all items no-form"  <begin> // victor Jun 15, 00
    if( bAllMatching ) {                // ... all are no-form: same name as the section
        // RHF INIC Mar 26, 2004
        if( bOutput )
            csNewName.Format( _T("%s_ORD"), NPT(iSymSec)->GetName().c_str() );
        else
            csNewName.Format( _T("%s"), NPT(iSymSec)->GetName().c_str() );
        // RHF END Mar 26, 2004
    }
    else                                // ... some with form: prefix plus section' name
    {
        csNewName.Format( _T("__%s"), NPT(iSymSec)->GetName().c_str() );
    }
    // naming depending to "all items no-form"  <end>   // victor Jun 15, 00

    if( bAllMatching ) // RHF Nov 15, 2002
        pSecT->SetOccGenerator(true); // RHF Nov 15, 2002

    auto pGroupT = std::make_shared<GROUPT>(CS2WS(csNewName), this);
    int iSymGroup = m_engineData->AddSymbol(pGroupT);

    pGroupT->SetSubType( GetFlowInProcess()->GetSubType() ); // RHF Dec 30, 2003

    pGroupT->SetSource( eGroupSource );
    pGroupT->SetGroupType( GROUPT::GroupOrRecord );
    pGroupT->SetLevel( pSecT->GetLevel() );
    // don't 'SetOwnerGPT()' - the owner is a Section!  // victor Jun 10, 00
    pGroupT->SetMinOccs( pSecT->GetMinOccs() );
    pGroupT->SetMaxOccs( pSecT->GetMaxOccs() );
    pGroupT->SetCDEGroup( NULL );       // no linked IMSA Group

    //////////////////////////////////////////////////////////////////////////
    // Now creating a fake CDEGroup to prevent erroneous behaviour in
    // SetDimAndParentGPT() later

    CDEGroup*   pCDEGroup=new CDEGroup();
    pCDEGroup->SetRIType(CDEFormBase::Record );
    pCDEGroup->SetMaxLoopOccs( pGroupT->GetMaxOccs() );

    pGroupT->SetCanDelete(true);

    pGroupT->SetCDEGroup( pCDEGroup );
    ASSERT( pGroupT->GetCDEGroup() != NULL );
    //////////////////////////////////////////////////////////////////////////

    // initialize occurrences to max-occs since this is a Record' group...
    // (no matter if belonging to a Work or any other Dict)
    // RHF INIC Aug 23, 2002 FIX OUTPUTCASE problem. Sections have been written with maxoccs!!
    if( bOutput || bAllMatching && Issamod == ModuleType::Entry ) { // RHF Sep 28, 2003 Add  bAllMatching && Issamod == ModuleType::Entry
        pGroupT->SetCurrentOccurrences(0);
        pGroupT->SetTotalOccurrences(0);
        pGroupT->SetDataOccurrences(0);
    }
    // RHF END Aug 23, 2002
    else {
        pGroupT->SetCurrentOccurrences( pGroupT->GetMaxOccs() );
        pGroupT->SetTotalOccurrences( pGroupT->GetMaxOccs() );
    }

    // create list of items for this section
    pGroupT->SetNumItems( iNumItems );
    if( pGroupT->GetNumItems() != iNumItems )
        return false;

    // looks for each no-form var
    int     iSymItem;

    iSymVar = pSecT->SYMTfvar;          // first Var in section
    iItem = 0;
    while( iSymVar > 0 ) {
        VART*   pVarT = VPT(iSymVar);

        iSymItem = 0;                   // 0: not selected for this group
        if( pVarT->GetClass() == CL_SING ) {
            // Sing Var: attach to Group
            if( pVarT->IsInAForm() == bWithForm ) {

                // GroupT is the of this var
                pVarT->SetOwnerGroup( iSymGroup );

                iSymItem = iSymVar;
            }
        }
        else if( NumItemsInMultSet( iSymVar, bWithForm ) ) {
            // Mult Var: generate a container group (type MultItem)
            iSymItem = AddGroupTForMultVar( iSymVar, eGroupSource, iSymGroup );

            if( !iSymItem )
                return false; // unable to add group for Mult var
        }

        // if selected, add to items-list of owner group
        if( iSymItem ) {
            InstallItemIntoGroupT( pGroupT.get(), iSymItem, iItem, -1 );
            iItem++;
        }

        // if needed, set the "occs-generator" condition// victor Oct 27, 00
        if( bAllMatching ) {                             // victor Oct 27, 00
            pVarT->SetOccGenerator( true );             // victor Oct 27, 00

            // RHF INIC Apr 24, 2002 Fix problem the Seccion single con 1 item multiple con subitems que no estan en form
            // El count retornaba 0 sin este arreglo!!!
            VART* pVartSubItem=pVarT;
            while( (pVartSubItem=pVartSubItem->GetNextSubItem() ) != NULL )
                pVartSubItem->SetOccGenerator( true );
            // RHF END Apr 24, 2002
        }

        // advance to next item to be scanned
        iSymVar = ( pVarT->GetClass() == CL_SING ) ? pVarT->SYMTfwd : PassOverMultSet( iSymVar );
    }

    return true;
}

int CEngineDriver::AddGroupTForMultVar( int iSymMultVar, GROUPT::Source eGroupSource, int iSymOwner ) {
    bool    bWithForm = ( eGroupSource == GROUPT::Source::FrmFile );
    int     iGroupType = GROUPT::MultItem;

    CString csNewName;
    csNewName.Format( _T("__%s"), NPT(iSymMultVar)->GetName().c_str() );

    auto pGroupT = std::make_shared<GROUPT>(CS2WS(csNewName), this);
    int iSymGroup = m_engineData->AddSymbol(pGroupT);

    pGroupT->SYMTfwd = 0;
    pGroupT->SetFlow( GetFlowInProcess() );
    pGroupT->SetSubType( GetFlowInProcess()->GetSubType() ); // RHF Dec 30, 2003

    pGroupT->SetGroupType( iGroupType );
    pGroupT->SYMTowner = iSymOwner;
    pGroupT->SetOwnerGPT();                             // victor Jun 10, 00

    // create list of items for this Mult Var container
    int     iNumItems = NumItemsInMultSet( iSymMultVar, bWithForm );

    pGroupT->SetNumItems( iNumItems );
    if( pGroupT->GetNumItems() != iNumItems )
        return 0;

    // create an IMSA group (for internal consistency)  // RHF+VC Sep 02, 99
    // RHF COM Feb 08, 2001pGroupT->SetCDEGroup( new CDEGroup() );             // RHF+VC Sep 02, 99

    // RHF INIC Feb 08, 2001
    CDEGroup*   pCDEGroup=new CDEGroup();
    ASSERT( NPT(iSymMultVar)->IsA(SymbolType::Variable) );
    VART*   pVart = VPT(iSymMultVar);
    if( pVart->GetOwnerSymItem() == 0 ) // items has OwnerSymItems == 0
        pCDEGroup->SetRIType(CDEFormBase::Item );
    else
        pCDEGroup->SetRIType(CDEFormBase::SubItem );
    pCDEGroup->SetTypeName( WS2CS(pVart->GetName()) );

    pGroupT->SetCDEGroup( pCDEGroup );
    ASSERT( pGroupT->GetCDEGroup() != NULL );

    pGroupT->SetCanDelete(true);

    VART*       pVarT=VPT(iSymMultVar);

    bool    bIsSubItem     = ( pVarT->GetOwnerSymItem() > 0 );
    pCDEGroup->SetRIType( bIsSubItem ? CDEFormBase::SubItem : CDEFormBase::Item );

    // RHF END Feb 08, 2001

    // setup other features of this GroupT slot
    pGroupT->SetSource( eGroupSource );
    pGroupT->SetLevel( GPT(iSymOwner)->GetLevel() );
    pGroupT->SetMinOccs( 0 );
    pGroupT->SetMaxOccs( ( pVarT->GetDictItem() )->GetOccurs() );
    pGroupT->SetCurrentOccurrences( pGroupT->GetMaxOccs() );
    pGroupT->SetTotalOccurrences( pGroupT->GetMaxOccs() );    //??? what???

#ifdef _DEBUG
    TRACE( _T("\n\n... AddGroupTForMultVar %s, Group %s: %d Items {GroupSource=%d, GroupType=%d, owner=%s}\n"),
        m_pEngineArea->DumpGroupTName(iSymMultVar).GetString(), m_pEngineArea->DumpGroupTName(iSymGroup).GetString(), pGroupT->GetNumItems(), pGroupT->GetSource(), pGroupT->GetGroupType(),
        m_pEngineArea->DumpGroupTName(iSymOwner).GetString() );
#endif

    // fill-up entries of the list of items: 1st, the mult-var itself...
    int     iItem = 0;
    int     iSymItem = iSymMultVar;
    int     iFlowOrder;
    if( VPT(iSymItem)->IsInAForm() == bWithForm ) {

        iFlowOrder = ( bWithForm ) ? m_iGlobalFlowOrder++ : -1;

        InstallItemIntoGroupT( pGroupT.get(), iSymItem, iItem, iFlowOrder );
#ifdef _DEBUG
        TRACE( _T("...... InstallItemIntoGroup (first): Item %s into Group %s\n"),
            m_pEngineArea->DumpGroupTName(iSymItem).GetString(), m_pEngineArea->DumpGroupTName(iSymGroup).GetString() );
#endif

        VPT(iSymItem)->SetOwnerGroup( iSymGroup );

        iItem++;
    }

    // ... then, the remaining members
    iSymItem = VPT(iSymItem)->SYMTfwd;
    while( iSymItem > 0 &&
        NPT(iSymItem)->GetType() == SymbolType::Variable &&
        VPT(iSymItem)->GetOwnerSymItem() == iSymMultVar ) {

        if( VPT(iSymItem)->IsInAForm() == bWithForm ) {

            iFlowOrder = ( bWithForm ) ? m_iGlobalFlowOrder++ : -1;

            InstallItemIntoGroupT( pGroupT.get(), iSymItem, iItem, iFlowOrder );
#ifdef _DEBUG
            TRACE( _T("...... InstallItemIntoGroup (subitem): Item %s into Group %s\n"),
                m_pEngineArea->DumpGroupTName(iSymItem).GetString(), m_pEngineArea->DumpGroupTName(iSymGroup).GetString() );
#endif

            // this GroupT is a GIP owner of this var
            VPT(iSymItem)->SetOwnerGroup( iSymGroup );

            iItem++;
        }

        iSymItem = VPT(iSymItem)->SYMTfwd;
    }

    return iSymGroup;                   // new approach // victor Jan 28, 00
}

void CEngineDriver::InstallItemIntoGroupT( GROUPT* pGroupT, int iSymItem, int iItem, int iFlowOrder ) {
    bool    bDone = pGroupT->SetItemDescription( iItem, iSymItem, iFlowOrder );
    ASSERT( bDone );

    if( !pGroupT->SYMTfirst )
        pGroupT->SYMTfirst = iSymItem;
    pGroupT->SYMTlast = iSymItem;
}

// a couple of definitions to make code below more readable
#define IS_ITEM(x)      (VPT(x)->GetOwnerVarT() == NULL)
#define HAS_SUBITEMS(x) (VPT(x)->GetNextSubItem() != NULL)

int CEngineDriver::NumItemsInMultSet( int iSymMultVar, bool bWithForm, bool* pAllMatching ) {
    int     iSymItem = iSymMultVar;
    bool isMulVarMatching = (VPT(iSymItem)->IsInAForm() == bWithForm);
    int     iNumItems = isMulVarMatching ? 1 : 0;

    // RHF+rcl INIC Jun 14, 2004
    // Fix bug: a record (not required) having 2 items (one single and other multiple)
    // and the multiple item is in the form but the other item is not on form
    // --> The record is not written out!!
    if (!isMulVarMatching)
        if( pAllMatching != NULL )
            *pAllMatching = false;

    iSymItem = VPT(iSymItem)->SYMTfwd;
    while( iSymItem > 0 &&
        NPT(iSymItem)->GetType() == SymbolType::Variable &&
        VPT(iSymItem)->GetOwnerSymItem() == iSymMultVar ) {

        if( VPT(iSymItem)->IsInAForm() == bWithForm )
            iNumItems += 1;
        else if( pAllMatching != NULL )                 // victor Jun 15, 00
            *pAllMatching = false;                      // victor Jun 15, 00

        iSymItem = VPT(iSymItem)->SYMTfwd;
    }

    return iNumItems;
}

int CEngineDriver::PassOverMultSet( int iSymMultVar ) {
    int     iSymItem = iSymMultVar;

    iSymItem = VPT(iSymItem)->SYMTfwd;
    while( iSymItem > 0 &&
        NPT(iSymItem)->GetType() == SymbolType::Variable &&
        VPT(iSymItem)->GetOwnerSymItem() == iSymMultVar ) {

        iSymItem = VPT(iSymItem)->SYMTfwd;
    }

    return iSymItem;
}

// RHF Init Dec 17, 2003 BUCEN_2003 Changes
int CEngineDriver::GenerateAbsoluteFlowOrder( CFlowCore*  pFlowCore ) {
    int                     iInitialFlowOrder=m_iAbsoluteFlowOrder;

    pFlowCore->FlowStripRestart();
    CFlowAtom*              pAtom = ( pFlowCore->FlowStripGetSize() > 0 ) ? pFlowCore->FlowStripCurr() : NULL;
    CFlowAtom::AtomType     xAtomType;

    while( pAtom != NULL ) {
        xAtomType = pAtom->GetAtomType();

        switch( xAtomType )
        {
        case CFlowAtom::AtomType::BeforeStrip:// before the actual strip
            break;

        case CFlowAtom::AtomType::GroupHead:  // head of Group
            {
                CFlowGroupHead* pGroupHead  = (CFlowGroupHead*) pAtom;
                int             iSymbol     = pGroupHead->GetSymbol();
                GROUPT* pGroupT = GPT(iSymbol);

                pGroupT->SetAbsoluteFlowOrder( m_iAbsoluteFlowOrder );
                m_iAbsoluteFlowOrder++;
            }

            break;

        case CFlowAtom::AtomType::GroupTail:  // tail of Group
            break;

        case CFlowAtom::AtomType::HTOcc:      // Head/Tail of occurrence
            break;

        case CFlowAtom::AtomType::BlockHead:  // blocks
        case CFlowAtom::AtomType::BlockTail:
            break;

        case CFlowAtom::AtomType::Item:       // item
            {
                CFlowItem*  pFlowItem   = (CFlowItem*) pAtom;
                int         iSymbol     = pFlowItem->GetSymbol();
                VART*       pVarT = VPT(iSymbol);

                pVarT->SetAbsoluteFlowOrder( m_iAbsoluteFlowOrder );
                m_iAbsoluteFlowOrder++;
            }
            break;

        case CFlowAtom::AtomType::BeyondStrip:// beyond the actual strip
            break;

        default:
            ASSERT( 0 );            // unknown atom!!!
            break;
        }

        // get next atom
        pAtom = ( xAtomType != CFlowAtom::AtomType::BeyondStrip ) ? pFlowCore->FlowStripNext() : NULL;
    }

    pFlowCore->FlowStripRestart();

    return m_iAbsoluteFlowOrder-iInitialFlowOrder;
}
// RHF END Dec 17, 2003 BUCEN_2003 Changes
