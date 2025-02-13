//---------------------------------------------------------------------------
//  File name: DeWrite.cpp
//
//  Description:
//          Implementation of write-nodes' support for the Entry' driver
//
//  History:    Date       Author   Comment
//              ---------------------------
//              .. ... ..   ..      many, many prehistoric tailoring
//              30 May 01   vc      Adding methods to fit CsDriver behavior
//              25 Jul 01   vc      Reestructuring deWrite:
//                                  ... moved from cexEntry to zEntryO
//                                  ... now concentrating pRunCase & full-case-in-memory management
//                                  ... other methods not related to pRunCase were moved to new file EntDrv.cpp of zEntryO
//
//---------------------------------------------------------------------------
#include "STDAFX.H"
#include <zPlatformO/PlatformInterface.h>
#include <zToolsO/Tools.h>
#include <zMessageO/Messages.h>
#include <zDictO/DDClass.h>
#include <ZBRIDGEO/npff.h>
#include <zCaseO/Case.h>
#include <zCaseO/CaseItemReference.h>
#include <zDataO/DataRepository.h>
#include <engine/EXENTRY.H>
#include <engine/Engine.h>
#include <engine/Dicx.h>
#include <Zissalib/CsDriver.h>


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[]= __FILE__;
#endif


///////////////////////////////////////////////////////////////////////////////
//
// --> full-case in memory
//
///////////////////////////////////////////////////////////////////////////////

void CEntryDriver::TakeCareOfEndingNode( int iEndingLevel, bool bIgnoreWrite ) { // victor Jun 07, 01
    // TakeCareOfEndingNode: only when the ending level is NOT THE LEVEL-0
    // ... taken from DeFuncs/deendlevel, reproduces lots of stuff formerly down there
    // ... specially tailored to fit CsDriver behavior

    //~old comments from deendlevel
    //~behavior: the sequential execution of the program strip must be broken
    //~          and the next-prog must be set to Post of either the parent
    //~          of source level, or to the same level, depending on the
    //~          nature of the source proc being executed:
    //~
    //~      ... when arised in a Pre-level or Post-level, set the Post of parent level
    //~
    //~      ... otherwise, set the Post of the same level

    if( iEndingLevel > 1 )
    {
        if( m_aLoadedLevels[iEndingLevel - 1].m_isNew ) // if this was a new node, remove it
        {
            Pre74_CaseLevel* pParentLevel = m_aLoadedLevels[iEndingLevel - 2].m_caseLevel;
            Pre74_CaseLevel* pLevelToUpdate = m_aLoadedLevels[iEndingLevel - 1].m_caseLevel;
            pParentLevel->RemoveChildLevel(pLevelToUpdate);
        }

        else // otherwise update it
            m_pEntryDriver->UpdateLevelNodeAtEnd(iEndingLevel);
    }
}

bool CEntryDriver::CheckBeforeWriteEndingNode( int iSymDic, int iMaxLevel, int iEndingLevel, bool bIgnoreWrite ) { // victor May 30, 01
    // CheckBeforeWriteEndingNode: final controls before write a node
    // ... taken from wexEntry/execute_EndOfLevel, at the middle of the function
    // ... specially tailored to fit CsDriver behavior
    bool        bIsReady = true;        // false... will generate a Reenter' request
    DICT*       pDicT    = DPT(iSymDic);
    CString     csNodeText;             // identification of this level-node

    if( iEndingLevel > 0 ) {
        const TCHAR*     pszNodeType = ( iEndingLevel <= 1 ) ? _T("case") : _T("node");
        const CDataDict* pDataDict   = pDicT->GetDataDict();
        const DictLevel& dict_level  = pDataDict->GetLevel( iEndingLevel - 1 );
        csNodeText.Format( _T("'%s/%s' %s"), dict_level.GetName().GetString(), dict_level.GetLabel().GetString(), pszNodeType );
    }

    if( !bIgnoreWrite ) {
        // checking whether id-fields are actually ready
        if( iEndingLevel >= 1 ) {
            if( !QidReady( iEndingLevel ) ) {
                issaerror( MessageType::Warning, 1026 ); // Q-Id of level not prepared
            }
        }

        // checkpoint for verifying id-fields (only when ending Level-1)
        if( iEndingLevel == 1 ) {
            bool    bColliding = CheckIdCollision( iSymDic );

            if( bColliding )
                bIsReady = false;
        }

        // confirmation to write this node (only when ending maximum Level)
        if( bIsReady && iEndingLevel >= iMaxLevel ) {

            // 20140814 don't display all the level information unless there are multiple levels
            const std::wstring prompt_message = ( iMaxLevel == 1 ) ? MGF::GetMessageText(89254) :
                                                                     FormatTextCS2WS(MGF::GetMessageText(89255).c_str(), csNodeText.GetString());

            if( m_pPifFile->GetApplication()->GetShowEndCaseMessage() )
            {
#ifdef WIN_DESKTOP
                if( AfxMessageBox(prompt_message.c_str(), MB_YESNO | MB_APPLMODAL | MB_DEFBUTTON1 ) == IDNO )
#else
                const std::wstring accept_case_title = MGF::GetMessageText(89256, _T("Accept Case"));
                
                if( PlatformInterface::GetInstance()->GetApplicationInterface()->ShowModalDialog(accept_case_title, prompt_message, MB_YESNO) == IDNO )
#endif
                {
                    // RHF INIC Feb 07, 2001
                    if( Decurmode == ADVMODE ) {
                        SetAddModeFlags();                // victor May 30, 01
                    }
                    // RHF END Feb 07, 2001

                    // ... operator' decission: don't write this node
                    bIsReady = false;
                }
            }
        }
    }

    return bIsReady;
}


bool CEntryDriver::WriteEndingNode( int iEndingLevel, bool bIgnoreWrite ) { // victor May 30, 01
    // WriteEndingNode: simplified method to write a node
    // ... taken from wexEntry/execute_EndOfLevel, just at the bottom of the function
    // ... specially tailored to fit CsDriver behavior
    bool    bDone = true;

    if( iEndingLevel < 1 ) {            // ending Level 0:
        m_bMustEndEntrySession = true;
    }
    else {                              // ending Levels 1+:
        Decurmode = ADDMODE;            // to avoid infinite Advance // RHF 19/3/96

        // copy the contents from memory to the casetainer
        m_pEntryDriver->UpdateLevelNode(iEndingLevel);

        if( iEndingLevel == 1 )
        {
            if( !bIgnoreWrite )
                WriteData();

            ResetCaseLevels();
        }

        if( !bIgnoreWrite ) {
            // if coming from keyboard, add to written-sons of parent node
            if( LevCtGetSource( iEndingLevel ) == Keyboard ) {
                LevCtAddWrittenSon( iEndingLevel - 1 );
            }
        }
    }

    return bDone;
}


long CEntryDriver::WriteData()
{
    DICT* pDicT = DIP(0);
    DICX* pDicX = DIX(0);

    // get initial key (is an empty string for new cases) and current key
    TCHAR pszInitialKey[512];
    TCHAR pszCurrentKey[512];

    pDicT->GetPrimaryKey(pszInitialKey);
    pDicT->GetPrimaryKey(pszCurrentKey,true);

    // REPO_TEMP temporarily using the exwritecase code to write out the data
    bool bRet = false;
    long lWriteFilePos = -1;

    Case& data_case = pDicX->GetCase();
    Pre74_Case* pCase = data_case.GetPre74_Case();
    Pre74_CaseLevel* pRootLevel = pCase->GetRootLevel();

    try
    {
        pCase->FinalizeLevel(pRootLevel, true, true, pDicX->GetCase().GetCaseConstructionReporter());
    }

    catch( const CaseHasNoValidRecordsException& )
    {
        pCase->ApplySpecialOutputKey(pCase, pRootLevel, pszCurrentKey);
    }

    m_pEngineDriver->UpdateCaseNotesLevelKeys_pre80(pDicX);

    // update the verified and delete flags
    data_case.SetVerified(IsVerify());
    data_case.SetDeleted(false);

    // clear the partial save status
    data_case.SetPartialSaveStatus(PartialSaveMode::None);

    // write out the case
    try
    {
        pDicX->GetDataRepository().WriteCasetainer(&data_case, GetWriteCaseParameter());

        ClearWriteCaseParameter();

        _tcscpy(pDicX->current_key, data_case.GetKey()); // update the current key
        lWriteFilePos = 0;
    }

    catch( const DataRepositoryException::Error& exception )
    {
        lWriteFilePos = -1;
        issaerror(MessageType::Warning, 10104, exception.GetErrorMessage().c_str());
    }


    bRet = ( lWriteFilePos >= 0 );

    if( !bRet )
        issaerror( MessageType::Abort, 4017, pDicX->GetDataRepository().GetName(DataRepositoryNameType::Full).GetString());

    if( bRet )
    {
#ifdef WIN_DESKTOP
        // send a message to update the stats file
        if( AfxGetMainWnd() != nullptr )
            AfxGetMainWnd()->SendMessage(WM_IMSA_WRITECASE, 0, (LPARAM)&data_case);
#endif
    }

   return lWriteFilePos;
}


bool CEntryDriver::PartialSaveCase(bool bClearSkipped/* = false*/)
{
    DICT* pDicT = DIP(0);
    DICX* pDicX = DIX(0);

    // REPO_TEMP temporarily using the exwritecase code to write out the data
    bool bRet = false;
    bool bKeyChanged = false;

    Case& data_case = pDicX->GetCase();
    Pre74_Case* pCase = data_case.GetPre74_Case();
    Pre74_CaseLevel* pRootLevel = pCase->GetRootLevel();

    pDicT->SetPrimaryKey(true); // refresh the current primary key

    // copy the contents from memory, for each level, to the casetainer
    int iCurrentLevel = m_pEntryDriver->GetActiveLevel();

    for( int i = iCurrentLevel; i >= 1; i-- )
        m_pEntryDriver->UpdateLevelNode(i,!bClearSkipped);

    Pre74_CaseLevel* pCaseLevel = m_aLoadedLevels[iCurrentLevel - 1].m_caseLevel;
    CString csCurrentKey = pCase->GetKey();

    try
    {
        // don't use the warning reporter as required records that haven't been reached yet may be added
        pCase->FinalizeLevel(pRootLevel,true,true,NULL);
    }

    catch( const CaseHasNoValidRecordsException& )
    {
        TCHAR pszCurrentKey[512];
        pDicT->GetPrimaryKey(pszCurrentKey,true);
        pCase->ApplySpecialOutputKey(pCase, pRootLevel, pszCurrentKey);
    }

    m_pEngineDriver->UpdateCaseNotesLevelKeys_pre80(pDicX);

    bKeyChanged = ( csCurrentKey != data_case.GetPre74_Case()->GetKey() );

    // update the verified and delete flags
    data_case.SetVerified(false);
    data_case.SetDeleted(false);

    // update the partial save status
    DEFLD* pField = m_pCsDriver->GetCurDeFld();
    VART* pVarT = nullptr;

    if( pField != nullptr && pField->GetSymbol() > 0 )
        pVarT = VPT(pField->GetSymbol());

    PartialSaveMode partial_save_mode = PartialSaveMode::None;
    std::shared_ptr<CaseItemReference> partial_save_case_item_reference;

    if( pVarT != nullptr )
    {
        switch( GetPartialMode() )
        {
            case ADD_MODE:
                partial_save_mode = PartialSaveMode::Add;
                break;

            case MODIFY_MODE:
                partial_save_mode = PartialSaveMode::Modify;
                break;

            case VERIFY_MODE:
                partial_save_mode = PartialSaveMode::Verify;
                break;

            default:
                ASSERT(0);
        }

        if( partial_save_mode != PartialSaveMode::None )
        {
            partial_save_case_item_reference = std::make_shared<CaseItemReference>(*pVarT->GetCaseItem(), pCase->GetLevelKey(pCaseLevel));
            m_pIntDriver->ConvertIndex(*pField, *partial_save_case_item_reference);
        }
    }

    data_case.SetPartialSaveStatus(partial_save_mode, std::move(partial_save_case_item_reference));

    // write out the case
    try
    {
        pDicX->GetDataRepository().WriteCasetainer(&data_case, GetWriteCaseParameter());

        // update the parameter so that it is refreshed for the next partial or complete save
        SetWriteCaseParameter(WriteCaseParameter::CreateModifyParameter(data_case));

        bRet = true;
    }

    catch( const DataRepositoryException::Error& exception )
    {
        issaerror(MessageType::Warning, 10104, exception.GetErrorMessage().c_str());
    }

    if( bRet && bKeyChanged )
        WindowsDesktopMessage::Send(WM_IMSA_KEY_CHANGED, &data_case);

    pDicT->SetPrimaryKey(false); // after save inital and current conditions are same

    return bRet;
}


void CEntryDriver::PrepareCaseFromEngineForQuestionnaireViewer(DICT* pDicT, Case& data_case)
{
    // this override is only for the main input file
    if( pDicT != DIP(0) )
    {
        CEngineDriver::PrepareCaseFromEngineForQuestionnaireViewer(pDicT, data_case);
        return;
    }

    ASSERT(data_case.m_pre74Case == nullptr);
    Pre74_Case* pCase = data_case.GetPre74_Case();
    Pre74_CaseLevel* pRootLevel = pCase->GetRootLevel();

    // copy the case values that are currently updated in the Case object (e.g., notes)
    DICX* pDicX = pDicT->GetDicX();
    UpdateCaseNotesLevelKeys_pre80(pDicX);
    data_case = pDicX->GetCase();

    // clear the records
    data_case.GetRootCaseLevel().Reset();

    // copy the contents from memory, for each level, to the casetainer
    int iCurrentLevel = m_pEntryDriver->GetActiveLevel();

    for( int i = iCurrentLevel; i >= 1; i-- )
        m_pEntryDriver->UpdateLevelNode(i, true, &data_case);

    try
    {
        // don't use the warning reporter as required records that haven't been reached yet may be added
        pCase->FinalizeLevel(pRootLevel, true, true, nullptr);
    }

    catch( const CaseHasNoValidRecordsException& )
    {
        TCHAR pszCurrentKey[512];
        pDicT->GetPrimaryKey(pszCurrentKey,true);
        pCase->ApplySpecialOutputKey(pCase, pRootLevel, pszCurrentKey);
    }

    data_case.ApplyPre74_Case(pCase);

    // since this case is currently being entered, clear some flags
    data_case.SetVerified(false);
    data_case.SetDeleted(false);
    data_case.SetPartialSaveStatus(PartialSaveMode::None);
}


void CEntryDriver::ResetCaseLevels()
{
    DICX* pDicX = DIX(0);
    pDicX->ResetCaseObjects();
}


bool CEntryDriver::IsAddedNode(int iLevel)
{
    bool    bAddedNode = true;
#ifdef REMOVED // REPO_TEMP need a different way of doing this
    if( m_pRunCase != NULL ) {
        Case2::CCaseLevel*  pLevel;

        if( ( pLevel = m_pRunCase->GetCurrentLevel( iLevel ) ) != NULL )
            bAddedNode = pLevel->IsNew();
    }
#endif
    bAddedNode = !UsingWriteCaseParameter() || GetWriteCaseParameter()->IsInsertParameter();

    return bAddedNode;
}
