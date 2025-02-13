#include "StdAfx.h"
#include "CSPro.h"
#include "AboutMenuDialogs.h"
#include "CommonStoreDlg.h"
#include "FakeDropDoc.h"
#include "FontPrefDlg.h"
#include "HidDTmpl.h"
#include "NewFileCreator.h"
#include "OnKeyCharacterMapDlg.h"
#include "SelectAppDlg.h"
#include "SelectDocsDlg.h"
#include "StartDlg.h"
#include "VersionShifterDlg.h"
#include <zToolsO/FileIO.h>
#include <zUtilO/ArrUtil.h>
#include <zUtilO/Filedlg.h>
#include <zUtilO/FileUtil.h>
#include <zUtilO/TreeCtrlHelpers.h>
#include <zUtilO/WinFocSw.h>
#include <zUtilF/ChoiceDlg.h>
#include <zUtilF/SettingsDlg.h>
#include <zListingO/Lister.h>
#include <zDictF/Ddgview.h>
#include <zFormO/DragOptions.h>
#include <zTableF/TabView.h>
#include <zTableF/TabChWnd.h>
#include <zTableF/TabView.h>
#include <zTableF/TabChWnd.h>
#include <engine/trace_macros.h>
#include <afxvisualmanageroffice2007.h>


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


namespace
{
    bool FindString(const CStringArray& arrStrings, const CString& sString)
    {
        for( int iIndex = 0; iIndex < arrStrings.GetSize(); iIndex++ )
        {
            if( sString.CompareNoCase(arrStrings.GetAt(iIndex)) == 0 )
                return true;
        }

        return false;
    }

    bool FindString(const std::vector<CString>& values, wstring_view value)
    {
        const auto& search = std::find_if(values.cbegin(), values.cend(),
                             [&value](const CString& compare_value) { return SO::EqualsNoCase(compare_value, value); });
        return ( search != values.cend() );
    }
}


#define SOFTWARE_DEVELOPER _T("U.S. Census Bureau")

bool AddExternalDictionaryToTree(FileTreeNode* application_file_tree_item, const CString& sDictFileName);
BOOL AddFormToApp(FileTreeNode* pAppObjID,const CString& sDictFile);

CString GetDictFilenameFromFormFile(const CString& sFormFileName);

void DropDDWFromApp(FileTreeNode* pSelectedID,FileTreeNode* pParentID);
void DropFormFromApp(FileTreeNode* pSelectedID,FileTreeNode* pParentID);


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
    ON_COMMAND(ID_FILE_ADD_FILE, OnAddFiles)
    ON_UPDATE_COMMAND_UI(ID_FILE_ADD_FILE, OnUpdateIsApplicationOpen)
    ON_COMMAND(ID_FILE_DROP_FILE, OnDropFiles)
    ON_UPDATE_COMMAND_UI(ID_FILE_DROP_FILE, OnUpdateIsApplicationOpen)
    ON_COMMAND(ID_FILE_SETTINGS, OnCSProSettings)
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
    ON_COMMAND_RANGE(ID_TOOLS_DATAVIEWER, ID_TOOLS_TEXTCONVERTER, OnRunTool)
    ON_UPDATE_COMMAND_UI_RANGE(ID_TOOLS_DATAVIEWER, ID_TOOLS_TEXTCONVERTER, OnUpdateTool)
    ON_COMMAND(ID_TOOLS_VERSION_SHIFTER, OnToolsVersionShifter)
    ON_UPDATE_COMMAND_UI(ID_TOOLS_VERSION_SHIFTER, OnToolsVersionShifter)

    ON_COMMAND(ID_HELP_WHAT_IS_NEW, OnHelpWhatIsNew)
    ON_COMMAND(ID_HELP_EXAMPLES, OnHelpExamples)
    ON_COMMAND(ID_HELP_TROUBLESHOOTING, OnHelpTroubleshooting)
    ON_COMMAND(ID_HELP_MAILING_LIST, OnHelpMailingList)
    ON_COMMAND(ID_HELP_GOOGLEPLAY, OnHelpAndroidApp)
    ON_COMMAND(ID_HELP_SHOW_SYNC_LOG, OnHelpShowSyncLog)

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
    HINSTANCE hInst= AfxLoadLibrary(_T("zDictF.dll"));
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
    hInst= AfxLoadLibrary(_T("zTableF.dll"));
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
    hInst= AfxLoadLibrary(_T("zFormF.dll"));
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
    hInst= AfxLoadLibrary(_T("zOrderF.dll"));
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

#if defined(BETA_BUILD) && !defined(_DEBUG)
    // display an introduction message if this is the first time they've opened this beta build
    LPCTSTR lpszBetaKeyName = _T("Beta Release Date");
    int iBetaReleaseDate = AfxGetApp()->GetProfileInt(_T("Settings"),lpszBetaKeyName,0);

    if( iBetaReleaseDate != Versioning::GetReleaseDate() )
    {
        CString csBetaMessage;
        csBetaMessage.Format(_T("Thank you for testing a beta version of %s. This software has not been thoroughly tested and generally should not be used for production data collection. A list of new features will be displayed when you close this dialog."),CSPRO_VERSION);
        AfxMessageBox(csBetaMessage);

        AfxGetApp()->WriteProfileInt(_T("Settings"),lpszBetaKeyName, Versioning::GetReleaseDate());
        AfxGetApp()->HtmlHelp(HID_BASE_COMMAND + ID_HELP_WHAT_IS_NEW);

        bOpenStartDlg = false;
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
    if(!m_pSessionDoc->IsFileOpen(lpszFileName)) {
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
            std::wstring extension = PortableFunctions::PathGetFileExtension(sFileName);

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
        std::optional<std::tuple<CString, AppFileType>> filename_and_type = NewFileCreator::InteractiveMode();

        if( !filename_and_type.has_value() )
            return;

        // there is some special processing for certain file types
        if( std::get<1>(*filename_and_type) == AppFileType::ApplicationEntry ||
            std::get<1>(*filename_and_type) == AppFileType::Form )
        {
            assert_cast<CCSProApp*>(AfxGetApp())->m_bFileNew = true;
        }

        CDocument* pDoc = OpenDocumentFile(std::get<0>(*filename_and_type));

        if( pDoc == nullptr )
            throw CSProException(_T("There was an error opening %s"), std::get<0>(*filename_and_type).GetString());

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
    if(IsFileOpen(sFileName)) {
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
    CMainFrame* pFrame = assert_cast<CMainFrame*>(AfxGetMainWnd());
    CMDIChildWnd* pActiveWnd = ((CMDIFrameWnd*)pFrame)->MDIGetActive();

    return ( pActiveWnd != nullptr ) ? pActiveWnd->GetActiveDocument() : nullptr;
}


void CCSProApp::OnUpdateIsDocumentOpen(CCmdUI* pCmdUI)
{
    pCmdUI->Enable(GetActiveDocument() != nullptr);
}


bool CCSProApp::IsApplicationOpen()
{
    std::vector<CAplDoc*> application_docs = GetTopLevelAssociatedDocuments<CAplDoc>(nullptr, true);
    return !application_docs.empty();
}

void CCSProApp::OnUpdateIsApplicationOpen(CCmdUI* pCmdUI)
{
    pCmdUI->Enable(IsApplicationOpen());
}


template<typename T>
std::vector<T*> CCSProApp::GetTopLevelAssociatedDocuments(const TCHAR* filename/* = nullptr*/, bool stop_after_first_document/* = false*/)
{
    if( filename == nullptr )
    {
        if( const CDocument* pDoc = GetActiveDocument(); pDoc != nullptr )
            filename = pDoc->GetPathName();

        else
            return { };
    }

    std::vector<T*> documents;

    CMainFrame* pFrame = assert_cast<CMainFrame*>(AfxGetMainWnd());
    const CObjTreeCtrl& ObjTree = pFrame->GetDlgBar().m_ObjTree;
    HTREEITEM hItem = ObjTree.GetRootItem();

    while( hItem != nullptr)
    {
        auto node_filename_matches = [&](FileTreeNode* file_tree_node)
        {
            return SO::EqualsNoCase(file_tree_node->GetPath(), filename);
        };

        FileTreeNode* file_tree_node = ObjTree.GetFileTreeNode(hItem);

        // this document is a candidate if its filename matches...
        bool document_is_candidate = node_filename_matches(file_tree_node);

        // ...or if one of its childrens' filenames matches
        if( !document_is_candidate )
        {
            document_is_candidate = TreeCtrlHelpers::FindInTree(ObjTree, ObjTree.GetChildItem(hItem),
                [&](HTREEITEM hChildItem)
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
    CMainFrame* pFrame = assert_cast<CMainFrame*>(AfxGetMainWnd());

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
        DictionaryDictTreeNode* dictionary_dict_tree_node = pFrame->GetDlgBar().m_DictTree.GetDictionaryTreeNode(*pDoc);
        pFrame->GetDlgBar().m_DictTree.ReleaseDictionaryNode(*dictionary_dict_tree_node);

        //update the tab controls
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

        //update the tab controls
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
        FormOrderAppTreeNode* form_order_app_tree_node = pFrame->GetDlgBar().m_OrderTree.GetFormOrderAppTreeNode(*pOrderDoc);
        pOrderDoc->ReleaseDicts();
        pFrame->GetDlgBar().m_OrderTree.ReleaseOrderNode(*form_order_app_tree_node);

        //Remove the order from the object tree
        FileTreeNode* pObjID = pFrame->GetDlgBar().m_ObjTree.FindNode(pDoc);
        pFrame->GetDlgBar().m_ObjTree.DeleteNode(*pObjID);

        //update the tab controls
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

        TableSpecTabTreeNode* table_spec_tab_tree_node = pFrame->GetDlgBar().m_TableTree.GetTableSpecTabTreeNode(*pTabDoc);
        pTabDoc->ReleaseDicts();
        pFrame->GetDlgBar().m_TableTree.ReleaseTableNode(*table_spec_tab_tree_node);

        //Remove the table from the object tree
        FileTreeNode* pObjID = pFrame->GetDlgBar().m_ObjTree.FindNode(pDoc);
        pFrame->GetDlgBar().m_ObjTree.DeleteNode(*pObjID);

        //update the tab controls
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

    CMainFrame* pFrame = assert_cast<CMainFrame*>(AfxGetMainWnd());
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
    CString sNewDictName = sNewFormName.Left(sNewFormName.ReverseFind('.')) + FileExtensions::WithDot::Dictionary;
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

    CMainFrame* pFrame = assert_cast<CMainFrame*>(AfxGetMainWnd());
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
        case EngineAppType::Tabulation:
            sDocExt = FileExtensions::TabulationApplication;
            sDocFilter = _T("Tabulation Applications (*.xtb)");
            break;
        case EngineAppType::Batch:
            sDocExt = FileExtensions::BatchApplication;
            sDocFilter = _T("Batch Edit Applications (*.bch)");
            break;
        default:
            ASSERT(!_T("Invalid app type for save as"));
            break;
    }

    CString csFilter = sDocFilter + _T("|*.") + sDocExt + _T("|All Files (*.*)|*.*||");  // BMD 04 Sep 2003

    CString sNewAppName;

    bool bOK = false;
    while (!bOK) {

        CIMSAFileDialog dlg (FALSE, sDocExt, nullptr, OFN_HIDEREADONLY, csFilter );
        CString sAppPath = application.GetApplicationFilename();
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
                    if (GetDoc(file_association.GetNewFilename()) != nullptr) {
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
   CMainFrame* pFrame = assert_cast<CMainFrame*>(AfxGetMainWnd());
   Application& application = pApplDoc->GetAppObject();

    switch (pApplDoc->GetEngineAppType())
    {
        case EngineAppType::Entry:
        {
            CString sMainFormName = application.GetFormFilenames().front();
            CString sNewMainFormName = sNewAppName.Left(sNewAppName.ReverseFind('.')) + FileExtensions::WithDot::Form;
            CString sAssocLabel;
            if (application.GetFormFilenames().size() == 1) {
                sAssocLabel = _T("Form File");
            }
            else {
                sAssocLabel = _T("Form File 1");
            }

            aplFileAssociationsDlg.m_fileAssociations.emplace_back(FileAssociation::Type::FormFile, sAssocLabel, true, sMainFormName, sNewMainFormName);

            CFormTreeCtrl& formTree = pFrame->GetDlgBar().m_FormTree;
            CFormNodeID* pID = formTree.GetFormNode(sMainFormName);
            ASSERT(pID != nullptr && pID->GetFormDoc());
            CDEFormFile* pFormFile = &pID->GetFormDoc()->GetFormFile();

            CString sDictName = pFormFile->GetDictionaryFilename();
            CString sNewDictName = sNewMainFormName.Left(sNewMainFormName.ReverseFind('.')) + FileExtensions::WithDot::Dictionary;
            CString sDictAssocLabel;
            sDictAssocLabel.Format(_T("%s Dictionary"), sAssocLabel.GetString());
            aplFileAssociationsDlg.m_fileAssociations.emplace_back(FileAssociation::Type::Dictionary, sDictAssocLabel, true, sDictName, sNewDictName);

            if (application.GetFormFilenames().size() > 1) { // first form is main form
                for (int iForm = 1; iForm < (int)application.GetFormFilenames().size(); ++iForm) {

                    CString sFormName = application.GetFormFilenames()[iForm];

                    sAssocLabel.Format(_T("Form File %d"), iForm+1);

                    aplFileAssociationsDlg.m_fileAssociations.emplace_back(FileAssociation::Type::FormFile, sAssocLabel, true, sFormName, GenUniqueFileName(sFormName));

                    // dictionary associated with the form
                    CFormNodeID* pNodeID  = formTree.GetFormNode(sFormName);
                    ASSERT(pNodeID && pNodeID->GetFormDoc());
                    CDEFormFile* pNodeFormFile = &pNodeID->GetFormDoc()->GetFormFile();

                    CString csDictName = pNodeFormFile->GetDictionaryFilename();
                    CString csNewDictName = sNewMainFormName.Left(sNewMainFormName.ReverseFind('.')) + FileExtensions::WithDot::Dictionary;
                    CString csDictAssocLabel;
                    csDictAssocLabel.Format(_T("%s Dictionary"), sAssocLabel.GetString());
                    aplFileAssociationsDlg.m_fileAssociations.emplace_back(FileAssociation::Type::Dictionary, csDictAssocLabel, true, csDictName, GenUniqueFileName(csDictName));
                }
            }
            break;
        }
        case EngineAppType::Tabulation:
        {
            ASSERT(application.GetTabSpecFilenames().size() == 1);
            CTabulateDoc* pDoc = DYNAMIC_DOWNCAST(CTabulateDoc, GetDoc(application.GetTabSpecFilenames().front()));
            ASSERT_VALID(pDoc);
            CTabSet* pTabSpec = pDoc->GetTableSpec();
            ASSERT_VALID(pTabSpec);
            CString sOrigInputDictName = pTabSpec->GetDictFile();
            CString sNewInputDictName = sNewAppName.Left(sNewAppName.ReverseFind('.')) + FileExtensions::WithDot::Dictionary;
            aplFileAssociationsDlg.m_fileAssociations.emplace_back(FileAssociation::Type::Dictionary, _T("Input Dictionary"), true, sOrigInputDictName, sNewInputDictName);
            break;
        }
        case EngineAppType::Batch:
        {
            ASSERT(application.GetFormFilenames().size() == 1);
            CString sMainOrdName = application.GetFormFilenames().front();
            COrderTreeCtrl& orderTree = pFrame->GetDlgBar().m_OrderTree;
            FormOrderAppTreeNode* form_order_app_tree_node = orderTree.GetFormOrderAppTreeNode(sMainOrdName);
            ASSERT(form_order_app_tree_node->GetOrderDocument() != nullptr);
            CDEFormFile* pOrderFile = &form_order_app_tree_node->GetOrderDocument()->GetFormFile();
            CString sOrigInputDictName = pOrderFile->GetDictionaryFilename();
            CString sNewInputDictName = sNewAppName.Left(sNewAppName.ReverseFind('.')) + FileExtensions::WithDot::Dictionary;
            aplFileAssociationsDlg.m_fileAssociations.emplace_back(FileAssociation::Type::Dictionary, _T("Input Dictionary"), true, sOrigInputDictName, sNewInputDictName);
            break;
        }
        default:
            ASSERT(!_T("INVALID APP TYPE FOR SAVE AS"));
    }

    // external dictionaries (but not ws or one used by form)
    int iExtDict = 1;
    bool bWorkingStorage = false;
    CString sOldWorkDictName;
    for( const DictionaryDescription& dictionary_description : application.GetDictionaryDescriptions() ) {
        if(dictionary_description.GetDictionaryType() == DictionaryType::External) {
            int iAssoc;
            for (iAssoc = 0; iAssoc < (int)aplFileAssociationsDlg.m_fileAssociations.size(); ++iAssoc) {
                if( SO::EqualsNoCase(aplFileAssociationsDlg.m_fileAssociations[iAssoc].GetOriginalFilename(), dictionary_description.GetDictionaryFilename()) )
                    break;
            }
            if (iAssoc >= (int)aplFileAssociationsDlg.m_fileAssociations.size()) {
                // this dictionary wasn't used already in a form file
                CString sAssocLabel;
                sAssocLabel.Format(_T("External Dictionary %d"), iExtDict++);

                aplFileAssociationsDlg.m_fileAssociations.emplace_back(FileAssociation::Type::Dictionary, sAssocLabel, true,
                                                                       WS2CS(dictionary_description.GetDictionaryFilename()), GenUniqueFileName(dictionary_description.GetDictionaryFilename()));
            }
        } else if (dictionary_description.GetDictionaryType() == DictionaryType::Working) {
            bWorkingStorage = true;
            sOldWorkDictName = WS2CS(dictionary_description.GetDictionaryFilename());
        }
    }

    if (bWorkingStorage) {
        aplFileAssociationsDlg.m_workingStorageType = CAplFileAssociationsDlg::WorkingStorageType::ReadOnly;
        aplFileAssociationsDlg.m_bWorkingStorage = true;
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
            CDocument* pDoc = GetDoc(file_association.GetOriginalFilename());

            if (file_association.GetType() == FileAssociation::Type::FormFile) {
                // for form files, need to save related dict first (with new name) so that form won't
                // save it with old name (since form save saves dictionary too)

                ASSERT(iAssoc+1 < (int)aplFileAssociationsDlg.m_fileAssociations.size());
                ASSERT(aplFileAssociationsDlg.m_fileAssociations[iAssoc+1].GetType() == FileAssociation::Type::Dictionary);
                // can skip this we created a new doc instead of renaming existing on
                if (!aCreateNewDocFlags[iAssoc+1]) {
                    const FileAssociation& dict_file_association = aplFileAssociationsDlg.m_fileAssociations[iAssoc + 1];
                    CDocument* pDictDoc = GetDoc(dict_file_association.GetOriginalFilename());
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
    std::wstring sNewWorkDictName = NewFileCreator::GetDefaultWorkingStorageDictionaryFilename(sNewAppName);
    if (aplFileAssociationsDlg.m_bWorkingStorage) {
        std::wstring sOldWorkDictName = NewFileCreator::GetDefaultWorkingStorageDictionaryFilename(pApplDoc->GetPathName());
        CDocument* pWSDDoc = GetDoc(sOldWorkDictName);
        if (pWSDDoc == nullptr) {
            // JH 2/6/05 do search instead of using def name
            // to support legacy apps that have non std ws dict names (eg benchmark)
            sOldWorkDictName = pApplDoc->GetAppObject().GetFirstDictionaryFilenameOfType(DictionaryType::Working);
            pWSDDoc = GetDoc(sOldWorkDictName);
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
            CDocument* pDoc = GetDoc(file_association.GetOriginalFilename());

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
                CMainFrame* pFrame = assert_cast<CMainFrame*>(AfxGetMainWnd());
                CDDTreeCtrl& dictTree = pFrame->GetDlgBar().m_DictTree;
                CDDDoc* pDictDoc = DYNAMIC_DOWNCAST(CDDDoc, pDoc);
                HTREEITEM hItem = dictTree.InsertDictionary(pDictDoc->GetDict()->GetLabel(),file_association.GetNewFilename(),nullptr);
                TVITEM pItem;
                pItem.hItem = hItem;
                pItem.mask  = TVIF_CHILDREN ;
                pItem.cChildren = 1;
                dictTree.SetItem(&pItem);

                if (!dictTree.OpenDictionary(file_association.GetNewFilename(),FALSE)) {
                    CString sMsg;
                    sMsg.Format(IDS_SAVE_AS_SAVE_ERROR, file_association.GetNewFilename().GetString());
                    AfxMessageBox(sMsg);
                    return false;
                }

                // release ref for old dictionary for this app
                DictionaryDictTreeNode* old_dictionary_dict_tree_node = dictTree.GetDictionaryTreeNode(file_association.GetOriginalFilename());
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

    switch (pApplDoc->GetEngineAppType()) {
        case EngineAppType::Entry:
            {
                ASSERT(application.GetFormFilenames().size() > 0);
                for (int iForm = 0; iForm < (int)application.GetFormFilenames().size(); ++iForm) {

                    // get new name from file assocs
                    const CString sOrigFormName = application.GetFormFilenames()[iForm];
                    const FileAssociation* pAssoc = aplFileAssociationsDlg.FindFileAssoc(sOrigFormName);
                    ASSERT(pAssoc);

                    // change name in form list
                    application.RenameFormFilename(sOrigFormName, pAssoc->GetNewFilename());

                    // change app name in child frame wnd
                    CFormDoc* pFormDoc = DYNAMIC_DOWNCAST(CFormDoc, GetDoc(sOrigFormName));
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
                    RenameFormFile(pApplDoc, sOrigFormName, pAssoc->GetNewFilename(), aDictNames, aSkipDictFlags);
                }
            }
            break;
        case EngineAppType::Tabulation:
            {
                ASSERT(application.GetTabSpecFilenames().size() == 1);

                CTabulateDoc* pTabDoc = DYNAMIC_DOWNCAST(CTabulateDoc, GetDoc(application.GetTabSpecFilenames().front()));
                ASSERT_VALID(pTabDoc);
                CTabSet* pTabSpec = pTabDoc->GetTableSpec();
                ASSERT_VALID(pTabSpec);
                const CString sOrigXTSName = application.GetTabSpecFilenames().front();
                const CString sNewXTSName = sNewAppName.Left(sNewAppName.ReverseFind('.')) + FileExtensions::WithDot::TableSpec;

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

                application.RenameTabSpecFilename(sOrigXTSName, sNewXTSName);
            }
            break;
        case EngineAppType::Batch:
            {
                ASSERT(!application.GetFormFilenames().empty());
                for (int iForm = 0; iForm < (int)application.GetFormFilenames().size(); ++iForm) {
                    const CString sOrigOrdName = application.GetFormFilenames()[iForm];
                    const CString sNewOrdName = sNewAppName.Left(sNewAppName.ReverseFind('.')) + FileExtensions::WithDot::Order;

                    // change app name in order frame wnd
                    COrderDoc* pOrderDoc = DYNAMIC_DOWNCAST(COrderDoc, GetDoc(sOrigOrdName));
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
                    application.RenameFormFilename(sOrigOrdName, sNewOrdName);

                }
            }
            break;
        default:
            ASSERT(!_T("INVALID APP TYPE FOR SAVE AS"));
    }

    // external dictionaries (not used by form)
    for( const DictionaryDescription& dictionary_description : application.GetDictionaryDescriptions() ) {
        CString sOrigFName = WS2CS(dictionary_description.GetDictionaryFilename());

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
            application.RenameExternalDictionaryFilename(sOrigFName, CS2WS(pAssoc->GetNewFilename()));

        } else if (dictionary_description.GetDictionaryType() == DictionaryType::Working) {
            std::wstring sNewWorkDictName = NewFileCreator::GetDefaultWorkingStorageDictionaryFilename(sNewAppName);
            RenameDictionaryFile(pApplDoc, sOrigFName, WS2CS(sNewWorkDictName), false);

            // change name in dictionary list
            application.RenameExternalDictionaryFilename(sOrigFName, sNewWorkDictName);
        }
    }

    RenameAppCodeFile(pApplDoc, sNewAppName + FileExtensions::WithDot::Logic);
    RenameMessageFile(pApplDoc, sNewAppName + FileExtensions::WithDot::Message);

    if (pApplDoc->GetEngineAppType() == EngineAppType::Entry) {
        const CString sNewQsfName = sNewAppName + FileExtensions::WithDot::QuestionText;
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
    CMainFrame* pFrame = assert_cast<CMainFrame*>(AfxGetMainWnd());

    // change name of dict doc
    CDocument* pDictDoc = GetDoc(sOldDictFName);
    ASSERT_VALID(pDictDoc);
    //pDictDoc->SetPathName(sNewDictFName, bAddToMRU);
    pDictDoc->SetPathName(sNewDictFName,false);
    pDictDoc->SetModifiedFlag(TRUE);

    // change name in dict tree
    CDDTreeCtrl& dictTree = pFrame->GetDlgBar().m_DictTree;
    DictionaryDictTreeNode* dictionary_dict_tree_node = dictTree.GetDictionaryTreeNode(sOldDictFName);
    dictionary_dict_tree_node->SetPath(CS2WS(sNewDictFName));

    // find node in object tree
    RenameNodeInObjTree(pParentDoc, sOldDictFName, sNewDictFName);
}

void CCSProApp::RenameFormFile(CAplDoc* pApplDoc,
                               CString sOrigFormName,
                               const CString& sNewFormName,
                               const CStringArray& aNewDictNames,
                               const CArray<bool, bool&>& aDictSkipFlags)
{
    CMainFrame* pFrame = assert_cast<CMainFrame*>(AfxGetMainWnd());

    // change name in form doc
    CFormDoc* pFormDoc = DYNAMIC_DOWNCAST(CFormDoc, GetDoc(sOrigFormName));
    ASSERT_VALID(pFormDoc);
    //const bool bAddToMRU = (pApplDoc == nullptr); // only to add to mru for standlaone forms
    //pFormDoc->SetPathName(sNewFormName, bAddToMRU);
    pFormDoc->SetPathName(sNewFormName,false);
    pFormDoc->SetModifiedFlag(TRUE);

    // change name in form spec
    CDEFormFile* pFormSpec = &pFormDoc->GetFormFile();
    pFormSpec->SetFileName(sNewFormName);

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
    CFormNodeID* pFormNodeId = formTree.GetFormNode(sOrigFormName);
    pFormNodeId->SetFFName(sNewFormName);
}


void CCSProApp::RenameOrdFile(CAplDoc* pApplDoc,
                              CString sOrigOrdName,
                              const CString& sNewOrdName,
                              const CStringArray& aNewDictNames,
                              const CArray<bool, bool&>& aDictSkipFlags)
{
    CMainFrame* pFrame = assert_cast<CMainFrame*>(AfxGetMainWnd());

    // change name in order doc
    COrderDoc* pOrderDoc = DYNAMIC_DOWNCAST(COrderDoc, GetDoc(sOrigOrdName));
    ASSERT_VALID(pOrderDoc);
    const bool bAddToMRU = (pApplDoc == nullptr); // only to add to mru for standlaone ord
    pOrderDoc->SetPathName(sNewOrdName, bAddToMRU);
    pOrderDoc->SetModifiedFlag(TRUE);

    // change name in ord spec
    CDEFormFile* pOrderSpec = &pOrderDoc->GetFormFile();
    pOrderSpec->SetFileName(sNewOrdName);

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
    FormOrderAppTreeNode* form_order_app_tree_node = orderTree.GetFormOrderAppTreeNode(sOrigOrdName);
    form_order_app_tree_node->SetPath(CS2WS(sNewOrdName));
}


void CCSProApp::RenameTabSpecFile(CAplDoc* pApplDoc,
                                  CString sOrigXTSName,
                                  const CString& sNewXTSName,
                                  const CString& sNewDictName)
{
    // update xts name
    Application& application = pApplDoc->GetAppObject();
    ASSERT(application.GetTabSpecFilenames().size() == 1);
    CTabulateDoc* pTabDoc = DYNAMIC_DOWNCAST(CTabulateDoc, GetDoc(application.GetTabSpecFilenames().front()));
    ASSERT_VALID(pTabDoc);
    CTabSet* pTabSpec = pTabDoc->GetTableSpec();
    ASSERT_VALID(pTabSpec);
    pTabSpec->SetSpecFile(sNewXTSName);
    pTabDoc->SetPathName(sNewXTSName, FALSE);
    pTabDoc->SetModifiedFlag(TRUE);

    // change name in tab tree
    CMainFrame* pFrame = assert_cast<CMainFrame*>(AfxGetMainWnd());
    CTabTreeCtrl& tabTree = pFrame->GetDlgBar().m_TableTree;
    TableSpecTabTreeNode* table_spec_tab_tree_node = tabTree.GetTableSpecTabTreeNode(sOrigXTSName);
    table_spec_tab_tree_node->SetPath(CS2WS(sNewXTSName));

    pTabSpec->SetDictFile(sNewDictName);
    pTabDoc->SetDictFileName(sNewDictName);

    // change name in obj tree
    RenameNodeInObjTree(pApplDoc, sOrigXTSName, sNewXTSName);
}

void CCSProApp::RenameAppCodeFile(CAplDoc* pAplDoc, const CString& new_filename)
{
    std::shared_ptr<TextSourceEditable> logic_text_source = pAplDoc->GetLogicMainCodeFileTextSource();
    RenameNodeInObjTree(pAplDoc, logic_text_source->GetFilename(), new_filename);
    logic_text_source->SetNewFilename(CS2WS(new_filename));

    if( pAplDoc->GetAppObject().GetAppSrcCode() != nullptr )
        pAplDoc->GetAppObject().GetAppSrcCode()->SetModifiedFlag(true);
}

void CCSProApp::RenameMessageFile(CAplDoc* pAplDoc, const CString& new_filename)
{
    auto message_text_source = pAplDoc->GetMessageTextSource();
    RenameNodeInObjTree(pAplDoc, message_text_source->GetFilename(), new_filename);
    message_text_source->SetNewFilename(CS2WS(new_filename));
}

void CCSProApp::RenameQSFFile(CAplDoc* pApplDoc,
                              const CString& sNewFName)
{
    Application& application = pApplDoc->GetAppObject();
    RenameNodeInObjTree(pApplDoc, application.GetQuestionTextFilename(), sNewFName);
    application.SetQuestionTextFilename(sNewFName);
    pApplDoc->m_pQuestMgr->SetModifiedFlag(true);
}

void CCSProApp::RenameNodeInObjTree(CDocument* pTopLevelDoc, wstring_view old_filename, const CString& sNewFName)
{
    CMainFrame* pFrame = assert_cast<CMainFrame*>(AfxGetMainWnd());
    CObjTreeCtrl& objTree = pFrame->GetDlgBar().m_ObjTree;
    FileTreeNode* pObjNode = nullptr;
    if (pTopLevelDoc == nullptr) {
        // no top level doc - this is top level (app, standalone form, standalone dict)
        pObjNode = objTree.FindNode(old_filename);
    }
    else {
        FileTreeNode* pTopLevelItemId = objTree.FindNode(pTopLevelDoc->GetPathName());
        ASSERT(pTopLevelItemId);
        pObjNode = objTree.FindChildNodeRecursive(*pTopLevelItemId, old_filename);
    }
    ASSERT(pObjNode);

    pObjNode->SetRenamedPath(CS2WS(sNewFName));
}


void CCSProApp::CloseApplication(CDocument* pDoc)
{
    CMainFrame* pFrame = assert_cast<CMainFrame*>(AfxGetMainWnd());

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
    CMainFrame* pFrame = assert_cast<CMainFrame*>(AfxGetMainWnd());
    CObjTreeCtrl& ObjTree = pFrame->GetDlgBar().m_ObjTree;

    //---------------- Application

    if(pDoc->IsKindOf(RUNTIME_CLASS(CAplDoc)))
    {
        SaveRFL();
        hObjItem = assert_cast<CAplDoc*>(pDoc)->BuildAllTrees();
        FileTreeNode* pObjNode = ObjTree.FindNode(sFileName);
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
        CString sLabel = assert_cast<CDDDoc*>(pDoc)->GetDict()->GetLabel();

        //Insert the dictionary in the objtree
        hObjItem = ObjTree.InsertNode(TVI_ROOT, std::make_unique<DictionaryFileTreeNode>(CS2WS(sFileName)));
        FileTreeNode* pObjNode = ObjTree.FindNode(sFileName);
        pObjNode->SetDocument(pDoc);

        CDDTreeCtrl& dictTree = pFrame->GetDlgBar().m_DictTree;
        DictionaryDictTreeNode* dictionary_dict_tree_node = dictTree.GetDictionaryTreeNode(*pDoc);
        HTREEITEM hItem = nullptr;
        if(dictionary_dict_tree_node == nullptr) {
            hItem = dictTree.InsertDictionary(sLabel, sFileName, assert_cast<CDDDoc*>(pDoc));
            assert_cast<CDDDoc*>(pDoc)->SetDictTreeCtrl(&pFrame->GetDlgBar().m_DictTree);
            assert_cast<CDDDoc*>(pDoc)->InitTreeCtrl();
        }
        else {
            dictionary_dict_tree_node->AddRef();
            hItem = dictionary_dict_tree_node->GetHItem();
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

        hObjItem = ObjTree.InsertFormNode(TVI_ROOT, CS2WS(csFileName), AppFileType::Form);
        FileTreeNode* pObjNode = ObjTree.FindNode(csFileName);
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

        hObjItem = ObjTree.InsertFormNode(TVI_ROOT, CS2WS(csFileName), AppFileType::Order);
        FileTreeNode* pObjNode = ObjTree.FindNode(csFileName);
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

        hObjItem = ObjTree.InsertTableNode(TVI_ROOT, CS2WS(fileName));
        FileTreeNode* pObjNode = ObjTree.FindNode(fileName);
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
    if(sExt.CompareNoCase(FileExtensions::WithDot::Order) == 0 ){
        (*m_pRecentFileList).Remove(nIndex);
        AfxMessageBox(_T("Cannot open .ord files"));
        return FALSE;
    }

    if(IsFileOpen(sFileName)) {
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


void CCSProApp::AddFileToApp(FileTreeNode& application_file_tree_item, const FileAssociation& file_association)
{
    const CString& new_filename = file_association.GetNewFilename();

    //Get the application
    CAplDoc* pAplDoc = (CAplDoc*)application_file_tree_item.GetDocument();
    Application& application = pAplDoc->GetAppObject();

    //if it exists get its pID and do an addref && add its label HITEM on to the object tree under the app;
    //else insert the file into its tree with out opening the file
    //also add the label under the HITEM of the app
    CMainFrame* pFrame = assert_cast<CMainFrame*>(AfxGetMainWnd());
    CObjTreeCtrl& ObjTree = pFrame->GetDlgBar().m_ObjTree;

    auto insert_application_item_under_parent = [&](wstring_view parent_filename, std::unique_ptr<FileTreeNode> file_tree_node)
    {
        // find the parent's node
        FileTreeNode* parent_file_tree_item = ObjTree.FindChildNodeRecursive(application_file_tree_item, parent_filename);

        if( parent_file_tree_item != nullptr )
            ObjTree.InsertNode(parent_file_tree_item->GetHItem(), std::move(file_tree_node));
    };


    BOOL success = FALSE;

    if( file_association.GetType() == FileAssociation::Type::Dictionary )
    {
        if( AddExternalDictionaryToTree(&application_file_tree_item, new_filename) )
        {
            application.AddExternalDictionaryFilename(new_filename);
            success = true;
        }
    }

    else if( file_association.GetType() == FileAssociation::Type::FormFile )
    {
        ASSERT(application.GetEngineAppType() == EngineAppType::Entry);
        success = AddFormToApp(&application_file_tree_item, new_filename);
    }

    else if( file_association.GetType() == FileAssociation::Type::Report )
    {
        // create a unique name based on the filename
        CString report_name = CIMSAString::MakeName(PortableFunctions::PathGetFilenameWithoutExtension<CString>(new_filename));
        AfxGetMainWnd()->SendMessage(UWM::CSPro::CreateUniqueName, (WPARAM)&report_name, (LPARAM)pAplDoc);

        application.AddReport(CS2WS(report_name), CS2WS(new_filename));
        ObjTree.InsertNode(application_file_tree_item.GetHItem(), std::make_unique<ReportFileTreeNode>(CS2WS(new_filename)));
        success = TRUE;
    }

    else if( file_association.GetType() == FileAssociation::Type::Logic )
    {
        application.AddCodeFile(CodeFile(CodeType::LogicExternal, TextSourceEditable::FindOpenOrCreate(CS2WS(new_filename))));
        insert_application_item_under_parent(pAplDoc->GetLogicMainCodeFileTextSource()->GetFilename(), std::make_unique<CodeFileTreeNode>(application.GetCodeFiles().back()));
        success = TRUE;
    }

    else if( file_association.GetType() == FileAssociation::Type::Message )
    {
        application.AddMessageFile(std::make_shared<TextSourceExternal>(CS2WS(new_filename)));
        insert_application_item_under_parent(pAplDoc->GetMessageTextSource()->GetFilename(), std::make_unique<MessageFileTreeNode>(CS2WS(new_filename), true));
        success = TRUE;
    }

    else
    {
        AfxMessageBox(_T("Invalid file type"));
    }

    if( success )
        pAplDoc->SetModifiedFlag(TRUE);
}



void CCSProApp::AddResourceFolderToApp(CAplDoc* pDoc, const CString& folder_name)
{
    const auto& resource_folders = pDoc->GetAppObject().GetResourceFolders();

    if( std::find_if(resource_folders.cbegin(), resource_folders.cend(),
        [&](const auto& fn) { return ( fn.CompareNoCase(folder_name) == 0 ); }) != resource_folders.cend() )
    {
        // no need to add it if it's already been added
        return;
    }

    pDoc->GetAppObject().AddResourceFolder(folder_name);
    pDoc->SetModifiedFlag(TRUE);

    CMainFrame* pFrame = assert_cast<CMainFrame*>(AfxGetMainWnd());
    CObjTreeCtrl& ObjTree = pFrame->GetDlgBar().m_ObjTree;

    ObjTree.InsertNode(ObjTree.FindNode(pDoc)->GetHItem(), std::make_unique<ResourceFolderTreeNode>(CS2WS(folder_name)));
}


bool AddExternalDictionaryToTree(FileTreeNode* application_file_tree_item, const CString& dictionary_filename)
{
    CMainFrame* pFrame = assert_cast<CMainFrame*>(AfxGetMainWnd());
    CObjTreeCtrl& ObjTree = pFrame->GetDlgBar().m_ObjTree;

    CDDTreeCtrl& dictTree = pFrame->GetDlgBar().m_DictTree;
    DictionaryDictTreeNode* dictionary_dict_tree_node = dictTree.GetDictionaryTreeNode(dictionary_filename);

    //Add ref to the form (We need to add refs to dependencies too !!!)
    if( dictionary_dict_tree_node != nullptr )
    {
        dictionary_dict_tree_node->AddRef();
    }

    //Add the new Dict item to the Dict Tree
    else
    {
        try
        {
            LabelSet dictionary_label_set = JsonStream::GetValueFromSpecFile<LabelSet, CDataDict>(JK::labels, dictionary_filename);

            TVITEM pItem;
            pItem.hItem = dictTree.InsertDictionary(dictionary_label_set.GetLabel(), dictionary_filename, nullptr);
            pItem.mask = TVIF_CHILDREN;
            pItem.cChildren = 1;
            dictTree.SetItem(&pItem);
        }

        catch( const CSProException& )
        {
		    return false;
        }
    }
        
    //insert the Label into the Tree
    ObjTree.InsertNode(application_file_tree_item->GetHItem(), std::make_unique<DictionaryFileTreeNode>(CS2WS(dictionary_filename)));

    return true;
}


BOOL AddFormToApp(FileTreeNode* pAppObjID,const CString& sFormFile) {

        BOOL bRet = FALSE;

        CAplDoc* pDoc = (CAplDoc*)pAppObjID->GetDocument();
        ASSERT(pDoc != nullptr);

        if(pDoc->GetEngineAppType() != EngineAppType::Entry) {
                AfxMessageBox(_T("Can add form files only to a Dataentry application"));
                return FALSE;
        }


        CMainFrame* pFrame = assert_cast<CMainFrame*>(AfxGetMainWnd());
        CObjTreeCtrl& ObjTree = pFrame->GetDlgBar().m_ObjTree;
        CFormTreeCtrl& formTree = pFrame->GetDlgBar().m_FormTree;

        CFormNodeID* pID = formTree.GetFormNode(sFormFile);

                        //Get the Label from the Tree
        CString sLabel;
        if(pID)
            sLabel = formTree.GetItemText(pID->GetHItem());


        BOOL bFormAlreadyOpen = TRUE;

        if( pID == nullptr ) {
                //Add the new Dict item to the Dict Tree
                bFormAlreadyOpen = FALSE;
                CSpecFile  specFile(TRUE);
                BOOL bOK = specFile.Open(sFormFile,CFile::modeRead);
                if (bOK){
                        sLabel = ValFromHeader(specFile, CSPRO_CMD_LABEL);
                        HTREEITEM hItem = formTree.InsertFormFile(sLabel, sFormFile, nullptr); //Here the ref is added

                        TVITEM pItem;
                        pItem.hItem = hItem;
                        pItem.mask  = TVIF_CHILDREN ;
                        pItem.cChildren = 1;
                        formTree.SetItem(&pItem);

                        pID = formTree.GetFormNode(sFormFile);
                        specFile.Close();
                }
                else {
                        return FALSE;
                }


        }

        //Add ref to the form (We need to add refs to dependencies too !!!)
        if(bFormAlreadyOpen)
                pID->AddRef();


        //We need to add refs to dependencies too !!!
        formTree.InsertFormDependencies(pID);

        //Get the hItem unde which we have to insert this new label
        HTREEITEM  hParent = pAppObjID->GetHItem();

        //Get the HITEM which has the label as "[TabSpecs]"
        //If it doesnt exist Just add one

/*      HTREEITEM hChild = ObjTree.GetChildItem(hParent);
        BOOL bFound = FALSE;
        while(hChild) {
                CString sString = ObjTree.GetItemText(hChild);
                if(sString.CompareNoCase(FORMS) == 0 ) {
                        hParent = hChild ;
                        bFound = TRUE;
                        break;
                }
                hChild = ObjTree.GetNextSiblingItem(hChild);

        }
        if(!bFound) {
                hParent = ObjTree.InsertNode(FORMS,hParent);
        }*/


        //insert the Label into  the Tree
        ObjTree.InsertFormNode(hParent, CS2WS(sFormFile), AppFileType::Form);

        pDoc->GetAppObject().AddFormFilename(sFormFile);

        bRet = TRUE;

        return bRet;
}


CString GetDictFilenameFromFormFile(const CString& sFormFileName)
{
    CSpecFile frmFile(TRUE);
    std::vector<std::wstring> dictionary_filenames;

    if( !frmFile.Open(sFormFileName, CFile::modeRead) )
    {
        AfxMessageBox(FormatText(_T("File %s Could not be opened"), sFormFileName.GetString()));
    }

    else
    {
        dictionary_filenames = GetFileNameArrayFromSpecFile(frmFile, CSPRO_DICTS);
        ASSERT(dictionary_filenames.size() == 1);
        frmFile.Close();
    }

    return dictionary_filenames.empty() ? CString() : WS2CS(dictionary_filenames.front());
}


void CCSProApp::OnDropFiles()
{
    std::optional<CAplDoc*> pAplDoc = GetActiveApplication(nullptr, _T("Select Application to Drop Files From"));

    if( !pAplDoc.has_value() || pAplDoc == nullptr )
    {
        ASSERT(!pAplDoc.has_value());
        return;
    }

    CAplDoc* pDropFromApp = *pAplDoc;

    // build up list of docs that user can choose to drop
    std::vector<CDocument*> documents = GetDropCandidatesForApplication(pDropFromApp);

    // if external code or reports are being edited, update the text in case the tree is redrawn
    SyncExternalTextSources(pDropFromApp);

    // add the fake report, external code, external message, and resource folder documents
    std::vector<std::unique_ptr<FakeDropDoc>> fake_drop_docs;

    auto add_fake_docs = [&](const auto& filenames_or_text_sources, AppFileType app_file_type, size_t i)
    {
        for( ; i < filenames_or_text_sources.size(); ++i )
        {
            fake_drop_docs.emplace_back(std::make_unique<FakeDropDoc>(filenames_or_text_sources[i], app_file_type, i));
            documents.emplace_back(fake_drop_docs.back().get());
        }
    };

    // reports
    add_fake_docs(pDropFromApp->GetAppObject().GetReportNamedTextSources(), AppFileType::Report, 0);

    // code files
    size_t code_file_index = 0;

    for( const CodeFile& code_file : pDropFromApp->GetAppObject().GetCodeFiles() )
    {
        if( !code_file.IsLogicMain() )
        {
            fake_drop_docs.emplace_back(std::make_unique<FakeDropDoc>(code_file, code_file_index));
            documents.emplace_back(fake_drop_docs.back().get());
        }

        ++code_file_index;
    }

    // messages
    add_fake_docs(pDropFromApp->GetAppObject().GetMessageTextSources(), AppFileType::Message, 1);

    // resource folders
    add_fake_docs(pDropFromApp->GetAppObject().GetResourceFolders(), AppFileType::ResourceFolder, 0);


    if( documents.empty() )
    {
        AfxMessageBox(_T("There are no files that can be dropped from the selected application."));
        return;
    }

    // choose which items to drop from selected apps
    SelectDocsDlg select_docs_dlg(documents, _T("Select Files to Drop"));

    select_docs_dlg.SetRelativePathAdjuster(pDropFromApp->GetPathName());

    if( select_docs_dlg.DoModal() != IDOK )
        return;

    documents = select_docs_dlg.GetSelectedDocuments();

    CMainFrame* pFrame = assert_cast<CMainFrame*>(AfxGetMainWnd());
    CObjTreeCtrl& ObjTree = pFrame->GetDlgBar().m_ObjTree;
    FileTreeNode* pAppItemID = ObjTree.FindNode(pDropFromApp);
    ASSERT(pAppItemID != nullptr);

    bool redraw_tree = false;

    // 20111207 changed the order so that the deleting of external code would work
    for( size_t i = documents.size() - 1; i < documents.size(); --i )
    {
        CDocument* pDocument = documents[i];
        ASSERT_VALID(pDocument);

        FileTreeNode* pDropItemID = ObjTree.FindChildNodeRecursive(*pAppItemID, pDocument->GetPathName());  // 20111130
        ASSERT(pDropItemID != nullptr);

        if( pDocument->IsKindOf(RUNTIME_CLASS(CDDDoc)) )
        {
            // drop the selected dictionary from the app
            DropDDWFromApp(pDropItemID, pAppItemID);
        }

        else if( pDocument->IsKindOf(RUNTIME_CLASS(CFormDoc)) )
        {
            // drop the selected form from the app
            DropFormFromApp(pDropItemID, pAppItemID);
        }

        else if( pDocument->IsKindOf(RUNTIME_CLASS(FakeDropDoc)) )
        {
            const FakeDropDoc* pFakeLogicDoc = (const FakeDropDoc*)pDocument;

            switch( pFakeLogicDoc->GetAppFileType() )
            {
                case AppFileType::Report:
                    pDropFromApp->GetAppObject().DropReport(pFakeLogicDoc->GetArrayIndex());
                    redraw_tree = true;
                    break;

                case AppFileType::Code:
                    pDropFromApp->GetAppObject().DropCodeFile(pFakeLogicDoc->GetArrayIndex());
                    redraw_tree = true;
                    break;

                case AppFileType::Message:
                    pDropFromApp->GetAppObject().DropMessageFile(pFakeLogicDoc->GetArrayIndex());
                    break;

                case AppFileType::ResourceFolder:
                    pDropFromApp->GetAppObject().DropResourceFolder(pFakeLogicDoc->GetArrayIndex());
                    break;

                default:
                    throw ProgrammingErrorException();
            }

            ObjTree.DeleteNode(*pDropItemID);
        }

        else
        {
            ASSERT(false);
        }
    }

    pDropFromApp->SetModifiedFlag(TRUE);

    if( redraw_tree )
        pDropFromApp->RefreshExternalLogicAndReportNodes();

    //Update Tabs
    pFrame->GetDlgBar().UpdateTabs();
}


std::vector<CDocument*> CCSProApp::GetDropCandidatesForApplication(const CAplDoc* pAplDoc)
{
    const Application& application = pAplDoc->GetAppObject();

    std::vector<CDocument*> documents;

    // get external dictionaries for the app
    for( const auto& sDictName : application.GetExternalDictionaryFilenames() )
    {
        // get dict type
        const DictionaryDescription* dictionary_description = application.GetDictionaryDescription(sDictName);
        ASSERT(dictionary_description != nullptr && dictionary_description->GetDictionaryType() != DictionaryType::Unknown);

        // don't allow input dictionaries to be dropped
        if (dictionary_description->GetDictionaryType() == DictionaryType::Input) {
            continue;
        }

        // only consider working storage dicts for entry and batch apps (not cross tab)
        if (dictionary_description->GetDictionaryType() == DictionaryType::Working && pAplDoc->GetEngineAppType() == EngineAppType::Tabulation) {
            continue;
        }

        // add the dictionary to the list of candidates
        CDocument* pEDictDoc = GetDoc(sDictName);
        ASSERT(pEDictDoc);
        documents.emplace_back(pEDictDoc);
    }

    // get form files for the app (data entry only)
    if (pAplDoc->GetEngineAppType() == EngineAppType::Entry) {
        if (application.GetFormFilenames().size() > 1) { // don't allow delete first form
            for (int iForm = 1; iForm < (int)application.GetFormFilenames().size(); ++iForm) {
                CString sFormName = application.GetFormFilenames()[iForm];

                // add the form to the list of candidates
                CDocument* pFormDoc = GetDoc(sFormName);
                ASSERT(pFormDoc);
                documents.emplace_back(pFormDoc);
            }
        }
    }

    return documents;
}


void DropDDWFromApp(FileTreeNode* pSelectedID,FileTreeNode* pParentID)
{
    CMainFrame* pFrame = assert_cast<CMainFrame*>(AfxGetMainWnd());
    CObjTreeCtrl& ObjTree = pFrame->GetDlgBar().m_ObjTree;
    CDDTreeCtrl& dictTree = pFrame->GetDlgBar().m_DictTree;

    CAplDoc* pAplDoc = DYNAMIC_DOWNCAST(CAplDoc, pParentID->GetDocument());
    ASSERT(pAplDoc != nullptr);

    if( pSelectedID->GetDocument() ) {
        CDDDoc* pDictDoc = DYNAMIC_DOWNCAST(CDDDoc, pSelectedID->GetDocument());
        ASSERT(pDictDoc != nullptr);

        //Save the Dictionary if it is modified
        if(pDictDoc->IsModified()) {
            CString sFileName(pDictDoc->GetTitle());
            CString sMsg;
            sMsg.FormatMessage(_T("Do you want to save the %1 dictionary"), sFileName.GetString());
            if(AfxMessageBox(sMsg,MB_YESNO ) == IDYES){
                    pDictDoc->SaveModified();
            }
        }
    }


    pAplDoc->GetAppObject().DropExternalDictionaryFilename(pSelectedID->GetPath());
    pAplDoc->SetModifiedFlag(TRUE);

    //Release the dictionary
    DictionaryDictTreeNode* dictionary_dict_tree_node = dictTree.GetDictionaryTreeNode(pSelectedID->GetPath());
    dictTree.ReleaseDictionaryNode(*dictionary_dict_tree_node);

    //Delete the hItem for the dict from the object tree
    ObjTree.DeleteNode(*pSelectedID);

    pAplDoc->OpenAllDocuments();
    pAplDoc->SetAppObjects();

}


void DropFormFromApp(FileTreeNode* pSelectedID,FileTreeNode* pParentID)
{
    CMainFrame* pFrame = assert_cast<CMainFrame*>(AfxGetMainWnd());
    CObjTreeCtrl& ObjTree = pFrame->GetDlgBar().m_ObjTree;
    CFormTreeCtrl& formTree = pFrame->GetDlgBar().m_FormTree;

    CAplDoc* pAplDoc = DYNAMIC_DOWNCAST(CAplDoc, pParentID->GetDocument());
    ASSERT(pAplDoc != nullptr);

    if( pAplDoc->GetAppObject().GetFormFilenames().size() == 1 ) {
        AfxMessageBox(_T("There has to be at least one form file in the data entry application"));
        return;
    }

    //Save the form file if it is modified
    if( pSelectedID->GetDocument()) {
        CFormDoc* pFormDoc = DYNAMIC_DOWNCAST(CFormDoc, pSelectedID->GetDocument());
        ASSERT(pFormDoc != nullptr);

        //Save the form if it is modified
        if(pFormDoc->IsModified()) {
            CString sFileName(pFormDoc->GetTitle());
            CString sMsg;
            sMsg.FormatMessage(_T("Do you want to save the %1 form file"), sFileName.GetString());
            if(AfxMessageBox(sMsg,MB_YESNO ) == IDYES){
                    pFormDoc->SaveModified();
            }
        }
    }

    //Drop the form
    pAplDoc->GetAppObject().DropFormFilename(pSelectedID->GetPath());
    pAplDoc->SetModifiedFlag(TRUE);
    //release the form from the tab tree
    CFormNodeID* pFormID = formTree.GetFormNode(pSelectedID->GetPath());
    //Release the form dependencies
    formTree.ReleaseFormDependencies(pFormID);
    //release the formnode
    formTree.ReleaseFormNodeID(pFormID);

    //Delete the hItem for the form from the object tree
    ObjTree.DeleteNode(*pSelectedID);

    pAplDoc->OpenAllDocuments();
    pAplDoc->SetAppObjects();
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
    CMainFrame* pFrame = assert_cast<CMainFrame*>(AfxGetMainWnd());
    CMDlgBar& DlgBar = pFrame->GetDlgBar();
    CString sTabString = DICT_TAB_LABEL;

    EngineAppType appType = EngineAppType::Invalid;
    if(pDoc->IsKindOf(RUNTIME_CLASS(CAplDoc))) {
        Application& appObject = assert_cast<CAplDoc*>(pDoc)->GetAppObject();
        appType = appObject.GetEngineAppType();

        if(appType == EngineAppType::Entry) {
            CString sFormFile  = appObject.GetFormFilenames().front();
            CFormTreeCtrl& formTree = DlgBar.m_FormTree;
            CFormNodeID* pNode = formTree.GetFormNode(sFormFile);
            if(pNode) {
                pDoc = pNode->GetFormDoc();
            }
        }
        else if(appType == EngineAppType::Batch) {
            sTabString = ORDER_TAB_LABEL;
            CString sOrderFile  = appObject.GetFormFilenames().front();
            COrderTreeCtrl& orderTree = DlgBar.m_OrderTree;
            FormOrderAppTreeNode* form_order_app_tree_node = orderTree.GetFormOrderAppTreeNode(sOrderFile);
            if(form_order_app_tree_node != nullptr ) {
                pDoc = form_order_app_tree_node->GetOrderDocument();
            }
        }
        else if(appType == EngineAppType::Tabulation) {
            CString sTableFile  = appObject.GetTabSpecFilenames().front();
            CTabTreeCtrl& tableTree = DlgBar.m_TableTree;
            TableSpecTabTreeNode* table_spec_tab_tree_node = tableTree.GetTableSpecTabTreeNode(sTableFile);
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
        DictionaryDictTreeNode* dictionary_dict_tree_node = dictTree.GetDictionaryTreeNode(sDictName);
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


CDocument* CCSProApp::GetDoc(wstring_view filename)
{
    CMainFrame* pFrame = assert_cast<CMainFrame*>(AfxGetMainWnd());
    std::optional<AppFileType> app_file_type = GetAppFileTypeFromFileExtension(PortableFunctions::PathGetFileExtension(filename));

    auto get_document_from_tree_node =
        [](TreeNode* tree_node_with_document)
        {
            return ( tree_node_with_document != nullptr ) ? tree_node_with_document->GetDocument() : nullptr;
        };

    // if the extension type is unknown, check each document type
    if( app_file_type.value_or(AppFileType::Dictionary) == AppFileType::Dictionary )
    {
        return get_document_from_tree_node(pFrame->GetDlgBar().m_DictTree.GetDictionaryTreeNode(filename));
    }

    if( app_file_type.value_or(AppFileType::Form) == AppFileType::Form )
    {
        CFormNodeID* pNode = pFrame->GetDlgBar().m_FormTree.GetFormNode(filename);
        if( pNode != nullptr )
            return pNode->GetFormDoc();
    }

    if( app_file_type.value_or(AppFileType::Order) == AppFileType::Order )
    {
        return get_document_from_tree_node(pFrame->GetDlgBar().m_OrderTree.GetFormOrderAppTreeNode(filename));
    }

    if( app_file_type.value_or(AppFileType::TableSpec) == AppFileType::TableSpec )
    {
        TableSpecTabTreeNode* table_spec_tab_tree_node = pFrame->GetDlgBar().m_TableTree.GetTableSpecTabTreeNode(filename);
        if( table_spec_tab_tree_node != nullptr )
            return table_spec_tab_tree_node->GetTabDoc();
    }

    if( IsApplicationType(app_file_type.value_or(AppFileType::ApplicationEntry)) )
    {
        return get_document_from_tree_node(pFrame->GetDlgBar().m_ObjTree.FindNode(filename));
    }

    return nullptr;
}


void CCSProApp::OnAddFiles()
{
    std::optional<CAplDoc*> pAplDoc = GetActiveApplication(nullptr, _T("Select Application to Add Files Into"));

    if( !pAplDoc.has_value() || pAplDoc == nullptr )
    {
        ASSERT(!pAplDoc.has_value());
        return;
    }

    CAplDoc* pInsertToApp = *pAplDoc;
    Application& application = pInsertToApp->GetAppObject();

    // if external code or reports are being edited, update the text in case the tree is redrawn
    SyncExternalTextSources(pInsertToApp);

    // put up dialog to get file(s) to insert
    CAplFileAssociationsDlg aplFileAssociationsDlg;
    aplFileAssociationsDlg.m_sAppName = pInsertToApp->GetPathName();
    aplFileAssociationsDlg.m_sTitle = _T("Add Files to Application ") + application.GetName();

    aplFileAssociationsDlg.m_fileAssociations.emplace_back(FileAssociation::Type::Dictionary, _T("External Dictionary 1"), false);
    aplFileAssociationsDlg.m_fileAssociations.emplace_back(FileAssociation::Type::Dictionary, _T("External Dictionary 2"), false);

    if( pInsertToApp->GetEngineAppType() == EngineAppType::Entry )
        aplFileAssociationsDlg.m_fileAssociations.emplace_back(FileAssociation::Type::FormFile, _T("Form File"), false);

    aplFileAssociationsDlg.m_fileAssociations.emplace_back(FileAssociation::Type::Report, _T("Report"), false);

    aplFileAssociationsDlg.m_fileAssociations.emplace_back(FileAssociation::Type::Logic, _T("External Logic File"), false);

    aplFileAssociationsDlg.m_fileAssociations.emplace_back(FileAssociation::Type::Message, ("External Message File"), false);

    aplFileAssociationsDlg.m_fileAssociations.emplace_back(FileAssociation::Type::ResourceFolder, _T("Resource Folder"), false);

    // find out if there is already a working storage dictionary
    std::wstring working_storage_dictionary_filename = application.GetFirstDictionaryFilenameOfType(DictionaryType::Working);

    if( !working_storage_dictionary_filename.empty() ) {
        aplFileAssociationsDlg.m_bWorkingStorage = TRUE;
        aplFileAssociationsDlg.m_sWSDName = WS2CS(working_storage_dictionary_filename);
        aplFileAssociationsDlg.m_workingStorageType = CAplFileAssociationsDlg::WorkingStorageType::ReadOnly;
    }
    else {
        aplFileAssociationsDlg.m_bWorkingStorage = FALSE;
        aplFileAssociationsDlg.m_workingStorageType = CAplFileAssociationsDlg::WorkingStorageType::Editable;
    }

    if(aplFileAssociationsDlg.DoModal() != IDOK) {
        return;
    }

    // if we need to create new form file, check find dictionary name for the new form before we add anything
    for (int i = 0; i < (int)aplFileAssociationsDlg.m_fileAssociations.size(); ++i) {
        FileAssociation& file_association = aplFileAssociationsDlg.m_fileAssociations[i];

        if (file_association.GetType() == FileAssociation::Type::FormFile) {
            if (!SO::IsBlank(file_association.GetNewFilename())) {
                CFileStatus fStatus;
                BOOL bFileExists = CFile::GetStatus(file_association.GetNewFilename(), fStatus);
                file_association.SetNewFilename(fStatus.m_szFullName); // use full name in case user didn't enter path
                if (!bFileExists) {
                    // try adding "dcf" to form to get dict name
                    CString sNewFormDict = file_association.GetNewFilename().Left(file_association.GetNewFilename().ReverseFind('.')) + FileExtensions::WithDot::Dictionary;
                    if (CFile::GetStatus(sNewFormDict,fStatus)) {
                        // that dictionary already exists - get new dict name from user
                        CString csExt = FileExtensions::Dictionary;
                        CString sFilter = _T("All Files (*.*)|*.*||");
                        sFilter = _T("Data Dictionary Files (*.dcf)|*.dcf|") + sFilter;

                        CFileDialog fileDlg(FALSE, csExt, nullptr, OFN_HIDEREADONLY | OFN_CREATEPROMPT | OFN_PATHMUSTEXIST, sFilter, nullptr);
                        fileDlg.m_ofn.lpstrTitle = _T("Enter or Select Dictionary For New Form");
                        if(fileDlg.DoModal() == IDOK) {
                            sNewFormDict = fileDlg.GetPathName();
                            if (sNewFormDict.Mid(sNewFormDict.ReverseFind('.') + 1).CompareNoCase(csExt) != 0) {
                                    sNewFormDict += _T(".") + csExt;
                            }
                        }
                        else {
                            return; // cancel
                        }
                    }

                    // create the new form file and dictionary
                    try
                    {
                        NewFileCreator::CreateFormFile(file_association.GetNewFilename(), sNewFormDict, false);
                    }

                    catch( const CSProException& exception )
                    {
                        ErrorMessage::Display(exception);
                    }
                }
            }
        }
    }

    CMainFrame* pFrame = assert_cast<CMainFrame*>(AfxGetMainWnd());
    CObjTreeCtrl& ObjTree = pFrame->GetDlgBar().m_ObjTree;
    FileTreeNode* pAppItemID = ObjTree.FindNode(pInsertToApp);
    ASSERT(pAppItemID != nullptr);

    std::vector<CString> dictionary_names_in_use;

    for( const CDataDict* dictionary : pInsertToApp->GetAllDictsInApp() )
        dictionary_names_in_use.emplace_back(dictionary->GetName());


    // add forms and external dicts
    for( FileAssociation& file_association : aplFileAssociationsDlg.m_fileAssociations )
    {
        try
        {
            if( SO::IsBlank(file_association.GetNewFilename()) )
                continue;

            // make sure we have a full path for all the filenames (in case someone types in the name
            // rather than using the ... button)
            file_association.MakeNewFilenameFullPath(application.GetApplicationFilename());

            auto check_file_exists = [&]()
            {
                if( !PortableFunctions::FileIsRegular(file_association.GetNewFilename()) )
                {
                    throw CSProException(_T("The file %s could not be included because it is does not exist."),
                                         PortableFunctions::PathGetFilename(file_association.GetNewFilename()));
                }
            };

            auto ensure_dictionary_name_not_in_use = [&](const CString& dictionary_filename)
            {
                CString dictionary_name = JsonStream::GetValueFromSpecFile<CString, CDataDict>(JK::name, dictionary_filename);

                if( FindString(dictionary_names_in_use, dictionary_name) )
                {
                    throw CSProException(_T("Unable to add the dictionary '%s' to this application.\nThis dictionary has a name, '%s', that already exists in the application."),
                                         PortableFunctions::PathGetFilename(dictionary_filename), dictionary_name.GetString());
                }

                dictionary_names_in_use.emplace_back(dictionary_filename);
            };

            // if this is a dictionary and doesn't exist, create one (already did same for forms above)
            if( file_association.GetType() == FileAssociation::Type::Dictionary )
            {
                CFileStatus fStatus;
                BOOL bFileExists = CFile::GetStatus(file_association.GetNewFilename(), fStatus);
                file_association.SetNewFilename(fStatus.m_szFullName); // get full path in case user just entered name
                if(!bFileExists){
                    NewFileCreator::CreateDictionary(file_association.GetNewFilename());
                }

                ensure_dictionary_name_not_in_use(file_association.GetNewFilename());
            }

            // form file
            else if( file_association.GetType() == FileAssociation::Type::FormFile )
            {
                // check that the form file isn't already in use
                if( FindString(application.GetFormFilenames(), file_association.GetNewFilename()) )
                {
                    throw CSProException(_T("Unable to add the form file file \"%s\" to this application because it is already included."),
                                         PortableFunctions::PathGetFilename(file_association.GetNewFilename()));
                }

                // form file - check that the dict is ok to add
                CString sDictFileName = GetDictFilenameFromFormFile(file_association.GetNewFilename());
                ensure_dictionary_name_not_in_use(sDictFileName);
            }

            // report
            else if( file_association.GetType() == FileAssociation::Type::Report )
            {
                // make sure the file isn't already used
                const auto& report_named_text_sources = application.GetReportNamedTextSources();
                const auto& name_search = std::find_if(
                    report_named_text_sources.cbegin(),
                    report_named_text_sources.cend(),
                    [&](const auto& rnts) { return SO::EqualsNoCase(file_association.GetNewFilename(), rnts->text_source->GetFilename()); });

                if( name_search != report_named_text_sources.cend() )
                {
                    throw CSProException(_T("Unable to add the report file \"%s\" to this application because it is already included."),
                                         PortableFunctions::PathGetFilename(file_association.GetNewFilename()));
                }

                // if the report does not exist, create a default one
                if( !PortableFunctions::FileIsRegular(file_association.GetNewFilename()) )
                    CreateDefaultReport(CS2WS(file_association.GetNewFilename()));

                check_file_exists();
            }

            // logic or message file
            else if( file_association.GetType() == FileAssociation::Type::Logic ||
                     file_association.GetType() == FileAssociation::Type::Message )
            {
                bool logic_file = ( file_association.GetType() == FileAssociation::Type::Logic );

                // only add valid files
                check_file_exists();

                // make sure the file isn't already used
                if( logic_file ? IsFilenameInUse(application.GetCodeFiles(), file_association.GetNewFilename()) :
                                 IsFilenameInUse(application.GetMessageTextSources(), file_association.GetNewFilename()) )
                {
                    CString msg;
                    msg.Format(logic_file ? IDS_LOGIC_ALREADY_IN_APP : IDS_MESSAGE_ALREADY_IN_APP,
                               PortableFunctions::PathGetFilename(file_association.GetNewFilename()));
                    throw CSProException(msg);
                }
            }

            // resource folder
            else
            {
                ASSERT(file_association.GetType() == FileAssociation::Type::ResourceFolder);

                if( !PortableFunctions::FileIsDirectory(file_association.GetNewFilename()) )
                {
                    CString msg;
                    msg.Format(IDS_RESOURCE_FOLDER_INVALID, file_association.GetNewFilename().GetString());
                    throw CSProException(msg);
                }

                if( ContainsStringInVectorNoCase(application.GetResourceFolders(), file_association.GetNewFilename()) )
                {
                    CString msg;
                    msg.Format(IDS_RESOURCE_FOLDER_ALREADY_IN_APP, file_association.GetNewFilename().GetString());
                    throw CSProException(msg);
                }
            }


            if( file_association.GetType() == FileAssociation::Type::ResourceFolder )
            {
                AddResourceFolderToApp((CAplDoc*)pAppItemID->GetDocument(), file_association.GetNewFilename());
            }

            else
            {
                AddFileToApp(*pAppItemID, file_association);
            }
        }

        catch( const CSProException& exception )
        {
            ErrorMessage::Display(exception);
        }
    }

    // adding a working storage dictionary 
    if( aplFileAssociationsDlg.m_bWorkingStorage && working_storage_dictionary_filename.empty() )
    {
        ASSERT(aplFileAssociationsDlg.m_workingStorageType == CAplFileAssociationsDlg::WorkingStorageType::Editable);

        try
        {
            constexpr const TCHAR* working_storage_dictionary_name = _T("WS_DICT");

            if( FindString(dictionary_names_in_use, working_storage_dictionary_name) )
                throw CSProException(_T("Unable to add a working storage dictionary because a dictionary already exists with the name '%s'."), working_storage_dictionary_name);

            working_storage_dictionary_filename = NewFileCreator::CreateWorkingStorageDictionary(application);
            AddExternalDictionaryToTree(pAppItemID, WS2CS(working_storage_dictionary_filename));
        }

        catch( const CSProException& exception )
        {
            ErrorMessage::Display(exception);
        }
    }

    pInsertToApp->OpenAllDocuments();
    CView* pView = (CView*)assert_cast<CMainFrame*>(AfxGetMainWnd())->MDIGetActive()->GetActiveView();

    if(assert_cast<CMainFrame*>(AfxGetMainWnd())->MDIGetActive()->IsKindOf(RUNTIME_CLASS(CDictChildWnd))) {
        CDDTreeCtrl& dictTree= assert_cast<CMainFrame*>(AfxGetMainWnd())->GetDlgBar().m_DictTree;
        HTREEITEM hItem = dictTree.GetDictionaryTreeNode(*pView->GetDocument())->GetHItem();
        dictTree.SelectItem(hItem);
    }
    else {
        pView->RedrawWindow();
    }

    pInsertToApp->SetAppObjects();
    pInsertToApp->ReconcileDictTypes();
}


void CCSProApp::CreateDefaultReport(const std::wstring& report_filename)
{
    ASSERT(!PortableFunctions::FileIsRegular(report_filename));

    // if a HTML report, see if the user wants to use one of the templates
    if( FileExtensions::IsFilenameHtml(report_filename) )
    {
        std::wstring templates_directory = Html::GetDirectory(Html::Subdirectory::Templates);
        std::wstring basic_report_template = PortableFunctions::PathAppendToPath(templates_directory, _T("report-basic.html"));
        std::wstring sections_report_template = PortableFunctions::PathAppendToPath(templates_directory, _T("report-sections.html"));

        if( PortableFunctions::FileIsRegular(basic_report_template) && PortableFunctions::FileIsRegular(sections_report_template) )
        {
            ChoiceDlg choice_dlg(0);

            choice_dlg.SetTitle(_T("Would you like to use one of the HTML report templates?"));

            choice_dlg.AddChoice(_T("Basic report"));
            choice_dlg.AddChoice(_T("Report with sections and a header/footer"));

            if( choice_dlg.DoModal() == IDOK )
            {
                const std::wstring& desired_template = ( choice_dlg.GetSelectedChoiceIndex() == 0 ) ? basic_report_template :
                                                                                                      sections_report_template;
                PortableFunctions::FileCopy(desired_template, report_filename, true);
                return;
            }
        }
    }

    // if here, the user wants a blank file
    FileIO::WriteText(report_filename, _T(""), true);
}


//Update for CSBatch support 05/22/00
BOOL CCSProApp::Reconcile(CDocument *pDoc, CString& csErr, bool bSilent, bool bAutoFix)
{
    BOOL  bRet = FALSE;
    CString sFileName = pDoc->GetPathName();
    CString sExt(PathFindExtension(sFileName.GetBuffer(_MAX_PATH)));

    sFileName.ReleaseBuffer();
    if(sExt.CompareNoCase(FileExtensions::WithDot::EntryApplication) == 0 ||
                sExt.CompareNoCase(FileExtensions::WithDot::TabulationApplication) == 0 || sExt.CompareNoCase(FileExtensions::WithDot::BatchApplication) == 0){
        ASSERT(pDoc->IsKindOf(RUNTIME_CLASS(CAplDoc)));
        bRet = assert_cast<CAplDoc*>(pDoc)->Reconcile(csErr, bSilent, bAutoFix);
    }
    else if(sExt.CompareNoCase(FileExtensions::WithDot::Form) == 0) {
        ASSERT(pDoc->IsKindOf(RUNTIME_CLASS(CFormDoc)));

        // first stuff dictionary pointers into formfile object
        CMainFrame* pFrame = assert_cast<CMainFrame*>(AfxGetMainWnd());
        CDDTreeCtrl& dictTree = pFrame->GetDlgBar().m_DictTree;
        CDEFormFile* pFormFile = &assert_cast<CFormDoc*>(pDoc)->GetFormFile();

        DictionaryDictTreeNode* dictionary_dict_tree_node = dictTree.GetDictionaryTreeNode(pFormFile->GetDictionaryFilename());
        CDDDoc* pDDDoc = dictionary_dict_tree_node->GetDDDoc();
        if(pDDDoc) {
            pFormFile->SetDictionary(pDDDoc->GetSharedDictionary());
        }

        // then reconcile
        bRet = assert_cast<CFormDoc*>(pDoc)->GetFormFile().Reconcile(csErr, bSilent, bAutoFix);
        if (!bRet) {
            CFormTreeCtrl& formTree = pFrame->GetDlgBar().m_FormTree;
            CFormNodeID* pFormID  = formTree.GetFormNode(sFileName);
            pFormID->GetFormDoc()->SetModifiedFlag();
            formTree.ReBuildTree();
        }
    }
    else if(sExt.CompareNoCase(FileExtensions::WithDot::Order) == 0) {
        ASSERT(pDoc->IsKindOf(RUNTIME_CLASS(COrderDoc)));

        // first stuff dictionary pointers into formfile object
        CMainFrame* pFrame = assert_cast<CMainFrame*>(AfxGetMainWnd());
        CDDTreeCtrl& dictTree = pFrame->GetDlgBar().m_DictTree;
        CDEFormFile* pOrderFile = &assert_cast<COrderDoc*>(pDoc)->GetFormFile();

        DictionaryDictTreeNode* dictionary_dict_tree_node = dictTree.GetDictionaryTreeNode(pOrderFile->GetDictionaryFilename());
        CDDDoc* pDDDoc = dictionary_dict_tree_node->GetDDDoc();
        if(pDDDoc) {
            pOrderFile->SetDictionary(pDDDoc->GetSharedDictionary());
        }

        // then reconcile
        bRet = assert_cast<COrderDoc*>(pDoc)->GetFormFile().OReconcile(csErr, bSilent, bAutoFix);

        if (!bRet) {
            COrderTreeCtrl& orderTree = pFrame->GetDlgBar().m_OrderTree;
            FormOrderAppTreeNode* form_order_app_tree_node = orderTree.GetFormOrderAppTreeNode(sFileName);
            form_order_app_tree_node->GetOrderDocument()->SetModifiedFlag();
            orderTree.ReBuildTree();
        }
    }
    else if(sExt.CompareNoCase(FileExtensions::WithDot::TableSpec) == 0) {
        ASSERT(pDoc->IsKindOf(RUNTIME_CLASS(CTabulateDoc)));
        bRet = ((CTabulateDoc*)pDoc)->Reconcile(csErr, bSilent, bAutoFix);
    }
    else {
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
    CMainFrame* pFrame = assert_cast<CMainFrame*>(AfxGetMainWnd());
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
        FileTreeNode* file_tree_node = ObjTree.FindNode(sDictName);
        if( file_tree_node != nullptr ) {
            DictionaryDictTreeNode* dictionary_dict_tree_node = dictTree.GetDictionaryTreeNode(sDictName);
            ASSERT(dictionary_dict_tree_node != nullptr);
            dictionary_dict_tree_node->Release();
            ObjTree.DeleteNode(*file_tree_node);
        }
    }
    else if(pDoc->IsKindOf(RUNTIME_CLASS(CTabulateDoc))){
        //Check if the dictionaries are free hanging on the object tree and remove them
        CTabulateDoc* pTabDoc = (CTabulateDoc*)pDoc;
        CString sDictName = pTabDoc->GetDictFileName();

        FileTreeNode* file_tree_node = ObjTree.FindNode(sDictName);
        if( file_tree_node != nullptr ) {
            DictionaryDictTreeNode* dictionary_dict_tree_node = dictTree.GetDictionaryTreeNode(sDictName);
            ASSERT(dictionary_dict_tree_node != nullptr);
            dictionary_dict_tree_node->Release();
            ObjTree.DeleteNode(*file_tree_node);
        }

    }
    else if(pDoc->IsKindOf(RUNTIME_CLASS(COrderDoc))){
        //Check if the dictionaries are free hanging on the object tree and remove them
        COrderDoc* pOrderDoc = assert_cast<COrderDoc*>(pDoc);
        CString sDictName = pOrderDoc->GetFormFile().GetDictionaryFilename();
        FileTreeNode* file_tree_node = ObjTree.FindNode(sDictName);
        if( file_tree_node != nullptr ) {
            DictionaryDictTreeNode* dictionary_dict_tree_node = dictTree.GetDictionaryTreeNode(sDictName);
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
    CMainFrame* pFrame = assert_cast<CMainFrame*>(AfxGetMainWnd());
    CObjTreeCtrl& ObjTree = pFrame->GetDlgBar().m_ObjTree;
    CDDTreeCtrl& dictTree = pFrame->GetDlgBar().m_DictTree;
    CFormTreeCtrl& formTree = pFrame->GetDlgBar().m_FormTree;
    CTabTreeCtrl& tableTree = pFrame->GetDlgBar().m_TableTree;
    COrderTreeCtrl& orderTree = pFrame->GetDlgBar().m_OrderTree;

    const Application& application = pAplDoc->GetAppObject();
    EngineAppType appType = pAplDoc->GetEngineAppType();

    //Do Forms
    if(appType == EngineAppType::Entry) {
        for( const CString& form_filename : application.GetFormFilenames() ) {
            FileTreeNode* file_tree_node = ObjTree.FindNode(form_filename);
            CFormNodeID* pFormID = formTree.GetFormNode(form_filename);

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
        for( const CString& tab_spec_filename : application.GetTabSpecFilenames() ) {
            FileTreeNode* file_tree_node = ObjTree.FindNode(tab_spec_filename);
            TableSpecTabTreeNode* table_spec_tab_tree_node = tableTree.GetTableSpecTabTreeNode(tab_spec_filename);
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
        for( const CString& order_filename : application.GetFormFilenames() ) {
            FileTreeNode* file_tree_node = ObjTree.FindNode(order_filename);
            FormOrderAppTreeNode* form_order_app_tree_node = orderTree.GetFormOrderAppTreeNode(order_filename);
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
    for( const CString& dictionary_name : application.GetExternalDictionaryFilenames() ) {
        FileTreeNode* file_tree_node = ObjTree.FindNode(dictionary_name);
        if( file_tree_node != nullptr ) {
            DictionaryDictTreeNode* dictionary_dict_tree_node = dictTree.GetDictionaryTreeNode(dictionary_name);
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
    std::wstring examples_folder = PortableFunctions::PathAppendToPath(GetWindowsSpecialFolder(WindowsSpecialFolder::Documents),
                                                                       FormatTextCS2WS(_T("CSPro\\Examples %0.1f"), CSPRO_VERSION_NUMBER));

    if( PortableFunctions::FileIsDirectory(examples_folder) )
    {
        OpenContainingFolder(examples_folder);
    }

    else
    {
        AfxMessageBox(FormatText(_T("The Examples folder could not be located. It is generally found here: %s"), examples_folder.c_str()));
    }
}


void CCSProApp::OnHelpTroubleshooting()
{
    CTroubleshootingDialog dlg;
    dlg.DoModal();
}


void CCSProApp::OnHelpMailingList()
{
    Viewer().ViewHtmlUrl(_T("https://public.govdelivery.com/accounts/USCENSUS/subscriber/new?topic_id=USCENSUS_11799"));
}


void CCSProApp::OnHelpAndroidApp()
{
    Viewer().ViewHtmlUrl(_T("https://play.google.com/store/apps/details?id=gov.census.cspro.csentry"));
}


void CCSProApp::OnHelpShowSyncLog()
{
    std::wstring log_filename = PortableFunctions::PathAppendToPath(GetAppDataPath(), _T("sync.log"));

    if( PortableFunctions::FileExists(log_filename) )
    {
        OpenContainingFolder(log_filename);
    }

    else
    {
        AfxMessageBox(FormatText(_T("The sync.log file could not be located. It is generally found here: %s"), log_filename.c_str()));
    }
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
    if constexpr(IsBetaBuild())
    {
        VersionShifterDlg dlg;

        if( dlg.GetNumVersions() < 2 )
            AfxMessageBox(_T("More than one version of CSPro was not found on your computer."));

        else
            dlg.DoModal();
    }
}

void CCSProApp::OnToolsVersionShifter(CCmdUI* pCmdUI)
{
    // hide the version shifter unless they are running the beta
    if( !IsBetaBuild() && pCmdUI->m_pMenu != nullptr )
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


void CCSProApp::OnCSProSettings()
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
               ( nID == ID_TOOLS_DATAVIEWER )          ? CSProExecutables::Program::DataViewer :
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
                        filename_argument = PortableFunctions::PathRemoveFileExtension(*filename_argument) + FileExtensions::WithDot::Pff;

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
        Listing::Lister::View(*filename_argument);
    }

    else if( filename_argument.has_value() )
    {
        CSProExecutables::RunProgramOpeningFile(tool_type, *filename_argument);
    }

    else
    {
        CSProExecutables::RunProgram(tool_type);
    }
}
