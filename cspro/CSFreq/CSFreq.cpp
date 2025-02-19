#include "StdAfx.h"
#include "CSFreq.h"
#include "FreqDoc.h"
#include "FreqView.h"
#include "MainFrm.h"
#include <zUtilO/CommandLineParsers.h>
#include <zUtilO/imsaDlg.H>
#include <zUtilF/CommonControls.h>


/////////////////////////////////////////////////////////////////////////////
// CSFreqApp

BEGIN_MESSAGE_MAP(CSFreqApp, CWinApp)
    ON_COMMAND(ID_APP_ABOUT, OnAppAbout)
    ON_COMMAND(ID_FILE_OPEN, OnFileOpen)
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CSFreqApp construction

CSFreqApp::CSFreqApp()
    :   m_iReturnCode(0)
{
    InitializeCSProEnvironment();

    EnableHtmlHelp();
}


/////////////////////////////////////////////////////////////////////////////
// The one and only CSFreqApp object

CSFreqApp theApp;

/////////////////////////////////////////////////////////////////////////////
// CSFreqApp initialization


BOOL CSFreqApp::InitInstance()
{
    InitializeCommonControls();

    __super::InitInstance();

    // Initialize OLE libraries
    if (!AfxOleInit())
        return FALSE;

    AfxEnableControlContainer();

    // Standard initialization
    // If you are not using these features and wish to reduce the size
    //  of your final executable, you should remove from the following
    //  the specific initialization routines you do not need.

    // Change the registry key under which our settings are stored.
    // TODO: You should modify this string to be something appropriate
    // such as the name of your company or organization.
    SetRegistryKey(L"U.S. Census Bureau");

    LoadStdProfileSettings(8);  // Load standard INI file options (including MRU)

    // Register the application's document templates.  Document templates
    //  serve as the connection between documents, frame windows and views.

    CSingleDocTemplate* pDocTemplate;
    pDocTemplate = new CSingleDocTemplate(
        IDR_MAINFRAME,
        RUNTIME_CLASS(CSFreqDoc),
        RUNTIME_CLASS(CMainFrame),       // main SDI frame window
        RUNTIME_CLASS(CSFreqView));
    AddDocTemplate(pDocTemplate);

    // Register file type
    EnableShellOpen();
    RegisterShellFileTypes(TRUE);

    // Parse command line for standard shell commands, DDE, file open
    ConnectionStringCommandLineParser command_line_parser(&m_connectionStringFileSimulator);
    ParseCommandLine(command_line_parser);

    // Dispatch commands specified on the command line
    if (!ProcessShellCommand(command_line_parser))
        return FALSE;

    // size and center the window
    m_pMainWnd->MoveWindow(0, 0, 800, 700);
    m_pMainWnd->CenterWindow();

    // Dispatch commands specified on the command line
    if( command_line_parser.m_nShellCommand == CCommandLineInfo::FileNew )
    {
        OnFileOpen();
    }

    else if( command_line_parser.m_nShellCommand == CCommandLineInfo::FileOpen )
    {
        OpenDocumentFile(command_line_parser.m_strFileName);
        ManageLanguageDlgBar();
    }

    else if( !ProcessShellCommand(command_line_parser) )
    {
        return FALSE;
    }

    CMainFrame* pmainframe = (CMainFrame*)m_pMainWnd;

    pmainframe->m_menu.LoadMenu(IDR_MAINFRAME);
    pmainframe->m_menu.LoadToolbar(IDR_MAINFRAME);
    pmainframe->m_hMenuDefault = pmainframe->m_menu.Detach();
    pmainframe->OnUpdateFrameMenu(pmainframe->m_hMenuDefault);

    // when m_iReturnCode is 1 it means that a PFF is being run, so the
    // main window can be hidden and the program can close after execution
    if( m_iReturnCode == 1 )
    {
        CSFreqDoc* pDoc = (CSFreqDoc*)pmainframe->GetActiveDocument();
        m_pMainWnd->ShowWindow(SW_HIDE);
        pDoc->RunBatch();
        return FALSE;
    }

    else if( m_iReturnCode == 8 )
    {
        AfxMessageBox(L"Failed to run CSFreq");
    }

    // The one and only window has been initialized, so show and update it.
    m_pMainWnd->ShowWindow(SW_SHOW);
    m_pMainWnd->UpdateWindow();

    return TRUE;
}


// App command to run the dialog
void CSFreqApp::OnAppAbout()
{
    CIMSAAboutDlg about_dlg(WindowsWS::LoadString(AFX_IDS_APP_TITLE), LoadIcon(IDR_MAINFRAME));
    about_dlg.DoModal();
}


/////////////////////////////////////////////////////////////////////////////
// CSFreqApp message handlers



/////////////////////////////////////////////////////////////////////////////////
//
//  void CSFreqApp::OnFileOpen()
//
/////////////////////////////////////////////////////////////////////////////////
void CSFreqApp::OnFileOpen()
{
    OpenFileDlg open_file_dlg(0, L"dcf, fqf, csdb, csdbe", AfxGetApp()->GetProfileString(L"Settings", L"Last Open", L""),
                              L"Frequency Specification, Data Dictionary, or CSPro DB Files|*.dcf;*.fqf;*.csdb;*.csdbe|All Files (*.*)|*.*||");
    open_file_dlg.SetTitle(L"Open Frequency, Dictionary, or Data File");

    if( open_file_dlg.DoModal() != IDOK )
        return;

    const std::wstring wide_file_path = TC::ToWide(open_file_dlg.GetFilePath());

    AfxGetApp()->AddToRecentFileList(wide_file_path.c_str());

    OpenDocumentFile(wide_file_path.c_str());
    ManageLanguageDlgBar();
}


void CSFreqApp::ManageLanguageDlgBar()
{
    CMainFrame* const pMainFrame = assert_cast<CMainFrame*>(m_pMainWnd);
    CSFreqDoc* const pDocument = assert_cast<CSFreqDoc*>(pMainFrame->GetActiveDocument());
    const CDataDict* const dictionary = pDocument->GetDataDict();
    ASSERT(dictionary != nullptr);

    const bool show_language_bar = ( dictionary->GetLanguages().size() > 1 );

    if( show_language_bar )
        pMainFrame->GetLangDlgBar().UpdateLanguageList(*dictionary);

    pMainFrame->GetReBar().GetReBarCtrl().ShowBand(1, show_language_bar);
}
