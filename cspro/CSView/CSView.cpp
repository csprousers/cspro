#include "StdAfx.h"
#include "CSView.h"
#include "ViewDoc.h"
#include "ViewView.h"
#include <zUtilO/ImsaDlg.h>
#include <zUtilF/CommonControls.h>


namespace
{
    // The one and only CSViewApp object
    CSViewApp theApp;
}


BEGIN_MESSAGE_MAP(CSViewApp, CWinAppEx)
    ON_COMMAND(ID_FILE_OPEN, OnFileOpen)
    ON_COMMAND(ID_APP_ABOUT, OnAppAbout)
    ON_COMMAND(ID_HELP_FINDER, OnHelpFinder)
    ON_COMMAND(ID_HELP, OnHelp)
END_MESSAGE_MAP()


CSViewApp::CSViewApp()
{
    InitializeCSProEnvironment();

    EnableHtmlHelp();
}


BOOL CSViewApp::InitInstance()
{
    InitializeCommonControls();

    __super::InitInstance();

    // Initialize OLE libraries
    if( !AfxOleInit() )
        return FALSE;

    AfxEnableControlContainer();

    // Standard initialization
    // If you are not using these features and wish to reduce the size
    //  of your final executable, you should remove from the following
    //  the specific initialization routines you do not need.

    // Change the registry key under which our settings are stored.
    SetRegistryKey(L"U.S. Census Bureau");

    LoadStdProfileSettings();  // Load standard INI file options (including MRU)

    InitContextMenuManager();

    // Register the application's document templates.  Document templates
    //  serve as the connection between documents, frame windows and views.
    CSingleDocTemplate* const pDocTemplate = new CSingleDocTemplate(
        IDR_MAINFRAME,
        RUNTIME_CLASS(ViewDoc),
        RUNTIME_CLASS(CMainFrame),       // main SDI frame window
        RUNTIME_CLASS(ViewView));

    if( pDocTemplate == nullptr )
        return FALSE;

    AddDocTemplate(pDocTemplate);

    // Parse command line for standard shell commands, DDE, file open
    CCommandLineInfo cmdInfo;
    ParseCommandLine(cmdInfo);

    // create a new document if no document was specified on the command line, or if it could not be opened
    if( cmdInfo.m_nShellCommand != CCommandLineInfo::FileOpen ||
        OpenDocumentFile(cmdInfo.m_strFileName) == nullptr )
    {
        OnFileNew();
    }

    if( m_pMainWnd == nullptr )
        return FALSE;

    CMainFrame* const pMainFrame = assert_cast<CMainFrame*>(m_pMainWnd);

    // The main window has been initialized, so show and update it
    pMainFrame->ShowWindow(m_nCmdShow);
    pMainFrame->UpdateWindow();

    return TRUE;
}


void CSViewApp::OnAppAbout()
{
    CIMSAAboutDlg about_dlg(WindowsWS::LoadString(AFX_IDS_APP_TITLE), LoadIcon(IDR_MAINFRAME));
    about_dlg.DoModal();
}
