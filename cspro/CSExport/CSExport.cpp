#include "StdAfx.h"
#include "CSExport.h"
#include "ExptDoc.h"
#include "ExptView.h"
#include "MainFrm.h"
#include <zUtilO/CommandLineParsers.h>
#include <zUtilO/imsaDlg.H>
#include <zUtilF/CommonControls.h>


/////////////////////////////////////////////////////////////////////////////
// CExportApp

BEGIN_MESSAGE_MAP(CExportApp, CWinApp)
    //{{AFX_MSG_MAP(CExportApp)
    ON_COMMAND(ID_APP_ABOUT, OnAppAbout)
    ON_COMMAND(ID_FILE_OPEN, OnFileOpen)
    //}}AFX_MSG_MAP
    // Standard file based document commands
    ON_COMMAND(ID_FILE_NEW, CWinApp::OnFileNew)
    ON_COMMAND(ID_FILE_OPEN, CWinApp::OnFileOpen)
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CExportApp construction

CExportApp::CExportApp()
    :   m_pExportDoc(nullptr)
{
    InitializeCSProEnvironment();

    EnableHtmlHelp();
}


/////////////////////////////////////////////////////////////////////////////
// The one and only CExportApp object

CExportApp theApp;

/////////////////////////////////////////////////////////////////////////////
// CExportApp initialization

BOOL CExportApp::InitInstance()
{
    // 20130222 start of code added to prevent crashes (upon loading a file at start time) on non-XP machines
    // see here: http://stackoverflow.com/questions/6633515/mfc-app-assert-fail-at-crecentfilelistadd-on-command-line-fileopen

    InitializeCommonControls();

    __super::InitInstance();

    // Initialize OLE libraries
    if (!AfxOleInit())
        return FALSE;

    // 20130222 end of code


    AfxEnableControlContainer();

    // Standard initialization
    // If you are not using these features and wish to reduce the size
    //  of your final executable, you should remove from the following
    //  the specific initialization routines you do not need.

    // Change the registry key under which our settings are stored.
    // TODO: You should modify this string to be something appropriate
    // such as the name of your company or organization.
    SetRegistryKey(_T("U.S. Census Bureau"));

    LoadStdProfileSettings(8);  // Load standard INI file options (including MRU)

    // Register the application's document templates.  Document templates
    //  serve as the connection between documents, frame windows and views.

    CSingleDocTemplate* pDocTemplate;
    pDocTemplate = new CSingleDocTemplate(
        IDR_MAINFRAME,
        RUNTIME_CLASS(CExportDoc),
        RUNTIME_CLASS(CMainFrame),       // main SDI frame window
        RUNTIME_CLASS(CExportView));
    AddDocTemplate(pDocTemplate);

    // Parse command line for standard shell commands, DDE, file open
    ConnectionStringCommandLineParser command_line_parser(&m_connectionStringFileSimulator);
    ParseCommandLine(command_line_parser);

    // Dispatch commands specified on the command line
    if (!ProcessShellCommand(command_line_parser)){
        return FALSE;
    }

    m_pMainWnd->MoveWindow(0, 0, 720, 660); // 20090901 moved from OnFileOpen method
    m_pMainWnd->CenterWindow(); // to prohibit multiple resizes during program execution

    // Dispatch commands specified on the command line
    switch( command_line_parser.m_nShellCommand )
    {
        case CCommandLineInfo::FileNew:
            OnFileOpen();
            break;
        case CCommandLineInfo::FileOpen:
            OpenDocumentFile(command_line_parser.m_strFileName);
            ManageLanguageDlgBar();
            break;
        default:
            if (!ProcessShellCommand(command_line_parser)) {
                return FALSE;
            }
    }

    CMainFrame* pmainframe = (CMainFrame*)m_pMainWnd;
    pmainframe->m_menu.LoadMenu(IDR_MAINFRAME);
    pmainframe->m_menu.LoadToolbar(IDR_MAINFRAME);
    pmainframe->m_hMenuDefault = pmainframe->m_menu.Detach();
    pmainframe->OnUpdateFrameMenu(pmainframe->m_hMenuDefault);

    ASSERT(pDocTemplate);

    //  UPON APPLICATION LAUNCH: CExportDoc is not available until this point
    //  in the code.  Before this point, any reference to CExportDoc* will be nullptr.
    //  This is the first place ManageLanguageDlgBar can be successfuly called.
    //  For subsequent File Open calls, CExportDoc* is saved and reused.
    //  That is why ManageLanguageDlgBar() is called after OpenDocumentFile() and
    //  at the end of CExportApp::OnFileOpen().
    POSITION pos           = pDocTemplate->GetFirstDocPosition();
    CExportDoc* pExportDoc = DYNAMIC_DOWNCAST(CExportDoc,pDocTemplate->GetNextDoc(pos));
    m_pExportDoc           = pExportDoc;
    ManageLanguageDlgBar();

    if (command_line_parser.m_nShellCommand == CCommandLineInfo::FileOpen) {
        pmainframe->SetActiveView(pExportDoc->m_pTreeView);
        pmainframe->InitialUpdateFrame(pExportDoc, TRUE);
    }


    if(pExportDoc && pExportDoc->m_batchmode){//Document and batch mode
        m_pMainWnd->ShowWindow(SW_HIDE);
        pExportDoc->GetTreeView()->SendMessage(UWM::CSExport::RefreshView);
        pExportDoc->RunBatch();
        return TRUE;
    }

    // The one and only window has been initialized, so show and update it.
    m_pMainWnd->ShowWindow(SW_SHOW);
    m_pMainWnd->UpdateWindow();

    return TRUE;
}

// App command to run the dialog
void CExportApp::OnAppAbout()
{
    CIMSAAboutDlg about_dlg(WindowsWS::LoadString(AFX_IDS_APP_TITLE), LoadIcon(IDR_MAINFRAME));
    about_dlg.DoModal();
}

/////////////////////////////////////////////////////////////////////////////
// CExportApp message handlers


void CExportApp::OnFileOpen()
{
    OpenFileDlg open_file_dlg(0, L"dcf, exf, csdb, csdbe", AfxGetApp()->GetProfileString(L"Settings", L"Last Open", L""),
                              L"Export Specification, Data Dictionary, or CSPro DB Files|*.dcf;*.exf;*.csdb;*.csdbe|All Files (*.*)|*.*||");
    open_file_dlg.SetTitle(L"Open Export, Dictionary, or Data File");

    if( open_file_dlg.DoModal() != IDOK )
        return;

    const std::wstring wide_file_path = TC::ToWide(open_file_dlg.GetFilePath());

    AfxGetApp()->AddToRecentFileList(wide_file_path.c_str());

    OpenDocumentFile(wide_file_path.c_str());
    ManageLanguageDlgBar();
}


void CExportApp::DeletePifInfos()
{
    int iCount = m_arrPifInfo.GetSize();
    for(int iIndex =0; iIndex < iCount ; iIndex ++) {
        delete m_arrPifInfo[iIndex];
    }
    m_arrPifInfo.RemoveAll();
}


void CExportApp::ManageLanguageDlgBar() const
{
    if( m_pExportDoc == nullptr )
        return;

    CMainFrame* pMainFrame = assert_cast<CMainFrame*>(m_pMainWnd);
    const CDataDict* dictionary = m_pExportDoc->GetDataDict();

    bool show_language_bar = ( dictionary != nullptr && dictionary->GetLanguages().size() > 1 );

    if( show_language_bar )
        pMainFrame->GetLangDlgBar().UpdateLanguageList(*dictionary);

    pMainFrame->GetReBar().GetReBarCtrl().ShowBand(1, show_language_bar);
}
