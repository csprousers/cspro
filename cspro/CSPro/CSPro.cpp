#include "StdAfx.h"
#include "CSPro.h"
#include "AboutMenuDialogs.h"
#include "CommonStoreDlg.h"
#include "DesignerApplicationLoaderWithFileSupport.h"
#include "FontPrefDlg.h"
#include "HidDTmpl.h"
#include "InteractiveNewFileCreator.h"
#include "OnKeyCharacterMapDlg.h"
#include "SelectAppDlg.h"
#include "SelectDocsDlg.h"
#include "StartDlg.h"
#include "VersionShifterDlg.h"
#include <zToolsO/FileIO.h>
#include <zUtilO/ArrUtil.h>
#include <zUtilO/FileUtil.h>
#include <zUtilO/TreeCtrlHelpers.h>
#include <zUtilO/WinFocSw.h>
#include <zUtilF/CommonControls.h>
#include <zUtilF/DocViewIterators.h>
#include <zUtilF/ManageCredentialsDlg.h>
#include <zUtilF/resource_shared.h>
#include <zListingO/Lister.h>
#include <zDesignerF/ManageFilesDlg.h>
#include <zDictF/Ddgview.h>
#include <zFormO/DragOptions.h>
#include <zTableF/TabView.h>
#include <zTableF/TabChWnd.h>
#include <zNetwork/SyncLog.h>
#include <engine/trace_macros.h>
#include <afxvisualmanageroffice2007.h>


#define SOFTWARE_DEVELOPER _T("U.S. Census Bureau")

CString GetDictFilenameFromFormFile(const CString& sFormFileName);


/////////////////////////////////////////////////////////////////////////////
// CCSProApp

BEGIN_MESSAGE_MAP(CCSProApp, CWinApp)
    //{{AFX_MSG_MAP(CCSProApp)
    ON_COMMAND(ID_APP_ABOUT, OnAppAbout)
    ON_COMMAND_EX_RANGE(ID_FILE_MRU_FILE1, ID_FILE_MRU_FILE16, OnOpenRecentFile)
    ON_COMMAND(ID_FILE_NEW, OnFileNew)
    ON_COMMAND(ID_FILE_OPEN, OnFileOpen)
    ON_COMMAND(ID_FILE_CSPRO_CLOSE, OnFileClose)
    ON_UPDATE_COMMAND_UI(ID_FILE_CSPRO_CLOSE, OnUpdateIsDocumentOpen)
    ON_COMMAND(ID_FILE_CSPRO_SAVE, OnFileSave)
    ON_UPDATE_COMMAND_UI(ID_FILE_CSPRO_SAVE, OnUpdateIsDocumentOpen)
    ON_COMMAND(ID_FILE_CSPRO_SAVE_AS, OnFileSaveAs)
    ON_UPDATE_COMMAND_UI(ID_FILE_CSPRO_SAVE_AS, OnUpdateIsDocumentOpen)
    ON_COMMAND(ID_FILE_MANAGE_FILES, OnManageFiles)
    ON_UPDATE_COMMAND_UI(ID_FILE_MANAGE_FILES, OnUpdateIsApplicationOpen)
    ON_COMMAND(ID_FILE_MANAGE_CREDENTIALS, OnManageCredentials)
    ON_COMMAND(ID_VIEW_ONKEY_CHAR_MAP, OnOnKeyCharacterMap)
    ON_COMMAND(ID_VIEW_COMMONSTORE, OnCommonStore)
    ON_COMMAND(ID_CHANGE_TAB, OnChangeTab) // 20100406
    ON_COMMAND(ID_VIEW_NAMES, OnViewNames)
    ON_UPDATE_COMMAND_UI(ID_VIEW_NAMES, OnUpdateViewNames)
    ON_COMMAND(ID_VIEW_NAMES_WITH_LABELS, OnViewAppendLabelsToNames)
    ON_UPDATE_COMMAND_UI(ID_VIEW_NAMES_WITH_LABELS, OnUpdateViewAppendLabelsToNames)
    ON_UPDATE_COMMAND_UI(ID_FILE_MRU_FILE1, OnUpdateRecentFileMenu)
    ON_UPDATE_COMMAND_UI(ID_WINDOW_DICTS, OnUpdateWindowDicts)
    ON_UPDATE_COMMAND_UI(ID_WINDOW_FORMS, OnUpdateWindowForms)
    ON_UPDATE_COMMAND_UI(ID_WINDOW_ORDER, OnUpdateWindowOrder)
    ON_UPDATE_COMMAND_UI(ID_WINDOW_TABLES, OnUpdateWindowTables)
    ON_COMMAND(ID_WINDOW_DICTS, OnWindowDicts)
    ON_COMMAND(ID_WINDOW_FORMS, OnWindowForms)
    ON_COMMAND(ID_WINDOW_ORDER, OnWindowOrder)
    ON_COMMAND(ID_WINDOW_TABLES, OnWindowTables)
    ON_COMMAND(ID_VIEW_FULLSCREEN, OnFullscreen)
    ON_UPDATE_COMMAND_UI(ID_VIEW_FULLSCREEN, OnUpdateFullscreen)

    ON_COMMAND_RANGE(ID_VIEW_LISTING, ID_VIEW_LISTING, OnRunTool)
    ON_COMMAND_RANGE(ID_TOOLS_DATAMANAGER, ID_TOOLS_TEXTCONVERTER, OnRunTool)
    ON_UPDATE_COMMAND_UI_RANGE(ID_TOOLS_DATAMANAGER, ID_TOOLS_TEXTCONVERTER, OnUpdateTool)
    ON_COMMAND(ID_TOOLS_VERSION_SHIFTER, OnToolsVersionShifter)
    ON_UPDATE_COMMAND_UI(ID_TOOLS_VERSION_SHIFTER, OnToolsVersionShifter)

    ON_COMMAND(ID_HELP_WHAT_IS_NEW, OnHelpWhatIsNew)
    ON_COMMAND(ID_HELP_EXAMPLES, OnHelpExamples)
    ON_COMMAND(ID_HELP_TROUBLESHOOTING, OnHelpTroubleshooting)
    ON_COMMAND(ID_HELP_MAILING_LIST, OnHelpMailingList)
    ON_COMMAND(ID_HELP_GOOGLEPLAY, OnHelpAndroidApp)
    ON_COMMAND(ID_HELP_SHOW_SYNC_LOG, OnHelpShowSyncLog)
    ON_COMMAND(ID_HELP_CSPROUSERS_FORUM, OnHelpCSProUsersForum)
    ON_COMMAND(ID_HELP_CSPROUSERS_GITHUB, OnHelpCSProUsersGitHub)

    //}}AFX_MSG_MAP
    // Standard file based document commands
    ON_COMMAND(ID_FILE_NEW, CWinApp::OnFileNew)
    ON_COMMAND(ID_FILE_OPEN, CWinApp::OnFileOpen)
    // Standard print setup command
    ON_COMMAND(ID_FILE_PRINT_SETUP, CWinApp::OnFilePrintSetup)
    ON_COMMAND(ID_PREFERENCES_FONTS, OnPreferencesFonts)
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CCSProApp construction

CCSProApp::CCSProApp()
    :   m_pWindowFocusMgr(nullptr),
        m_bFileNew(false)

{
    InitializeCSProEnvironment();

    EnableHtmlHelp();
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CCSProApp object

CCSProApp theApp;

/////////////////////////////////////////////////////////////////////////////
// CCSProApp initialization

// Helper classes to be used with the window focus manager (see InitInstance below)
// Need one of these for each of the types of windows that you want to switch focus between.

// focus switcher for batch edit source view
struct COSourceEditViewFocusSwitcher : public CViewFocusSwitcher
{
    virtual bool MatchWindow(CWnd* pWnd) const
    {
        return pWnd->IsKindOf(RUNTIME_CLASS(COSourceEditView));
    }

    virtual void SetFocus(CWnd* pWnd)
    {
        ASSERT(pWnd->IsKindOf(RUNTIME_CLASS(COSourceEditView)));
        CViewFocusSwitcher::SetFocus(pWnd);
        COSourceEditView* pView = DYNAMIC_DOWNCAST(COSourceEditView, pWnd);
        pView->GetEditCtrl()->SetFocus();
    }
};


// focus switcher for tab source view
struct CTSourceEditViewFocusSwitcher : public CViewFocusSwitcher
{
    virtual bool MatchWindow(CWnd* pWnd) const
    {
        return pWnd->IsKindOf(RUNTIME_CLASS(CTSourceEditView));
    }

    virtual void SetFocus(CWnd* pWnd)
    {
        ASSERT(pWnd->IsKindOf(RUNTIME_CLASS(CTSourceEditView)));
        CViewFocusSwitcher::SetFocus(pWnd);
        CTSourceEditView* pView = DYNAMIC_DOWNCAST(CTSourceEditView, pWnd);
        pView->GetEditCtrl()->SetFocus();
    }
};

// focus switcher for tab grid view
struct CTabViewFocusSwitcher : public CViewFocusSwitcher
{
    virtual bool MatchWindow(CWnd* pWnd) const
    {
        return pWnd->IsKindOf(RUNTIME_CLASS(CTabView));
    }

    virtual void SetFocus(CWnd* pWnd)
    {
        ASSERT(pWnd->IsKindOf(RUNTIME_CLASS(CTabView)));
        CViewFocusSwitcher::SetFocus(pWnd);
        CTabView* pView = DYNAMIC_DOWNCAST(CTabView, pWnd);
        pView->GetGrid()->SetFocus();
    }
};

// focus switcher for tree control tabs (dictionary, forms, tables,...)
struct CMDlgTabFocusSwitcher : public CWindowFocusSwitcher
{
    virtual void FindWindows(CWindowFocusMgr* pMgr, CWnd* pAppMainWnd)
    {
        CMainFrame* pMainFrame = DYNAMIC_DOWNCAST(CMainFrame, pAppMainWnd);
        CArray<CWnd*> aTabs;
        pMainFrame->GetDlgBar().GetTabWindows(aTabs);
        for (int i = 0; i < aTabs.GetCount(); ++i) {
            pMgr->AddWindow(aTabs.GetAt(i), this);
        }
    }

    virtual bool MatchWindow(CWnd* pWnd) const
    {
        return pWnd->GetParent()->IsKindOf(RUNTIME_CLASS(CMDlgBar)) &&
                !pWnd->IsKindOf(RUNTIME_CLASS(CTabCtrl));
    }

    virtual void SetFocus(CWnd* pWnd)
    {
        CMDlgBar* pParentDlg = DYNAMIC_DOWNCAST(CMDlgBar, pWnd->GetParent());
        pParentDlg->SelectTab(pWnd);
        pWnd->SetFocus();
    }

    virtual bool MustBeVisible() const
    {
        return false;
    }
};

// focus switcher for edit windows in tab view (messages, compile output)
struct CTabViewContainerFocusSwitcher : public CWindowFocusSwitcher
{
    virtual void FindWindows(CWindowFocusMgr* pMgr, CWnd* pAppMainWnd)
    {
        CFrameWnd* pMainFrame = DYNAMIC_DOWNCAST(CFrameWnd, pAppMainWnd);
        pMgr->AddWindowsRec(pMainFrame->GetActiveFrame(), this);
    }

    virtual bool MatchWindow(CWnd* pWnd) const
    {
        return (pWnd->GetParent()->IsKindOf(RUNTIME_CLASS(COXTabViewContainer)) &&
             pWnd->IsKindOf(RUNTIME_CLASS(CLogicCtrl)));
    }

    virtual void SetFocus(CWnd* pWnd)
    {
        COXTabViewContainer* pParentDlg = DYNAMIC_DOWNCAST(COXTabViewContainer, pWnd->GetParent());
        if (!pParentDlg->IsActivePage(pWnd)) {
            int nIndex;
            if (pParentDlg->FindPage(pWnd, nIndex)) {
                pParentDlg->SetActivePageIndex(nIndex);
            }
        }
        pWnd->SetFocus();
    }

    virtual bool MustBeVisible() const
    {
        return false;
    }
};



BOOL CCSProApp::InitInstance()
{
    InitializeCommonControls();

    AfxOleInit();
    AfxEnableControlContainer();

    // Setup the window focus manager - handles changing focus between
    // main app windows on a hotkey (for accessibility)
    // Called in PreTranslateMessage
    m_pWindowFocusMgr = new CWindowFocusMgr;
    m_pWindowFocusMgr->AddSwitcher(new COSourceEditViewFocusSwitcher);
    m_pWindowFocusMgr->AddSwitcher(new CTSourceEditViewFocusSwitcher);
    m_pWindowFocusMgr->AddSwitcher(new CTabViewFocusSwitcher);
    m_pWindowFocusMgr->AddSwitcher(new CViewFocusSwitcher);
    m_pWindowFocusMgr->AddSwitcher(new CTabViewContainerFocusSwitcher);
    m_pWindowFocusMgr->AddSwitcher(new CMDlgTabFocusSwitcher);

    // Standard initialization
        // If you are not using these features and wish to reduce the size
        //  of your final executable, you should remove from the following
        //  the specific initialization routines you do not need.

        // Change the registry key under which our settings are stored.
        // TODO: You should modify this string to be something appropriate
        // such as the name of your company or organization.

    SetRegistryKey(SOFTWARE_DEVELOPER);

    LoadStdProfileSettings(10);  // Load standard INI file options (including MRU)

    // set the look and feel of the property grids to the Office 2007 style
    CMFCVisualManagerOffice2007::SetStyle(CMFCVisualManagerOffice2007::Office2007_ObsidianBlack);
    CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManagerOffice2007));

    // gsf
    m_csWndClassName = IMSA_WNDCLASS_CSPRO;
    m_hIcon = LoadIcon(IDR_MAINFRAME);

    // Register the application's document templates.  Document templates
    //  serve as the connection between documents, frame windows and views.

    //Template for the session object
    CMultiDocTemplate* pSessionTemplate = new CHiddenDocTemplate(
            IDR_MEASURTYPE,
            RUNTIME_CLASS(CCSProDoc),
            RUNTIME_CLASS(CChildFrame), // custom MDI child frame
            RUNTIME_CLASS(CCSProView));
    AddDocTemplate(pSessionTemplate);

    HINSTANCE hOldRes = AfxGetResourceHandle();
    HINSTANCE hInst = AfxLoadLibrary(_T("zDictF.dll"));
    AfxSetResourceHandle(hInst);

    //Template for the dictionary
    CMultiDocTemplate* pDictTemplate = new CMultiDocTemplate(
            IDR_DICT_FRAME,
            RUNTIME_CLASS(CDDDoc),
            RUNTIME_CLASS(CDictChildWnd), // custom MDI child frame
            RUNTIME_CLASS(CDDGView));
    AddDocTemplate(pDictTemplate);


    AfxSetResourceHandle(hOldRes);

    hOldRes = AfxGetResourceHandle();
    hInst = AfxLoadLibrary(_T("zTableF.dll"));
    AfxSetResourceHandle(hInst);

    //Template for the tables
    CMultiDocTemplate* pTableTemplate = new CMultiDocTemplate(
            IDR_TABLE_FRAME,
            RUNTIME_CLASS(CTabulateDoc),
            RUNTIME_CLASS(CTableChildWnd), // custom MDI child frame
            RUNTIME_CLASS(CTabView));
    AddDocTemplate(pTableTemplate);


    AfxSetResourceHandle(hOldRes);

    hOldRes = AfxGetResourceHandle();
    hInst = AfxLoadLibrary(_T("zFormF.dll"));
    AfxSetResourceHandle(hInst);

    //Template for the forms
    CMultiDocTemplate* pFormTemplate = new CMultiDocTemplate(
            IDR_FORM_FRAME,
            RUNTIME_CLASS(CFormDoc),
            RUNTIME_CLASS(CFormChildWnd), // custom MDI child frame
            RUNTIME_CLASS(CFormScrollView));
    AddDocTemplate(pFormTemplate);

    AfxSetResourceHandle(hOldRes);

    hOldRes = AfxGetResourceHandle();
    hInst = AfxLoadLibrary(_T("zOrderF.dll"));
    AfxSetResourceHandle(hInst);

    //Template for the orders
    CMultiDocTemplate* pOrderTemplate = new CMultiDocTemplate(
            IDR_ORDER_FRAME,
            RUNTIME_CLASS(COrderDoc),
            RUNTIME_CLASS(COrderChildWnd), // custom MDI child frame
            RUNTIME_CLASS(COSourceEditView));
    AddDocTemplate(pOrderTemplate);

    AfxSetResourceHandle(hOldRes);

    //Template for the Application Object
    m_pAplTemplate = new CHiddenDocTemplate(
            IDR_APLTYPE,
            RUNTIME_CLASS(CAplDoc),
            RUNTIME_CLASS(CChildFrame), // custom MDI child frame
            RUNTIME_CLASS(CCSProView));
    AddDocTemplate(m_pAplTemplate);

    // create main MDI Frame window
    CMainFrame* pMainFrame = new CMainFrame;
    if (!pMainFrame->LoadFrame(IDR_MAINFRAME))
            return FALSE;
    m_pMainWnd = pMainFrame;

    //Set the doc template for the tree to open up dicts when needed
    pMainFrame->GetDlgBar().m_DictTree.SetDocTemplate(pDictTemplate);
    //Set the doc template for the tree to open up dicts when needed
    pMainFrame->GetDlgBar().m_TableTree.SetDocTemplate(pTableTemplate);

    pMainFrame->GetDlgBar().m_FormTree.SetDocTemplate(pFormTemplate);

    pMainFrame->GetDlgBar().m_OrderTree.SetDocTemplate(pOrderTemplate);

    // This code replace the MFC crated menus with the ownerdrawn versions

    pDictTemplate->m_hMenuShared = pMainFrame->DictMenu();

    pTableTemplate->m_hMenuShared = pMainFrame->TableMenu();
    pFormTemplate->m_hMenuShared = pMainFrame->FormMenu();
    pOrderTemplate->m_hMenuShared = pMainFrame->OrderMenu();
    pMainFrame->m_hMenuDefault = pMainFrame->DefaultMenu();
    pMainFrame->OnUpdateFrameMenu(pMainFrame->m_hMenuDefault);


    // Enable drag/drop open
    m_pMainWnd->DragAcceptFiles();

    // Parse command line for standard shell commands, DDE, file open
    CCommandLineInfo cmdInfo;
    ParseCommandLine(cmdInfo);

    if(cmdInfo.m_nShellCommand != CCommandLineInfo::FileOpen)
            cmdInfo.m_nShellCommand = CCommandLineInfo::FileNothing ;

    // The main window has been initialized, so show and update it.
    pMainFrame->ShowWindow(m_nCmdShow);
    pMainFrame->UpdateWindow();

    bool bOpenStartDlg = true;

#ifndef _DEBUG
    if constexpr(Versioning::IsBeta)
    {
        // display an introduction message if this is the first time they've opened this beta build
        constexpr const wchar_t* BetaKeyName = L"Beta Release Date";
        const int beta_release_data = AfxGetApp()->GetProfileInt(L"Settings", BetaKeyName, 0);

        if( beta_release_data != Versioning::GetReleaseDate() )
        {
            AfxMessageBox(FormatText("Thank you for testing a beta version of %s. "
                                     "This software has not been thoroughly tested and generally should not be used for production data collection. "
                                     "A list of new features will be displayed when you close this dialog.",
                                     Versioning::CSProVersionText));

            AfxGetApp()->WriteProfileInt(L"Settings" ,BetaKeyName, Versioning::GetReleaseDate());
            AfxGetApp()->HtmlHelp(HID_BASE_COMMAND + ID_HELP_WHAT_IS_NEW);

            bOpenStartDlg = false;
        }
    }
#endif

    m_pSessionDoc = assert_cast<CCSProDoc*>(pSessionTemplate->CreateNewDocument());

    // Dispatch commands specified on the command line
    if(cmdInfo.m_nShellCommand == CCommandLineInfo::FileOpen) {
        CDocument* pDoc = nullptr;
        pDoc = OpenDocumentFile(cmdInfo.m_strFileName);
        if (!pDoc)  {
            return FALSE;
        }
        else {
            BOOL bRet = UpdateViews(pDoc);

            //Reconcile the stuff
            if (bRet){      // if no errs occurred during update views, reconcile the file(s)
                CString csErr;
                Reconcile (pDoc, csErr, false, true);    // silent no; autofix yes
            }

            return bRet;
        }
    }
    else {
        if (!ProcessShellCommand(cmdInfo)) {
                return FALSE;
        }

        // 20100311 if the user launched CSPro without specifying a particular file, we will
        // check whether or not the user wants to allow for automatic updates

        // this means that the user hasn't been asked yet (first time after an install)
        /* taken out 20120618
        if( AfxGetApp()->GetProfileInt(_T("Settings"),_T("CheckForUpdates"),0) == 2 )
        {
            int response = AfxMessageBox(_T("Do you want CSPro to automatically check for and download updates from the Internet?"),MB_YESNO);
            AfxGetApp()->WriteProfileInt(_T("Settings"),_T("CheckForUpdates"),response == IDYES);
        }*/

        if( bOpenStartDlg )
        {
            CStartDlg start_dlg;

            if( start_dlg.DoModal() == IDOK )
            {
                if( start_dlg.m_iChoice == 0 )
                {
                    OnFileNew();
                }

                else if( start_dlg.m_iSelection < 1 )
                {
                    OnFileOpen();
                }

                else
                {
                    CString csTemp;
                    csTemp.Format(_T("File%d"), start_dlg.m_iSelection);
                    CString csFile = AfxGetApp()->GetProfileString(_T("Recent File List"),csTemp,_T("?"));

                    CDocument* pDoc = OpenDocumentFile(csFile);

                    if( pDoc != nullptr )
                    {
                        BOOL bRet = UpdateViews(pDoc);

                        //Reconcile the stuff
                        if (bRet)      // if no errs occurred during update views, reconcile the file(s)
                        {
                            CString csErr;
                            Reconcile(pDoc, csErr, false, true);    // silent no; autofix yes
                        }

                        return bRet;
                    }
                }
            }
        }
    }

    return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CCSProApp message handlers


CDocument* CCSProApp::OpenDocumentFile(LPCTSTR lpszFileName)
{
    //Get the file extension and perform the check before doing an open
    if(!m_pSessionDoc->IsFileOpen(TC::ToUtf8(lpszFileName))) {
        //We have to set the current directory to the path of lpszFileName for the
        //Absolute path thing to work
        //Remember PathStrpPath & PathRemovefileSpec require shlwapi.h for compiling
        //shlwapi.lib for linking   and the version of the shlwapi.dll > 4.0 (this stuff
        //comes with IE4.0

        TCHAR szCurDirectory[_MAX_PATH];
        GetCurrentDirectory(_MAX_PATH,szCurDirectory);

        CString sPath(lpszFileName);
        CString sFileName(lpszFileName);
        PathRemoveFileSpec(sPath.GetBuffer(_MAX_PATH));
        sPath.ReleaseBuffer();
        PathStripPath(sFileName.GetBuffer(_MAX_PATH));
        sFileName.ReleaseBuffer();

        SetCurrentDirectory(sPath);

        CDocument* pDoc = CWinApp::OpenDocumentFile(sFileName);

        if( pDoc == nullptr )
        {
            // error messages for some types are displayed in that document type's OnOpenDocument
            const std::string extension = PortableFunctions::PathGetFileExtension(UTF8_TODO::GetUtf8(sFileName));

            if( !SO::EqualsOneOfNoCase(extension, FileExtensions::Dictionary,
                                                  FileExtensions::EntryApplication,
                                                  FileExtensions::BatchApplication,
                                                  FileExtensions::TabulationApplication) )
            {
                CString sError;
                if (sPath.IsEmpty()) {
                    sError = _T("File: ") + sFileName + _T(" cannot be opened!");
                }
                else {
                    sError = _T("File: ") + sPath + _T("\\") + sFileName + _T(" cannot be opened!");
                }
                sError += _T("\n\nFile must exist and have an extension .ENT, .BCH, .XTB (version 3.0 or higher), .DCF, or .FMF");
                AfxMessageBox(sError);
            }
        }
        SetCurrentDirectory(szCurDirectory);
        return pDoc;
    }

    else {
        CString sMsg;
        sMsg.FormatMessage(IDS_FILEOPEN,lpszFileName);
        AfxMessageBox(sMsg);
        return nullptr;
    }
}

void CCSProApp::OnFileNew()
{
    try
    {
        std::optional<std::tuple<std::string, AppFileType>> file_path_and_type = InteractiveNewFileCreator::InteractiveMode();

        if( !file_path_and_type.has_value() )
            return;

        // there is some special processing for certain file types
        if( std::get<1>(*file_path_and_type) == AppFileType::ApplicationEntry ||
            std::get<1>(*file_path_and_type) == AppFileType::Form )
        {
            assert_cast<CCSProApp*>(AfxGetApp())->m_bFileNew = true;
        }

        CDocument* const pDoc = OpenDocumentFile(TC::ToWide(std::get<0>(*file_path_and_type)).c_str());

        if( pDoc == nullptr )
            throw CSProException("There was an error opening " + std::get<0>(*file_path_and_type));

        // Reconcile the stuff
        if( UpdateViews(pDoc) )
        {
            // if no errs occurred during update views, reconcile the file(s)
            CString csErr;
            Reconcile(pDoc, csErr, false, true);    // silent no; autofix yes
        }
    }

    catch( const CSProException& exception )
    {
        ErrorMessage::Display(exception);
    }
}


void CCSProApp::OnFileOpen()
{
    CString  sFileName;
    CString sFilter= _T("Applications & Dictionaries (*.ent;*.bch;*.xtb;*.dcf)|*.ent;*.bch;*.xtb;*.dcf|");
    sFilter += _T("Data Entry Applications (*.ent)|*.ent|");
    sFilter += _T("Batch Edit Applications (*.bch)|*.bch|");
    sFilter += _T("Tabulation Applications (*.xtb)|*.xtb|");
    sFilter += _T("Dictionaries (*.dcf)|*.dcf|");
    sFilter += _T("Form Files (*.fmf)|*.fmf|");
    sFilter += _T("All Files (*.*)|*.*||");

    CString sDir = GetProfileString(_T("Settings"),_T("Last Folder"));
    if(!sDir.IsEmpty())
            SetCurrentDirectory(sDir);

    CIMSAFileDialog fileDlg(TRUE,_T("*"),nullptr,OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
            sFilter,nullptr,CFD_NO_DIR);

    if(fileDlg.DoModal() == IDOK) {
            sFileName= fileDlg.GetPathName();

            CString sPath(sFileName);
            PathRemoveFileSpec(sPath.GetBuffer(_MAX_PATH));
            sPath.ReleaseBuffer();
            AfxGetApp()->WriteProfileString(_T("Settings"),_T("Last Folder"),sPath);
    }
    else {
            return;
    }

    //Check if the file is already open
    if(IsFileOpen(UTF8_TODO::GetUtf8(sFileName))) {
            CString sMsg;
            sMsg.FormatMessage(IDS_FILEOPEN, sFileName.GetString());
            AfxMessageBox(sMsg);

            //Get the document frame and show it
            CDocument* pOpenDoc = IsDocOpen(sFileName);
            SetInitialViews(pOpenDoc);
            return;
    }


    /**************************************************/
    //Recent File List stuff
    SaveRFL();

    //If not open then do it
    CDocument* pDoc = OpenDocumentFile(sFileName);

    RestoreRFL();
    m_pRecentFileList->Add(sFileName);


    if (!pDoc){
        CString sMsg;
        sMsg.FormatMessage(_T("Tried, but failed to open %1"), sFileName.GetString());
        AfxMessageBox(sMsg);
        return;
    }

    //Reconcile the stuff
    if (UpdateViews(pDoc))  // if no errs occurred during update views, reconcile the file(s)
    {
        CString csErr;
        Reconcile(pDoc, csErr, false, true);    // silent no; autofix yes
    }
}


int CCSProApp::ExitInstance()
{
    CFmtFont::Delete();
    delete m_pWindowFocusMgr;

    return CWinApp::ExitInstance();
}


CDocument* CCSProApp::GetActiveDocument()
{
    CMainFrame* const pFrame = assert_cast<CMainFrame*>(AfxGetMainWnd());
    CMDIChildWnd* pActiveWnd = ((CMDIFrameWnd*)pFrame)->MDIGetActive();

    return ( pActiveWnd != nullptr ) ? pActiveWnd->GetActiveDocument() : nullptr;
}


void CCSProApp::OnUpdateIsDocumentOpen(CCmdUI* pCmdUI)
{
    pCmdUI->Enable(GetActiveDocument() != nullptr);
}


bool CCSProApp::IsApplicationOpen()
{
    const std::vector<CAplDoc*> application_docs = GetTopLevelAssociatedDocuments<CAplDoc>(nullptr, true);
    return !application_docs.empty();
}

void CCSProApp::OnUpdateIsApplicationOpen(CCmdUI* pCmdUI)
{
    pCmdUI->Enable(IsApplicationOpen());
}


template<typename T>
std::vector<T*> CCSProApp::GetTopLevelAssociatedDocuments(const TCHAR* filename/* = nullptr*/, bool stop_after_first_document/* = false*/)
{
    std::vector<T*> documents;

    if( filename == nullptr )
    {
        const CDocument* const pDoc = GetActiveDocument();

        if( pDoc == nullptr )
            return documents;

        filename = pDoc->GetPathName();
    }

    CMainFrame* const pFrame = assert_cast<CMainFrame*>(AfxGetMainWnd());
    const CObjTreeCtrl& ObjTree = pFrame->GetDlgBar().m_ObjTree;
    HTREEITEM hItem = ObjTree.GetRootItem();

    while( hItem != nullptr)
    {
        auto node_filename_matches = [&](FileTreeNode* const file_tree_node)
        {
            return SO::EqualsNoCase(file_tree_node->GetPath(), filename);
        };

        FileTreeNode* const file_tree_node = ObjTree.GetFileTreeNode(hItem);

        // this document is a candidate if its filename matches...
        bool document_is_candidate = node_filename_matches(file_tree_node);

        // ...or if one of its childrens' filenames matches
        if( !document_is_candidate )
        {
            document_is_candidate = TreeCtrlHelpers::FindInTree(ObjTree, ObjTree.GetChildItem(hItem), true,
                [&](const HTREEITEM hChildItem)
                {
                    return node_filename_matches(ObjTree.GetFileTreeNode(hChildItem));
                });
        }

        // the document must match the requested type
        if( document_is_candidate )
        {
            T* pDoc = dynamic_cast<T*>(file_tree_node->GetDocument());

            if( pDoc != nullptr )
            {
                documents.emplace_back(pDoc);

                if( stop_after_first_document )
                    break;
            }
        }

        // traverse siblings
        hItem = ObjTree.GetNextSiblingItem(hItem);
    }

    return documents;
}


std::vector<CDocument*> CCSProApp::GetSelectedTopLevelAssociatedDocuments(const TCHAR* dialog_title)
{
    std::vector<CDocument*> documents = GetTopLevelAssociatedDocuments<CDocument>();

    if( documents.size() > 1 )
    {
        SelectDocsDlg select_docs_dlg(documents, dialog_title);

        if( select_docs_dlg.DoModal() == IDOK )
            documents = select_docs_dlg.GetSelectedDocuments();

        else
            documents.clear();
    }

    return documents;
}


std::optional<CAplDoc*> CCSProApp::GetActiveApplication(const TCHAR* filename, const TCHAR* dialog_title)
{
    std::vector<CAplDoc*> application_docs = GetTopLevelAssociatedDocuments<CAplDoc>(filename);

    if( application_docs.empty() )
        return nullptr;

    else if( application_docs.size() == 1 )
        return application_docs.front();

    // query for which document to work with
    SelectAppDlg select_app_dlg(application_docs, dialog_title);

    if( select_app_dlg.DoModal() == IDOK )
        return select_app_dlg.GetSelectedApplicaton();

    return std::nullopt;
}


void CCSProApp::OnFileClose()
{
    for( CDocument* pDoc : GetSelectedTopLevelAssociatedDocuments(_T("Select Applications to Close")) )
        CloseDocument(pDoc);
}

void CCSProApp::CloseDocument(CDocument* pDoc)
{
    CMainFrame* const pFrame = assert_cast<CMainFrame*>(AfxGetMainWnd());

    if(pDoc->IsKindOf(RUNTIME_CLASS(CDDDoc))) {
        CDDDoc* pDictDoc = assert_cast<CDDDoc*>(pDoc);
        if(!pDictDoc->IsDocOK())
            return;

        if (!pDictDoc->SaveModified()) {
            return;
        }

        //Remove the dictionary from the object tree
        FileTreeNode* pObjID = pFrame->GetDlgBar().m_ObjTree.FindNode(pDoc);
        pFrame->GetDlgBar().m_ObjTree.DeleteNode(*pObjID);

        //release the reference
        DictionaryDictTreeNode* const dictionary_dict_tree_node = pFrame->GetDlgBar().m_DictTree.GetDictionaryTreeNode(*pDoc);
        pFrame->GetDlgBar().m_DictTree.ReleaseDictionaryNode(*dictionary_dict_tree_node);

        // update the tab controls
        pFrame->GetDlgBar().UpdateTabs();

        HTREEITEM hItem = pFrame->GetDlgBar().m_ObjTree.GetSelectedItem();
        if(hItem){
            pFrame->GetDlgBar().m_ObjTree.SetActiveObject();
        }

    }

    else if(pDoc->IsKindOf(RUNTIME_CLASS(CFormDoc)))
    {
        CFormDoc* pFormDoc = assert_cast<CFormDoc*>(pDoc);
        if(pFormDoc->IsFormModified())
            pFormDoc->SetModifiedFlag(TRUE);

        if (!pFormDoc->SaveModified()) {
            return;
        }

        //release the reference
        CFormNodeID* pID = pFrame->GetDlgBar().m_FormTree.GetFormNode(pFormDoc);
        pFormDoc->ReleaseDicts();
        pFrame->GetDlgBar().m_FormTree.ReleaseFormNodeID(pID);

        //Remove the form from the object tree
        FileTreeNode* pObjID = pFrame->GetDlgBar().m_ObjTree.FindNode(pDoc);
        pFrame->GetDlgBar().m_ObjTree.DeleteNode(*pObjID);

        // update the tab controls
        pFrame->GetDlgBar().UpdateTabs();

        HTREEITEM hItem = pFrame->GetDlgBar().m_ObjTree.GetSelectedItem();
        if(hItem){
            pFrame->GetDlgBar().m_ObjTree.SetActiveObject();
        }

    }

    else if(pDoc->IsKindOf(RUNTIME_CLASS(COrderDoc)))
    {
        COrderDoc* pOrderDoc = assert_cast<COrderDoc*>(pDoc);
        if(pOrderDoc->IsOrderModified())
            pOrderDoc->SetModifiedFlag(TRUE);

        if (!pOrderDoc->SaveModified()) {
            return;
        }

        //release the reference
        FormOrderAppTreeNode* const form_order_app_tree_node = pFrame->GetDlgBar().m_OrderTree.GetFormOrderAppTreeNode(*pOrderDoc);
        pOrderDoc->ReleaseDicts();
        pFrame->GetDlgBar().m_OrderTree.ReleaseOrderNode(*form_order_app_tree_node);

        //Remove the order from the object tree
        FileTreeNode* pObjID = pFrame->GetDlgBar().m_ObjTree.FindNode(pDoc);
        pFrame->GetDlgBar().m_ObjTree.DeleteNode(*pObjID);

        // update the tab controls
        pFrame->GetDlgBar().UpdateTabs();

        HTREEITEM hItem = pFrame->GetDlgBar().m_ObjTree.GetSelectedItem();
        if(hItem){
            pFrame->GetDlgBar().m_ObjTree.SetActiveObject();
        }

    }

    else if(pDoc->IsKindOf(RUNTIME_CLASS(CTabulateDoc)))
    {
        CTabulateDoc* pTabDoc = (CTabulateDoc*)pDoc;

        if(pTabDoc->IsTabModified())
            pTabDoc->SetModifiedFlag(TRUE);

        if (!pTabDoc->SaveModified()) {
            return;
        }
        //release the reference

        TableSpecTabTreeNode* const table_spec_tab_tree_node = pFrame->GetDlgBar().m_TableTree.GetTableSpecTabTreeNode(*pTabDoc);
        pTabDoc->ReleaseDicts();
        pFrame->GetDlgBar().m_TableTree.ReleaseTableNode(*table_spec_tab_tree_node);

        //Remove the table from the object tree
        FileTreeNode* pObjID = pFrame->GetDlgBar().m_ObjTree.FindNode(pDoc);
        pFrame->GetDlgBar().m_ObjTree.DeleteNode(*pObjID);

        // update the tab controls
        pFrame->GetDlgBar().UpdateTabs();

        HTREEITEM hItem = pFrame->GetDlgBar().m_ObjTree.GetSelectedItem();
        if(hItem){
            pFrame->GetDlgBar().m_ObjTree.SetActiveObject();
        }

    }

    else if( pDoc->IsKindOf(RUNTIME_CLASS(CAplDoc)) )
    {
        CloseApplication(pDoc);
    }

    else
    {
        AfxMessageBox(_T("Doc not yet supported"));
    }
}


void CCSProApp::OnFileSave()
{
    for( CDocument* pDoc : GetSelectedTopLevelAssociatedDocuments(_T("Select Applications to Save")) )
        pDoc->OnSaveDocument(pDoc->GetPathName());
}


void CCSProApp::OnFileSaveAs()
{
    CDocument* pDoc = GetActiveDocument();

    if( pDoc == nullptr )
        return;

    // try to save the application
    if( std::optional<CAplDoc*> pAplDoc = GetActiveApplication(pDoc->GetPathName(), _T("Select Application to Save")); pAplDoc != nullptr )
    {
        if( pAplDoc.has_value() )
            SaveAsApplication(*pAplDoc);
    }

    // if there is no application open, a dictionary or form file may be open
    else
    {
        if( CDDDoc* pDictDoc = dynamic_cast<CDDDoc*>(pDoc); pDictDoc != nullptr )
            SaveAsDictionary(pDictDoc);

        else if( CFormDoc* pFormDoc = dynamic_cast<CFormDoc*>(pDoc); pFormDoc != nullptr )
            SaveAsFormFile(pFormDoc);

        else
            ASSERT(false);
    }
}


bool CCSProApp::SaveAsDictionary(CDDDoc* pDictDoc)
{
    ASSERT_VALID(pDictDoc);

    if (!pDictDoc->IsDocOK()) {
        return false;
    }

    CDocTemplate* pDocTempl = pDictDoc->GetDocTemplate();
    CString sDocFilter;
    pDocTempl->GetDocString(sDocFilter, CDocTemplate::filterName);
    CString sDocExt;
    pDocTempl->GetDocString(sDocExt, CDocTemplate::filterExt);

    CString csFilter = sDocFilter + _T("|*") + sDocExt + _T("|All Files (*.*)|*.*||");  // BMD 04 Sep 2003
    CString sDocExtNoPeriod = sDocExt.Right(sDocExt.GetLength() - 1);
    CIMSAFileDialog dlg (FALSE, sDocExtNoPeriod, nullptr, OFN_HIDEREADONLY, csFilter );
    bool bOK = false;
    while (!bOK) {
        if (dlg.DoModal() == IDCANCEL) {
            return false;
        }
        if (dlg.GetPathName() == pDictDoc->GetPathName()) {
            AfxMessageBox(_T("Can't replace viewed file. Use a different name."));
            continue;
        }
        CFileStatus status;
        if (CFile::GetStatus(dlg.GetPathName(), status)) {
            CString csMessage = dlg.GetPathName() + _T(" already exists.\nDo you want to replace it?");
            if (AfxMessageBox(csMessage,MB_YESNO|MB_DEFBUTTON2|MB_ICONEXCLAMATION) != IDYES) {
                continue;
            }
        }

        bOK =true;
    }

    RenameDictionaryFile(nullptr, pDictDoc->GetPathName(), dlg.GetPathName(), true);
    bool bSaveOk = pDictDoc->OnSaveDocument(dlg.GetPathName());
        if (!bSaveOk) {
                CString sMsg;
                sMsg.Format(IDS_SAVE_AS_SAVE_ERROR, dlg.GetPathName().GetString());
                AfxMessageBox(sMsg);
                return false;
        }

    pDictDoc->SetPathName(dlg.GetPathName()); // 20120207 moved from the RenameDictionaryFile function

    CMainFrame* const pFrame = assert_cast<CMainFrame*>(AfxGetMainWnd());
    pFrame->GetDlgBar().UpdateTabs();

    return true;
}


bool CCSProApp::SaveAsFormFile(CFormDoc* pFormDoc)
{
    ASSERT_VALID(pFormDoc);
    CDocTemplate* pDocTempl = pFormDoc->GetDocTemplate();
    CString sDocFilter;
    pDocTempl->GetDocString(sDocFilter, CDocTemplate::filterName);
    CString sDocExt;
    pDocTempl->GetDocString(sDocExt, CDocTemplate::filterExt);
    CString csFilter = sDocFilter + _T("|*") + sDocExt + _T("|All Files (*.*)|*.*||");  // BMD 04 Sep 2003
    CString sDocExtNoPeriod = sDocExt.Right(sDocExt.GetLength() - 1);
    CIMSAFileDialog dlg (FALSE, sDocExtNoPeriod, nullptr, OFN_HIDEREADONLY, csFilter );
    bool bOK = false;
    while (!bOK) {
        if (dlg.DoModal() == IDCANCEL) {
            return false;
        }
        if (dlg.GetPathName() == pFormDoc->GetPathName()) {
            AfxMessageBox(_T("Can't replace viewed file.  Use a different name."));
            continue;
        }
        CFileStatus status;
        if (CFile::GetStatus(dlg.GetPathName(), status)) {
            CString csMessage = dlg.GetPathName() + _T(" already exists.\nDo you want to replace it?");
            if (AfxMessageBox(csMessage,MB_YESNO|MB_DEFBUTTON2|MB_ICONEXCLAMATION) != IDYES) {
                continue;
            }
        }

        bOK =true;
    }

    const CString sNewFormName = dlg.GetPathName();

    // put up pif dlg to get dictionary file name
    CAplFileAssociationsDlg aplFileAssociationsDlg;
    CDEFormFile* pFormFile = &pFormDoc->GetFormFile();

    CString sDictName = pFormFile->GetDictionaryFilename();
    CString sNewDictName = sNewFormName.Left(sNewFormName.ReverseFind('.')) + UTF8_TODO::GetCString(FileExtensions::WithDot(FileExtensions::Dictionary));
    aplFileAssociationsDlg.m_fileAssociations.emplace_back(FileAssociation::Type::Dictionary, _T("Form File Dictionary"), true, sDictName, sNewDictName);

    aplFileAssociationsDlg.m_workingStorageType = CAplFileAssociationsDlg::WorkingStorageType::Hidden;
    aplFileAssociationsDlg.m_sTitle = _T("Save As");
    if(aplFileAssociationsDlg.DoModal() != IDOK) {
            return false;
    }

    CArray<bool, bool&> aCreateNewDocFlags;
    aCreateNewDocFlags.SetSize(1);
    if (!DuplicateSharedFilesForSaveAs(aplFileAssociationsDlg, aCreateNewDocFlags)) {
            return false;
    }

    CStringArray aDictNames;
    aDictNames.Add(aplFileAssociationsDlg.m_fileAssociations.front().GetNewFilename());

    RenameFormFile(nullptr, pFormDoc->GetPathName(), sNewFormName, aDictNames, aCreateNewDocFlags); // aCreateNewDocFlags means we skip any dicts we created new docs for

    bool bSaveOk = pFormDoc->OnSaveDocument(sNewFormName);
        if (!bSaveOk) {
                CString sMsg;
                sMsg.Format(IDS_SAVE_AS_SAVE_ERROR, sNewFormName.GetString());
                AfxMessageBox(sMsg);
                return false;
        }

    pFormDoc->SetPathName(sNewFormName); // 20120207 moved from RenameFormFile

    CMainFrame* const pFrame = assert_cast<CMainFrame*>(AfxGetMainWnd());
    pFrame->GetDlgBar().UpdateTabs();

    return true;
}


bool CCSProApp::SaveAsApplication(CAplDoc* pApplDoc)
{
   ASSERT_VALID(pApplDoc);
   Application& application = pApplDoc->GetAppObject();
       // FIXME: if doc is in multiple open apps (aAssociations.GetSize() > 1)
        // then need to leave both copies open so as not to screw up existing app.

    // get filename for base object

    CString sDocFilter;
    CString sDocExt;

    switch (pApplDoc->GetEngineAppType()) {
        case EngineAppType::Entry:
            sDocExt = FileExtensions::EntryApplication;
            sDocFilter = _T("Data Entry Applications (*.ent)");
            break;
        case EngineAppType::Batch:
            sDocExt = FileExtensions::BatchApplication;
            sDocFilter = _T("Batch Edit Applications (*.bch)");
            break;
        case EngineAppType::Tabulation:
            sDocExt = FileExtensions::TabulationApplication;
            sDocFilter = _T("Tabulation Applications (*.xtb)");
            break;
        default:
            ASSERT(false); // INVALID APP TYPE FOR SAVE AS
            break;
    }

    CString csFilter = sDocFilter + _T("|*.") + sDocExt + _T("|All Files (*.*)|*.*||");  // BMD 04 Sep 2003

    CString sNewAppName;

    bool bOK = false;
    while (!bOK) {

        CIMSAFileDialog dlg (FALSE, sDocExt, nullptr, OFN_HIDEREADONLY, csFilter );
        CString sAppPath = UTF8_TODO::GetCString(application.GetApplicationFilePath());
        PathRemoveFileSpec(sAppPath.GetBuffer(MAX_PATH));
        sAppPath.ReleaseBuffer();
        dlg.m_ofn.lpstrInitialDir = sAppPath;

        if (dlg.DoModal() == IDCANCEL) {
            return false;
        }
        if (dlg.GetPathName() == pApplDoc->GetPathName()) {
            AfxMessageBox(_T("Can't replace the current file.  Use a different name."));
            continue;
        }
        CFileStatus status;
        if (CFile::GetStatus(dlg.GetPathName(), status)) {
            CString csMessage = dlg.GetPathName() + _T(" already exists.\nDo you want to replace it?");
            if (AfxMessageBox(csMessage,MB_YESNO|MB_DEFBUTTON2|MB_ICONEXCLAMATION) != IDYES) {
                continue;
            }
        }

        sNewAppName = dlg.GetPathName();
        bOK =true;
    }

    // put up dialog to allow users to modify other object file names
    std::unique_ptr<CAplFileAssociationsDlg> pAplFileAssociationsDlg(new CAplFileAssociationsDlg);

    GetFileAssocsForSaveAs(pApplDoc, sNewAppName, *pAplFileAssociationsDlg);
        pAplFileAssociationsDlg->m_sAppName = sNewAppName;
        pAplFileAssociationsDlg->m_sTitle = _T("Save As");
        bool bKeepAsking = true;
        while (bKeepAsking) {
            if (pAplFileAssociationsDlg->DoModal() != IDOK) {
                    return false; // user cancel
            }

            // check if any files will overwrite existing files
            bKeepAsking = false;
            for (int iAssoc = 0; iAssoc < (int)pAplFileAssociationsDlg->m_fileAssociations.size(); ++iAssoc) {
                FileAssociation& file_association = pAplFileAssociationsDlg->m_fileAssociations[iAssoc];

                // make sure we have a full path for all the filenames (in case someone types in the name
                // rather than using the ... button)
                CString sNewAppPath = sNewAppName;
                PathRemoveFileSpec(sNewAppPath.GetBuffer(MAX_PATH));
                sNewAppPath.ReleaseBuffer();

                file_association.MakeNewFilenameFullPath(sNewAppPath);

                bool bFileExists = PortableFunctions::FileIsRegular(file_association.GetNewFilename());

                // check for overwrite existing file
                if (file_association.GetOriginalFilename() != file_association.GetNewFilename() && bFileExists ) {

                    // see if the new file is already open by another app
                    if (GetDoc(UTF8_TODO::GetUtf8(file_association.GetNewFilename())) != nullptr) {
                        // error - don't write over file that exists
                        CString sMsg;
                        sMsg.Format(IDS_OVERWRITE_OPEN_FILE, file_association.GetNewFilename().GetString());
                        AfxMessageBox(sMsg);
                        bKeepAsking = true;
                        break;
                    }
                    else {
                        // just a file on disk - give a warning
                        CString sMsg;
                        sMsg.Format(IDS_OVERWRITE_FILE, file_association.GetNewFilename().GetString());
                        UINT res = AfxMessageBox(sMsg, MB_YESNOCANCEL);
                        if (res == IDCANCEL) {
                            return false; // user cancel
                        }
                        if (res == IDNO) {
                            bKeepAsking = true;
                            break;
                        }
                    }
                }

                // don't allow user to rename dictionary in a form file that already exists on disk
                if (bFileExists && file_association.GetType() == FileAssociation::Type::FormFile) {
                    CString sFormDictName = GetDictFilenameFromFormFile(file_association.GetNewFilename());
                    ASSERT(iAssoc + 1 < (int)pAplFileAssociationsDlg->m_fileAssociations.size());
                    FileAssociation& next_file_association = pAplFileAssociationsDlg->m_fileAssociations[iAssoc + 1];
                    // use GetStatus to ensure we have full path for comparison
                    CFileStatus fileStatus;
                    CFile::GetStatus(next_file_association.GetNewFilename(),fileStatus);
                    if (sFormDictName.CompareNoCase(fileStatus.m_szFullName) != 0) {
                        CString sMsg;
                        sMsg.Format(IDS_CANT_RENAME_DICT_IN_FORM, file_association.GetNewFilename().GetString(), sFormDictName.GetString(),
                                    next_file_association.GetNewFilename().GetString());
                        AfxMessageBox(sMsg);
                        next_file_association.SetNewFilename(sFormDictName);
                    }
                }
            }

            if (bKeepAsking) {
                CAplFileAssociationsDlg* pNewaplFileAssociationsDlg = new CAplFileAssociationsDlg;
                pNewaplFileAssociationsDlg->m_fileAssociations = pAplFileAssociationsDlg->m_fileAssociations;
                pNewaplFileAssociationsDlg->m_sAppName = pAplFileAssociationsDlg->m_sAppName;
                pNewaplFileAssociationsDlg->m_sTitle = pAplFileAssociationsDlg->m_sTitle;
                pNewaplFileAssociationsDlg->m_workingStorageType = pAplFileAssociationsDlg->m_workingStorageType;
                pNewaplFileAssociationsDlg->m_bWorkingStorage = pAplFileAssociationsDlg->m_bWorkingStorage;
                pAplFileAssociationsDlg.reset(pNewaplFileAssociationsDlg);
            }
        }

        // check if any of the form or dict files are shared by other open apps - if so we need to create new
        // docs for them so we don't screw up the other open apps
        CArray<bool, bool&> aCreateNewDocFlags;
        aCreateNewDocFlags.SetSize((int)pAplFileAssociationsDlg->m_fileAssociations.size());
        if (!DuplicateSharedFilesForSaveAs(*pAplFileAssociationsDlg, aCreateNewDocFlags)) {
                return false;
        }

    // Save the files with the new names to get a copy disk (do this first before the rename
    // so that reconcile will work)
    SaveAppFilesWithNewNames(pApplDoc, sNewAppName, *pAplFileAssociationsDlg, aCreateNewDocFlags);

    // now go through and change names in app
    RenameApp(pApplDoc, sNewAppName, *pAplFileAssociationsDlg, aCreateNewDocFlags);

    //CString sOldAppName = pApplDoc->GetPathName();

    //Savy moved SetPathName before OnSave to fix "Save As" bug . //If you do not save the path
    //before the OnSaveDocument. You will not have an associated application for the form as the FormChildWnd Pathname
    //has already been renamed in the RenameApp call above.

    pApplDoc->SetPathName(sNewAppName,false); // 20111230 moved from the RenameApp function

    // do the save
    bool bSaveOk = pApplDoc->OnSaveDocument(sNewAppName);



        if (!bSaveOk) {
                CString sMsg;
                sMsg.Format(IDS_SAVE_AS_SAVE_ERROR, sNewAppName.GetString());
                AfxMessageBox(sMsg);
                return false;
        }

        else
            pApplDoc->SetPathName(sNewAppName);

        return true;
}

void CCSProApp::GetFileAssocsForSaveAs(CAplDoc* pApplDoc, const CString& sNewAppName, CAplFileAssociationsDlg& aplFileAssociationsDlg)
{
   CMainFrame* const pFrame = assert_cast<CMainFrame*>(AfxGetMainWnd());
   Application& application = pApplDoc->GetAppObject();

    switch (pApplDoc->GetEngineAppType())
    {
        case EngineAppType::Entry:
        {
            CString sMainFormName = UTF8_TODO::GetCString(application.GetFormFilePaths().front());
            CString sNewMainFormName = sNewAppName.Left(sNewAppName.ReverseFind('.')) + UTF8_TODO::GetCString(FileExtensions::WithDot(FileExtensions::Form));
            CString sAssocLabel;
            if (application.GetFormFilePaths().size() == 1) {
                sAssocLabel = _T("Form File");
            }
            else {
                sAssocLabel = _T("Form File 1");
            }

            aplFileAssociationsDlg.m_fileAssociations.emplace_back(FileAssociation::Type::FormFile, sAssocLabel, true, sMainFormName, sNewMainFormName);

            CFormTreeCtrl& formTree = pFrame->GetDlgBar().m_FormTree;
            CFormNodeID* const pID = formTree.GetFormNode(UTF8_TODO::GetUtf8(sMainFormName));
            ASSERT(pID != nullptr && pID->GetFormDoc());
            CDEFormFile* pFormFile = &pID->GetFormDoc()->GetFormFile();

            CString sDictName = pFormFile->GetDictionaryFilename();
            CString sNewDictName = sNewMainFormName.Left(sNewMainFormName.ReverseFind('.')) + UTF8_TODO::GetCString(FileExtensions::WithDot(FileExtensions::Dictionary));
            CString sDictAssocLabel;
            sDictAssocLabel.Format(_T("%s Dictionary"), sAssocLabel.GetString());
            aplFileAssociationsDlg.m_fileAssociations.emplace_back(FileAssociation::Type::Dictionary, sDictAssocLabel, true, sDictName, sNewDictName);

            if (application.GetFormFilePaths().size() > 1) { // first form is main form
                for (int iForm = 1; iForm < static_cast<int>(application.GetFormFilePaths().size()); ++iForm) {

                    CString sFormName = UTF8_TODO::GetCString(application.GetFormFilePaths()[iForm]);

                    sAssocLabel.Format(_T("Form File %d"), iForm+1);

                    aplFileAssociationsDlg.m_fileAssociations.emplace_back(FileAssociation::Type::FormFile, sAssocLabel, true, sFormName, GenUniqueFileName(sFormName));

                    // dictionary associated with the form
                    CFormNodeID* const pNodeID = formTree.GetFormNode(UTF8_TODO::GetUtf8(sFormName));
                    ASSERT(pNodeID && pNodeID->GetFormDoc());
                    CDEFormFile* const pNodeFormFile = &pNodeID->GetFormDoc()->GetFormFile();

                    CString csDictName = pNodeFormFile->GetDictionaryFilename();
                    CString csNewDictName = sNewMainFormName.Left(sNewMainFormName.ReverseFind('.')) + UTF8_TODO::GetCString(FileExtensions::WithDot(FileExtensions::Dictionary));
                    CString csDictAssocLabel;
                    csDictAssocLabel.Format(_T("%s Dictionary"), sAssocLabel.GetString());
                    aplFileAssociationsDlg.m_fileAssociations.emplace_back(FileAssociation::Type::Dictionary, csDictAssocLabel, true, csDictName, GenUniqueFileName(csDictName));
                }
            }
            break;
        }

        case EngineAppType::Batch:
        {
            ASSERT(application.GetFormFilePaths().size() == 1);
            CString sMainOrdName = UTF8_TODO::GetCString(application.GetFormFilePaths().front());
            COrderTreeCtrl& orderTree = pFrame->GetDlgBar().m_OrderTree;
            FormOrderAppTreeNode* const form_order_app_tree_node = orderTree.GetFormOrderAppTreeNode(UTF8_TODO::GetUtf8(sMainOrdName));
            ASSERT(form_order_app_tree_node->GetOrderDocument() != nullptr);
            CDEFormFile* const pOrderFile = &form_order_app_tree_node->GetOrderDocument()->GetFormFile();
            CString sOrigInputDictName = pOrderFile->GetDictionaryFilename();
            CString sNewInputDictName = sNewAppName.Left(sNewAppName.ReverseFind('.')) + UTF8_TODO::GetCString(FileExtensions::WithDot(FileExtensions::Dictionary));
            aplFileAssociationsDlg.m_fileAssociations.emplace_back(FileAssociation::Type::Dictionary, _T("Input Dictionary"), true, sOrigInputDictName, sNewInputDictName);
            break;
        }

        case EngineAppType::Tabulation:
        {
            ASSERT(application.GetTableSpecFilePaths().size() == 1);
            CTabulateDoc* const pDoc = DYNAMIC_DOWNCAST(CTabulateDoc, GetDoc(application.GetTableSpecFilePaths().front()));
            ASSERT_VALID(pDoc);
            CTabSet* pTabSpec = pDoc->GetTableSpec();
            ASSERT_VALID(pTabSpec);
            CString sOrigInputDictName = pTabSpec->GetDictFile();
            CString sNewInputDictName = sNewAppName.Left(sNewAppName.ReverseFind('.')) + UTF8_TODO::GetCString(FileExtensions::WithDot(FileExtensions::Dictionary));
            aplFileAssociationsDlg.m_fileAssociations.emplace_back(FileAssociation::Type::Dictionary, _T("Input Dictionary"), true, sOrigInputDictName, sNewInputDictName);
            break;
        }

        default:
            ASSERT(false); // INVALID APP TYPE FOR SAVE AS
    }

    // external dictionaries (but not ws or one used by form)
    int iExtDict = 1;
    bool bWorkingStorage = false;
    CString sOldWorkDictName;
    for( const DictionaryDescription& dictionary_description : application.GetDictionaryDescriptions() ) {
        if(dictionary_description.GetDictionaryType() == DictionaryType::External) {
            int iAssoc;
            for (iAssoc = 0; iAssoc < (int)aplFileAssociationsDlg.m_fileAssociations.size(); ++iAssoc) {
                if( SO::EqualsNoCase(aplFileAssociationsDlg.m_fileAssociations[iAssoc].GetOriginalFilename(), dictionary_description.GetDictionaryFilePath()) )
                    break;
            }
            if (iAssoc >= (int)aplFileAssociationsDlg.m_fileAssociations.size()) {
                // this dictionary wasn't used already in a form file
                CString sAssocLabel;
                sAssocLabel.Format(_T("External Dictionary %d"), iExtDict++);

                aplFileAssociationsDlg.m_fileAssociations.emplace_back(FileAssociation::Type::Dictionary, sAssocLabel, true,
                                                                       UTF8_TODO::GetCString(dictionary_description.GetDictionaryFilePath()), GenUniqueFileName(UTF8_TODO::GetCString(dictionary_description.GetDictionaryFilePath())));
            }
        } else if (dictionary_description.GetDictionaryType() == DictionaryType::Working) {
            bWorkingStorage = true;
            sOldWorkDictName = UTF8_TODO::GetCString(dictionary_description.GetDictionaryFilePath());
        }
    }

    if (bWorkingStorage) {
        aplFileAssociationsDlg.m_workingStorageType = CAplFileAssociationsDlg::WorkingStorageType::ReadOnly;
        aplFileAssociationsDlg.m_bWorkingStorage = TRUE;
    }
    else {
        aplFileAssociationsDlg.m_workingStorageType = CAplFileAssociationsDlg::WorkingStorageType::Hidden;
    }
}

bool CCSProApp::SaveAppFilesWithNewNames(CAplDoc* pApplDoc, const CString& sNewAppName, const CAplFileAssociationsDlg& aplFileAssociationsDlg,
    const CArray<bool, bool&>& aCreateNewDocFlags)
{
    // got all the object names, now save them
    for (int iAssoc = 0; iAssoc < (int)aplFileAssociationsDlg.m_fileAssociations.size(); ++iAssoc) {

        // can skip this we created a new doc instead of renaming existing on
        if (aCreateNewDocFlags[iAssoc]) {
            continue;
        }

        const FileAssociation& file_association = aplFileAssociationsDlg.m_fileAssociations[iAssoc];

        if (file_association.GetOriginalFilename().CompareNoCase(file_association.GetNewFilename()) != 0) {
            CDocument* pDoc = GetDoc(UTF8_TODO::GetUtf8(file_association.GetOriginalFilename()));

            if (file_association.GetType() == FileAssociation::Type::FormFile) {
                // for form files, need to save related dict first (with new name) so that form won't
                // save it with old name (since form save saves dictionary too)

                ASSERT(iAssoc+1 < (int)aplFileAssociationsDlg.m_fileAssociations.size());
                ASSERT(aplFileAssociationsDlg.m_fileAssociations[iAssoc+1].GetType() == FileAssociation::Type::Dictionary);
                // can skip this we created a new doc instead of renaming existing on
                if (!aCreateNewDocFlags[iAssoc+1]) {
                    const FileAssociation& dict_file_association = aplFileAssociationsDlg.m_fileAssociations[iAssoc + 1];
                    CDocument* pDictDoc = GetDoc(UTF8_TODO::GetUtf8(dict_file_association.GetOriginalFilename()));
                    pDictDoc->SetPathName(dict_file_association.GetNewFilename(), FALSE);
                    bool bSaveOk = pDictDoc->OnSaveDocument(dict_file_association.GetNewFilename());
                    pDictDoc->SetPathName(dict_file_association.GetOriginalFilename(), FALSE); // reset again in case of error - will do real rename later
                    if (!bSaveOk) {
                        CString sMsg;
                        sMsg.Format(IDS_SAVE_AS_SAVE_ERROR, dict_file_association.GetNewFilename().GetString());
                        AfxMessageBox(sMsg);
                        return false;
                    }
                    pDictDoc->SetModifiedFlag(FALSE); // so that form save won't save it under old name, will be set to true later
                                                      // during rename
                }

                // skip over dictionary since we handled already
                iAssoc++;
            }

            pDoc->SetPathName(file_association.GetNewFilename(), FALSE);
            bool bSaveOk = pDoc->OnSaveDocument(file_association.GetNewFilename());
            pDoc->SetPathName(file_association.GetOriginalFilename(), FALSE); // so that form save won't save it under old name,
                                                                              // will be set to true later // during rename
            if (!bSaveOk) {
                CString sMsg;
                sMsg.Format(IDS_SAVE_AS_SAVE_ERROR, file_association.GetNewFilename().GetString());
                AfxMessageBox(sMsg);
                return false;
            }
        }
    }

    // working storage
    std::wstring sNewWorkDictName = UTF8_TODO::GetWide(NewFileCreator::GetDefaultWorkingStorageDictionaryFilePath(UTF8_TODO::GetUtf8(sNewAppName)));
    if (aplFileAssociationsDlg.m_bWorkingStorage) {
        std::wstring sOldWorkDictName = UTF8_TODO::GetWide(NewFileCreator::GetDefaultWorkingStorageDictionaryFilePath(UTF8_TODO::GetUtf8(pApplDoc->GetPathName())));
        CDocument* pWSDDoc = GetDoc(UTF8_TODO::GetUtf8(sOldWorkDictName));
        if (pWSDDoc == nullptr) {
            // JH 2/6/05 do search instead of using def name
            // to support legacy apps that have non std ws dict names (eg benchmark)
            sOldWorkDictName = UTF8_TODO::GetWide(pApplDoc->GetAppObject().GetFirstDictionaryFilePathOfType(DictionaryType::Working));
            pWSDDoc = GetDoc(UTF8_TODO::GetUtf8(sOldWorkDictName));
        }
        ASSERT_VALID(pWSDDoc);
        pWSDDoc->SetPathName(sNewWorkDictName.c_str(), FALSE);
        bool bSaveOk = pWSDDoc->OnSaveDocument(sNewWorkDictName.c_str());
        pWSDDoc->SetPathName(sOldWorkDictName.c_str(), FALSE);
        if (!bSaveOk) {
            CString sMsg;
            sMsg.Format(IDS_SAVE_AS_SAVE_ERROR, sNewWorkDictName.c_str());
            AfxMessageBox(sMsg);
            return false;
        }
    }

    return true;
}

bool CCSProApp::DuplicateSharedFilesForSaveAs(CAplFileAssociationsDlg& aplFileAssociationsDlg, CArray<bool, bool&>& aCreateNewDocFlags)
{
    for (int iAssoc = 0; iAssoc < (int)aplFileAssociationsDlg.m_fileAssociations.size(); ++iAssoc) {
        const FileAssociation& file_association = aplFileAssociationsDlg.m_fileAssociations[iAssoc];

        aCreateNewDocFlags[iAssoc] = false;

        if (file_association.GetOriginalFilename().CompareNoCase(file_association.GetNewFilename()) != 0) {
            CDocument* pDoc = GetDoc(UTF8_TODO::GetUtf8(file_association.GetOriginalFilename()));

            if( GetTopLevelAssociatedDocuments<CDocument>(file_association.GetOriginalFilename()).size() > 1 ) {
                // more than one open app uses this file

                // should only ever be allowed for a dictionary
                ASSERT(file_association.GetType() == FileAssociation::Type::Dictionary);

                // save the document out under the new name (change name temporarily and then change it back)
                aCreateNewDocFlags[iAssoc] = true;
                BOOL bWasModified = pDoc->IsModified();
                pDoc->SetPathName(file_association.GetNewFilename(), FALSE);
                BOOL bSaveResult = pDoc->OnSaveDocument(file_association.GetNewFilename());
                pDoc->SetPathName(file_association.GetOriginalFilename(), FALSE);
                pDoc->SetModifiedFlag(bWasModified);

                if (!bSaveResult) {
                    CString sMsg;
                    sMsg.Format(IDS_SAVE_AS_SAVE_ERROR, file_association.GetNewFilename().GetString());
                    AfxMessageBox(sMsg);
                    return false;
                }

                // now open the new document
                CMainFrame* const pFrame = assert_cast<CMainFrame*>(AfxGetMainWnd());
                CDDTreeCtrl& dictTree = pFrame->GetDlgBar().m_DictTree;

                dictTree.AddDictionary(UTF8_TODO::GetUtf8(file_association.GetNewFilename()), nullptr);

                if (!dictTree.OpenDictionary(UTF8_TODO::GetUtf8(file_association.GetNewFilename()), FALSE)) {
                    CString sMsg;
                    sMsg.Format(IDS_SAVE_AS_SAVE_ERROR, file_association.GetNewFilename().GetString());
                    AfxMessageBox(sMsg);
                    return false;
                }

                // release ref for old dictionary for this app
                DictionaryDictTreeNode* old_dictionary_dict_tree_node = dictTree.GetDictionaryTreeNode(UTF8_TODO::GetUtf8(file_association.GetOriginalFilename()));
                ASSERT(old_dictionary_dict_tree_node != nullptr);
                old_dictionary_dict_tree_node->Release();
                ASSERT(old_dictionary_dict_tree_node->GetRefCount() > 0); // should be another app using it
            }
        }
    }

    return true;
}


void CCSProApp::RenameApp(CAplDoc* pApplDoc, const CString& sNewAppName, const CAplFileAssociationsDlg& aplFileAssociationsDlg,
    const CArray<bool, bool&>& aCreateNewDocFlags)
{
    ASSERT_VALID(pApplDoc);
    Application& application = pApplDoc->GetAppObject();

    // track dictionaries linked to forms/tabs/ords so we don't mistake them for
    // external dicts and rename them twice
    CStringArray aMainDicts;

    switch (pApplDoc->GetEngineAppType())
    {
        case EngineAppType::Entry:
            {
                ASSERT(application.GetFormFilePaths().size() > 0);
                for (int iForm = 0; iForm < static_cast<int>(application.GetFormFilePaths().size()); ++iForm) {

                    // get new name from file assocs
                    const std::string original_form_file_path = application.GetFormFilePaths()[iForm];
                    const FileAssociation* pAssoc = aplFileAssociationsDlg.FindFileAssoc(UTF8_TODO::GetCString(original_form_file_path));
                    ASSERT(pAssoc);

                    // change name in form list
                    application.RenameFormFilePath(original_form_file_path, UTF8_TODO::GetUtf8(pAssoc->GetNewFilename()));

                    // change app name in child frame wnd
                    CFormDoc* pFormDoc = DYNAMIC_DOWNCAST(CFormDoc, GetDoc(original_form_file_path));
                    ASSERT_VALID(pFormDoc);
                    CFormChildWnd* pFormChildWnd = (CFormChildWnd*) pFormDoc->GetView()->GetParentFrame();
                    ASSERT(pFormChildWnd);
                    pFormChildWnd->SetApplicationName(sNewAppName);

                    // get dictionary children new names
                    CDEFormFile* pFormSpec = &pFormDoc->GetFormFile();
                    CStringArray aDictNames;
                    CArray<bool, bool&> aSkipDictFlags;

                    const CString sOrigDictName = pFormSpec->GetDictionaryFilename();
                    int iPifInd = aplFileAssociationsDlg.FindFileAssocIndex(sOrigDictName);
                    ASSERT(iPifInd != NONE);
                    const FileAssociation& dict_file_association = aplFileAssociationsDlg.m_fileAssociations[iPifInd];
                    aDictNames.Add(dict_file_association.GetNewFilename());
                    aSkipDictFlags.Add((bool)aCreateNewDocFlags[iPifInd]); // skip rename of dict if this was one we created
                    aMainDicts.Add(sOrigDictName);

                    // rename form and child dicts
                    RenameFormFile(pApplDoc, UTF8_TODO::GetCString(original_form_file_path), pAssoc->GetNewFilename(), aDictNames, aSkipDictFlags);
                }
            }
            break;

        case EngineAppType::Batch:
            {
                ASSERT(!application.GetFormFilePaths().empty());
                for (int iForm = 0; iForm < static_cast<int>(application.GetFormFilePaths().size()); ++iForm) {
                    const CString sOrigOrdName = UTF8_TODO::GetCString(application.GetFormFilePaths()[iForm]);
                    const CString sNewOrdName = sNewAppName.Left(sNewAppName.ReverseFind('.')) + UTF8_TODO::GetCString(FileExtensions::WithDot(FileExtensions::Order));

                    // change app name in order frame wnd
                    COrderDoc* pOrderDoc = DYNAMIC_DOWNCAST(COrderDoc, GetDoc(UTF8_TODO::GetUtf8(sOrigOrdName)));
                    COrderChildWnd* pOrderChildWnd = (COrderChildWnd*) pOrderDoc->GetView()->GetParentFrame();
                    ASSERT(pOrderChildWnd);
                    pOrderChildWnd->SetApplicationName(sNewAppName);

                                        // get dictionary children and change names
                    CDEFormFile* pOrderSpec = &pOrderDoc->GetFormFile();
                    CStringArray aNewDictNames;
                    CArray<bool, bool&> aSkipDictFlags;

                    const CString sOrigDictName = pOrderSpec->GetDictionaryFilename();
                    int iPifInd = aplFileAssociationsDlg.FindFileAssocIndex(sOrigDictName);
                    ASSERT(iPifInd != NONE);
                    const FileAssociation& dict_file_association = aplFileAssociationsDlg.m_fileAssociations[iPifInd];
                    aNewDictNames.Add(dict_file_association.GetNewFilename());
                    aSkipDictFlags.Add((bool)aCreateNewDocFlags[iPifInd]); // skip rename of dict if this was one we created
                    aMainDicts.Add(sOrigDictName);

                    RenameOrdFile(pApplDoc, sOrigOrdName, sNewOrdName, aNewDictNames, aSkipDictFlags);

                    // change name in form list
                    application.RenameFormFilePath(UTF8_TODO::GetUtf8(sOrigOrdName), UTF8_TODO::GetUtf8(sNewOrdName));

                }
            }
            break;

        case EngineAppType::Tabulation:
            {
                ASSERT(application.GetTableSpecFilePaths().size() == 1);

                CTabulateDoc* const pTabDoc = DYNAMIC_DOWNCAST(CTabulateDoc, GetDoc(application.GetTableSpecFilePaths().front()));
                ASSERT_VALID(pTabDoc);
                CTabSet* pTabSpec = pTabDoc->GetTableSpec();
                ASSERT_VALID(pTabSpec);
                const CString sOrigXTSName = UTF8_TODO::GetCString(application.GetTableSpecFilePaths().front());
                const CString sNewXTSName = sNewAppName.Left(sNewAppName.ReverseFind('.')) + UTF8_TODO::GetCString(FileExtensions::WithDot(FileExtensions::TableSpec));

                // get dictionary
                const CString sOldDictName = pTabSpec->GetDictFile();
                const FileAssociation* pDictAssoc = aplFileAssociationsDlg.FindFileAssoc(sOldDictName);
                ASSERT(pDictAssoc);
                aMainDicts.Add(sOldDictName);

                // change dictionary name (if it isn't one we created)
                if (!aCreateNewDocFlags[aplFileAssociationsDlg.FindFileAssocIndex(sOldDictName)]) {
                    RenameDictionaryFile(pApplDoc, sOldDictName, pDictAssoc->GetNewFilename(), false);
                }
                else {
                    RenameNodeInObjTree(pApplDoc, sOldDictName, pDictAssoc->GetNewFilename());
                }

                RenameTabSpecFile(pApplDoc, sOrigXTSName, sNewXTSName, pDictAssoc->GetNewFilename());

                application.RenameTableSpecFilePath(UTF8_TODO::GetUtf8(sOrigXTSName), UTF8_TODO::GetUtf8(sNewXTSName));
            }
            break;

        default:
            ASSERT(false); // INVALID APP TYPE FOR SAVE AS
    }

    // external dictionaries (not used by form)
    for( const DictionaryDescription& dictionary_description : application.GetDictionaryDescriptions() ) {
        CString sOrigFName = UTF8_TODO::GetCString(dictionary_description.GetDictionaryFilePath());

        // skip dicts already renamed above
        if (FindInArray(aMainDicts, sOrigFName) >= 0) {
            continue;
        }

        if(dictionary_description.GetDictionaryType() == DictionaryType::External) {
            // external dictionary

            // get the new file name for the dict from assocs
            const FileAssociation* pAssoc = aplFileAssociationsDlg.FindFileAssoc(sOrigFName);
            ASSERT(pAssoc);

            // rename it
            // if this is doc we created, skip it, don't need to rename
            if (!aCreateNewDocFlags[aplFileAssociationsDlg.FindFileAssocIndex(sOrigFName)]) {
                RenameDictionaryFile(pApplDoc, sOrigFName, pAssoc->GetNewFilename(), false);
            }
            else {
                RenameNodeInObjTree(pApplDoc, sOrigFName, pAssoc->GetNewFilename());
            }

            // change name in dictionary list
            application.RenameExternalDictionaryFilePath(UTF8_TODO::GetUtf8(sOrigFName), UTF8_TODO::GetUtf8(pAssoc->GetNewFilename()));

        } else if (dictionary_description.GetDictionaryType() == DictionaryType::Working) {
            std::wstring sNewWorkDictName = UTF8_TODO::GetWide(NewFileCreator::GetDefaultWorkingStorageDictionaryFilePath(UTF8_TODO::GetUtf8(sNewAppName)));
            RenameDictionaryFile(pApplDoc, sOrigFName, WS2CS(sNewWorkDictName), false);

            // change name in dictionary list
            application.RenameExternalDictionaryFilePath(UTF8_TODO::GetUtf8(sOrigFName), UTF8_TODO::GetUtf8(sNewWorkDictName));
        }
    }

    RenameAppCodeFile(pApplDoc, PortableFunctions::PathAppendFileExtension(sNewAppName, UTF8_TODO::GetWide(FileExtensions::Logic)));
    RenameMessageFile(pApplDoc, PortableFunctions::PathAppendFileExtension(sNewAppName, UTF8_TODO::GetWide(FileExtensions::Message)));

    if (pApplDoc->GetEngineAppType() == EngineAppType::Entry) {
        const CString sNewQsfName = sNewAppName + UTF8_TODO::GetCString(FileExtensions::WithDot(FileExtensions::QuestionText));
        RenameQSFFile(pApplDoc, sNewQsfName);
    }

    // rename the top-level app doc and node itself
    RenameNodeInObjTree(nullptr, pApplDoc->GetPathName(), sNewAppName);

    // 20111230 removed because the file hasn't been saved at this point ... we'll set the path name when it's been saved
    // pApplDoc->SetPathName(sNewAppName);

    // FIXME: handle case where dict or form is shared by > 1 open app

    // do the save
    pApplDoc->SetModifiedFlag(TRUE);
}

void CCSProApp::RenameDictionaryFile(CDocument* pParentDoc,
                                     CString sOldDictFName,
                                     const CString& sNewDictFName,
                                     bool /*bAddToMRU*/)
{
    CMainFrame* const pFrame = assert_cast<CMainFrame*>(AfxGetMainWnd());

    // change name of dict doc
    CDocument* pDictDoc = GetDoc(UTF8_TODO::GetUtf8(sOldDictFName));
    ASSERT_VALID(pDictDoc);
    //pDictDoc->SetPathName(sNewDictFName, bAddToMRU);
    pDictDoc->SetPathName(sNewDictFName,false);
    pDictDoc->SetModifiedFlag(TRUE);

    // change name in dict tree
    CDDTreeCtrl& dictTree = pFrame->GetDlgBar().m_DictTree;
    DictionaryDictTreeNode* const dictionary_dict_tree_node = dictTree.GetDictionaryTreeNode(UTF8_TODO::GetUtf8(sOldDictFName));
    dictionary_dict_tree_node->SetPath(UTF8_TODO::GetUtf8(sNewDictFName));

    // find node in object tree
    RenameNodeInObjTree(pParentDoc, sOldDictFName, sNewDictFName);
}

void CCSProApp::RenameFormFile(CAplDoc* pApplDoc,
                               CString sOrigFormName,
                               const CString& sNewFormName,
                               const CStringArray& aNewDictNames,
                               const CArray<bool, bool&>& aDictSkipFlags)
{
    CMainFrame* const pFrame = assert_cast<CMainFrame*>(AfxGetMainWnd());

    // change name in form doc
    CFormDoc* pFormDoc = DYNAMIC_DOWNCAST(CFormDoc, GetDoc(UTF8_TODO::GetUtf8(sOrigFormName)));
    ASSERT_VALID(pFormDoc);
    //const bool bAddToMRU = (pApplDoc == nullptr); // only to add to mru for standlaone forms
    //pFormDoc->SetPathName(sNewFormName, bAddToMRU);
    pFormDoc->SetPathName(sNewFormName,false);
    pFormDoc->SetModifiedFlag(TRUE);

    // change name in form spec
    CDEFormFile* pFormSpec = &pFormDoc->GetFormFile();
    pFormSpec->SetFilePath(UTF8_TODO::GetUtf8(sNewFormName));

    // set the name in the object tree
    RenameNodeInObjTree(pApplDoc, sOrigFormName, sNewFormName);

    // get dictionary children and change names
    const CString sOrigDictName = pFormSpec->GetDictionaryFilename();
    const CString& sNewDictName = aNewDictNames[0];
    if (!aDictSkipFlags[0]) {
            RenameDictionaryFile(pApplDoc != 0 ? (CDocument*) pApplDoc : (CDocument*) pFormDoc,
                                                    sOrigDictName, sNewDictName, false);
    }
    else {
            RenameNodeInObjTree(pApplDoc != 0 ? (CDocument*) pApplDoc : (CDocument*) pFormDoc,
                                                    sOrigDictName, sNewDictName);
    }

    pFormSpec->SetDictionaryFilename(sNewDictName);

    // change name in form tree control
    CFormTreeCtrl& formTree = pFrame->GetDlgBar().m_FormTree;
    CFormNodeID* pFormNodeId = formTree.GetFormNode(UTF8_TODO::GetUtf8(sOrigFormName));
    pFormNodeId->SetFFName(sNewFormName);
}


void CCSProApp::RenameOrdFile(CAplDoc* pApplDoc,
                              CString sOrigOrdName,
                              const CString& sNewOrdName,
                              const CStringArray& aNewDictNames,
                              const CArray<bool, bool&>& aDictSkipFlags)
{
    CMainFrame* const pFrame = assert_cast<CMainFrame*>(AfxGetMainWnd());

    // change name in order doc
    COrderDoc* pOrderDoc = DYNAMIC_DOWNCAST(COrderDoc, GetDoc(UTF8_TODO::GetUtf8(sOrigOrdName)));
    ASSERT_VALID(pOrderDoc);
    const bool bAddToMRU = (pApplDoc == nullptr); // only to add to mru for standlaone ord
    pOrderDoc->SetPathName(sNewOrdName, bAddToMRU);
    pOrderDoc->SetModifiedFlag(TRUE);

    // change name in ord spec
    CDEFormFile* pOrderSpec = &pOrderDoc->GetFormFile();
    pOrderSpec->SetFilePath(UTF8_TODO::GetUtf8(sNewOrdName));

    // set the name in the object tree
    RenameNodeInObjTree(pApplDoc, sOrigOrdName, sNewOrdName);

    // get dictionary children and change names
    const CString sOrigDictName = pOrderSpec->GetDictionaryFilename();
    const CString& sNewDictName = aNewDictNames[0];
    if (!aDictSkipFlags[0]) {
        RenameDictionaryFile(pApplDoc != 0 ? (CDocument*) pApplDoc : (CDocument*) pOrderDoc,
            sOrigDictName, sNewDictName, false);
    }
    else {
        RenameNodeInObjTree(pApplDoc != 0 ? (CDocument*) pApplDoc : (CDocument*) pOrderDoc,
            sOrigDictName, sNewDictName);
    }
    pOrderSpec->SetDictionaryFilename(sNewDictName);

    // change name in order tree control
    COrderTreeCtrl& orderTree = pFrame->GetDlgBar().m_OrderTree;
    FormOrderAppTreeNode* const form_order_app_tree_node = orderTree.GetFormOrderAppTreeNode(UTF8_TODO::GetUtf8(sOrigOrdName));
    form_order_app_tree_node->SetPath(UTF8_TODO::GetUtf8(sNewOrdName));
}


void CCSProApp::RenameTabSpecFile(CAplDoc* pApplDoc,
                                  CString sOrigXTSName,
                                  const CString& sNewXTSName,
                                  const CString& sNewDictName)
{
    // update xts name
    Application& application = pApplDoc->GetAppObject();
    ASSERT(application.GetTableSpecFilePaths().size() == 1);
    CTabulateDoc* const pTabDoc = DYNAMIC_DOWNCAST(CTabulateDoc, GetDoc(application.GetTableSpecFilePaths().front()));
    ASSERT_VALID(pTabDoc);
    CTabSet* pTabSpec = pTabDoc->GetTableSpec();
    ASSERT_VALID(pTabSpec);
    pTabSpec->SetSpecFile(sNewXTSName);
    pTabDoc->SetPathName(sNewXTSName, FALSE);
    pTabDoc->SetModifiedFlag(TRUE);

    // change name in tab tree
    CMainFrame* const pFrame = assert_cast<CMainFrame*>(AfxGetMainWnd());
    CTabTreeCtrl& tabTree = pFrame->GetDlgBar().m_TableTree;
    TableSpecTabTreeNode* const table_spec_tab_tree_node = tabTree.GetTableSpecTabTreeNode(UTF8_TODO::GetUtf8(sOrigXTSName));
    table_spec_tab_tree_node->SetPath(UTF8_TODO::GetUtf8(sNewXTSName));

    pTabSpec->SetDictFile(sNewDictName);
    pTabDoc->SetDictFileName(sNewDictName);

    // change name in obj tree
    RenameNodeInObjTree(pApplDoc, sOrigXTSName, sNewXTSName);
}

void CCSProApp::RenameAppCodeFile(CAplDoc* pAplDoc, const CString& new_filename)
{
    const std::shared_ptr<TextSourceEditable> logic_text_source = pAplDoc->GetLogicMainCodeFileTextSource();
    RenameNodeInObjTree(pAplDoc, UTF8_TODO::GetWide(logic_text_source->GetFilePath()), new_filename);
    logic_text_source->SetNewFilePath(UTF8_TODO::GetUtf8(new_filename));

    if( pAplDoc->GetAppObject().GetAppSrcCode() != nullptr )
        pAplDoc->GetAppObject().GetAppSrcCode()->SetModifiedFlag(true);
}

void CCSProApp::RenameMessageFile(CAplDoc* pAplDoc, const CString& new_filename)
{
    const std::shared_ptr<TextSourceEditable> message_text_source = pAplDoc->GetMessageTextSource();
    RenameNodeInObjTree(pAplDoc, UTF8_TODO::GetWide(message_text_source->GetFilePath()), new_filename);
    message_text_source->SetNewFilePath(UTF8_TODO::GetUtf8(new_filename));
}

void CCSProApp::RenameQSFFile(CAplDoc* pApplDoc, const CString& sNewFName)
{
    Application& application = pApplDoc->GetAppObject();
    RenameNodeInObjTree(pApplDoc, UTF8_TODO::GetWide(application.GetQuestionTextFilePath()), sNewFName);
    application.SetQuestionTextFilePath(UTF8_TODO::GetUtf8(sNewFName));
    pApplDoc->m_pQuestMgr->SetModifiedFlag(true);
}

void CCSProApp::RenameNodeInObjTree(CDocument* pTopLevelDoc, wstring_view old_filename, const CString& sNewFName)
{
    CMainFrame* const pFrame = assert_cast<CMainFrame*>(AfxGetMainWnd());
    CObjTreeCtrl& objTree = pFrame->GetDlgBar().m_ObjTree;
    FileTreeNode* pObjNode = nullptr;
    if (pTopLevelDoc == nullptr) {
        // no top level doc - this is top level (app, standalone form, standalone dict)
        pObjNode = objTree.FindNode(UTF8_TODO::GetUtf8(old_filename));
    }
    else {
        FileTreeNode* pTopLevelItemId = objTree.FindNode(UTF8_TODO::GetUtf8(pTopLevelDoc->GetPathName()));
        ASSERT(pTopLevelItemId);
        pObjNode = objTree.FindChildNodeRecursive(*pTopLevelItemId, UTF8_TODO::GetUtf8(old_filename));
    }
    ASSERT(pObjNode);

    pObjNode->SetRenamedPath(UTF8_TODO::GetUtf8(sNewFName));
}


void CCSProApp::CloseApplication(CDocument* pDoc)
{
    CMainFrame* const pFrame = assert_cast<CMainFrame*>(AfxGetMainWnd());

    VERIFY(pDoc->IsKindOf(RUNTIME_CLASS(CAplDoc)));

    if(assert_cast<CAplDoc*>(pDoc)->IsAppModified()) {
        CString sMsg;
        sMsg.FormatMessage(IDS_APPMODIFIED, pDoc->GetPathName().GetString());
        int iRet = AfxMessageBox(sMsg,MB_YESNOCANCEL);
        if(iRet == IDYES) {
            pDoc->OnSaveDocument(pDoc->GetPathName());
        }
        else if(iRet == IDCANCEL) {
            return;
        }

    }

    pDoc->OnCloseDocument();
    HTREEITEM hItem = pFrame->GetDlgBar().m_ObjTree.GetSelectedItem();
    if(hItem){
        pFrame->GetDlgBar().m_ObjTree.SetActiveObject();
    }
    pFrame->GetDlgBar().UpdateTabs();

    //The active window will not change 'cos this application has multiple associations
    CMDIChildWnd* pActiveWnd = ((CMDIFrameWnd*)pFrame)->MDIGetActive();
    if(pActiveWnd && pActiveWnd->IsKindOf(RUNTIME_CLASS(CFormChildWnd))) {
        if(pActiveWnd->GetActiveView()->IsKindOf(RUNTIME_CLASS(CFSourceEditView))) {
           pFrame->SendMessage(UWM::Form::ShowSourceCode, 0, (LPARAM)pActiveWnd->GetActiveDocument());
        }
    }
    else if(pActiveWnd && pActiveWnd->IsKindOf(RUNTIME_CLASS(COrderChildWnd))) {
        pFrame->SendMessage(UWM::Order::ShowSourceCode, 0, (LPARAM)pActiveWnd->GetActiveDocument());
    }
}


bool CCSProApp::UpdateViews(CDocument* pDoc)
{
    CWaitCursor wait;

    HTREEITEM hObjItem = nullptr;
    CString sFileName = pDoc->GetPathName();
    CMainFrame* const pFrame = assert_cast<CMainFrame*>(AfxGetMainWnd());
    CObjTreeCtrl& ObjTree = pFrame->GetDlgBar().m_ObjTree;

    //---------------- Application

    if(pDoc->IsKindOf(RUNTIME_CLASS(CAplDoc)))
    {
        SaveRFL();
        hObjItem = assert_cast<CAplDoc*>(pDoc)->BuildAllTrees();
        FileTreeNode* pObjNode = ObjTree.FindNode(UTF8_TODO::GetUtf8(sFileName));
        pObjNode->SetDocument(pDoc);

        if(!assert_cast<CAplDoc*>(pDoc)->OpenAllDocuments()) {
            assert_cast<CAplDoc*>(pDoc)->OnCloseDocument();
            return FALSE;
        }

        RestoreRFL();
        m_pRecentFileList->Add(sFileName);
        ObjTree.DefaultExpand(hObjItem);
    }

    //-----------------Dictionary

    else if(pDoc->IsKindOf(RUNTIME_CLASS(CDDDoc)))
    {
        //Insert the dictionary in the objtree
        hObjItem = ObjTree.InsertNode(TVI_ROOT, std::make_unique<DictionaryFileTreeNode>(UTF8_TODO::GetUtf8(sFileName)));
        FileTreeNode* pObjNode = ObjTree.FindNode(UTF8_TODO::GetUtf8(sFileName));
        pObjNode->SetDocument(pDoc);

        CDDTreeCtrl& dictTree = pFrame->GetDlgBar().m_DictTree;
        const bool adding_new_dict_tree_node = ( dictTree.GetDictionaryTreeNode(*pDoc) == nullptr );

        dictTree.AddDictionary(UTF8_TODO::GetUtf8(sFileName), assert_cast<CDDDoc*>(pDoc));

        if( adding_new_dict_tree_node ) {
            assert_cast<CDDDoc*>(pDoc)->SetDictTreeCtrl(&pFrame->GetDlgBar().m_DictTree);
            assert_cast<CDDDoc*>(pDoc)->InitTreeCtrl();
        }

        //Update the tabs
        ObjTree.DefaultExpand(hObjItem);
        pFrame->GetDlgBar().m_tabCtl.UpdateTabs();
    }

    // ---------------------- Form spec

    else if(pDoc->IsKindOf(RUNTIME_CLASS(CFormDoc)))
    {
        SaveRFL();
        CFormDoc* pFormDoc = (CFormDoc*) pDoc;

        pFormDoc->SetFormTreeCtrl (&pFrame->GetDlgBar().m_FormTree);

        if ( !pFormDoc->LoadFormSpecFile (sFileName) )  // load up the spec file
        {
            pFormDoc->ReleaseDicts();
            pFormDoc->OnCloseDocument();
            return false;
        }

        pFormDoc->BuildAllTrees();
        if (!pFormDoc->LoadDictSpecFile(FALSE)){
            pFormDoc->ReleaseDicts();
            pFormDoc->OnCloseDocument();
            return false;
        }
        if(pFormDoc){
            CFormScrollView* pView = (CFormScrollView*)pFormDoc->GetView();
            ASSERT(pView);
            int iNumForms = pFormDoc->GetFormFile().GetNumForms();
            if (iNumForms > 0){
                pFormDoc->SetCurFormIndex (0);
                pView->SetFormIndex (0);
                pView->RecreateGrids (0);
                pView->SetupFontRelatedOffsets();
            }
        }
        pFormDoc->InitTreeCtrl();       // finally the form tree ctrl will be built herein


        // this blk adds my form onto the Object tab
        const CString& csFileName = pFormDoc->GetPathName();

        hObjItem = ObjTree.InsertFormNode(TVI_ROOT, UTF8_TODO::GetUtf8(csFileName), AppFileType::Form);
        FileTreeNode* pObjNode = ObjTree.FindNode(UTF8_TODO::GetUtf8(csFileName));
        pObjNode->SetDocument(pFormDoc);

        pFrame->GetDlgBar().m_tabCtl.UpdateTabs();

        RestoreRFL();
        m_pRecentFileList->Add(csFileName);
        ObjTree.DefaultExpand(hObjItem);
    }

    //----------------Order spec

    else if(pDoc->IsKindOf(RUNTIME_CLASS(COrderDoc)))
    {
        COrderDoc* pOrderDoc = assert_cast<COrderDoc*>(pDoc);
        SaveRFL();

        pOrderDoc->SetOrderTreeCtrl (&pFrame->GetDlgBar().m_OrderTree);


        if (!pOrderDoc->LoadOrderSpecFile (sFileName) )        // load up the spec file
        {
            pOrderDoc->ReleaseDicts();
            pOrderDoc->OnCloseDocument();
            return false;
        }
        pOrderDoc->BuildAllTrees();

        if (!pOrderDoc->LoadDictSpecFile())
        {
            pOrderDoc->ReleaseDicts();
            pOrderDoc->OnCloseDocument();
            return false;
        }

        pOrderDoc->InitTreeCtrl();      // finally the form tree ctrl will be built herein


        // this blk adds my order onto the Object tab
        const CString& csFileName = pOrderDoc->GetPathName();

        hObjItem = ObjTree.InsertFormNode(TVI_ROOT, UTF8_TODO::GetUtf8(csFileName), AppFileType::Order);
        FileTreeNode* pObjNode = ObjTree.FindNode(UTF8_TODO::GetUtf8(csFileName));
        pObjNode->SetDocument(pOrderDoc);

        pFrame->GetDlgBar().m_tabCtl.UpdateTabs();
        RestoreRFL();
        m_pRecentFileList->Add(csFileName);
        ObjTree.DefaultExpand(hObjItem);
    }

    //----------------Table spec

    else if(pDoc->IsKindOf(RUNTIME_CLASS(CTabulateDoc)))
    {
        CTabulateDoc* pTabDoc = (CTabulateDoc*)pDoc;
        SaveRFL();
        pTabDoc->SetTabTreeCtrl(&pFrame->GetDlgBar().m_TableTree);

        pTabDoc->BuildAllTrees();

        BOOL b = pTabDoc->LoadSpecFile(sFileName);
        if (!b) {
            AfxMessageBox(_T("Failed to load Tab file"));
            pTabDoc->ReleaseDicts();
            pTabDoc->OnCloseDocument();
            return false;
        }
        pTabDoc->InitTreeCtrl();

        const CString& fileName = pTabDoc->GetPathName();

        hObjItem = ObjTree.InsertTableNode(TVI_ROOT, UTF8_TODO::GetUtf8(fileName));
        FileTreeNode* pObjNode = ObjTree.FindNode(UTF8_TODO::GetUtf8(fileName));
        pObjNode->SetDocument(pTabDoc);

        pFrame->GetDlgBar().m_tabCtl.UpdateTabs();
        RestoreRFL();
        m_pRecentFileList->Add(fileName);
        ObjTree.DefaultExpand(hObjItem);
    }

    else
    {
        AfxMessageBox(_T("Doc not yet supported"));
        return false;
    }

    // Update screen
    pFrame->GetDlgBar().m_tabCtl.UpdateTabs();

    if(hObjItem)
    {
        pFrame->GetDlgBar().m_ObjTree.SelectItem(hObjItem);
        pFrame->GetDlgBar().m_ObjTree.SetActiveObject();
    }

    SetInitialViews(pDoc);
    DropHObjects(pDoc);

    return true;
}

/***************************************************************************
This function returns CDocument Object of the given File name if the given is "OPEN"
as an Object in the memory
****************************************************************************/

CDocument* CCSProApp::IsDocOpen(LPCTSTR lpszFileName)const
{
    // find the highest confidence
    POSITION pos = AfxGetApp()->GetFirstDocTemplatePosition();
    CDocTemplate::Confidence bestMatch = CDocTemplate::noAttempt;
    CDocTemplate* pBestTemplate = nullptr;
    CDocument* pOpenDocument = nullptr;

    TCHAR szPath[_MAX_PATH];
    ASSERT(lstrlen(lpszFileName) < _countof(szPath));
    TCHAR szTemp[_MAX_PATH];
    if (lpszFileName[0] == '\"')
        ++lpszFileName;
    lstrcpyn(szTemp, lpszFileName, _MAX_PATH);
    LPTSTR lpszLast = _tcsrchr(szTemp, '\"');
    if (lpszLast != nullptr)
        *lpszLast = 0;
    AfxFullPath(szPath, szTemp);
    TCHAR szLinkName[_MAX_PATH];
    if (AfxResolveShortcut(AfxGetMainWnd(), szPath, szLinkName, _MAX_PATH))
        lstrcpy(szPath, szLinkName);

    while (pos != nullptr)
    {
        CDocTemplate* pTemplate = (CDocTemplate*)AfxGetApp()->GetNextDocTemplate(pos);
        ASSERT_KINDOF(CDocTemplate, pTemplate);

        CDocTemplate::Confidence match;
        ASSERT(pOpenDocument == nullptr);
        match = pTemplate->MatchDocType(szPath, pOpenDocument);
        if (match > bestMatch)
        {
            bestMatch = match;
            pBestTemplate = pTemplate;
        }
        if (match == CDocTemplate::yesAlreadyOpen)
            break;      // stop here
    }

    return pOpenDocument;
}

//Call Back function for the browseforfolder dialog
int CALLBACK BrowseCallbackProc( HWND hwnd, UINT uMsg, LPARAM /*lParam*/, LPARAM lpData )
{
    if (uMsg == BFFM_INITIALIZED)
    {
        // Set the initial folder
        SendMessage(hwnd, BFFM_SETSELECTION, TRUE, lpData);
    }

    return 0;
}

BOOL CCSProApp::OnOpenRecentFile(UINT nID)
{
    CWaitCursor wait;
    ASSERT_VALID(this);
    ASSERT(m_pRecentFileList != nullptr);

    ASSERT(nID >= ID_FILE_MRU_FILE1);
    ASSERT(nID < ID_FILE_MRU_FILE1 + (UINT)m_pRecentFileList->GetSize());
    int nIndex = nID - ID_FILE_MRU_FILE1;
    ASSERT((*m_pRecentFileList)[nIndex].GetLength() != 0);

    TRACE2("MRU: open file (%d) '%s'.\n", (nIndex) + 1, (*m_pRecentFileList)[nIndex].GetString());
    CDocument* pDoc = nullptr;
    CString sFileName = (*m_pRecentFileList)[nIndex];

    CString sFName = sFileName;
    CString sExt(PathFindExtension(sFName.GetBuffer(_MAX_PATH)));
    sFName.ReleaseBuffer();
    if(sExt.CompareNoCase(UTF8_TODO::GetCString(FileExtensions::WithDot(FileExtensions::Order))) == 0 ){
        (*m_pRecentFileList).Remove(nIndex);
        AfxMessageBox(_T("Cannot open .ord files"));
        return FALSE;
    }

    if(IsFileOpen(UTF8_TODO::GetUtf8(sFileName))) {
        CString sMsg;
        sMsg.FormatMessage(IDS_FILEOPEN, sFileName.GetString());
        AfxMessageBox(sMsg);

        //Get the document frame and show it
        CDocument* pOpenDoc = IsDocOpen(sFileName);
        SetInitialViews(pOpenDoc);
        return TRUE;
        }

    CFileStatus fStatus;
    BOOL bRet = CFile::GetStatus(sFileName,fStatus);
    if(!bRet) {
        CString sMsg;
        sMsg.FormatMessage(_T("%1 File not found"), sFileName.GetString());
        AfxMessageBox(sMsg);
                (*m_pRecentFileList).Remove(nIndex);
        return FALSE;
    }

    pDoc = OpenDocumentFile(sFileName);

    if(pDoc) {
        CString csErr;
        if(!UpdateViews(pDoc)){
            (*m_pRecentFileList).Remove(nIndex);
            return FALSE;
        }
        Reconcile (pDoc, csErr, false, true);    // silent no; autofix yes
    }

    return TRUE;
}


void CCSProApp::AddResourceToApplication(CAplDoc& application_doc, AppResource resource)
{
    CMainFrame* const pFrame = assert_cast<CMainFrame*>(AfxGetMainWnd());
    CObjTreeCtrl& ObjTree = pFrame->GetDlgBar().m_ObjTree;

    FileTreeNode* const application_file_tree_node = ObjTree.FindNode(&application_doc);
    ASSERT(application_file_tree_node != nullptr);

    ObjTree.InsertNode(*application_file_tree_node, std::make_unique<ResourceFileTreeNode>(resource.GetPath()));

    application_doc.GetAppObject().AddResource(std::move(resource));
    application_doc.SetModifiedFlag();
}


CString GetDictFilenameFromFormFile(const CString& sFormFileName)
{
    CSpecFile frmFile(TRUE);
    std::vector<std::string> dictionary_file_paths;

    if( !frmFile.Open(sFormFileName, CFile::modeRead) )
    {
        AfxMessageBox(FormatText(_T("File %s Could not be opened"), sFormFileName.GetString()));
    }

    else
    {
        dictionary_file_paths = GetFileNameArrayFromSpecFile(frmFile, CSPRO_DICTS);
        ASSERT(dictionary_file_paths.size() == 1);
        frmFile.Close();
    }

    return dictionary_file_paths.empty() ? CString() :
                                           UTF8_TODO::GetCString(dictionary_file_paths.front());
}


void CCSProApp::SyncExternalTextSources(CAplDoc* pAppDoc)
{
    CMDlgBar& dlgBar = assert_cast<CMainFrame*>(AfxGetMainWnd())->GetDlgBar();
    CTreeCtrl* pTreeCtrl;

    if( pAppDoc->GetEngineAppType() == EngineAppType::Entry )
        pTreeCtrl = &dlgBar.m_FormTree;

    else if( pAppDoc->GetEngineAppType() == EngineAppType::Batch )
        pTreeCtrl = &dlgBar.m_OrderTree;

    else
        return;

    HTREEITEM hItem = pTreeCtrl->GetSelectedItem();

    if( hItem == nullptr )
        return;

    auto pID = pTreeCtrl->GetItemData(hItem);

    if( pID == 0 )
        return;

    auto send_message = [](auto pID, auto message)
    {
        if( pID->GetTextSource() != nullptr )
            AfxGetMainWnd()->SendMessage(message, 0, reinterpret_cast<LPARAM>(pID));
    };

    if( pAppDoc->GetEngineAppType() == EngineAppType::Entry )
    {
        send_message((CFormID*)pID, UWM::Form::PutSourceCode);
    }

    else
    {
        send_message(reinterpret_cast<AppTreeNode*>(pID), UWM::Order::PutSourceCode);
    }
}


// Function name        : CCSProApp::SetInitialViews
// Description      : Sets the initial views for the docs (Project is not yet implemented)
// Return type          : void
// Argument         : CDocument* pDoc
void CCSProApp::SetInitialViews(CDocument* pDoc)
{
    CMainFrame* const pFrame = assert_cast<CMainFrame*>(AfxGetMainWnd());
    CMDlgBar& DlgBar = pFrame->GetDlgBar();
    CString sTabString = DICT_TAB_LABEL;

    EngineAppType appType = EngineAppType::Invalid;
    if(pDoc->IsKindOf(RUNTIME_CLASS(CAplDoc))) {
        Application& appObject = assert_cast<CAplDoc*>(pDoc)->GetAppObject();
        appType = appObject.GetEngineAppType();

        if(appType == EngineAppType::Entry) {
            const std::string& form_file_path = appObject.GetFormFilePaths().front();
            CFormTreeCtrl& formTree = DlgBar.m_FormTree;
            CFormNodeID* const pNode = formTree.GetFormNode(form_file_path);
            if( pNode != nullptr ) {
                pDoc = pNode->GetFormDoc();
            }
        }
        else if(appType == EngineAppType::Batch) {
            sTabString = ORDER_TAB_LABEL;
            const std::string& order_file_path = appObject.GetFormFilePaths().front();
            COrderTreeCtrl& orderTree = DlgBar.m_OrderTree;
            FormOrderAppTreeNode* const form_order_app_tree_node = orderTree.GetFormOrderAppTreeNode(order_file_path);
            if( form_order_app_tree_node != nullptr ) {
                pDoc = form_order_app_tree_node->GetOrderDocument();
            }
        }
        else if(appType == EngineAppType::Tabulation) {
            const std::string& table_spec_file_path = appObject.GetTableSpecFilePaths().front();
            CTabTreeCtrl& tableTree = DlgBar.m_TableTree;
            TableSpecTabTreeNode* const table_spec_tab_tree_node = tableTree.GetTableSpecTabTreeNode(table_spec_file_path);
            if(table_spec_tab_tree_node != nullptr) {
                pDoc = table_spec_tab_tree_node->GetTabDoc();
            }
        }
        else {
            return;
        }
    }
    if(!pDoc)
        return;

    CString sDictName;
    if(pDoc->IsKindOf(RUNTIME_CLASS(CFormDoc)) || (appType == EngineAppType::Entry)){
        sDictName = assert_cast<CFormDoc*>(pDoc)->GetFormFile().GetDictionaryFilename();
        if(!m_bFileNew){
            sTabString = FORM_TAB_LABEL;
        }
    }
    else if(pDoc->IsKindOf(RUNTIME_CLASS(COrderDoc)) || (appType == EngineAppType::Batch)){
        sDictName = assert_cast<COrderDoc*>(pDoc)->GetFormFile().GetDictionaryFilename();
    }
    else if(pDoc->IsKindOf(RUNTIME_CLASS(CTabulateDoc)) || (appType == EngineAppType::Tabulation)) {
        sDictName = ((CTabulateDoc*)pDoc)->GetTableSpec()->GetDictFile();
    }
    else if(pDoc->IsKindOf(RUNTIME_CLASS(CDDDoc))) {
        sDictName = assert_cast<CDDDoc*>(pDoc)->GetPathName();
    }

    //Get the dictionary name to be selected on the dict tree

    if(!sDictName.IsEmpty()) {
        CDDTreeCtrl& dictTree = DlgBar.m_DictTree;
        DictionaryDictTreeNode* const dictionary_dict_tree_node = dictTree.GetDictionaryTreeNode(UTF8_TODO::GetUtf8(sDictName));
        if(dictionary_dict_tree_node != nullptr) {
            if((appType != EngineAppType::Invalid)) {
                if(IsDictNew(dictionary_dict_tree_node->GetDDDoc())) {
                    pDoc = dictionary_dict_tree_node->GetDDDoc();
                }
            }
            HTREEITEM hItem = dictionary_dict_tree_node->GetHItem();

            if(hItem) {
                dictTree.SelectItem(hItem);
            }
        }
    }



    POSITION pos = pDoc->GetFirstViewPosition();
    CView* pView = nullptr;
    if(pDoc->IsKindOf(RUNTIME_CLASS(CFormDoc))){
        pView= assert_cast<CFormDoc*>(pDoc)->GetView();
    }
    else {
        pView = pDoc->GetNextView(pos);
    }
    if(pView) {
        CFrameWnd* pWnd = pView->GetParentFrame();
        if(pWnd->IsKindOf(RUNTIME_CLASS(CMDIChildWnd))) {
            pWnd->ActivateFrame(SW_SHOWMAXIMIZED);
        }
        CFormScrollView* pFormView = DYNAMIC_DOWNCAST(CFormScrollView,pView);
        if(pFormView && m_bFileNew){
            m_bFileNew = false;
            if (pDoc != nullptr) {
                int iNum = assert_cast<CFormDoc*>(pDoc)->GetCurForm()->GetNumItems();
                if (iNum == 0) {
                    pFormView->CreateNewFF();
                }
            }
        }
    }

    //Select the dictionary tab
    pFrame->GetDlgBar().SelectTab(sTabString, 0);
}


BOOL CCSProApp::PreTranslateMessage(MSG* pMsg)
{
    // test for switch window focus via keystroke
    if( m_pWindowFocusMgr->PreTranslateMessage(m_pMainWnd, pMsg) )
        return TRUE;

    return CWinApp::PreTranslateMessage(pMsg);
}


CDocument* CCSProApp::GetDoc(const std::string_view file_path_sv)
{
    CMainFrame* const pFrame = assert_cast<CMainFrame*>(AfxGetMainWnd());
    const std::optional<AppFileType> app_file_type = GetAppFileTypeFromFileExtension(PortableFunctions::PathGetFileExtension(file_path_sv));

    auto get_document_from_tree_node =
        [](TreeNode* const tree_node_with_document)
        {
            return ( tree_node_with_document != nullptr ) ? tree_node_with_document->GetDocument() :
                                                            nullptr;
        };

    // if the extension type is unknown, check each document type
    if( app_file_type.value_or(AppFileType::Dictionary) == AppFileType::Dictionary )
    {
        return get_document_from_tree_node(pFrame->GetDlgBar().m_DictTree.GetDictionaryTreeNode(file_path_sv));
    }

    if( app_file_type.value_or(AppFileType::Form) == AppFileType::Form )
    {
        CFormNodeID* const pNode = pFrame->GetDlgBar().m_FormTree.GetFormNode(file_path_sv);

        if( pNode != nullptr )
            return pNode->GetFormDoc();
    }

    if( app_file_type.value_or(AppFileType::Order) == AppFileType::Order )
    {
        return get_document_from_tree_node(pFrame->GetDlgBar().m_OrderTree.GetFormOrderAppTreeNode(file_path_sv));
    }

    if( app_file_type.value_or(AppFileType::TableSpec) == AppFileType::TableSpec )
    {
        TableSpecTabTreeNode* const table_spec_tab_tree_node = pFrame->GetDlgBar().m_TableTree.GetTableSpecTabTreeNode(file_path_sv);

        if( table_spec_tab_tree_node != nullptr )
            return table_spec_tab_tree_node->GetTabDoc();
    }

    if( IsApplicationType(app_file_type.value_or(AppFileType::ApplicationEntry)) )
    {
        return get_document_from_tree_node(pFrame->GetDlgBar().m_ObjTree.FindNode(file_path_sv));
    }

    return nullptr;
}


void CCSProApp::ManageFiles(const cs::cref_optional<std::string> initial_path_to_select)
{
    const std::optional<CAplDoc*> active_application_doc = GetActiveApplication(nullptr, L"Select Application to Manage Files");

    if( !active_application_doc.has_value() || *active_application_doc == nullptr )
    {
        ASSERT(!active_application_doc.has_value());
        return;
    }

    CAplDoc* const application_doc = *active_application_doc;

    // if external code or reports are being edited, update the text in case the tree is redrawn
    SyncExternalTextSources(application_doc);

    DesignerApplicationLoaderWithFileSupport application_loader(*application_doc);

    auto new_application = std::make_unique<Application>(application_doc->GetAppObject());

    ManageFilesDlg dlg(*new_application, application_loader, CObjTreeCtrl::CreateAppFileTypeImageList());

    if( initial_path_to_select.has_value() )
        dlg.InitiallySelectPath(*initial_path_to_select);

    if( dlg.DoModal() != IDOK )
        return;

    // replace the current application this with one
    application_doc->SetModifiedFlag();
    application_doc->ReplaceAppObject(std::move(new_application));

    // mark any other objects modified
    for( const std::string& file_path : dlg.GetNonApplicationObjectsModified() )
    {
        ForeachDoc<CDocument>(
            [&](CDocument& document)
            {
                if( SO::EqualsNoCase(file_path, document.GetPathName()) )
                {
                    document.SetModifiedFlag();
                    return false;
                }

                return true;
            });
    }

    // redraw trees and open documents as needed
    PostManageFilesChanges(*application_doc, dlg);
}


void CCSProApp::OnManageFiles()
{
    ManageFiles(std::nullopt);
}


void CCSProApp::PostManageFilesChanges(CAplDoc& application_doc, const ManageFilesDlg& dlg)
{
    CMainFrame* const pFrame = assert_cast<CMainFrame*>(AfxGetMainWnd());
    CObjTreeCtrl& ObjTree = pFrame->GetDlgBar().m_ObjTree;

    Application& application = application_doc.GetAppObject();

    const FileTreeNode* const application_file_tree_node = ObjTree.FindNode(&application_doc);
    ASSERT(application_file_tree_node != nullptr);

    // 1) remove any entries from the Files tree, and possibly the Dicts and Forms tree
    for( const auto& [path, app_file_type] : dlg.GetFilesRemoved() )
    {
        FileTreeNode* const remove_file_tree_node = ObjTree.FindChildNodeRecursive(*application_file_tree_node, path);
        ASSERT(remove_file_tree_node != nullptr);

        if( remove_file_tree_node != nullptr )
            ObjTree.DeleteNode(*remove_file_tree_node);

        // form files
        if( app_file_type == AppFileType::Form )
        {
            pFrame->GetDlgBar().m_FormTree.RemoveFormFile(path);
        }

        // external dictionaries
        else if( app_file_type == AppFileType::Dictionary )
        {
            pFrame->GetDlgBar().m_DictTree.RemoveDictionary(path);
        }
    }


    // 2) update any external code files in case the code type changed
    for( const CodeFile& code_file : application.GetCodeFiles() )
    {
        CodeFileTreeNode* const code_file_tree_node = assert_nullable_cast<CodeFileTreeNode*>(
            ObjTree.FindChildNodeRecursive(*application_file_tree_node, code_file.GetFilePath()));

        if( code_file_tree_node != nullptr )
            code_file_tree_node->Update(code_file);
    }


    // 3) add any new entries to the Files tree, and possibly the Dicts and Forms tree
    for( const auto& [path, app_file_type] : dlg.GetFilesAdded() )
    {
        auto insert_application_item_under_parent = [&](const std::string_view parent_file_path_sv, std::unique_ptr<FileTreeNode> file_tree_node)
        {
            // find the parent's node
            FileTreeNode* const parent_file_tree_node = ObjTree.FindChildNodeRecursive(*application_file_tree_node, parent_file_path_sv);

            if( parent_file_tree_node != nullptr )
                ObjTree.InsertNode(parent_file_tree_node->GetHItem(), std::move(file_tree_node));
        };

        // form files
        if( app_file_type == AppFileType::Form )
        {
            pFrame->GetDlgBar().m_FormTree.AddFormFile(path, nullptr, true);
            ObjTree.InsertFormNode(application_file_tree_node, path, AppFileType::Form);
        }

        // external dictionaries
        else if( app_file_type == AppFileType::Dictionary )
        {
            pFrame->GetDlgBar().m_DictTree.AddDictionary(path, nullptr);
            ObjTree.InsertNode(*application_file_tree_node, std::make_unique<DictionaryFileTreeNode>(path));
        }

        // external code files
        else if( app_file_type == AppFileType::Code )
        {
            const CodeFile* const code_file = application.GetCodeFile(path);
            ASSERT(code_file != nullptr);

            // external code is added under the parent's node
            if( code_file != nullptr )
            {
                insert_application_item_under_parent(application_doc.GetLogicMainCodeFileTextSource()->GetFilePath(),
                                                     std::make_unique<CodeFileTreeNode>(*code_file));
            }
        }

        // external message files
        else if( app_file_type == AppFileType::Message )
        {
            // external messages are added under the parent's node
            insert_application_item_under_parent(application_doc.GetMessageTextSource()->GetFilePath(),
                                                 std::make_unique<MessageFileTreeNode>(path, true));
        }

        // reports
        else if( app_file_type == AppFileType::Report )
        {
            ObjTree.InsertNode(*application_file_tree_node, std::make_unique<ReportFileTreeNode>(path));
        }

        // resources
        else if( app_file_type == AppFileType::Resource )
        {
            ObjTree.InsertNode(*application_file_tree_node, std::make_unique<ResourceFileTreeNode>(path));
        }

        else
        {
            ASSERT(false);
        }
    }


    // open all related documents, which will also set the application objects (SetAppObjects)
    application_doc.OpenAllDocuments();
    application_doc.ReconcileDictTypes();

    // invalidate the Files tree in case any labels changed
    ObjTree.Invalidate();

    // update the tab controls
    pFrame->GetDlgBar().UpdateTabs();

    // redraw the active view
    assert_cast<CMainFrame*>(AfxGetMainWnd())->MDIGetActive()->GetActiveView()->RedrawWindow();
}


//Update for CSBatch support 05/22/00
BOOL CCSProApp::Reconcile(CDocument *pDoc, CString& csErr, bool bSilent, bool bAutoFix)
{
    BOOL  bRet = FALSE;
    CString sFileName = pDoc->GetPathName();
    const std::string extension = PortableFunctions::PathGetFileExtension(UTF8_TODO::GetUtf8(sFileName));

    if( SO::EqualsOneOfNoCase(extension, FileExtensions::EntryApplication,
                                         FileExtensions::TabulationApplication,
                                         FileExtensions::BatchApplication) )
    {
        ASSERT(pDoc->IsKindOf(RUNTIME_CLASS(CAplDoc)));
        bRet = assert_cast<CAplDoc*>(pDoc)->Reconcile(csErr, bSilent, bAutoFix);
    }

    else if( SO::EqualsNoCase(extension, FileExtensions::Form) )
    {
        ASSERT(pDoc->IsKindOf(RUNTIME_CLASS(CFormDoc)));

        // first stuff dictionary pointers into formfile object
        CMainFrame* const pFrame = assert_cast<CMainFrame*>(AfxGetMainWnd());
        CDDTreeCtrl& dictTree = pFrame->GetDlgBar().m_DictTree;
        CDEFormFile* pFormFile = &assert_cast<CFormDoc*>(pDoc)->GetFormFile();

        DictionaryDictTreeNode* const dictionary_dict_tree_node = dictTree.GetDictionaryTreeNode(UTF8_TODO::GetUtf8(pFormFile->GetDictionaryFilename()));
        CDDDoc* pDDDoc = dictionary_dict_tree_node->GetDDDoc();
        if(pDDDoc) {
            pFormFile->SetDictionary(pDDDoc->GetSharedDictionary());
        }

        // then reconcile
        bRet = assert_cast<CFormDoc*>(pDoc)->GetFormFile().Reconcile(csErr, bSilent, bAutoFix);
        if (!bRet) {
            CFormTreeCtrl& formTree = pFrame->GetDlgBar().m_FormTree;
            CFormNodeID* const pFormID = formTree.GetFormNode(UTF8_TODO::GetUtf8(sFileName));
            pFormID->GetFormDoc()->SetModifiedFlag();
            formTree.ReBuildTree();
        }
    }

    else if( SO::EqualsNoCase(extension, FileExtensions::Order) )
    {
        ASSERT(pDoc->IsKindOf(RUNTIME_CLASS(COrderDoc)));

        // first stuff dictionary pointers into formfile object
        CMainFrame* const pFrame = assert_cast<CMainFrame*>(AfxGetMainWnd());
        CDDTreeCtrl& dictTree = pFrame->GetDlgBar().m_DictTree;
        CDEFormFile* const pOrderFile = &assert_cast<COrderDoc*>(pDoc)->GetFormFile();

        DictionaryDictTreeNode* const dictionary_dict_tree_node = dictTree.GetDictionaryTreeNode(UTF8_TODO::GetUtf8(pOrderFile->GetDictionaryFilename()));
        CDDDoc* pDDDoc = dictionary_dict_tree_node->GetDDDoc();
        if(pDDDoc) {
            pOrderFile->SetDictionary(pDDDoc->GetSharedDictionary());
        }

        // then reconcile
        bRet = assert_cast<COrderDoc*>(pDoc)->GetFormFile().OReconcile(csErr, bSilent, bAutoFix);

        if (!bRet) {
            COrderTreeCtrl& orderTree = pFrame->GetDlgBar().m_OrderTree;
            FormOrderAppTreeNode* const form_order_app_tree_node = orderTree.GetFormOrderAppTreeNode(UTF8_TODO::GetUtf8(sFileName));
            form_order_app_tree_node->GetOrderDocument()->SetModifiedFlag();
            orderTree.ReBuildTree();
        }
    }

    else if( SO::EqualsNoCase(extension, FileExtensions::TableSpec) )
    {
        ASSERT(pDoc->IsKindOf(RUNTIME_CLASS(CTabulateDoc)));
        bRet = ((CTabulateDoc*)pDoc)->Reconcile(csErr, bSilent, bAutoFix);
    }

    else
    {
        bRet = true;
    }

    return bRet;
}

bool CCSProApp::IsDictNew(const CDDDoc* pDoc)
{
    // Check if there are any non-ID items in the dictionary; if not then consider it as new
    ASSERT(pDoc);

    for( const DictLevel& dict_level : pDoc->GetDict()->GetLevels() )
    {
        for( int r = 0; r < dict_level.GetNumRecords(); ++r )
        {
            if( dict_level.GetRecord(r)->GetNumItems() > 0 )
                return false;
        }
    }

    return true;
}


/////////////////////////////////////////////////////////////////////////////
//
//                         CCSProApp::OnViewNames
//
/////////////////////////////////////////////////////////////////////////////

void CCSProApp::OnViewNames()
{
    SharedSettings::ToggleViewNamesInTree();
    assert_cast<CMainFrame*>(AfxGetMainWnd())->GetDlgBar().UpdateTrees();
}

void CCSProApp::OnUpdateViewNames(CCmdUI* pCmdUI)
{
    pCmdUI->SetCheck(SharedSettings::ViewNamesInTree());
}

void CCSProApp::OnViewAppendLabelsToNames()
{
    SharedSettings::ToggleAppendLabelsToNamesInTree();
    assert_cast<CMainFrame*>(AfxGetMainWnd())->GetDlgBar().UpdateTrees();
}

void CCSProApp::OnUpdateViewAppendLabelsToNames(CCmdUI* pCmdUI)
{
    pCmdUI->SetCheck(SharedSettings::AppendLabelsToNamesInTree());
}


void CCSProApp::OnChangeTab() // 20100406
{
    CMDlgBar& dlgBar = assert_cast<CMainFrame*>(AfxGetMainWnd())->GetDlgBar();

    int curSelection = dlgBar.m_tabCtl.GetCurFocus();

    curSelection++;

    if( curSelection == dlgBar.m_tabCtl.GetItemCount() ) // loop back to 1 (skipping the "Files" tab)
        curSelection = 1;

    dlgBar.m_tabCtl.SetCurFocus(curSelection);
}


void CCSProApp::OnUpdateRecentFileMenu(CCmdUI* pCmdUI)
{
    ASSERT_VALID(this);
    if (m_pRecentFileList == nullptr) // no MRU files
        pCmdUI->Enable(FALSE);
    else
        m_pRecentFileList->UpdateMenu(pCmdUI);
}

void CCSProApp::SaveRFL()
{
    m_arrRFLStrings.RemoveAll();

    if(m_pRecentFileList) {

        for(int iIndex = 0; iIndex < m_pRecentFileList->GetSize(); iIndex++) {
             CString sName = (*m_pRecentFileList)[0];
             if(!sName.IsEmpty())
                 m_arrRFLStrings.Add(sName);
             m_pRecentFileList->Remove(0);
        }
     }
}

void CCSProApp::RestoreRFL()
{
    if(m_pRecentFileList) {
        for(int iIndex = 0; iIndex < m_pRecentFileList->GetSize(); iIndex++) {
            m_pRecentFileList->Remove(0);
        }
        for( int iIndex = m_arrRFLStrings.GetSize() -1; iIndex >= 0; iIndex--) {
            // JH 11/29/05 - added try/catch to catch CFileException::badPath thrown
            // by CRecentFileList::Add when one of the recent file list strings
            // has a path that is no longer valid e.g. was on a removeable drive
            // that has been removed.
            TRY {
                m_pRecentFileList->Add(m_arrRFLStrings[iIndex]);
            }
            CATCH( CFileException, pEx )
            {
            }
            END_CATCH
        }
    }
}


//Remove the hanging objects from the object tree
void CCSProApp::DropHObjects(CDocument* pDoc)
{
    CMainFrame* const pFrame = assert_cast<CMainFrame*>(AfxGetMainWnd());
    CObjTreeCtrl& ObjTree = pFrame->GetDlgBar().m_ObjTree;
    CDDTreeCtrl& dictTree = pFrame->GetDlgBar().m_DictTree;

    ASSERT(pDoc);
    if(pDoc->IsKindOf(RUNTIME_CLASS(CDDDoc))){
        return ; //Nothing needs to be done
    }
    else if(pDoc->IsKindOf(RUNTIME_CLASS(CFormDoc))){
        //Check if the dictionaries are free hanging on the object tree and remove them
        CFormDoc* pFormDoc = assert_cast<CFormDoc*>(pDoc);
        CString sDictName = pFormDoc->GetFormFile().GetDictionaryFilename();
        FileTreeNode* const file_tree_node = ObjTree.FindNode(UTF8_TODO::GetUtf8(sDictName));
        if( file_tree_node != nullptr ) {
            DictionaryDictTreeNode* const dictionary_dict_tree_node = dictTree.GetDictionaryTreeNode(UTF8_TODO::GetUtf8(sDictName));
            ASSERT(dictionary_dict_tree_node != nullptr);
            dictionary_dict_tree_node->Release();
            ObjTree.DeleteNode(*file_tree_node);
        }
    }
    else if(pDoc->IsKindOf(RUNTIME_CLASS(CTabulateDoc))){
        //Check if the dictionaries are free hanging on the object tree and remove them
        CTabulateDoc* pTabDoc = (CTabulateDoc*)pDoc;
        CString sDictName = pTabDoc->GetDictFileName();

        FileTreeNode* const file_tree_node = ObjTree.FindNode(UTF8_TODO::GetUtf8(sDictName));
        if( file_tree_node != nullptr ) {
            DictionaryDictTreeNode* const dictionary_dict_tree_node = dictTree.GetDictionaryTreeNode(UTF8_TODO::GetUtf8(sDictName));
            ASSERT(dictionary_dict_tree_node != nullptr);
            dictionary_dict_tree_node->Release();
            ObjTree.DeleteNode(*file_tree_node);
        }

    }
    else if(pDoc->IsKindOf(RUNTIME_CLASS(COrderDoc))){
        //Check if the dictionaries are free hanging on the object tree and remove them
        COrderDoc* pOrderDoc = assert_cast<COrderDoc*>(pDoc);
        CString sDictName = pOrderDoc->GetFormFile().GetDictionaryFilename();
        FileTreeNode* const file_tree_node = ObjTree.FindNode(UTF8_TODO::GetUtf8(sDictName));
        if( file_tree_node != nullptr ) {
            DictionaryDictTreeNode* const dictionary_dict_tree_node = dictTree.GetDictionaryTreeNode(UTF8_TODO::GetUtf8(sDictName));
            ASSERT(dictionary_dict_tree_node != nullptr);
            dictionary_dict_tree_node->Release();
            ObjTree.DeleteNode(*file_tree_node);
        }
    }
    else if(pDoc->IsKindOf(RUNTIME_CLASS(CAplDoc))){
        ProcessAppHObjects(assert_cast<CAplDoc*>(pDoc));
    }
    else {
        return;
    }
}

//Remove the hanging objects from the object tree
void CCSProApp::ProcessAppHObjects(CAplDoc* pAplDoc)
{
    CMainFrame* const pFrame = assert_cast<CMainFrame*>(AfxGetMainWnd());
    CObjTreeCtrl& ObjTree = pFrame->GetDlgBar().m_ObjTree;
    CDDTreeCtrl& dictTree = pFrame->GetDlgBar().m_DictTree;
    CFormTreeCtrl& formTree = pFrame->GetDlgBar().m_FormTree;
    CTabTreeCtrl& tableTree = pFrame->GetDlgBar().m_TableTree;
    COrderTreeCtrl& orderTree = pFrame->GetDlgBar().m_OrderTree;

    const Application& application = pAplDoc->GetAppObject();
    EngineAppType appType = pAplDoc->GetEngineAppType();

    //Do Forms
    if(appType == EngineAppType::Entry) {
        for( const std::string& form_file_path : application.GetFormFilePaths() ) {
            FileTreeNode* const file_tree_node = ObjTree.FindNode(form_file_path);
            CFormNodeID* pFormID = formTree.GetFormNode(form_file_path);

            if( file_tree_node != nullptr ) {
                ASSERT(pFormID);
                DropHObjects(pFormID->GetFormDoc());        // Remove free hanging dictionaries belonging to the form
                formTree.ReleaseFormDependencies(pFormID);  // remove form dependencies
                //release the form node
                formTree.ReleaseFormNodeID(pFormID);        //Release form node id
                ObjTree.DeleteNode(*file_tree_node);        //delete the node from the object tree
            }
            else {
                ASSERT(pFormID);
                DropHObjects(pFormID->GetFormDoc());        // Remove free hanging dictionaries belonging to the form
            }
        }
    }

    //Do tables if they exist
    if(appType == EngineAppType::Tabulation) {
        for( const std::string& table_spec_file_path : application.GetTableSpecFilePaths() ) {
            FileTreeNode* const file_tree_node = ObjTree.FindNode(table_spec_file_path);
            TableSpecTabTreeNode* const table_spec_tab_tree_node = tableTree.GetTableSpecTabTreeNode(table_spec_file_path);
            ASSERT(table_spec_tab_tree_node != nullptr);

            if( file_tree_node != nullptr ) {
                DropHObjects(table_spec_tab_tree_node->GetTabDoc());            // Remove free hanging dictionaries belonging to the table
                tableTree.ReleaseTableDependencies(*table_spec_tab_tree_node);  // remove table dependencies
                //release the table node
                tableTree.ReleaseTableNode(*table_spec_tab_tree_node);          //Release table node id
                ObjTree.DeleteNode(*file_tree_node);                            //delete the node from the object tree
            }
             else {
                DropHObjects(table_spec_tab_tree_node->GetTabDoc());            // Remove free hanging dictionaries belonging to the form
            }
        }
    }

    //Do Orders if they exist
    if(appType == EngineAppType::Batch) {
        for( const std::string& order_file_path : application.GetFormFilePaths() ) {
            FileTreeNode* const file_tree_node = ObjTree.FindNode(order_file_path);
            FormOrderAppTreeNode* const form_order_app_tree_node = orderTree.GetFormOrderAppTreeNode(order_file_path);
            ASSERT(form_order_app_tree_node != nullptr);

            if( file_tree_node != nullptr ) {
                DropHObjects(form_order_app_tree_node->GetOrderDocument());     // Remove free hanging dictionaries belonging to the table
                orderTree.ReleaseOrderDependencies(*form_order_app_tree_node);  // remove table dependencies
                //release the order node
                orderTree.ReleaseOrderNode(*form_order_app_tree_node);          // Release table node id
                ObjTree.DeleteNode(*file_tree_node);                            // delete the node from the object tree
            }
            else {
                DropHObjects(form_order_app_tree_node->GetOrderDocument());     // Remove free hanging dictionaries belonging to the form
            }
        }
    }

    //Do External Dictionaries if they exist
    for( const std::string& dictionary_file_path : application.GetExternalDictionaryFilePaths() ) {
        FileTreeNode* const file_tree_node = ObjTree.FindNode(dictionary_file_path);
        if( file_tree_node != nullptr ) {
            DictionaryDictTreeNode* const dictionary_dict_tree_node = dictTree.GetDictionaryTreeNode(dictionary_file_path);
            ASSERT(dictionary_dict_tree_node != nullptr);
            DropHObjects(dictionary_dict_tree_node->GetDDDoc());        // Remove free hanging dictionaries belonging to the table
            //release the dictionary node
            dictTree.ReleaseDictionaryNode(*dictionary_dict_tree_node); //Release table node id
            ObjTree.DeleteNode(*file_tree_node);                        //delete the node from the object tree
        }
    }
}

void CCSProApp::OnUpdateWindowDicts(CCmdUI* pCmdUI)
{
    CWnd* pWnd = ((CMainFrame*) AfxGetMainWnd())->GetActiveFrame();
    pWnd = pWnd->GetNextWindow();
    while (pWnd != nullptr) {
        if (pWnd->IsKindOf(RUNTIME_CLASS(CDictChildWnd))) {
            pCmdUI->Enable(TRUE);
            return;
        }
        pWnd = pWnd->GetNextWindow();
    }
    pCmdUI->Enable(FALSE);
}

void CCSProApp::OnUpdateWindowForms(CCmdUI* pCmdUI)
{
    CWnd* pWnd = ((CMainFrame*) AfxGetMainWnd())->GetActiveFrame();
    pWnd = pWnd->GetNextWindow();
    while (pWnd != nullptr) {
        if (pWnd->IsKindOf(RUNTIME_CLASS(CFormChildWnd))) {
            pCmdUI->Enable(TRUE);
            return;
        }
        pWnd = pWnd->GetNextWindow();
    }
    pCmdUI->Enable(FALSE);
}

void CCSProApp::OnUpdateWindowOrder(CCmdUI* pCmdUI)
{
    CWnd* pWnd = ((CMainFrame*) AfxGetMainWnd())->GetActiveFrame();
    pWnd = pWnd->GetNextWindow();
    while (pWnd != nullptr) {
        if (pWnd->IsKindOf(RUNTIME_CLASS(COrderChildWnd))) {
            pCmdUI->Enable(TRUE);
            return;
        }
        pWnd = pWnd->GetNextWindow();
    }
    pCmdUI->Enable(FALSE);
}

void CCSProApp::OnUpdateWindowTables(CCmdUI* pCmdUI)
{
    CWnd* pWnd = ((CMainFrame*) AfxGetMainWnd())->GetActiveFrame();
    pWnd = pWnd->GetNextWindow();
    while (pWnd != nullptr) {
        if (pWnd->IsKindOf(RUNTIME_CLASS(CTableChildWnd))) {
            pCmdUI->Enable(TRUE);
            return;
        }
        pWnd = pWnd->GetNextWindow();
    }
    pCmdUI->Enable(FALSE);
}

void CCSProApp::OnWindowDicts()
{
    CWnd* pWnd = ((CMainFrame*) AfxGetMainWnd())->GetActiveFrame();
    pWnd = pWnd->GetNextWindow();
    while (pWnd != nullptr) {
        if (pWnd->IsKindOf(RUNTIME_CLASS(CDictChildWnd))) {
            ((CFrameWnd*) pWnd)->ActivateFrame(SW_SHOW);
            break;
        }
        pWnd = pWnd->GetNextWindow();
    }
}

void CCSProApp::OnWindowForms()
{
    CWnd* pWnd = ((CMainFrame*) AfxGetMainWnd())->GetActiveFrame();
    pWnd = pWnd->GetNextWindow();
    while (pWnd != nullptr) {
        if (pWnd->IsKindOf(RUNTIME_CLASS(CFormChildWnd))) {
            ((CFrameWnd*) pWnd)->ActivateFrame(SW_SHOW);
            break;
        }
        pWnd = pWnd->GetNextWindow();
    }
}

void CCSProApp::OnWindowOrder()
{
    CWnd* pWnd = ((CMainFrame*) AfxGetMainWnd())->GetActiveFrame();
    pWnd = pWnd->GetNextWindow();
    while (pWnd != nullptr) {
        if (pWnd->IsKindOf(RUNTIME_CLASS(COrderChildWnd))) {
            ((CFrameWnd*) pWnd)->ActivateFrame(SW_SHOW);
            break;
        }
        pWnd = pWnd->GetNextWindow();
    }
}

void CCSProApp::OnWindowTables()
{
    CWnd* pWnd = ((CMainFrame*) AfxGetMainWnd())->GetActiveFrame();
    pWnd = pWnd->GetNextWindow();
    while (pWnd != nullptr) {
        if (pWnd->IsKindOf(RUNTIME_CLASS(CTableChildWnd))) {
            ((CFrameWnd*) pWnd)->ActivateFrame(SW_SHOW);
            break;
        }
        pWnd = pWnd->GetNextWindow();
    }
}


void CCSProApp::OnFullscreen()
{
    CMDlgBar& dlgBar = assert_cast<CMainFrame*>(AfxGetMainWnd())->GetDlgBar();
    CFrameWnd* pParentFrameWnd = dlgBar.GetParentFrame();
    pParentFrameWnd->ShowControlBar(&dlgBar, dlgBar.IsWindowVisible() ? FALSE : TRUE, FALSE);
}

void CCSProApp::OnUpdateFullscreen(CCmdUI* pCmdUI)
{
    CMDlgBar& dlgBar = assert_cast<CMainFrame*>(AfxGetMainWnd())->GetDlgBar();
    pCmdUI->SetCheck(dlgBar.IsWindowVisible() ? FALSE : TRUE);
}


void CCSProApp::OnHelpWhatIsNew()
{
    AfxGetApp()->HtmlHelp(HID_BASE_COMMAND + ID_HELP_WHAT_IS_NEW);
}


void CCSProApp::OnHelpExamples()
{
    const std::string examples_directory = Path::Combine(GetWindowsSpecialFolder(WindowsSpecialFolder::Documents),
                                                         FormatText("CSPro\\Examples %0.1f", Versioning::Number));

    if( PortableFunctions::FileIsDirectory(examples_directory) )
    {
        OpenContainingFolder(examples_directory);
    }

    else
    {
        AfxMessageBox(FormatText("The Examples folder could not be located. It is generally found here: %s", examples_directory.c_str()));
    }
}


void CCSProApp::OnHelpTroubleshooting()
{
    CTroubleshootingDialog dlg;
    dlg.DoModal();
}


void CCSProApp::OnHelpMailingList()
{
    Viewer().ViewHtmlUrl("https://public.govdelivery.com/accounts/USCENSUS/subscriber/new?topic_id=USCENSUS_11799");
}


void CCSProApp::OnHelpAndroidApp()
{
    Viewer().ViewHtmlUrl("https://play.google.com/store/apps/details?id=gov.census.cspro.csentry");
}


void CCSProApp::OnHelpShowSyncLog()
{
    try
    {
        const std::string sync_log_path = SyncLog::GetSyncLogPath();

        if( !PortableFunctions::FileIsRegular(sync_log_path) )
            throw CSProException("The sync.log file could not be located. It is generally found here: " + sync_log_path);

        OpenContainingFolder(sync_log_path);
    }

    catch( const CSProException& exception )
    {
        ErrorMessage::Display(exception);
    }
}


void CCSProApp::OnHelpCSProUsersForum()
{
    Viewer().ViewHtmlUrl(Html::CSProUsersForumUrl);
}


void CCSProApp::OnHelpCSProUsersGitHub()
{
    Viewer().ViewHtmlUrl("https://github.com/csprousers");
}


/////////////////////////////////////////////////////////////////////////////
//
//                             CCSProApp::OnAppAbout
//
/////////////////////////////////////////////////////////////////////////////

void CCSProApp::OnAppAbout()
{
    // 20120523 changed
    CAboutDialog dlg;
    dlg.DoModal();
}


void CCSProApp::OnPreferencesFonts()
{
    CFontPrefDlg fontPrefDlg;
    fontPrefDlg.DoModal();
}



void CCSProApp::OnToolsVersionShifter()
{
    if constexpr(Versioning::IsBeta)
    {
        VersionShifterDlg dlg;

        if( dlg.GetNumVersions() < 2 )
        {
            AfxMessageBox(_T("More than one version of CSPro was not found on your computer."));
            return;
        }

        dlg.DoModal();
    }
}

void CCSProApp::OnToolsVersionShifter(CCmdUI* pCmdUI)
{
    // hide the version shifter unless they are running the beta
    if( !Versioning::IsBeta && pCmdUI->m_pMenu != nullptr )
        pCmdUI->m_pMenu->DeleteMenu(pCmdUI->m_nID, MF_BYCOMMAND);
}


void CCSProApp::OnOnKeyCharacterMap()
{
    OnKeyCharacterMapDlg dlg;
    dlg.DoModal();
}


void CCSProApp::OnCommonStore()
{
    CommonStoreDlg dlg;
    dlg.DoModal();
}


void CCSProApp::OnManageCredentials()
{
    SettingsDlg settings_dlg;
    settings_dlg.DoModal();
}



// --------------------------------------------------------------------------
// tools
// --------------------------------------------------------------------------

namespace ToolHelpers
{
    constexpr CSProExecutables::Program GetToolTypeFromId(UINT nID)
    {
        return ( nID == ID_VIEW_LISTING )              ? CSProExecutables::Program::TextView :
               ( nID == ID_TOOLS_DATAMANAGER )         ? CSProExecutables::Program::DataManager:
               ( nID == ID_TOOLS_TEXTVIEW )            ? CSProExecutables::Program::TextView :
               ( nID == ID_TOOLS_TABLEVIEW )           ? CSProExecutables::Program::TblView :
               ( nID == ID_TOOLS_FREQ )                ? CSProExecutables::Program::CSFreq :
               ( nID == ID_TOOLS_DEPLOY )              ? CSProExecutables::Program::CSDeploy :
               ( nID == ID_TOOLS_PACK )                ? CSProExecutables::Program::CSPack :
               ( nID == ID_TOOLS_COMPARE )             ? CSProExecutables::Program::CSDiff:
               ( nID == ID_TOOLS_CONCAT )              ? CSProExecutables::Program::CSConcat :
               ( nID == ID_TOOLS_EXCEL2CSPRO )         ? CSProExecutables::Program::Excel2CSPro :
               ( nID == ID_TOOLS_EXPORT )              ? CSProExecutables::Program::CSExport :
               ( nID == ID_TOOLS_INDEX )               ? CSProExecutables::Program::CSIndex :
               ( nID == ID_TOOLS_REFORMAT )            ? CSProExecutables::Program::CSReFmt :
               ( nID == ID_TOOLS_SORT )                ? CSProExecutables::Program::CSSort :
               ( nID == ID_TOOLS_PARADATACONCAT )      ? CSProExecutables::Program::ParadataConcat :
               ( nID == ID_TOOLS_PARADATAVIEWER )      ? CSProExecutables::Program::ParadataViewer :
               ( nID == ID_TOOLS_CSCODE )              ? CSProExecutables::Program::CSCode :
               ( nID == ID_TOOLS_CSDOCUMENT )          ? CSProExecutables::Program::CSDocument :
               ( nID == ID_TOOLS_CSVIEW )              ? CSProExecutables::Program::CSView :
               ( nID == ID_TOOLS_PFFEDITOR )           ? CSProExecutables::Program::PffEditor :
               ( nID == ID_TOOLS_PRODUCTIONRUNNER )    ? CSProExecutables::Program::ProductionRunner :
               ( nID == ID_TOOLS_OPERATORSTATSVIEWER ) ? CSProExecutables::Program::OperatorStatisticsViewer :
               ( nID == ID_TOOLS_SAVEARRAYVIEWER )     ? CSProExecutables::Program::SaveArrayViewer :
               ( nID == ID_TOOLS_TEXTCONVERTER )       ? CSProExecutables::Program::TextConverter :
                                                         throw ProgrammingErrorException();
    }
}


void CCSProApp::OnUpdateTool(CCmdUI* pCmdUI)
{
    static std::map<UINT, bool> ToolExistsMap;

    // check if the tool exists
    bool tool_exists;
    const auto& tool_exists_lookup = ToolExistsMap.find(pCmdUI->m_nID);

    if( tool_exists_lookup != ToolExistsMap.cend() )
    {
        tool_exists = tool_exists_lookup->second;
    }

    else
    {
        tool_exists = CSProExecutables::GetExecutablePath(ToolHelpers::GetToolTypeFromId(pCmdUI->m_nID)).has_value();
        ToolExistsMap.try_emplace(pCmdUI->m_nID, tool_exists);
    }

    pCmdUI->Enable(tool_exists);
}


void CCSProApp::OnRunTool(UINT nID)
{
    CSProExecutables::Program tool_type = ToolHelpers::GetToolTypeFromId(nID);

    // some tools open with a argument
    enum class SpecialArgumentType { None, Application, Pff, Listing, SaveArrayFile };
    SpecialArgumentType special_argument_type = SpecialArgumentType::None;
    std::optional<EngineAppType> required_app_type;

    if( nID == ID_VIEW_LISTING )
    {
        special_argument_type = SpecialArgumentType::Listing;
    }

    else if( tool_type == CSProExecutables::Program::CSDeploy )
    {
        special_argument_type = SpecialArgumentType::Pff;
        required_app_type = EngineAppType::Entry;
    }

    else if( tool_type == CSProExecutables::Program::CSPack )
    {
        special_argument_type = SpecialArgumentType::Application;
    }

    else if( tool_type == CSProExecutables::Program::PffEditor )
    {
        special_argument_type = SpecialArgumentType::Pff;
    }

    else if( tool_type == CSProExecutables::Program::SaveArrayViewer )
    {
        special_argument_type = SpecialArgumentType::SaveArrayFile;
    }

    // process the argument
    std::optional<std::wstring> filename_argument;

    if( special_argument_type != SpecialArgumentType::None )
    {
        CMainFrame* frame = DYNAMIC_DOWNCAST(CMainFrame, AfxGetMainWnd());
        FileTreeNode* id = frame->GetDlgBar().m_ObjTree.GetActiveObject();

        if( id != nullptr )
        {
            CAplDoc* app_doc = DYNAMIC_DOWNCAST(CAplDoc, id->GetDocument());

            if( app_doc != nullptr )
            {
                if( !required_app_type.has_value() || *required_app_type == app_doc->GetEngineAppType() )
                {
                    filename_argument = CS2WS(((CAplDoc*)id->GetDocument())->GetPathName());

                    if( special_argument_type == SpecialArgumentType::Pff ||
                        special_argument_type == SpecialArgumentType::Listing ||
                        special_argument_type == SpecialArgumentType::SaveArrayFile )
                    {
                        filename_argument = UTF8_TODO::GetWide(PortableFunctions::PathReplaceFileExtension(UTF8_TODO::GetUtf8(*filename_argument), FileExtensions::Pff));

                        if( special_argument_type != SpecialArgumentType::Pff )
                        {
                            CNPifFile pff(WS2CS(*filename_argument));

                            if( !pff.LoadPifFile(true) )
                            {
                                filename_argument.reset();
                            }

                            else if( special_argument_type == SpecialArgumentType::Listing )
                            {
                                filename_argument = CS2WS(pff.GetListingFName());
                            }

                            else
                            {
                                ASSERT(special_argument_type == SpecialArgumentType::SaveArrayFile);
                                filename_argument = CS2WS(pff.GetSaveArrayFilename());
                            }
                        }
                    }

                    if( filename_argument.has_value() && !PortableFunctions::FileIsRegular(*filename_argument) )
                        filename_argument.reset();
                }
            }
        }
    }

    // open a listing file in its proper viewer
    if( special_argument_type == SpecialArgumentType::Listing && filename_argument.has_value() )
    {
        Listing::Lister::View(UTF8_TODO::GetUtf8(*filename_argument));
    }

    else if( filename_argument.has_value() )
    {
        CSProExecutables::RunProgramOpeningFile(tool_type, std::move(*filename_argument));
    }

    else
    {
        CSProExecutables::RunProgram(tool_type);
    }
}
