// TblView.cpp : Defines the class behaviors for the application.
//

#include "StdAfx.h"
#include "TblView.h"
#include "MainFrm.h"
#include "TableChartWnd.h"
#include "TblDoc.h"
#include <zUtilO/imsaDlg.H>
#include <zUtilO/Filedlg.h>
#include <zTableO/Table.h>
#include <zTableF/TabChWnd.h>
#include <zTableF/TabView.h>
#include <zTableF/TabDoc.h>
#include <fstream>


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CTblViewApp

BEGIN_MESSAGE_MAP(CTblViewApp, CWinApp)
    //{{AFX_MSG_MAP(CTblViewApp)
    ON_COMMAND(ID_APP_ABOUT, OnAppAbout)
    ON_COMMAND(ID_FILE_NEW, OnFileNew)
    ON_COMMAND(ID_VIEW_FULLSCREEN, OnFullscreen)
    ON_UPDATE_COMMAND_UI(ID_VIEW_FULLSCREEN, OnUpdateFullscreen)
    //}}AFX_MSG_MAP
    // Standard file based document commands
    ON_COMMAND(ID_FILE_NEW, CWinApp::OnFileNew)
    ON_COMMAND(ID_FILE_OPEN, CTblViewApp::OnFileOpen)
    // Standard print setup command
    ON_COMMAND(ID_FILE_PRINT_SETUP, CWinApp::OnFilePrintSetup)
    ON_COMMAND(ID_FILE_OPEN, OnFileOpen)
    ON_COMMAND(ID_FILE_SAVE_AS_VIEWER, OnFileSaveAs)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CTblViewApp construction

CTblViewApp::CTblViewApp()
{
    EnableHtmlHelp();
    m_bCalledAsChild = FALSE;
    // TODO: add construction code here,
    // Place all significant initialization in InitInstance
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CTblViewApp object

CTblViewApp theApp;

/////////////////////////////////////////////////////////////////////////////
// CTblViewApp initialization
class CTblViewCommandLineInfo : public CIMSACommandLineInfo  {
public:
    enum ExportFmt {EXP_FMT_NONE, EXP_FMT_HTML, EXPT_FMT_RTF, EXPT_FMT_TABDELIM, EXPT_FMT_ALL};
    ExportFmt m_fmt;
    bool m_bExportOnly;
    CString m_sExptFilename;

public:

    CTblViewCommandLineInfo() : m_bExportOnly(FALSE), m_fmt(EXP_FMT_NONE), CIMSACommandLineInfo() { }
    virtual void ParseParam(const TCHAR* pszParam, BOOL bFlag, BOOL bLast)
    {
        if (bFlag && !_tcscmp(pszParam, _T("EXPT"))) {
            m_bExportOnly = true;

        }
        else if (m_bExportOnly && m_fmt == EXP_FMT_NONE) {
            if (!_tcscmp(pszParam, _T("HTML"))) {
                m_fmt = EXP_FMT_HTML;
            }
            else if (!_tcscmp(pszParam, _T("RTF"))) {
                m_fmt = EXPT_FMT_RTF;
            }
            else if (!_tcscmp(pszParam, _T("TABDELIM"))) {
                m_fmt = EXPT_FMT_TABDELIM;
            }
            else if (!_tcscmp(pszParam, _T("ALL"))) { // GHM 20100224 added because I didn't get josh's file with this in it
                m_fmt = EXPT_FMT_ALL;
            }
            else {
                AfxMessageBox(_T("Error: invalid file format for export"));
                exit(-1);
            }
        }
        else if (m_bExportOnly && m_fmt != EXP_FMT_NONE && m_sExptFilename.IsEmpty()) {
            m_sExptFilename = pszParam;
        }
        else {
            CIMSACommandLineInfo::ParseParam(pszParam, bFlag, bLast);
        }
        if (bLast) {
            CCommandLineInfo::ParseLast(bLast);
        }
    }
};

BOOL CTblViewApp::InitInstance()
{
	// InitCommonControlsEx() is required on Windows XP if an application
	// manifest specifies use of ComCtl32.dll version 6 or later to enable
	// visual styles.  Otherwise, any window creation will fail.
	INITCOMMONCONTROLSEX InitCtrls;
	InitCtrls.dwSize = sizeof(InitCtrls);
	// Set this to include all the common control classes you want to use
	// in your application.
	InitCtrls.dwICC = ICC_WIN95_CLASSES;
	InitCommonControlsEx(&InitCtrls);

    CWinApp::InitInstance();

    // Initialize OLE libraries
    if( !AfxOleInit() )
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

    LoadStdProfileSettings(16);  // Load standard INI file options (including MRU)

    m_csModuleName = _T("CSPro Table Viewer");
    m_csWndClassName = IMSA_WNDCLASS_TABLEVIEW;
    // Register the application's document templates.  Document templates
    //  serve as the connection between documents, frame windows and views.

    CMultiDocTemplate* pDocTemplate;
    m_hIcon = LoadIcon(IDR_MAINFRAME);

    //Template for the tables
    CMultiDocTemplate* pTableTemplate = pDocTemplate = new CMultiDocTemplate(
        IDR_TBLVWERVW_FRAME,
        RUNTIME_CLASS(CTabulateDoc),
        RUNTIME_CLASS(TableChartWnd), // custom MDI child frame
        RUNTIME_CLASS(CTabView));
    AddDocTemplate(pTableTemplate);

    // Parse command line for standard shell commands, DDE, file open
    CTblViewCommandLineInfo cmdInfo;
    ParseCommandLine(cmdInfo);


    // If instance already exists, open files in it.   // BMD 03 Mar 2003
    CWnd* pPrevInstance = CWnd::FindWindow(m_csWndClassName, NULL);
    if (pPrevInstance && !cmdInfo.m_bExportOnly) {
        for (int i=0 ; i<cmdInfo.m_acsFileName.GetSize() ; i++) {
            IMSASendMessage(IMSA_WNDCLASS_TABLEVIEW, WM_IMSA_FILEOPEN, cmdInfo.m_acsFileName[i]);
        }
        return FALSE;
    }
    // Enable DDE Execute open
    EnableShellOpen();
    RegisterShellFileTypes(TRUE);

    // create main MDI Frame window
    CMainFrame* pMainFrame = new CMainFrame;
    if (!pMainFrame->LoadFrame(IDR_MAINFRAME))
        return FALSE;
    m_pMainWnd = pMainFrame;

    // Enable drag/drop open
    m_pMainWnd->DragAcceptFiles();


    pDocTemplate->m_hMenuShared = pMainFrame->TblViewMenu();
    pMainFrame->m_hMenuDefault = pMainFrame->DefaultMenu();
    pMainFrame->OnUpdateFrameMenu(pMainFrame->m_hMenuDefault);

    // Dispatch commands specified on the command line
    switch(cmdInfo.m_nShellCommand) {
    case CCommandLineInfo::FileNew:
        OnFileOpen();
        break;
    case CCommandLineInfo::FileOpen:
        for (int i=0 ; i<cmdInfo.m_acsFileName.GetSize() ; i++) {
            OpenDocumentFile(cmdInfo.m_acsFileName[i]);
        }
        break;
    default:
        if (!ProcessShellCommand(cmdInfo)) {
            return FALSE;
        }
    }
    m_bCalledAsChild = cmdInfo.m_bChildApp;
    if (m_bCalledAsChild) {
        // we are dependent ... show the quick quit toolbar button
        pMainFrame->m_pWndTabTBar->GetToolBarCtrl().HideButton(ID_QUICK_QUIT, FALSE);
    }

    // The main window has been initialized, so show and update it.
    pMainFrame->ShowWindow(m_nCmdShow);
    pMainFrame->UpdateWindow();
    pMainFrame->SendMessage(WM_IMSA_SETFOCUS);
    if (cmdInfo.m_bExportOnly) {
        int processNum = 0; // GHM 20100224 to allow export all

        do
        {
            CSpecFile specFile;
            CString exportFilename = cmdInfo.m_sExptFilename;

            if( cmdInfo.m_fmt == CTblViewCommandLineInfo::EXPT_FMT_ALL || processNum )
            {
                switch( processNum )
                {
                case 0:
                    cmdInfo.m_fmt = CTblViewCommandLineInfo::EXP_FMT_HTML;
                    processNum = 1;
                    exportFilename.Append(FileExtensions::WithDot::HTML);
                    break;
                case 1:
                    cmdInfo.m_fmt = CTblViewCommandLineInfo::EXPT_FMT_RTF;
                    processNum = 2;
                    exportFilename.Append(_T(".rtf"));
                    break;
                case 2:
                    cmdInfo.m_fmt = CTblViewCommandLineInfo::EXPT_FMT_TABDELIM;
                    processNum = 0; // we're done after this export
                    exportFilename.Append(_T(".txt"));
                    break;
                }
            }

            if( cmdInfo.m_fmt == CTblViewCommandLineInfo::EXPT_FMT_RTF )
                specFile.SetEncoding(Encoding::Ansi); // GHM 20120207 rtf files should be in ANSI

            if (!specFile.Open(exportFilename, CFile::modeWrite)) {
                CString sError;
                sError = _T("Error:  Could not open file ") + cmdInfo.m_sExptFilename;
                AfxMessageBox(sError,MB_ICONEXCLAMATION);
            }
            else {
                POSITION pos = pTableTemplate->GetFirstDocPosition();
                CDocument* pDoc = pTableTemplate->GetNextDoc(pos);
                ASSERT_VALID(pDoc);
                pos = pDoc->GetFirstViewPosition();
                CView* pView = pDoc->GetNextView(pos);
                ASSERT_VALID(pView);
                CTabView* pTabView = DYNAMIC_DOWNCAST(CTabView, pView);
                ASSERT_VALID(pTabView);

                _tofstream os(specFile.m_pStream);

                switch (cmdInfo.m_fmt) {
                    case CTblViewCommandLineInfo::EXP_FMT_HTML:
                        pTabView->GetGrid()->PutHTMLTable(os, false);
                        break;
                    case CTblViewCommandLineInfo::EXPT_FMT_RTF:
                        pTabView->GetGrid()->PutRTFTable(os, false, true);
                        break;
                    case CTblViewCommandLineInfo::EXPT_FMT_TABDELIM:
                        pTabView->GetGrid()->PutASCIITable(os, false);
                        break;
                }

                specFile.Close();
            }
        } while( processNum );

        pMainFrame->PostMessage(WM_COMMAND, ID_APP_EXIT);
    }

    return TRUE;
}


/////////////////////////////////////////////////////////////////////////////
// CTblViewApp message handlers


void CTblViewApp::OnFileNew()
{
    return; //nothing to new
}

void CTblViewApp::OnFileOpen()
{
    ASSERT(AfxGetMainWnd()->IsKindOf(RUNTIME_CLASS(CMDIFrameWnd)));
    CString csFile = AfxGetApp()->GetProfileString(_T("Settings"), _T("Last File"), _T(""));

    CIMSAFileDialog dlgFile(TRUE, FileExtensions::Table, csFile, OFN_ALLOWMULTISELECT | OFN_HIDEREADONLY | OFN_SHAREAWARE,
                            _T("CSPro Tables (*.tbw)|*.tbw|All Files (*.*)|*.*||"));

    constexpr int MaxFiles = 50;
    dlgFile.SetMultiSelectBuffer(MaxFiles);

    if( dlgFile.DoModal() != IDOK )
        return;

    if( dlgFile.m_aFileName.GetSize() > MaxFiles )
    {
        AfxMessageBox(FormatText(_T("You cannot view more than %d files at once.\n\nYou tried to view %d."), MaxFiles, dlgFile.m_aFileName.GetSize()));
        return;
    }

    for (int iFile = 1 ; iFile < dlgFile.m_aFileName.GetSize() ; iFile++) {
        OpenDocumentFile(dlgFile.m_aFileName[iFile]);
    }

    OpenDocumentFile(dlgFile.m_aFileName[0]);
}

/////////////////////////////////////////////////////////////////////////////
//
//                             CTblViewApp::OnFileSaveAs
//
// Save as is a little strange for the viewer, we actually use the SaveTables
// functionality from CSPro CrossTabl.  If we save as HTML, RTF,....
// then we just save off a copy of the tables in that format without changing
// the current doc.  If we save as CSPro TBW then we modify the current doc
// based on the save (since user can choose to save a subset of tables).
/////////////////////////////////////////////////////////////////////////////
void CTblViewApp::OnFileSaveAs()
{
    // call the CSPro SaveTables which brings up file save dialog and dialog to choose
    // which tables and then does the actual save and returns the filename to us.
    CTableChildWnd* pActTabFrame = DYNAMIC_DOWNCAST(CTableChildWnd, ((CFrameWnd*) m_pMainWnd)->GetActiveFrame());
    ASSERT(pActTabFrame != NULL);
    ASSERT(pActTabFrame->GetTabView());
    CString sFilename;
    if (pActTabFrame->GetTabView()->SaveTables(sFilename)) {

        // if save as TBW
        if ((sFilename.Right(3)).CompareNoCase(FileExtensions::Table) == 0) {
            CTabulateDoc* pDoc = DYNAMIC_DOWNCAST(CTabulateDoc, pActTabFrame->GetTabView()->GetDocument());
            pDoc->SetModifiedFlag(FALSE); // avoid any "save changes" msgs
            pDoc->SetPathName(sFilename); // update doc name in toolbar
            // if the user selected a subset of the tables for the new file
            // need to delete the tables that were not saved in the current doc
            // to reflect that change and make it look like a real "save as"
            pActTabFrame->GetTabView()->DeleteUnsavedTables();
        }
    }
}

/////////////////////////////////////////////////////////////////////////////
//
//                             CTextViewApp::OnAppAbout
//
/////////////////////////////////////////////////////////////////////////////

void CTblViewApp::OnAppAbout() {

    CIMSAAboutDlg dlg;
    dlg.m_hIcon = m_hIcon;
    dlg.m_csModuleName = m_csModuleName;
    dlg.DoModal();
}


void CTblViewApp::OnFullscreen()
{
    CTVDlgBar& dlgBar  = ((CMainFrame*)AfxGetMainWnd())->m_SizeDlgBar;
    CFrameWnd* pParentFrameWnd = dlgBar.GetParentFrame();

    if(dlgBar.IsWindowVisible()){
        pParentFrameWnd->ShowControlBar(&dlgBar, FALSE, FALSE);
    }
    else {
        pParentFrameWnd->ShowControlBar(&dlgBar, TRUE, FALSE);
    }

}

void CTblViewApp::OnUpdateFullscreen(CCmdUI* pCmdUI)
{
    CTVDlgBar& dlgBar  = ((CMainFrame*)AfxGetMainWnd())->m_SizeDlgBar;
    pCmdUI->Enable(TRUE);
    if(dlgBar.IsWindowVisible()) {
        pCmdUI->SetCheck(FALSE);
    }
    else {
        pCmdUI->SetCheck(TRUE);
    }
}

int CTblViewApp::ExitInstance()
{
    // TODO: Add your specialized code here and/or call the base class
    CFmtFont::Delete();
    return CWinApp::ExitInstance();
}

/////////////////////////////////////////////////////////////////////////////
//
//                             CTextViewApp::FindFile
//
/////////////////////////////////////////////////////////////////////////////

CTblViewDoc* CTblViewApp::FindFile(const CString& csFileName) const {

    CTblViewDoc* pRetDoc;
    POSITION pos = GetFirstDocTemplatePosition();
    while (pos) {
        CDocTemplate* pTemplate = GetNextDocTemplate(pos);
        POSITION pos2 = pTemplate->GetFirstDocPosition();
        while (pos2) {
            pRetDoc = (CTblViewDoc*)(pTemplate->GetNextDoc(pos2));
            if (pRetDoc != NULL) {
                if (pRetDoc->GetPathName().CompareNoCase(csFileName) == 0) {
                    return pRetDoc;
                }
            }
        }
    }
    return NULL;
}
