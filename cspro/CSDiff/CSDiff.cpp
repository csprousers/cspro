#include "StdAfx.h"
#include "CSDiff.h"
#include "Csdfdoc.h"
#include "Csdfview.h"
#include "MainFrm.h"
#include <zUtilO/Filedlg.h>
#include <zUtilO/imsaDlg.H>


// The one and only CCSDiffApp object
CCSDiffApp theApp;


BEGIN_MESSAGE_MAP(CCSDiffApp, CWinApp)
    ON_COMMAND(ID_APP_ABOUT, OnAppAbout)
    ON_COMMAND(ID_FILE_OPEN, OnFileOpen)
END_MESSAGE_MAP()


CCSDiffApp::CCSDiffApp()
    :   m_hIcon(nullptr)
{
    InitializeCSProEnvironment();

    EnableHtmlHelp();
}


BOOL CCSDiffApp::InitInstance()
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

    m_hIcon = LoadIcon(IDR_MAINFRAME);

    // Register the application's document templates.  Document templates
    //  serve as the connection between documents, frame windows and views.

    CSingleDocTemplate* pDocTemplate;
    pDocTemplate = new CSingleDocTemplate(
        IDR_MAINFRAME,
        RUNTIME_CLASS(CCSDiffDoc),
        RUNTIME_CLASS(CMainFrame),       // main SDI frame window
        RUNTIME_CLASS(CCSDiffView));
    AddDocTemplate(pDocTemplate);

    // Parse command line for standard shell commands, DDE, file open
    CCommandLineInfo cmdInfo;
    ParseCommandLine(cmdInfo);

    // Dispatch commands specified on the command line
    if (!ProcessShellCommand(cmdInfo))
        return FALSE;

    switch(cmdInfo.m_nShellCommand)
    {
        case CCommandLineInfo::FileNew:
            OnFileOpen();
            break;
        case CCommandLineInfo::FileOpen:
            OpenDocumentFile(cmdInfo.m_strFileName);
            break;
        default:
            if (!ProcessShellCommand(cmdInfo)) {
                return FALSE;
            }
    }

    CMainFrame* pMainFrame = static_cast<CMainFrame*>(m_pMainWnd);
    pMainFrame->GetMenu().LoadMenu(IDR_MAINFRAME);
    pMainFrame->GetMenu().LoadToolbar(IDR_MAINFRAME);
    pMainFrame->m_hMenuDefault = pMainFrame->GetMenu().Detach();
    pMainFrame->OnUpdateFrameMenu(pMainFrame->m_hMenuDefault);

    // The one and only window has been initialized, so show and update it.
    m_pMainWnd->ShowWindow(SW_SHOW);
    m_pMainWnd->UpdateWindow();

    return TRUE;
}


void CCSDiffApp::OnAppAbout()
{
    CIMSAAboutDlg about_dlg(WindowsWS::LoadString(AFX_IDS_APP_TITLE), m_hIcon);
    about_dlg.DoModal();
}


void CCSDiffApp::OnFileOpen()
{
    CIMSAFileDialog file_dlg(TRUE, _T("dcf, cmp"),
                             AfxGetApp()->GetProfileString(_T("Settings"),_T("Last Open"),_T("")),
                             OFN_HIDEREADONLY,
                             _T("Compare Specification (*.cmp) or Data Dictionary (*.dcf) Files|*.dcf;*.cmp|")
                             _T("Compare Specification Files (*.cmp)|*.cmp|")
                             _T("Data Dictionary Files (*.dcf)|*.dcf|")
                             _T("All Files (*.*)|*.*||"));

    file_dlg.m_ofn.lpstrTitle = _T("Open Compare Specification or Dictionary File");

    if( file_dlg.DoModal() == IDOK )
    {
        AfxGetApp()->AddToRecentFileList(file_dlg.GetPathName());
        OpenDocumentFile(file_dlg.GetPathName());
    }
}
