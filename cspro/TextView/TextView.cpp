//***************************************************************************
//  File name: TextView.cpp
//
//  Description:
//       Implmentation for TextView application
//
//  History:    Date       Author   Comment
//              ---------------------------
//              18 Nov 99   bmd     Created for CSPro 2.0
//              4 Jan 03    csc     removed OnFormatFont
//              16 Jan 04   csc     added drag file open feature
//
//***************************************************************************

#include "StdAfx.h"
#include "TextView.h"
#include "OpenInDataViewerDlg.h"
#include <zToolsO/RaiiHelpers.h>
#include <zUtilO/imsaDlg.H>
#include <zUtilO/Filedlg.h>


BEGIN_MESSAGE_MAP(CTextViewApp, CWinApp)
    ON_COMMAND(ID_APP_ABOUT, OnAppAbout)
    ON_UPDATE_COMMAND_UI(ID_VIEW_RULER, OnUpdateViewRuler)
    ON_COMMAND(ID_FILE_NEW, CWinApp::OnFileNew)
    ON_COMMAND(ID_FILE_OPEN, OnFileOpen)
    ON_COMMAND(ID_FILE_PRINT_SETUP, CWinApp::OnFilePrintSetup)
    ON_COMMAND(ID_FILE_PAGE_SETUP, OnFilePageSetup)
END_MESSAGE_MAP()


namespace
{
    TCHAR pszOpenLastUsed[] = _T("Last used") _T("\0") _T("*.*") _T("\0") _T("\0") _T("                                                         ");
}

OFOSM OpenFilesOnStartupManager;

CTextViewApp theApp;


/////////////////////////////////////////////////////////////////////////////
//
//                             CTextViewApp::CTextViewApp
//
/////////////////////////////////////////////////////////////////////////////

CTextViewApp::CTextViewApp()
{
    InitializeCSProEnvironment();

    EnableHtmlHelp();

    m_bCalledAsChild = FALSE;
    m_iNumColors = NONE;
}


/////////////////////////////////////////////////////////////////////////////
//
//                             CTextViewApp::InitInstance
//
/////////////////////////////////////////////////////////////////////////////

BOOL CTextViewApp::InitInstance()
{
    // if multiple files are being opened back to back (e.g, a batch application opening the imputation frequencies
    // and then the listing report), the OpenFilesOnStartupManager will ensure that they are opened in the proper order
    RAII::ModifyValueOnDestruction reset_open_file_directly(OpenFilesOnStartupManager.open_file_directly, true);
    OpenFilesOnStartupManager.open_file_directly = false;
 
    AfxOleInit();
    AfxEnableControlContainer();

    SetRegistryKey(_T("U.S. Census Bureau"));

    LoadStdProfileSettings(16);  // Load standard INI file options (including MRU)

    // Register the application's document templates.  Document templates
    //  serve as the connection between documents, frame windows and views.


    //**** starting app specific stuff

    m_csModuleName = _T("CSPro Text Viewer");
    m_csWndClassName = IMSA_WNDCLASS_TEXTVIEW;
    m_csFileOpenDefExt = _T("");        // not used in textview
    m_csFileOpenFilter.LoadString (IDS_MSG03);
    m_hIcon = LoadIcon(IDR_MAINFRAME);

    CMultiDocTemplate* pDocTemplate;
    pDocTemplate = new CMultiDocTemplate(
        IDR_VIEWTYPE,
        RUNTIME_CLASS(CTVDoc),
        RUNTIME_CLASS(CChildFrame), // custom MDI child frame
        RUNTIME_CLASS(CTVView));
    AddDocTemplate(pDocTemplate);

    // Parse command line for standard shell commands, DDE, file open
    CIMSACommandLineInfo cmdInfo;
    ParseCommandLine(cmdInfo);
    m_bCalledAsChild = cmdInfo.m_bChildApp;

    // If instance already exists, open files in it.   // BMD 03 Mar 2003
    CWnd* pPrevInstance = CWnd::FindWindow(m_csWndClassName, NULL);
    if (pPrevInstance) {
        for (int i=0 ; i<cmdInfo.m_acsFileName.GetSize() ; i++) {
            IMSASendMessage(IMSA_WNDCLASS_TEXTVIEW, WM_IMSA_FILEOPEN, cmdInfo.m_acsFileName[i]);
        }
        return FALSE;
    }

    // Register file type
    EnableShellOpen();
    RegisterShellFileTypes(TRUE);

    // create main MDI Frame window
    CMainFrame* pMainFrame = new CMainFrame;
    m_pMainWnd = pMainFrame;
    if (!pMainFrame->LoadFrame(IDR_MAINFRAME))
        return FALSE;

    // This code replace the MFC crated menus with the ownerdrawn versions
    pDocTemplate->m_hMenuShared = pMainFrame->TextViewMenu();
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

    pMainFrame->UpdateWindow();
    pMainFrame->SendMessage(WM_IMSA_SETFOCUS);
    pMainFrame->DragAcceptFiles(TRUE);      // csc 1/17/05


    // open any files that were opened (via inter-app messaging) during the startup process
    for( size_t i = 0; i < OpenFilesOnStartupManager.filenames.size(); ++i )
    {
        auto get_filename = [&]() -> CString
        {
            std::lock_guard<std::mutex> lock(OpenFilesOnStartupManager.filenames_mutex);
            return OpenFilesOnStartupManager.filenames[i];
        };
        
        OpenDocumentFile(get_filename());
    }

    return TRUE;
}


/////////////////////////////////////////////////////////////////////////////
//
//                             CTextViewApp::OnFileOpen
//
/////////////////////////////////////////////////////////////////////////////

void CTextViewApp::OnFileOpen()
{
    ASSERT(AfxGetMainWnd()->IsKindOf(RUNTIME_CLASS(CMainFrame)));

    CString csFile = AfxGetApp()->GetProfileString(_T("Settings"), _T("Last File"), _T(""));

    CIMSAFileDialog dlgFile(TRUE, nullptr, csFile, OFN_ALLOWMULTISELECT | OFN_HIDEREADONLY | OFN_SHAREAWARE, m_csFileOpenFilter);
    dlgFile.m_ofn.lpstrCustomFilter = pszOpenLastUsed;
    dlgFile.m_ofn.nMaxCustFilter = MAXFILTER;

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
//                             CTextViewApp::OnFilePageSetup
//
/////////////////////////////////////////////////////////////////////////////

void CTextViewApp::OnFilePageSetup() {

    CIMSAPageSetupDlg dlg;
    dlg.DoModal();
    UpdateAllAppViews(HINT_CHANGEFONT);
}


/////////////////////////////////////////////////////////////////////////////
//
//                           CTextViewApp::UpdateAllAppViews
//
//  This function updates all the views for all the documents in the
//  application.  (A little help on the undocumented functions here
//  from MSDN.)
//
/////////////////////////////////////////////////////////////////////////////

void CTextViewApp::UpdateAllAppViews(LPARAM lHint /*=0L*/) {

    POSITION pos = GetFirstDocTemplatePosition();
    while (pos) {
        CDocTemplate* pTemplate = GetNextDocTemplate(pos);
        POSITION pos2 = pTemplate->GetFirstDocPosition();
        while (pos2) {
            CDocument* pDocument = (CDocument*)(pTemplate->GetNextDoc(pos2));
            if (pDocument != NULL) {
                pDocument->UpdateAllViews(NULL, lHint);
            }
        }
    }
}


/////////////////////////////////////////////////////////////////////////////
//
//                             CTextViewApp::OnUpdateViewRuler
//
/////////////////////////////////////////////////////////////////////////////

void CTextViewApp::OnUpdateViewRuler(CCmdUI* pCmdUI) {

    pCmdUI->Enable(FALSE);
}


/////////////////////////////////////////////////////////////////////////////
//
//                             CTextViewApp::OnAppAbout
//
/////////////////////////////////////////////////////////////////////////////

void CTextViewApp::OnAppAbout() {

    CIMSAAboutDlg dlg;
    dlg.m_hIcon = m_hIcon;
    dlg.m_csModuleName = m_csModuleName;
    dlg.DoModal();
}


/////////////////////////////////////////////////////////////////////////////
//
//                             CTextViewApp::FindFile
//
/////////////////////////////////////////////////////////////////////////////

CTVDoc* CTextViewApp::FindFile(const CString& csFileName) const {

    CTVDoc* pRetDoc = NULL;
    POSITION pos = GetFirstDocTemplatePosition();
    while (pos) {
        CDocTemplate* pTemplate = GetNextDocTemplate(pos);
        POSITION pos2 = pTemplate->GetFirstDocPosition();
        while (pos2) {
            CTVDoc* pTVDoc = (CTVDoc*)(pTemplate->GetNextDoc(pos2));
            if (pTVDoc != NULL) {
                if (pTVDoc->GetPathName().CompareNoCase(csFileName) == 0) {
                    pRetDoc = pTVDoc;
                    if (!pRetDoc->IsReloadingOrClosing()) //if duplicate names exist it returns the one which is not in the state of reloading or closing
                        return pRetDoc;
                }
            }
        }
    }
    return pRetDoc;
}


CDocument* CTextViewApp::OpenDocumentFile(LPCTSTR lpszFileName)
{
    // if the file is a CSPro DB file, make sure that they want to open it in Text Viewer
    std::wstring extension = PortableFunctions::PathGetFileExtension(lpszFileName);

    if( SO::EqualsOneOfNoCase(extension, FileExtensions::Data::CSProDB, FileExtensions::Data::EncryptedCSProDB) )
    {
        if( OpenInDataViewerQuery(lpszFileName) )
            return nullptr;
    }

    // if not opened in Data Viewer, continue opening the document
    CDocument* pDocument = CWinApp::OpenDocumentFile(lpszFileName);
    POSITION pos = GetFirstDocTemplatePosition();
    CDocTemplate* pTemplate = NULL;
    while (pos) {
        pTemplate = GetNextDocTemplate(pos);
        break;
    }

    CTVDoc* pTVDoc = DYNAMIC_DOWNCAST(CTVDoc, pDocument);
    if (pTVDoc && pTVDoc->IsReloadingOrClosing()) { //if an older document is in the process of being closed. Allow the second copy to be opened.
        pDocument = pTemplate->OpenDocumentFile(lpszFileName);
    }

    return pDocument;
}


bool CTextViewApp::OpenInDataViewerQuery(const CString& filename)
{
    constexpr const TCHAR* RegistryOptions = _T("Options");
    constexpr const TCHAR* RegistrySetting = _T("DataViewerAutoLaunch");

    int iRegistrySetting = AfxGetApp()->GetProfileInt(RegistryOptions, RegistrySetting, -1);

    bool bOpenInDataViewer = ( iRegistrySetting == 1 );

    if( iRegistrySetting == -1 ) // there is no registry setting so show the dialog
    {
        COpenInDataViewerDlg dlg;
        dlg.DoModal();

        bOpenInDataViewer = dlg.OpenInDataViewer();

        if( dlg.RememberSetting() )
            AfxGetApp()->WriteProfileInt(RegistryOptions, RegistrySetting, bOpenInDataViewer ? 1 : 0);
    }

    if( bOpenInDataViewer )
    {
        ((CMainFrame*)AfxGetMainWnd())->OpenInDataViewer(filename);
        return true;
    }

    return false;
}
