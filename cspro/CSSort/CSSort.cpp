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
#include <zUtilO/Filedlg.h>
#include <zUtilO/imsaDlg.H>
#include <zUtilO/Interapp.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CSortApp

BEGIN_MESSAGE_MAP(CSortApp, CWinApp)
    //{{AFX_MSG_MAP(CSortApp)
    ON_COMMAND(ID_APP_ABOUT, OnAppAbout)
        // NOTE - the ClassWizard will add and remove mapping macros here.
        //    DO NOT EDIT what you see in these blocks of generated code!
    //}}AFX_MSG_MAP
    // Standard file based document commands
    ON_COMMAND(ID_FILE_OPEN, OnFileOpen)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSortApp construction

CSortApp::CSortApp()
{
    InitializeCSProEnvironment();

    EnableHtmlHelp();
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CSortApp object

CSortApp theApp;


/////////////////////////////////////////////////////////////////////////////
//
//                          CSortApp::InitInstance
//
/////////////////////////////////////////////////////////////////////////////

BOOL CSortApp::InitInstance()
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

    m_csModuleName = _T("CSPro Sort Data");
    m_hIcon = LoadIcon(IDR_MAINFRAME);
    m_iReturnCode = 0;

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
    CMainFrame* pMainFrame = (CMainFrame*) m_pMainWnd;
    static UINT toolbars[]={
        IDR_MAINFRAME,
    };
    pMainFrame->m_menu.LoadMenu(IDR_MAINFRAME);
    pMainFrame->m_menu.LoadToolbars(toolbars,1);
    //->SetMenu(&pMainFrame->m_menu);
    pMainFrame->m_hMenuDefault = pMainFrame->m_menu.Detach();
    pMainFrame->OnUpdateFrameMenu(pMainFrame->m_hMenuDefault);

    // The one and only window has been initialized, so show and update it.
    m_pMainWnd->ShowWindow(SW_SHOW);
    m_pMainWnd->UpdateWindow();

    return TRUE;
}


/////////////////////////////////////////////////////////////////////////////
//
//                          CSortApp::OnFileOpen
//
/////////////////////////////////////////////////////////////////////////////

void CSortApp::OnFileOpen() {

    CString csLastDict = AfxGetApp()->GetProfileString(_T("Settings"),_T("Last Open"),_T(""));
    CString csFilter;
    csFilter = _T("Sort Specification (*.ssf) or Data Dictionary (*.dcf) Files|*.dcf; *.ssf|Sort Specification Files (*.ssf)|*.ssf|Data Dictionary Files (*.dcf)|*.dcf|All Files (*.*)|*.*||");
    CIMSAFileDialog dlgFile(TRUE, _T("dcf, ssf"), csLastDict, OFN_HIDEREADONLY, csFilter);
    dlgFile.m_ofn.lpstrTitle = _T("Open Data Sort or Dictionary File");
    if (dlgFile.DoModal() == IDOK) {
        AfxGetApp()->AddToRecentFileList(dlgFile.GetPathName());
        OpenDocumentFile(dlgFile.GetPathName());
    }
}


/////////////////////////////////////////////////////////////////////////////
//
//                             CSortApp::ExitInstance
//
/////////////////////////////////////////////////////////////////////////////

int CSortApp::ExitInstance()
{
    // specify the return code here
    CWinApp::ExitInstance();
    return m_iReturnCode;
}


/////////////////////////////////////////////////////////////////////////////
//
//                             CSortApp::OnAppAbout
//
/////////////////////////////////////////////////////////////////////////////

void CSortApp::OnAppAbout() {

    CIMSAAboutDlg dlg;
    dlg.m_hIcon = m_hIcon;
    dlg.m_csModuleName = m_csModuleName;
    dlg.DoModal();
}
