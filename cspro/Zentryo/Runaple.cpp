// RunAplE.cpp: implementation of the CRunAplEntry class.
//
//////////////////////////////////////////////////////////////////////
#include "StdAfx.h"
#include "Runaple.h"
#include <zPlatformO/PlatformInterface.h>
#include <zUtilO/AppLdr.h>
#include <zUtilF/ChoiceDlg.h>
#include <zUtilF/TextInputDlg.h>
#include <zAppO/Properties/ApplicationProperties.h>
#include <ZBRIDGEO/npff.h>
#include <zCaseO/Case.h>
#include <zCaseO/CaseItemReference.h>
#include <zParadataO/Logger.h>
#include <zEngineO/FileApplicationLoader.h>
#include <zEngineO/PenReaderApplicationLoader.h>
#include <zEngineO/PenWriterApplicationLoader.h>
#include <zEngineO/ResponseProcessor.h>
#include <zEngineO/ValueSet.h>
#include <zSyncO/AppSyncParamRunner.h>
#include <zSyncO/IDropboxAuthDialog.h>
#include <zSyncO/ILoginDialog.h>
#include <zSyncO/SyncCredentialStore.h>

#ifdef WIN_DESKTOP
#include <zSyncF/LoginDialog.h>
#include <zSyncF/DropboxAuthDialog.h>
#else
class CLoginDialog : public ILoginDialog
{
    std::optional<std::tuple<CString, CString>> Show(const CString& server, bool show_invalid_error) override
    {
        CString username, password;
        std::tuple<CString, CString*, CString*> message_parameters = std::make_tuple(
            server,
            &username,
            &password);
        auto credentials = PlatformInterface::GetInstance()->GetApplicationInterface()->ShowLoginDialog(server, show_invalid_error);
        if (credentials)
            return std::make_optional(std::make_tuple(credentials->username, credentials->password));
        else
            return {};
    }
};
class DropboxAuthDialog : public IDropboxAuthDialog
{

    CString Show(CString csClientId) override
    {
        return PlatformInterface::GetInstance()->GetApplicationInterface()->AuthorizeDropbox(csClientId);
    }

};
#endif

#include <Cexentry/Entifaz.h>
#include <engine/DEFLD.H>
#include <engine/IntDrive.h>
#include <engine/ParadataDriver.h>

#ifdef WIN_DESKTOP
#include <zUtilO/TraceMsg.h>
#endif

#include <zEngineO/Userbar.h>


#if defined(_DEBUG) && defined(WIN_DESKTOP)
#undef THIS_FILE
static char THIS_FILE[]= __FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CRunAplEntry::CRunAplEntry(CNPifFile* pPifFile)
{
    m_pPifFile = pPifFile;
    m_pEntryIFaz = new CEntryIFaz;

    Init();
}

CNPifFile*  CRunAplEntry::GetPifFile(){
    return m_pPifFile;
}

CRunAplEntry::~CRunAplEntry()
{
    if( GetAppMode() == CRUNAPL_MODIFY || GetAppMode() == CRUNAPL_VERIFY )
        ModifyStop();

    SAFE_DELETE(m_pEntryIFaz);

    Stop();
    End( false );
}

void CRunAplEntry::Init()
{
    m_bFlagModifyStart = false;

    m_pCurField = NULL;
    m_pCurEngineField = NULL;

    m_bSetEndingModify = false; // 20110111

    m_bForward = true;
}

// Compile. true if OK
bool CRunAplEntry::LoadCompile()
{
    Application* pApplication = m_pPifFile->GetApplication();

    if( pApplication->GetAppLoader()->GetBinaryFileLoad() )
        pApplication->SetApplicationLoader(std::make_unique<PenReaderApplicationLoader>(pApplication, std::wstring()));

    else
    {
#ifdef WIN_DESKTOP
        if( BinaryGen::isGeneratingBinary() )
            pApplication->SetApplicationLoader(std::make_unique<PenWriterApplicationLoader>(pApplication, std::wstring()));

        else
            pApplication->SetApplicationLoader(std::make_unique<FileApplicationLoader>(pApplication));
#else
        throw ProgrammingErrorException();
#endif
    }


    bool bRet = CRunApl::LoadCompile();

    if( bRet ) {
        CString csParam;
        int     iDummyExecMode;

        // Call to CSPro Engine exapplinit (applload, attrload, compall..)
        // Errors are displayed trough issaerror function
        if(m_pPifFile->GetApplication()->IsCompiled() ) {
           if( !m_pEntryIFaz->C_ExentryInit1( m_pPifFile, &iDummyExecMode ) )
               bRet = false;
        }
        else if( !m_pEntryIFaz->C_ExentryInit( m_pPifFile, &iDummyExecMode ) )
            bRet = false;
    }

    return( bRet );
}

// Inform to engine that application was finished.
bool CRunAplEntry::End( const bool bCanExit )
{
    bool  bRet = CRunApl::End();

    if( bRet && m_pEntryIFaz )
        m_pEntryIFaz->C_ExentryEnd( (int) bCanExit );

    return( bRet );
}

// Let's get ready to run.
bool CRunAplEntry::Start( const int iMode )
{
    Init();

    if( !CRunApl::Start() )
        return false;

    bool bRet = false;

    if( iMode == CRUNAPL_MODIFY )
        bRet = m_pEntryIFaz->C_ExentryStart( CRUNAPL_MODIFY ); // Call to the Engine

    else if( iMode == CRUNAPL_ADD )
        bRet = m_pEntryIFaz->C_ExentryStart( CRUNAPL_ADD ); // Call to the Engine

    else if( iMode == CRUNAPL_VERIFY )
        bRet = m_pEntryIFaz->C_ExentryStart( CRUNAPL_VERIFY ); // Call to the Engine

    if( !bRet )
    {
        SetAppMode( CRUNAPL_NONE );
        return false;
    }

    SetAppMode(iMode);

    GetSettings()->SetEnterOutOfRange(false); // can potentially remove (look at changeset checked in on 20150114)

    // we need to display the user bar if the user pressed stop and then is starting to enter data again
    if( GetEntryDriver()->HasUserbar() )
        GetEntryDriver()->GetUserbar().Pause(false);

    if( iMode == CRUNAPL_ADD )
    {
        GetEntryDriver()->m_pIntDriver->m_pParadataDriver->LogEngineEvent(ParadataEngineEvent::SessionStart);
        GetEntryDriver()->m_pIntDriver->m_pParadataDriver->LogEngineEvent(ParadataEngineEvent::CaseStart);
    }

    return true;
}

// Closes LST.
bool CRunAplEntry::Stop()
{
    bool bRet = CRunApl::Stop();

    CEntryDriver* pEntryDriver = NULL;

    if( bRet && m_pEntryIFaz != NULL )
    {
        m_pEntryIFaz->C_ExentryStop();
        pEntryDriver = GetEntryDriver();

        GetEntryDriver()->m_pIntDriver->m_pParadataDriver->LogEngineEvent(ParadataEngineEvent::CaseStop);
        GetEntryDriver()->m_pIntDriver->m_pParadataDriver->LogEngineEvent(ParadataEngineEvent::SessionStop);
    }

    SetAppMode( CRUNAPL_NONE );

    if( pEntryDriver != NULL )
    {
        bool bClose = false;

        if( pEntryDriver->Exit_Code != 0 )
            bClose = true;

        else if( GetSettings() && GetSettings()->GetExitWhenFinish() )
        {
            pEntryDriver->Exit_Code = -1;
            bClose = true;
        }

        // we need to turn off the user bar if the user pressed stop
        if( GetEntryDriver()->HasUserbar() )
            PauseUserbar(true);

#ifdef WIN_DESKTOP
        if( bClose )
            AfxGetMainWnd()->PostMessage(WM_CLOSE);
#endif
    }

    return true;
}


Userbar* CRunAplEntry::GetUserbar()
{
    return GetEntryDriver()->HasUserbar() ? &GetEntryDriver()->GetUserbar() : nullptr;
}

void CRunAplEntry::PauseUserbar(bool pause)
{
    ASSERT(GetEntryDriver()->HasUserbar());
    GetEntryDriver()->GetUserbar().Pause(pause);
}


void CRunAplEntry::ExecuteCallbackUserFunction(int field_symbol_index, UserFunctionArgumentEvaluator& user_function_argument_evaluator)
{
    GetEntryDriver()->m_pIntDriver->ExecuteCallbackUserFunction(field_symbol_index, user_function_argument_evaluator);
}


// Start modification mode
bool CRunAplEntry::ModifyStart() {
    bool    bRet;

    if( !m_bFlagStart )
        return( false );

    if( m_bFlagModifyStart )
        return( false );
    m_bFlagModifyStart = TRUE;

    bRet = m_pEntryIFaz->C_ModifyStart( TRUE );

    return( bRet );
}

// Finish a modification session.
bool CRunAplEntry::ModifyStop() {
    if( !m_bFlagModifyStart )
        return( false );

    m_bFlagModifyStart = false;
    m_pEntryIFaz->C_ModifyStop();

    return( TRUE );
}

CDEItemBase* CRunAplEntry::EndGroup( bool bPostProc )
{
    m_bForward = true;

    DEFLD* pReachedFld = m_pEntryIFaz->C_EndGroup( bPostProc );

    CDEItemBase* pItemReached = ResetCurrentObjects( pReachedFld );

    return pItemReached;
}

CDEItemBase* CRunAplEntry::EndGroupOcc( bool bPostProc )
{
    DEFLD* pReachedFld = m_pEntryIFaz->C_EndGroupOcc( bPostProc );

    CDEItemBase* pItemReached = ResetCurrentObjects( pReachedFld );

    return pItemReached;
}

CDEItemBase* CRunAplEntry::NextField(BOOL bSaveCur, bool advance_past_block/* = false*/)
{
    ASSERT( m_bFlagStart );

    m_bForward = true;

    // save the current field
    if( bSaveCur )
    {
        CDEField* pField = (CDEField*)m_pCurField;
        PutVal(pField, pField->GetData());
    }

    // solve for the next field
    DEFLD* pNextFld = advance_past_block ? m_pEntryIFaz->C_AdvancePastBlock() :
                                           m_pEntryIFaz->C_GoToField(ENGINE_NEXTFIELD);

    CDEItemBase* pItemReached = ResetCurrentObjects(pNextFld);

    if( pItemReached != nullptr )
        RunPeriodicEvents();

    return pItemReached;
}


CDEItemBase* CRunAplEntry::PreviousField( BOOL bSaveCur/* = TRUE*/)
{
    m_bForward = false;

    ASSERT( m_bFlagStart );

    // Save the current field
    if( bSaveCur )
    {
        CDEField* pField = (CDEField*)m_pCurField;
        PutVal(pField, pField->GetData());
    }

    DEFLD* pBackFld = m_pEntryIFaz->C_GoToField(ENGINE_BACKFIELD);

    CDEItemBase* pItemReached = ResetCurrentObjects(pBackFld);

    return pItemReached;
}


// RHF INIC Nov 21, 2002
CDEField*  CRunAplEntry::GetDeField( DEFLD* pFld ) {
    ASSERT( pFld->getIndexes().isValid() );

    DEFLD_INFO FldInfo = m_pEntryIFaz->C_FldInfo(pFld);
    ASSERT( FldInfo.level >= 1 );

    VART* pVarT=FldInfo.vp;
    ASSERT( pVarT );
    CDEForm* pCDEForm=pVarT->GetForm();
    ASSERT( pCDEForm );

    return pCDEForm->GetField(pFld->GetSymbol());
}
// RHF END Nov 21, 2002


// RHF INIC Jul 28, 2000
// Asociate Engine item to a interface field
void CRunAplEntry::SetCurrentObjects( DEFLD* pFld ) {

    // gets the deepest index
    // RCL, May 26 2004

    int iOccur = pFld->GetIndexes().searchNotEmptyValue();

    if( iOccur < 1 ) iOccur = 1;

    m_pCurField = GetDeField( pFld );

    if( m_pCurField != NULL )
    {
        CDEGroup*   pGroup;

        if( ( pGroup = m_pCurField->GetParent() ) != NULL )
            pGroup->SetCurOccurrence( iOccur );

        ASSERT( pGroup != NULL );
    }

    ASSERT( m_pCurField != NULL );
}
// RHF END Jul 28, 2000


CDEItemBase* CRunAplEntry::MoveToField( const int iVar, const int iOcc, bool bPostProc )
{
    DEFLD DeFld;
    DeFld.SetSymbol( iVar );

    int iCurrentOcc = m_pEntryIFaz->C_GetGroupOcc( iVar );
    int iTargetOcc = ( iOcc > 0 ) ? iOcc : ( iCurrentOcc > 0 ) ? iCurrentOcc : 1;

    DeFld.useOnlyOneDimension( iTargetOcc );

    CDEItemBase* pItemReached = MoveToField( &DeFld, bPostProc );

    return pItemReached;
}


CDEItemBase* CRunAplEntry::MoveToFieldOrBlock(const CDEItemBase* pEntryEntity, bool bPostProc/* = true*/)
{
    if( pEntryEntity->isA(CDEFormBase::eItemType::Field) )
        return MoveToField((const CDEField*)pEntryEntity, bPostProc);

    else
    {
        ASSERT(pEntryEntity->isA(CDEFormBase::eItemType::Block));
        return MoveToBlock((const CDEBlock*)pEntryEntity, bPostProc);
    }
}

CDEItemBase* CRunAplEntry::MoveToField( const CDEField* pEntryField, bool bPostProc )
{
    CDEField* pField = (CDEField*) pEntryField;
    DEFLD DeFld;
    const CDictItem* pItem = pField->GetDictItem();

    if( pItem == NULL )
    {
        ASSERT( false );
        return NULL;
    }

    DeFld.SetSymbol( pItem->GetSymbol() );

    int iOcc = pField->GetRuntimeOccurrence();
    if( iOcc <= 0 )
        DeFld.useOnlyOneDimension( m_pEntryIFaz->C_GetGroupOcc( pItem->GetSymbol() ) );
    else
        DeFld.useOnlyOneDimension( iOcc );

    CDEItemBase* pItemReached = MoveToField( &DeFld, bPostProc );

    return pItemReached;
}

CDEItemBase* CRunAplEntry::MoveToBlock(const CDEBlock* pEntryBlock, bool bPostProc/* = true*/)
{
    DEFLD* pReachedFld = m_pEntryIFaz->C_MoveToBlock(pEntryBlock->GetSymbol(), bPostProc);

    CDEItemBase* pItemReached = ResetCurrentObjects(pReachedFld);

    return pItemReached;
}

CDEItemBase* CRunAplEntry::MoveToField( const DEFLD* pDeField, bool bPostProc/* = true*/)
{
    DEFLD* pReachedFld = m_pEntryIFaz->C_MoveToField( (DEFLD*)pDeField, bPostProc );

    CDEItemBase* pItemReached = ResetCurrentObjects( pReachedFld );

    return pItemReached;
}

CDEItemBase* CRunAplEntry::MoveToField(const CaseItemReference& case_item_reference, bool bPostProc/* = true*/)
{
    auto the3dObject = GetEntryDriver()->m_pIntDriver->ConvertIndex(case_item_reference);
    DEFLD cField(the3dObject->GetSymbol(), the3dObject->GetIndexes());
    return MoveToField(&cField, bPostProc);
}


void CRunAplEntry::EvaluateFieldAndIndices(const CDEField* pField, DEFLD& DeFld) const
{
    DeFld.SetSymbol(pField->GetSymbol());

    int iOcc = pField->GetRuntimeOccurrence();

    if( iOcc <= 0 )
        DeFld.useOnlyOneDimension(m_pEntryIFaz->C_GetGroupOcc(pField->GetSymbol()));

    else
        DeFld.useOnlyOneDimension(iOcc);
}

CString CRunAplEntry::GetVal(const CDEField* pField)
{
    DEFLD DeFld;
    EvaluateFieldAndIndices(pField, DeFld);
    return GetVal(&DeFld);
}


void CRunAplEntry::PutVal(const CDEField* pField, CString csValue, bool* value_has_been_modified/* = nullptr*/)
{
    ASSERT(csValue.Find('\r') == -1);

    DEFLD DeFld;
    EvaluateFieldAndIndices(pField, DeFld);
    m_pEntryIFaz->C_FldPutVal(&DeFld, csValue, value_has_been_modified);
}


int CRunAplEntry::GetStatus(int iVar, int iOcc)
{
    DEFLD DeFld;
    DeFld.SetSymbol(iVar);

    if( iOcc <= 0 )
        DeFld.useOnlyOneDimension( m_pEntryIFaz->C_GetGroupOcc( iVar ) );

    else
        DeFld.useOnlyOneDimension( iOcc );

    return m_pEntryIFaz->C_GetStatus(&DeFld);
}

int CRunAplEntry::GetStatus(const CDEField* pField)
{
    DEFLD DeFld;
    EvaluateFieldAndIndices(pField, DeFld);
    return m_pEntryIFaz->C_GetStatus(&DeFld);
}

int CRunAplEntry::GetStatus(const DEFLD* pDeField)
{
    return m_pEntryIFaz->C_GetStatus(pDeField);
}

int CRunAplEntry::GetStatus3(const DEFLD3* pDeField)
{
    return m_pEntryIFaz->C_GetStatus3(pDeField);
}

bool CRunAplEntry::IsPathOn()
{
    return m_pEntryIFaz->C_IsPathOn();
}

bool CRunAplEntry::SetStopNode(int iNode)
{
    return m_pEntryIFaz->C_SetStopNode(iNode);
}

bool CRunAplEntry::IsNewCase() {
    return( m_pEntryIFaz->C_IsNewCase() );
}

CString CRunAplEntry::GetVal(int iVar, int iOcc)
{
    DEFLD DeFld;

    DeFld.SetSymbol(iVar);

    if( iOcc <= 0 )
        DeFld.useOnlyOneDimension(m_pEntryIFaz->C_GetGroupOcc(iVar));

    else
        DeFld.useOnlyOneDimension(iOcc);

    return GetVal(&DeFld);
}

CString CRunAplEntry::GetVal(const DEFLD* pDeField)
{
    CString csValue;
    DEFLD3 DeField3;

    if( m_pEntryIFaz->GetEntryDriver()->MakeField3(&DeField3, pDeField) )
        csValue = m_pEntryIFaz->C_FldGetVal(&DeField3);

    return csValue;
}

// RHF INIC Jan 13, 2000
CCapi* CRunAplEntry::GetCapi() const
{
    return( m_pEntryIFaz->C_GetCapi() );
}

bool CRunAplEntry::GetDeFld( const CDEField* pEntryField, DEFLD* pDeFld ) const {
    CDEField* pField = (CDEField*) pEntryField;
    const CDictItem* pItem = pField->GetDictItem();
    bool bRet=false;

    if( pItem == NULL ) {
        ASSERT( false );
        return false;
    }
    else {
        pDeFld->SetSymbol( pItem->GetSymbol() );

        int iOcc = pField->GetRuntimeOccurrence();

        if( iOcc <= 0 )
            iOcc = m_pEntryIFaz->C_GetGroupOcc( pDeFld->GetSymbol() );

        pDeFld->setIndexValue(0, iOcc );

        bRet = true;
    }

    return bRet;
}
// RHF END Jan 13, 2000

// RHF INIC Feb 15, 2000
CDEItemBase* CRunAplEntry::EndLevel( bool bPostProcCurField, bool bPostProcAllOthers,
                                     int iNextLevelToCapture, bool bWriteNode)
{
    DEFLD* pReachedFld = m_pEntryIFaz->C_EndLevel( bPostProcCurField, bPostProcAllOthers,iNextLevelToCapture,bWriteNode, false );

    CDEItemBase* pItemReached = ResetCurrentObjects( pReachedFld );

    return pItemReached;
}

// iNodeNumRelative and iNodeNumAbsolute are 1 based if are filled correctly.
int CRunAplEntry::GetCurrentLevel( int* iNodeNumRelative, int* iNodeNumAbsolute ) const {
    int             iCurrentLevel;

    iCurrentLevel = m_pEntryIFaz->C_GetCurrentLevel();

    // RHF INIC Mar 23, 2001
    if( iNodeNumRelative != NULL || iNodeNumAbsolute != NULL ) {

             if( iNodeNumRelative != NULL )*iNodeNumRelative = -1;
             if( iNodeNumAbsolute != NULL ) *iNodeNumAbsolute = -1;

        if( GetAppMode() == CRUNAPL_MODIFY || GetAppMode() == CRUNAPL_VERIFY )
        {
            ASSERT(0); // REPO_TEMP: what is this used for?
#ifdef OLD
            int iNumRelative, iNumAbsolute;

            iNumRelative = m_pEntryIFaz->C_GetCurrentNodeNum( iCurrentLevel, &iNumAbsolute );

            if( iNodeNumRelative != NULL ) *iNodeNumRelative = iNumRelative;
            if( iNodeNumAbsolute != NULL ) *iNodeNumAbsolute = iNumAbsolute;
#endif
        }
    }
    // RHF END Mar 23, 2001

    return (iCurrentLevel <= 0) ? -1 : iCurrentLevel;
}

bool CRunAplEntry::IsNewNode() const {
     return( m_pEntryIFaz->C_IsNewNode() );
}


// RHF INIC Aug 09, 2000

CDEItemBase* CRunAplEntry::ResetCurrentObjects( DEFLD* pReachedFld )
{
    CDEItemBase* pItemReached = NULL;
    VART* pVarT = NULL;
    VART* pLastVarT = NULL;

    CDEGroup* pLastGroup=NULL;

    if( m_pCurField != NULL )
    {
        pLastGroup = m_pCurField->GetParent();

        int iVar=m_pCurField->GetSymbol();

        if( iVar > 0 )
        {
            VARX* pLastVarX = m_pEntryIFaz->GetVarX(iVar);

            if( pLastVarX != NULL )
                pLastVarT = pLastVarX->GetVarT();
        }
    }

    if( pReachedFld != NULL && pReachedFld->GetSymbol() > 0 )
    {
        // updates current field description with reached field
        m_pCurEngineField = pReachedFld;
        SetCurrentObjects( pReachedFld );

        pItemReached = m_pCurField;

        int iVar = pReachedFld->GetSymbol();

        DEFLD_INFO FldInfo = m_pEntryIFaz->C_FldInfo(pReachedFld);
        ASSERT( FldInfo.level >= 1 );

        pVarT=FldInfo.vp;
        ASSERT( pVarT );

        int iOcc = pReachedFld->getIndexValue(0);

        // 20110624 the CAPI text didn't work for multiply occurring items because iOcc was always 0
        //Savy 07/19/2017 - when subitem occurs, in 3d the index value of 2 gives the  occ of the subitem
        if( pVarT->GetDictItem() && pVarT->GetDictItem()->GetItemType() == ItemType::Subitem && pVarT->GetMaxOccs() > 1 )
            iOcc = pReachedFld->getIndexValue(2); //record->item->subitem

        else if( pVarT->GetMaxOccs() > 1 )
            iOcc = pReachedFld->getIndexValue(1);

        else if( pVarT->GetOwnerGPT()->GetMaxOccs() > 1 && pReachedFld->getIndexValue(1) ) // 20120709 multiply occurring subitems didn't handle occurrences correctly (iOcc was always 0 for these as well)
            iOcc = pReachedFld->getIndexValue(1);

        m_pEntryIFaz->RunGlobalOnFocus(iVar);
    }

    //FABN Nov 20, 2002

    //This is for refresh ONLY the current Form
    std::vector<CDEItemBase*> aGroupArray;
    CDEForm* pForm = pVarT ? pVarT->GetForm()   : NULL;
    CDEGroup* pGroup = pForm ? pForm->GetGroup()  : NULL;

    if( pGroup!=NULL )
        aGroupArray.emplace_back(pGroup);

    bool bIsNewFlow=false;
    bool bPrimaryFlow=false;
    if( pVarT != NULL && pLastVarT != NULL && pVarT->GetOwnerGPT() != NULL && pLastVarT->GetOwnerGPT() != NULL &&
        pVarT->GetOwnerGPT()->GetFlow() != pLastVarT->GetOwnerGPT()->GetFlow() )
    {
        bIsNewFlow = true;
    }

    if( pVarT != NULL && pVarT->GetOwnerGPT() != NULL && pVarT->GetOwnerGPT()->GetFlow() &&
        pVarT->GetOwnerGPT()->GetFlow()->GetSubType() == SymbolSubType::Primary )
    {
        bPrimaryFlow = true;
    }

    if( pLastGroup != NULL && pLastGroup != pGroup )
        aGroupArray.emplace_back(pLastGroup);

    bool bIsNewCase = IsNewCase();

    bool bFullRefresh = bIsNewFlow || bIsNewCase;

    std::vector<std::vector<int>> aOccsArray;     //aOccArray.GetAt(i) = array of occs to refresh, for group "i"
    int iNumGroups = (int)aGroupArray.size();

    if(!bFullRefresh)
    {
        //FABN Feb 17, 2003
        //For a given group in aGroupArray, it is possible to specify  an array of occurrences,
        //in order to refresh each {group,occ} in the tree. But, now the engine don't know exactly
        //which occs must be refreshed for a given group, so, every occ in {1,..,data occ} will be refreshed
        for( int iGroupIdx=0; iGroupIdx<iNumGroups; iGroupIdx++)
        {
            std::vector<int>& thisOccsArray = aOccsArray.emplace_back();
            CDEGroup* pAuxGroup = (CDEGroup*) aGroupArray[iGroupIdx];

            int iDataOccs = pAuxGroup ? pAuxGroup->GetDataOccs() : -1;

            for( int iOccIdx=1; iOccIdx<=iDataOccs; iOccIdx++)
                thisOccsArray.emplace_back( iOccIdx );
        }
    }

#ifdef WIN_DESKTOP
    std::vector<void*> vpArray
    {
        pItemReached,
        bFullRefresh ? nullptr : &aGroupArray,
        bFullRefresh ? nullptr : &aOccsArray
    };

    CWnd* pMainWnd = AfxGetMainWnd();
    if( pMainWnd && IsWindow(pMainWnd->GetSafeHwnd()) )
        pMainWnd->SendMessage(WM_IMSA_REFRESHCAPIGROUPS, reinterpret_cast<WPARAM>(&vpArray));
#endif

    // returns NULL when unable to go away from source field,
    // or when there is no more fields after the current group

    return pItemReached;
}

CDEItemBase* CRunAplEntry::InsertOcc( bool& bRet )
{
    DEFLD* pReachedFld = m_pEntryIFaz->C_InsertOcc( bRet );

    CDEItemBase* pItemReached = ResetCurrentObjects( pReachedFld );

    return pItemReached;
}

CDEItemBase* CRunAplEntry::DeleteOcc( bool& bRet )
{
    DEFLD* pReachedFld = m_pEntryIFaz->C_DeleteOcc( bRet );

    CDEItemBase* pItemReached = ResetCurrentObjects( pReachedFld );

    return pItemReached;
}

CDEItemBase* CRunAplEntry::SortOcc( const bool bAscending, bool& bRet )
{
    DEFLD* pReachedFld = m_pEntryIFaz->C_SortOcc( bAscending, bRet );

    CDEItemBase* pItemReached = ResetCurrentObjects( pReachedFld );

    return pItemReached;
}

CDEItemBase* CRunAplEntry::InsertOccAfter( bool& bRet )
{
    DEFLD* pReachedFld = m_pEntryIFaz->C_InsertOccAfter( bRet );

    CDEItemBase* pItemReached = ResetCurrentObjects( pReachedFld );

    return pItemReached;
}

CDEItemBase* CRunAplEntry::PreviousPersistentField()
{
    DEFLD* pReachedFld = m_pEntryIFaz->C_PreviousPersistentField();

    CDEItemBase* pItemReached = ResetCurrentObjects(pReachedFld);

    return pItemReached;
}

// RHF INIC Nov 07, 2000
bool CRunAplEntry::IsAutoEndGroup()
{
    return( m_pEntryIFaz->C_IsAutoEndGroup() );
}
// RHF END Nov 07, 2000

// RHF INIC Jan 25, 2001
CDEItemBase* CRunAplEntry::AdvanceToEnd( bool bStopOnNextNode, BOOL bSaveCur, int iStopNode/*=-1*/ )
{
    ASSERT( m_bFlagStart );

    // ... trust on new simplified method below         // victor Dec 10, 01
    DEFLD*          pReachedFld  = m_pEntryIFaz->C_BlindNodeAdvance( bStopOnNextNode, iStopNode ); // RHF Oct 29, 2002 Add iStopNode
    CDEItemBase*    pItemReached = ResetCurrentObjects( pReachedFld );

    return pItemReached;
}
// RHF END Jan 25, 2001


 // iNode >= 1. Delete the full branch.
bool CRunAplEntry::DeleteNode(const CaseKey& case_key, int iNode)
{
    if( GetAppMode() != CRUNAPL_NONE || iNode < 1 )
    {
         ASSERT(0);
         return false;
    }

    try
    {
        DataRepository* pInputRepo = GetInputRepository();
        auto data_case = pInputRepo->GetCaseAccess()->CreateCase();

        pInputRepo->ReadCase(*data_case, case_key.GetPositionInRepository());

        WriteCaseParameter write_case_parameter = WriteCaseParameter::CreateModifyParameter(*data_case);

        // delete the node
        auto case_levels = data_case->GetAllCaseLevels();

        if( iNode >= (int)case_levels.size() )
            return false;

        CaseLevel& case_level_to_delete = *case_levels[iNode];
        CaseLevel& parent_case_level = case_level_to_delete.GetParentCaseLevel();
        parent_case_level.RemoveChildCaseLevel(case_level_to_delete);

        // set the notes as modified because it is possible some were removed by the call to RemoveChildCaseLevel
        write_case_parameter.SetNotesModified();

        // and then write the case
        pInputRepo->WriteCase(*data_case, &write_case_parameter);

        return true;
    }

    catch( const DataRepositoryException::Error& exception )
    {
#ifdef WIN_DESKTOP
        ErrorMessage::Display(exception);
#else
        PlatformInterface::GetInstance()->GetApplicationInterface()->ShowModalDialog(_T(""), exception.GetErrorMessage(), MB_OK);
#endif
    }

    return false;
}


bool CRunAplEntry::PartialSaveCase(APP_MODE defaultMode, bool bClearSkipped)
{
    if (m_pEntryIFaz->GetEntryDriver()->GetPartialMode() == NO_MODE) {
        m_pEntryIFaz->GetEntryDriver()->SetPartialMode(defaultMode);
    }

    return m_pEntryIFaz->GetEntryDriver()->PartialSaveCase(bClearSkipped);
}


bool CRunAplEntry::HasSpecialFunction(SpecialFunction special_function)
{
    return m_pEntryIFaz->HasSpecialFunction(special_function);
}

double CRunAplEntry::ExecSpecialFunction(SpecialFunction special_function, double argument/* = 0*/)
{
    int iSymVar = m_pCurEngineField ? m_pCurEngineField->GetSymbol() : -1;

    if( iSymVar <= 0 || m_pEntryIFaz == NULL )
        return DEFAULT;

    return m_pEntryIFaz->ExecSpecialFunction(iSymVar, special_function, argument);
}


void CRunAplEntry::SetProgressForPreEntrySkip() // 20130415 so that skips launched from onchar/onkey/userbar treat the current field as unkeyed
{
    GetEntryDriver()->GetCsDriver()->SetProgressForPreEntrySkip();
}



void CRunAplEntry::ResetDoorCondition( void ) {
    // formally closing the CsDriver session
    m_pEntryIFaz->C_ResetDoorCondition();
}


bool CRunAplEntry::EditNote(bool case_note)
{
    return GetEntryDriver()->EditNote(case_note);
}

std::shared_ptr<const CaseItemReference> CRunAplEntry::ReviewNotes()
{
    return GetEntryDriver()->ReviewNotes();
}


bool CRunAplEntry::SetCurrentLanguage(wstring_view language_name)
{
    return m_pEntryIFaz->SetCurrentLanguage(language_name);
}

std::vector<Language> CRunAplEntry::GetLanguages(bool include_only_capi_languages/* = true*/) const
{
    return m_pEntryIFaz->GetLanguages(include_only_capi_languages);
}

bool CRunAplEntry::ChangeLanguage()
{
    // get the list of languages
    std::vector<Language> languages = m_pEntryIFaz->GetLanguages(false);

    // there is no need to display a language selection dialog if there are not multiple languages
    if( languages.size() < 2 )
    {
        ErrorMessage::Display(MGF::GetMessageText(MGF::SelectLanguageOnlyOneDefined));
        return false;
    }

    ChoiceDlg choice_dlg(0);
    choice_dlg.SetTitle(MGF::GetMessageText(MGF::SelectLanguageTitle));

    const auto& current_language_name = GetEntryDriver()->GetCurrentLanguageName();
    std::optional<int> default_choice_index;

    for( const Language& language : languages )
    {
        int choice_index = choice_dlg.AddChoice(language.GetLabel());

        if( !default_choice_index.has_value() && current_language_name == language.GetName() )
            default_choice_index = choice_index;
    }

    if( default_choice_index.has_value() )
        choice_dlg.SetDefaultChoiceIndex(*default_choice_index);

    if( choice_dlg.DoModalOnUIThread() != IDOK )
        return false;

    m_pEntryIFaz->SetCurrentLanguage(languages[choice_dlg.GetSelectedChoiceIndex()].GetName());

    if( HasSpecialFunction(SpecialFunction::OnChangeLanguage) )
        ExecSpecialFunction(SpecialFunction::OnChangeLanguage, 0);

    return true;
}


CDEFormFile* CRunAplEntry::GetFormFileInProcess() {
       return m_pEntryIFaz->GetFormFileInProcess();
}

CDEFormFile* CRunAplEntry::GetPrimaryFormFile() {
    return m_pEntryIFaz->GetPrimaryFormFile();
}

bool CRunAplEntry::InEnterMode() {
    return m_pEntryIFaz->InEnterMode();
}

int CRunAplEntry::GetNumLevels( bool bPrimaryFlow ) {
    return m_pEntryIFaz->GetNumLevels(bPrimaryFlow);
}

int CRunAplEntry::GetCurrentLevel() {
    return m_pEntryIFaz->GetCurrentLevel();
}

CString CRunAplEntry::GetCurrentKey( int iLevel ) {
    ASSERT( iLevel == -1 || iLevel >= 1 && iLevel <= (int)MaxNumberLevels );
    return m_pEntryIFaz->GetCurrentKey(iLevel);
}

CDEItemBase* CRunAplEntry::GetCurItemBase()
{
    return m_pCurField;
}

void CRunAplEntry::RunGlobalOnFocus( int iVar ) {
    m_pEntryIFaz->RunGlobalOnFocus(iVar);
}

bool CRunAplEntry::QidReady( int iLevel ) {
    return m_pEntryIFaz->QidReady( iLevel );
}

// RHF END Nov 08, 2002

bool CRunAplEntry::RestorePartial()
{
    CEntryDriver* pEntryDriver = GetEntryDriver();
    Case& data_case = pEntryDriver->GetInputCase();

    ASSERT(data_case.GetPartialSaveCaseItemReference() != nullptr);
    const auto& partial_save_case_item_reference = *data_case.GetPartialSaveCaseItemReference();

    // for multiple level applications, need to find the correct node
    CString csLevelKey = partial_save_case_item_reference.GetLevelKey();
    int iNodeNum = 0;

    if( !csLevelKey.IsEmpty() )
    {
        const auto& case_levels = data_case.GetAllCaseLevels();

        for( size_t i = 0; i < case_levels.size(); ++i )
        {
            if( case_levels[i]->GetLevelKey().Compare(csLevelKey) == 0 )
            {
                iNodeNum = (int)i;
                break;
            }
        }
    }

    if( iNodeNum >= 0 && SetStopNode(iNodeNum) )
    {
        CDEItemBase* pItemBase = AdvanceToEnd(false, true, iNodeNum);

        if( pItemBase != nullptr )
        {
            auto the3dObject = GetEntryDriver()->m_pIntDriver->ConvertIndex(partial_save_case_item_reference);
            pItemBase = MoveToField(the3dObject.get(), true);
        }

        return true;
    }

    else
    {
        CString csMsg = _T("Cannot move to ");

        if( !partial_save_case_item_reference.GetLevelKey().IsEmpty() )
            csMsg.AppendFormat(_T("node '%s', "), csLevelKey.GetString());

        csMsg.AppendFormat(_T("field '%s'"), partial_save_case_item_reference.GetName().GetString());

#ifdef WIN_DESKTOP
        AfxMessageBox(csMsg);
#else
        PlatformInterface::GetInstance()->GetApplicationInterface()->ShowModalDialog(_T(""),csMsg,MB_OK);
#endif
        return false;
    }
}


// if iSymvar < 0 apply to all the items
void CRunAplEntry::ToggleCapi( int iSymVar )
{
    m_pEntryIFaz->ToggleCapi( iSymVar );
}


// int iLevel zero base
int CRunAplEntry::GetKeyLen( const CDataDict* pDataDict, int iLevel )
{
    return m_pEntryIFaz->C_GetKeyLen( pDataDict, iLevel );
}

// RHF END Jul 21, 2003

void CRunAplEntry::SetupOperatorId()
{
    if( !UseHtmlDialogs() )
        return;

    const Application* application = m_pPifFile->GetApplication();
    CString operator_id = m_pPifFile->GetOpID();

    if( application->GetAskOperatorId() && SO::IsBlank(operator_id) )
    {
        TextInputDlg text_input_dlg;
        text_input_dlg.SetTitle(MGF::GetMessageText(89265, _T("Operator ID")));
        text_input_dlg.SetRequireInput(true);

        if( text_input_dlg.DoModalOnUIThread() != IDOK )
            throw CSProException(MGF::GetMessageText(89266));

        operator_id = WS2CS(text_input_dlg.GetTextInput());
    }

    SetOperatorId(operator_id);
}

void CRunAplEntry::SetOperatorId(const CString& operator_id)
{
    if( m_pEntryIFaz != NULL )
        m_pEntryIFaz->C_SetOperatorId(operator_id);
}

CString CRunAplEntry::GetOperatorId() const
{
    return ( m_pEntryIFaz == NULL ) ? _T("") : m_pEntryIFaz->C_GetOperatorId();
}


// RHF INIC Nov 06, 2003
bool CRunAplEntry::HasSomeRequest()
{
    return ( m_pEntryIFaz != NULL ) ? m_pEntryIFaz->C_HasSomeRequest() : false;
}

CDEItemBase* CRunAplEntry::RunCsDriver( bool bCheckRange )
{
    ASSERT( HasSomeRequest() );

    DEFLD* pReachedFld = m_pEntryIFaz->C_RunCsDriver(bCheckRange);

    CDEItemBase* pItemReached = ResetCurrentObjects( pReachedFld );

    return pItemReached;
}
// RHF END Nov 06, 2003

// BUCEN_2003 Changes End

//FABN Jan 2006
CDEForm* CRunAplEntry::GetForm( CDEItemBase* pItemBase, bool bPrimaryFlow )
{
    int             iFormNum    = pItemBase ? pItemBase->GetFormNum() : -1;
    CDEFormFile*    pFormFile   = bPrimaryFlow ? GetPrimaryFormFile() : GetFormFileInProcess();
    CDEForm*        pForm       = pFormFile ? pFormFile->GetForm(iFormNum) : NULL;
    return          pForm;
}

CDEFormBase* CRunAplEntry::GetEntryObject( CDEItemBase* pItem, bool bPrimaryFlow )
{
    //must perform data entry by item
    CDEFormBase* pEntryObject = pItem;
    CDEForm* pForm = GetForm( pItem, bPrimaryFlow );

    //at least one grid in the form => data entry by item
    if(pForm && pForm->GetNumItems(CDEFormBase::Roster ) > 0)
        return pItem;

    CDEGroup*   pParent = pItem ? pItem->GetParent() : NULL;
    if( pParent && pParent->GetItemType() == CDEFormBase::Roster )
    {
        CDERoster* pRoster = (CDERoster*) pParent;
        bool bEntryByRoster = false;
        if( bEntryByRoster )
        {
            //must perform data entry by roster
            pEntryObject = pRoster;
        }
    }

    return pEntryObject;
}

CDEFormBase* CRunAplEntry::GetCurEntryObject(bool bPrimaryFlow)
{
    return GetEntryObject( GetCurItemBase(), bPrimaryFlow );
}


bool CRunAplEntry::ProcessModify(double dPositionInRepository, bool* pbMoved, PartialSaveMode* partial_save_mode,
    CDEItemBase** ppItem, int iNode, ProcessModifyAction eModifyAction)
{
    *pbMoved = false;
    *partial_save_mode = PartialSaveMode::None;

    // load the case
    CEntryDriver* pEntryDriver = GetEntryDriver();
    Case& data_case = pEntryDriver->GetInputCase();

    try
    {
        GetInputRepository()->ReadCasetainer(data_case, dPositionInRepository);
    }

    catch( const DataRepositoryException::Error& exception )
    {
#ifdef WIN_DESKTOP
        ErrorMessage::Display(exception);
#else
        PlatformInterface::GetInstance()->GetApplicationInterface()->ShowModalDialog(_T(""), exception.GetErrorMessage(), MB_OK);
#endif
        return false;
    }

    GetEntryDriver()->m_pIntDriver->m_pParadataDriver->LogEngineEvent(ParadataEngineEvent::SessionStart);
    GetEntryDriver()->m_pIntDriver->m_pParadataDriver->LogEngineEvent(ParadataEngineEvent::CaseStart);

    pEntryDriver->SetWriteCaseParameter(WriteCaseParameter::CreateModifyParameter(data_case));

    // insert or add a level node (if necessary)
    bool bInsertNode = ( eModifyAction == ProcessModifyAction::InsertNode );

    if( bInsertNode || ( eModifyAction == ProcessModifyAction::AddNode ) )
    {
        Pre74_Case* pCase = data_case.GetPre74_Case();
        Pre74_CaseLevel* pCaseLevel = pCase->GetCaseLevelAtNodeNumber(iNode);
        ASSERT(pCaseLevel->GetLevelNum() > 1);

        Pre74_CaseLevel* pParentLevel = pCase->FindParentLevel(pCaseLevel);

        if( bInsertNode )
            pParentLevel->InsertChildLevel(pCaseLevel);

        else
        {
            Pre74_CaseLevel* pAddedCaseLevel = pParentLevel->AddChildLevelAtEnd(pCaseLevel);
            iNode = pCase->GetCaseLevelNodeNumber(pAddedCaseLevel);
        }
    }


    // process partial saves
    pEntryDriver->SetPartialMode(NO_MODE);

    if( data_case.IsPartial() )
    {
        APP_MODE eAppMode = NO_MODE;

        switch( data_case.GetPartialSaveMode() )
        {
            case PartialSaveMode::Add:
                eAppMode = ADD_MODE;
                break;

            case PartialSaveMode::Modify:
                eAppMode = MODIFY_MODE;
                break;

            case PartialSaveMode::Verify:
                eAppMode = VERIFY_MODE;
                break;
        }

        pEntryDriver->SetPartialMode(eAppMode);

        // query the user if they want to move to the last position, though having an OnStop function disables this functionality
        if( data_case.GetPartialSaveCaseItemReference() != nullptr && !HasSpecialFunction(SpecialFunction::OnStop) )
        {
            const std::wstring& partial_save_last_position_query = MGF::GetMessageText(MGF::PartialSaveGotoLastPosition);

#ifdef WIN_DESKTOP
            if( AfxMessageBox(partial_save_last_position_query.c_str(), MB_YESNO) == IDYES )
#else
            if( PlatformInterface::GetInstance()->GetApplicationInterface()->ShowModalDialog(
                MGF::GetMessageText(MGF::PartialSaveTitle), partial_save_last_position_query, MB_YESNO) == IDYES )
#endif
            {
                *pbMoved = RestorePartial();
                *partial_save_mode = data_case.GetPartialSaveMode();
            }
        }
    }

    if( *partial_save_mode == PartialSaveMode::None )
    {
        if( SetStopNode(iNode) )
            NextField(FALSE);

        *pbMoved = true;
    }

    *ppItem = m_pCurField;

    return true;
}

bool CRunAplEntry::ProcessModify(double dPositionInRepository,bool* pbMoved,PartialSaveMode* partial_save_mode,CDEItemBase** ppItem)
{
    // REPO_TEMP: should the above function just have default arguments
    return ProcessModify(dPositionInRepository,pbMoved,partial_save_mode,ppItem,0,ProcessModifyAction::GotoNode);
}


void CRunAplEntry::ProcessAdd()
{
    CEntryDriver* pEntryDriver = GetEntryDriver();
    pEntryDriver->ClearWriteCaseParameter();
    pEntryDriver->LoadPersistentFields();
    pEntryDriver->SetPartialMode(NO_MODE);
}

void CRunAplEntry::ProcessInsert(double insert_before_position_in_repository)
{
    CEntryDriver* pEntryDriver = GetEntryDriver();
    pEntryDriver->SetWriteCaseParameter(WriteCaseParameter::CreateInsertParameter(insert_before_position_in_repository));
    pEntryDriver->LoadPersistentFields(insert_before_position_in_repository);
    pEntryDriver->SetPartialMode(NO_MODE);
    pEntryDriver->SetInsertMode(true);
}


void CRunAplEntry::StopIfNecessary() // 20121023 for stopping after OnKey and OnChar calls
{
#ifdef WIN_DESKTOP
    if( GetEntryDriver()->m_pIntDriver->m_bStopProc )
        AfxGetMainWnd()->PostMessage(WM_IMSA_USERBAR_UPDATE,0,-1); // an easy way to stop (leveraging old work)
#endif
}


bool CRunAplEntry::IsStopRequested()
{
    return GetEntryDriver()->m_pIntDriver->m_bStopProc;
}


bool CRunAplEntry::IsStopRequestedAndTurnOffStop() // 20140131
{
    bool bStopProc = GetEntryDriver()->m_pIntDriver->m_bStopProc;
    GetEntryDriver()->m_pIntDriver->m_bStopProc = false;
    return bStopProc;
}


bool CRunAplEntry::ShowRefusalProcessor(ShowRefusalProcessorAction action, bool process_other_displayed_fields_in_block)
{
    DEFLD* pDeFld = m_pEntryIFaz->C_FldGetCurrent();

    if( pDeFld == nullptr )
        return false;

    bool found_hidden_refused_value = false;

    auto process_field = [&](VART* pVarT)
    {
        const ValueSet* value_set = pVarT->GetCurrentValueSet();

        if( value_set != nullptr )
        {
            ResponseProcessor* response_processor = value_set->GetResponseProcessor();

            if( response_processor->IsRefusedValueHidden() )
            {
                found_hidden_refused_value = true;

                if( action == ShowRefusalProcessorAction::ShowRefusedValues )
                    response_processor->AlwaysShowRefusedValue();
            }
        }
    };

    auto get_vart = [&](int symbol_index) -> VART*
    {
        VARX* pVarX = m_pEntryIFaz->GetVarX(symbol_index);
        return pVarX->GetVarT();
    };


    VART* pVarT = get_vart(pDeFld->GetSymbol());
    const EngineBlock* engine_block = nullptr;

    // see if this field should be processed along with other fields in the block
    if( process_other_displayed_fields_in_block )
    {
        engine_block = pVarT->GetEngineBlock();
        process_other_displayed_fields_in_block = ( engine_block != nullptr &&
                                                    engine_block->GetFormBlock().GetDisplayTogether() );
    }

    if( process_other_displayed_fields_in_block )
    {
        for( CDEField* this_field : engine_block->GetFields() )
        {
            process_field(get_vart(this_field->GetDictItem()->GetSymbol()));

            if( found_hidden_refused_value && action == ShowRefusalProcessorAction::ReturnWhetherRefusedValuesAreHidden )
                break;
        }
    }

    else
        process_field(pVarT);

    return found_hidden_refused_value;
}


bool CRunAplEntry::FinalizeInitializationTasks()
{
    try
    {
        SetupOperatorId();
    }

    catch( const CSProException& exception )
    {
        ErrorMessage::Display(exception);
        return false;
    }

    CEntryDriver* pEntryDriver = GetEntryDriver();

    pEntryDriver->m_pIntDriver->StartApplication();

    return pEntryDriver->OpenRepositories(true);
}

DataRepository* CRunAplEntry::GetInputRepository()
{
    return m_pEntryIFaz->GetEntryDriver()->GetInputRepository();
}

int CRunAplEntry::GetInputDictionaryKeyLength() const
{
    return m_pEntryIFaz->GetEntryDriver()->GetInputDictionaryKeyLength();
}

bool CRunAplEntry::RunSync(const AppSyncParameters& params)
{
    AppSyncParamRunner runner(GetEntryDriver()->m_pIntDriver->GetSyncClient());
    CLoginDialog loginDialog;
    SyncCredentialStore syncCredentialStore;
    DropboxAuthDialog dropboxAuthDialog;
    return runner.Run(params, *GetInputRepository(), &loginDialog, &dropboxAuthDialog, &syncCredentialStore);
}


void CRunAplEntry::RunPeriodicEvents()
{
    auto should_execute_next_periodic_action = [](std::optional<double>& timestamp, int minutes) -> bool
    {
        if( ( minutes == 0 ) || ( timestamp.has_value() && ( GetTimestamp() < *timestamp) ) )
            return false;

        timestamp =  GetTimestamp() + 60 * minutes;
        return true;
    };

    const Application* application = m_pPifFile->GetApplication();

    // if auto partial save is enabled, potentially save the case
    if( application->GetAutoPartialSave() )
    {
        if( should_execute_next_periodic_action(m_nextAutoPartialSaveTimestamp, application->GetAutoPartialSaveMinutes()) )
        {
            int iCurrentLevel = GetCurrentLevel();

            if( iCurrentLevel > 0 && QidReady(iCurrentLevel) )
            {
                bool bClearSkipped = false;

#ifdef WIN_DESKTOP
                if( AfxGetMainWnd() != NULL && ::IsWindow(AfxGetMainWnd()->GetSafeHwnd()) )
                    AfxGetMainWnd()->SendMessage(WM_IMSA_PARTIAL_SAVE,(WPARAM)bClearSkipped);
#else
                bool bFromLogic = true;
                PlatformInterface::GetInstance()->GetApplicationInterface()->PartialSave(bClearSkipped, bFromLogic);
#endif
            }
        }
    }


    // potentially record the paradata device state
    if( Paradata::Logger::IsOpen() )
    {
        if( should_execute_next_periodic_action(m_nextParadataDeviceStateTimestamp,
            application->GetApplicationProperties().GetParadataProperties().GetDeviceStateIntervalMinutes()) )
        {
            Paradata::Logger::LogEvent(std::make_shared<Paradata::DeviceStateEvent>());
        }
    }
}


// when opening a case in partial saved add mode, CRunApl::GetAppMode will be in CRUNAPL_MODIFY
// mode, so these functions give the true mode
bool CRunAplEntry::InAddMode() const
{
    return ( GetAppMode() == CRUNAPL_ADD ) || ( m_pEntryIFaz->GetEntryDriver()->GetPartialMode() == ADD_MODE );
}

bool CRunAplEntry::InModifyMode() const
{
    return ( GetAppMode() == CRUNAPL_MODIFY ) && ( m_pEntryIFaz->GetEntryDriver()->GetPartialMode() != ADD_MODE );
}

bool CRunAplEntry::InVerifyMode() const
{
    return ( GetAppMode() == CRUNAPL_VERIFY );
}


void CRunAplEntry::ViewCurrentCase()
{
    GetEntryDriver()->ViewCurrentCase();
}


void CRunAplEntry::ViewCase(const double position_in_repository)
{
    GetEntryDriver()->ViewCase(position_in_repository);
}
