#include "StdAfx.h"
#include "CSCode.h"
#include <zUtilO/imsaDlg.H>
#include <zUtilF/CommonControls.h>
#include <zUtilF/MDIFrameWndHelpers.h>


namespace
{
    // The one and only CSCodeApp object
    CSCodeApp theApp;
}


BEGIN_MESSAGE_MAP(CSCodeApp, CWinAppEx)
    ON_COMMAND(ID_FILE_NEW, OnFileNew)
    ON_COMMAND(ID_APP_ABOUT, OnAppAbout)
    ON_COMMAND(ID_HELP_FINDER, OnHelpFinder)
    ON_COMMAND(ID_HELP, OnHelp)
END_MESSAGE_MAP()


CSCodeApp::CSCodeApp()
{
    InitializeCSProEnvironment();

    EnableHtmlHelp();
}


BOOL CSCodeApp::InitInstance()
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

    LoadStdProfileSettings(_AFX_MRU_MAX_COUNT);  // Load standard INI file options (including MRU)

    InitContextMenuManager();

    // Register the application's document templates.  Document templates
    //  serve as the connection between documents, frame windows and views.
    CMultiDocTemplate* pDocTemplate = new CMultiDocTemplate(
        IDR_CODEFRAME,
        RUNTIME_CLASS(CodeDoc),
        RUNTIME_CLASS(CodeFrame),
        RUNTIME_CLASS(CodeView));

    if( pDocTemplate == nullptr )
        return FALSE;

    AddDocTemplate(pDocTemplate);

    // create main MDI Frame window
    CMainFrame* pMainFrame = new CMainFrame;

    if( pMainFrame == nullptr || !pMainFrame->LoadFrame(IDR_MAINFRAME) )
        return FALSE;

    m_pMainWnd = pMainFrame;

    // allow drag-and-drop of files
    pMainFrame->DragAcceptFiles();

    // open any files specified on the command line
    bool file_opened = false;

    for( const std::wstring& path : GetPathsFromCommandLine() )
    {
        if( OpenDocumentFile(path.c_str()) )
            file_opened = true;
    }

    // if no file was opened, create a new file
    if( !file_opened )
        OnFileNew();

    // The main window has been initialized, so show and update it
    pMainFrame->ShowWindow(m_nCmdShow);
    pMainFrame->UpdateWindow();

    return TRUE;
}


CDocument* CSCodeApp::OpenDocumentFile(LPCTSTR lpszFileName)
{
    return MDIFrameWndHelpers::OpenDocumentFileAndCloseSingleUnmodifiedPathlessDocument<CWinAppEx>(*this, lpszFileName);
}


CDocument* CSCodeApp::OpenDocumentFile(LPCTSTR lpszFileName, BOOL bAddToMRU)
{
    return MDIFrameWndHelpers::OpenDocumentFileAndCloseSingleUnmodifiedPathlessDocument<CWinAppEx>(*this, lpszFileName, bAddToMRU);
}


void CSCodeApp::OnAppAbout()
{
    CIMSAAboutDlg about_dlg(WindowsWS::LoadString(AFX_IDS_APP_TITLE), LoadIcon(IDR_MAINFRAME));
    about_dlg.DoModal();
}
