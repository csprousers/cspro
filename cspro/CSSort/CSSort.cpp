//***************************************************************************
//  File name: CSSort.cpp
//
//  Description:
//       CSSort implementation
//
//  History:    Date       Author   Comment
//              ---------------------------
//              21 Nov 00   bmd     Created for CSPro 2.1
//
//***************************************************************************

#include "StdAfx.h"
#include "CSSort.h"
#include "MainFrm.h"
#include "SortDoc.h"
#include "SortView.h"
#include <zUtilO/imsaDlg.H>


// The one and only CSortApp object
CSortApp theApp;


BEGIN_MESSAGE_MAP(CSortApp, CWinApp)
    ON_COMMAND(ID_FILE_OPEN, OnFileOpen)
    ON_COMMAND(ID_APP_ABOUT, OnAppAbout)
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CSortApp construction

CSortApp::CSortApp()
    :   m_iReturnCode(0)
{
    InitializeCSProEnvironment();

    EnableHtmlHelp();
}


BOOL CSortApp::InitInstance()
{
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
        RUNTIME_CLASS(CSortDoc),
        RUNTIME_CLASS(CMainFrame),       // main SDI frame window
        RUNTIME_CLASS(CSortView));
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

    // Dispatch commands specified on the command line
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

    // Add toolbar icons to menus
    CMainFrame* const pMainFrame = static_cast<CMainFrame*>(m_pMainWnd);

    pMainFrame->m_menu.LoadMenu(IDR_MAINFRAME);

    constexpr UINT toolbars[] = { IDR_MAINFRAME };
    pMainFrame->m_menu.LoadToolbars(toolbars,1);

    pMainFrame->m_hMenuDefault = pMainFrame->m_menu.Detach();
    pMainFrame->OnUpdateFrameMenu(pMainFrame->m_hMenuDefault);

    // The one and only window has been initialized, so show and update it.
    m_pMainWnd->ShowWindow(SW_SHOW);
    m_pMainWnd->UpdateWindow();

    return TRUE;
}


void CSortApp::OnFileOpen()
{
    OpenFileDlg open_file_dlg(0, L"dcf, ssf", AfxGetApp()->GetProfileString(L"Settings", L"Last Open", L""),
                              L"Sort Specification (*.ssf) or Data Dictionary (*.dcf) Files|*.dcf; *.ssf|"
                              L"Sort Specification Files (*.ssf)|*.ssf|"
                              L"Data Dictionary Files (*.dcf)|*.dcf|"
                              L"All Files (*.*)|*.*||");
    open_file_dlg.SetTitle(L"Open Sort Specification or Dictionary File");

    if( open_file_dlg.DoModal() != IDOK )
        return;

    const std::wstring wide_file_path = TC::ToWide(open_file_dlg.GetFilePath());

    AfxGetApp()->AddToRecentFileList(wide_file_path.c_str());

    OpenDocumentFile(wide_file_path.c_str());
}


int CSortApp::ExitInstance()
{
    __super::ExitInstance();

    return m_iReturnCode;
}


void CSortApp::OnAppAbout()
{
    CIMSAAboutDlg about_dlg(WindowsWS::LoadString(AFX_IDS_APP_TITLE), LoadIcon(IDR_MAINFRAME));
    about_dlg.DoModal();
}
