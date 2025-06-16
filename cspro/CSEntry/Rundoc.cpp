// RunDoc.cpp : implementation of the CEntryrunDoc class
//

#include "StdAfx.h"
#include "Rundoc.h"
#include "CSEntry.h"
#include "MainFrm.h"
#include "Opdlg.h"
#include "OperatorStatistics.h"
#include "OperatorStatisticsLog.h"
#include "RunView.h"
#include <zBridgeO/npff.h>
#include <zBridgeO/PifDlg.h>
#include <zParadataO/KeyingInstance.h>
#include <zCapiO/QSFView.h>


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[]= __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CEntryrunDoc

IMPLEMENT_DYNCREATE(CEntryrunDoc, CDocument)

BEGIN_MESSAGE_MAP(CEntryrunDoc, CDocument)
        //{{AFX_MSG_MAP(CEntryrunDoc)
        //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CEntryrunDoc construction/destruction

CEntryrunDoc::CEntryrunDoc()
{
    m_pPIFFile = NULL;
    m_pRunApl = NULL;
    m_pCurrField = NULL;
    m_appMode = NO_MODE;
    m_bQModified = FALSE;
    m_pOperatorStatisticsLog = NULL;
    m_bInteractiveEdit = false;
}

CEntryrunDoc::~CEntryrunDoc()
{
    SAFE_DELETE(m_pOperatorStatisticsLog);
}


/////////////////////////////////////////////////////////////////////////////////
//
//      void CEntryrunDoc::DeleteContents()
//
/////////////////////////////////////////////////////////////////////////////////
void CEntryrunDoc::DeleteContents()
{
    m_pCurrField = NULL;
    m_pPIFFile = NULL;
    m_pRunApl = NULL;
    m_appMode = NO_MODE;

    CMainFrame* pFrame = (CMainFrame*)AfxGetMainWnd();

    if( pFrame != NULL)
    {
        POSITION pos = this->GetFirstViewPosition();
        CView* pView = this->GetNextView(pos);

        if( pView != NULL && pView->GetSafeHwnd() != NULL && pView->IsKindOf(RUNTIME_CLASS(CCaseView)) )
        {
            pFrame->GetCaseView()->GetTreeCtrl().SetRedraw(FALSE);
            pFrame->GetCaseView()->GetTreeCtrl().DeleteAllItems();
            pFrame->GetCaseView()->GetTreeCtrl().SetRedraw(TRUE);
        }
    }

    if( m_pOperatorStatisticsLog != NULL )
    {
        m_pOperatorStatisticsLog->Save();
        SAFE_DELETE(m_pOperatorStatisticsLog);
    }

    CSize size(0,0);
    CDEBaseEdit::InitSize(size);
    CDocument::DeleteContents();
}

/////////////////////////////////////////////////////////////////////////////////
//
//      BOOL CEntryrunDoc::OnOpenDocument(LPCTSTR lpszPathName)
//
/////////////////////////////////////////////////////////////////////////////////
BOOL CEntryrunDoc::OnOpenDocument(LPCTSTR lpszPathName)
{
    if( !CDocument::OnOpenDocument(lpszPathName) )
        return FALSE;

    SetCurrentDirectory(PortableFunctions::PathGetDirectory(lpszPathName).c_str());

    m_pPIFFile = ((CEntryrunApp*)AfxGetApp())->m_pPifFile;
    m_pRunApl = ((CEntryrunApp*)AfxGetApp())->m_pRunAplEntry;

    ASSERT(m_pPIFFile != NULL && m_pRunApl != NULL );

    CWaitCursor wait;

    if( !InitApplication() )
        return FALSE;

    if( !m_pRunApl->FinalizeInitializationTasks() )
        return FALSE;

    // open the operator statistics log
    OpenOperatorStatisticsLog();

    // get the operator ID
    if( !UseHtmlDialogs() )
    {
        CString csOperatorId = m_pPIFFile->GetOpID();

        if( m_pPIFFile->GetApplication()->GetAskOperatorId() )
        {
            COPDlg opDlg;

            if( opDlg.DoModal() != IDOK )
                return FALSE;

            csOperatorId = opDlg.m_sOpID;
        }

        m_pRunApl->SetOperatorId(csOperatorId);
    }

    CMainFrame* pFrame = (CMainFrame*)AfxGetMainWnd();
    CEntryrunView* pRunView = pFrame->GetRunView();
    pRunView->BuildGrids();

    if( m_pPIFFile->GetApplication()->GetUseQuestionText() )
    {
        QSFView* const pQTView = pFrame->GetQTxtView();

        if( pQTView != nullptr )
        {
            pQTView->SetStyleCss(m_pRunApl->GetCapi().GetRuntimeStylesCss());
            pQTView->SetUpQuestionTextView(TC::ToUtf8(lpszPathName));
        }
    }

    return TRUE;
}

/////////////////////////////////////////////////////////////////////////////////
//
//      bool CEntryrunDoc::InitApplication()
//
/////////////////////////////////////////////////////////////////////////////////
bool CEntryrunDoc::InitApplication()
{
    CWaitCursor wait;

    SetCurField(NULL);

    Application* pApp = m_pPIFFile->GetApplication();

    // check for converted rosters
    for( const auto& pFF : pApp->GetRuntimeFormFiles() )
    {
        CIMSAString csErr;

        if( !pFF->Reconcile(csErr,true,false) ) // silent yes; autofix no
        {
            AfxMessageBox(  _T("Errors reconciling forms file(s) with data dictionary(s).\n")
                            _T("You must open this application in the Designer, make\n")
                            _T("appropriate changes, and save the application.\n\n")
                            _T("Press OK to see errors."));
            AfxMessageBox (csErr);
            return false;
        }
    }

    m_pRunApl = ((CEntryrunApp*)AfxGetApp())->m_pRunAplEntry;
    ASSERT(m_pRunApl);

    CMainFrame* pFrame = (CMainFrame*)AfxGetMainWnd();
    pFrame->m_iNumCases = 0;
    pFrame->m_iTotalPartial = 0;
    pFrame->m_iNumPartialAdd = 0;
    pFrame->m_iNumPartialModify = 0;
    pFrame->m_iVerified = 0;
    pFrame->m_iPartialVerify = 0;

    CEntryrunView* pRunView = pFrame->GetRunView();
    pRunView->RemoveAllGrids2(true);

    m_pRunApl->GetSettings()->SetEnterOutOfRange(false); // can potentially remove (look at changeset checked in on 20150114)

    return true;
}


/////////////////////////////////////////////////////////////////////////////////
//
//      int CEntryrunDoc::GetCurFormNum()
//
/////////////////////////////////////////////////////////////////////////////////
int CEntryrunDoc::GetCurFormNum()
{
        //Later one get the current field from engine
        if (m_pCurrField)
                return m_pCurrField->GetFormNum();
        else
                return -1;
}

//This is for now later on get it from the engine

/////////////////////////////////////////////////////////////////////////////////
//
//      CDEFormFile* CEntryrunDoc::GetCurFormFile()
//
/////////////////////////////////////////////////////////////////////////////////
CDEFormFile* CEntryrunDoc::GetCurFormFile()
{
    if(m_pPIFFile)  {
        Application* pApp = m_pPIFFile->GetApplication();
        if(pApp && m_pRunApl && this->GetCurField()){
            return pApp->GetRuntimeFormFiles()[GetCurField()->GetFormFileNumber()].get();
        }
        else {
            if( !pApp->GetRuntimeFormFiles().empty() )// RHF Jun 27, 2006
                //SAVY&& for now ; later on get it from engine
                return pApp->GetRuntimeFormFiles().front().get();
        }
    }

    return NULL;
}


/////////////////////////////////////////////////////////////////////////////////
//
//      void CEntryrunDoc::AddKeyStroke()
//
/////////////////////////////////////////////////////////////////////////////////
void CEntryrunDoc::AddKeyStroke()
{
    //Later one get the current field from engine
    if (m_pOperatorStatisticsLog) {
            COperatorStatistics* pObj = m_pOperatorStatisticsLog->GetCurrentStatsObj();
            if(pObj)
            {
                if( pObj->IsPaused() ) // GHM 20120213
                    pObj->Start();

                pObj->IncKStroke();
            }
    }


    Paradata::KeyingInstance* keying_instance = ((CMainFrame*)AfxGetMainWnd())->GetParadataKeyingInstance();
    if( keying_instance != nullptr )
    {
        keying_instance->UnPause();
        keying_instance->IncreaseKeystrokes();
    }
}


/////////////////////////////////////////////////////////////////////////////////
//
//      void CEntryrunDoc::AddEntryError()
//
/////////////////////////////////////////////////////////////////////////////////
void CEntryrunDoc::AddEntryError()
{
    //Later one get the current field from engine
    if (m_pOperatorStatisticsLog) {
            COperatorStatistics* pObj = m_pOperatorStatisticsLog->GetCurrentStatsObj();
            if(pObj)
                    pObj->IncNumErr();
    }

    Paradata::KeyingInstance* keying_instance = ((CMainFrame*)AfxGetMainWnd())->GetParadataKeyingInstance();
    if( keying_instance != nullptr )
        keying_instance->IncreaseKeyingErrors();
}



/////////////////////////////////////////////////////////////////////////////////
//
//      void CEntryrunDoc::AddVerifyError(BOOL bKeyerErr)
//
/////////////////////////////////////////////////////////////////////////////////
void CEntryrunDoc::AddVerifyError(BOOL bKeyerErr)
{
    //Later one get the current field from engine
    if (m_pOperatorStatisticsLog) {
            COperatorStatistics* pObj = m_pOperatorStatisticsLog->GetCurrentStatsObj();
            if(pObj && !bKeyerErr)
                    pObj->IncNumVErr();
    else if(pObj && bKeyerErr)
        pObj->IncNumKErr();
    }

    Paradata::KeyingInstance* keying_instance = ((CMainFrame*)AfxGetMainWnd())->GetParadataKeyingInstance();
    if( keying_instance != nullptr )
    {
        if( bKeyerErr == TRUE )
            keying_instance->IncreaseFieldsKeyerError();

        else
            keying_instance->IncreaseFieldsVerifierError();
    }
}


/////////////////////////////////////////////////////////////////////////////////
//
//      void CEntryrunDoc::IncVerifiedField(void)
//
/////////////////////////////////////////////////////////////////////////////////
void CEntryrunDoc::IncVerifiedField(void)
{
    //Later one get the current field from engine
    if (m_pOperatorStatisticsLog) {
            COperatorStatistics* pObj = m_pOperatorStatisticsLog->GetCurrentStatsObj();
            if(pObj)
        pObj->IncFldVerified();
    }

    Paradata::KeyingInstance* keying_instance = ((CMainFrame*)AfxGetMainWnd())->GetParadataKeyingInstance();
    if( keying_instance != nullptr )
        keying_instance->IncreaseFieldsVerified();
}


//Call this function after loading the objects of the application

/////////////////////////////////////////////////////////////////////////////////
//
//      std::string CEntryrunDoc::MakeTitle()
//
/////////////////////////////////////////////////////////////////////////////////
std::string CEntryrunDoc::MakeTitle(const bool reset_to_default/* = false*/)
{
    std::string window_title = "CSEntry";

    if( reset_to_default )
        m_windowTitleOverride.reset();

    if( m_pPIFFile != nullptr && m_pRunApl != nullptr )
    {
        window_title.append(" (")
                    .append(GetWindowTitle());

        const std::string data_source_name = m_pRunApl->GetInputRepository()->GetName(DataRepositoryNameType::Concise);

        if( !data_source_name.empty() )
        {
            window_title.append(" - Data: ")
                        .append(data_source_name);
        }

        window_title.push_back(')');
    }

    return window_title;
}


std::string CEntryrunDoc::GetWindowTitle() const
{
    return ( m_windowTitleOverride != nullptr && m_appMode != NO_MODE ) ? *m_windowTitleOverride :
           ( m_pPIFFile != nullptr )                                    ? UTF8_TODO::GetUtf8(m_pPIFFile->GetEvaluatedAppDescription(true)) :
                                                                          std::string();
}


void CEntryrunDoc::SetWindowTitle(std::string window_title)
{
    m_windowTitleOverride = std::make_unique<std::string>(std::move(window_title));
}


void CEntryrunDoc::OpenOperatorStatisticsLog()
{
    Application* const application = m_pPIFFile->GetApplication();
    ASSERT(application != nullptr);

    if( !application->GetCreateLogFile() )
        return;

    if( m_pOperatorStatisticsLog != nullptr )
    {
        if( m_pOperatorStatisticsLog->GetCurrentStatsObj() != nullptr )
            m_pOperatorStatisticsLog->Save();

        delete m_pOperatorStatisticsLog;
    }

    // base the log file path on the the application name when the data source is not file-based
    std::string log_file_path = m_pRunApl->GetInputRepository()->GetConnectionString().HasFilePath() ?
                                m_pRunApl->GetInputRepository()->GetConnectionString().GetFilePath() :
                                Path::RemoveExtension(UTF8_TODO::GetUtf8(m_pPIFFile->GetAppFName()));

    Path::MakeAppendExtension(log_file_path, FileExtensions::OperatorStatistics);

    m_pOperatorStatisticsLog = new COperatorStatisticsLog();

    m_pOperatorStatisticsLog->Open(std::move(log_file_path));
}
