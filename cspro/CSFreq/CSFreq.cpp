// CSFreq.cpp : Defines the class behaviors for the application.
//

#include "StdAfx.h"
#include "CSFreq.h"
#include "FreqDoc.h"
#include "FreqView.h"
#include "MainFrm.h"
#include <zUtilO/imsaDlg.H>
#include <zUtilO/Filedlg.h>

/////////////////////////////////////////////////////////////////////////////
// CSFreqApp

BEGIN_MESSAGE_MAP(CSFreqApp, CWinApp)
    ON_COMMAND(ID_APP_ABOUT, OnAppAbout)
    ON_COMMAND(ID_FILE_OPEN, OnFileOpen)
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CSFreqApp construction

CSFreqApp::CSFreqApp()
    :   m_hIcon(nullptr),
        m_iReturnCode(0)
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
    CWinApp::InitInstance();

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
    SetRegistryKey(_T("U.S. Census Bureau"));

    LoadStdProfileSettings(8);  // Load standard INI file options (including MRU)

    m_csModuleName = _T("CSPro Tabulate Frequencies");
    m_hIcon = LoadIcon(IDR_MAINFRAME);

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
    CCommandLineInfo cmdInfo;
    ParseCommandLine(cmdInfo);

    // Dispatch commands specified on the command line
    if (!ProcessShellCommand(cmdInfo))
        return FALSE;

    // size and center the window
    m_pMainWnd->MoveWindow(0, 0, 800, 700);
    m_pMainWnd->CenterWindow();

    // Dispatch commands specified on the command line
    if( cmdInfo.m_nShellCommand == CCommandLineInfo::FileNew )
        OnFileOpen();

    else if( cmdInfo.m_nShellCommand == CCommandLineInfo::FileOpen )
    {
        OpenDocumentFile(cmdInfo.m_strFileName);
        ManageLanguageDlgBar();
    }

    else if( !ProcessShellCommand(cmdInfo) )
        return FALSE;


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
        AfxMessageBox(_T("Failed to run CSFreq"));

    // The one and only window has been initialized, so show and update it.
    m_pMainWnd->ShowWindow(SW_SHOW);
    m_pMainWnd->UpdateWindow();

    return TRUE;
}


// App command to run the dialog
void CSFreqApp::OnAppAbout()
{
    CIMSAAboutDlg dlg;
    dlg.m_hIcon = m_hIcon;
    dlg.m_csModuleName = m_csModuleName;
    dlg.DoModal();
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
    CIMSAString csLastDict = AfxGetApp()->GetProfileString(_T("Settings"),_T("Last Open"),_T(""));
    CIMSAString csFilter;
    csFilter = _T("Frequency Specification, Data Dictionary, or CSPro DB Files|*.dcf;*.fqf;*.csdb;*.csdbe|All Files (*.*)|*.*||");
    CIMSAFileDialog dlgFile(TRUE, _T("dcf, fqf, csdb, csdbe"), csLastDict, OFN_HIDEREADONLY, csFilter);
    dlgFile.m_ofn.lpstrTitle = _T("Open Frequency, Dictionary, or Data File");
    if (dlgFile.DoModal() == IDOK) {
        AfxGetApp()->AddToRecentFileList(dlgFile.GetPathName());
        OpenDocumentFile(dlgFile.GetPathName());

        ManageLanguageDlgBar();
    }
}


void CSFreqApp::ManageLanguageDlgBar()
{
    CMainFrame* pMainFrame = assert_cast<CMainFrame*>(m_pMainWnd);
    CSFreqDoc* pDocument = assert_cast<CSFreqDoc*>(pMainFrame->GetActiveDocument());
    const CDataDict* dictionary = pDocument->GetDataDict();
    ASSERT(dictionary != nullptr);

    bool show_language_bar = ( dictionary->GetLanguages().size() > 1 );

    if( show_language_bar )
        pMainFrame->GetLangDlgBar().UpdateLanguageList(*dictionary);

    pMainFrame->GetReBar().GetReBarCtrl().ShowBand(1, show_language_bar);
}
