// entryrun.cpp : Defines the class behaviors for the application.
//

#include "StdAfx.h"
#include "CSEntry.h"
#include "MainFrm.h"
#include "Rundoc.h"
#include "RunView.h"
#include "LeftView.h"
#include "DynamicMenu.h"
#include <zToolsO/Serializer.h>
#include <zUtilO/AppLdr.h>
#include <zUtilO/CSProExecutables.h>
#include <zUtilO/imsaDlg.H>
#include <zUtilO/Filedlg.h>
#include <zUtilO/WinFocSw.h>
#include <ZBRIDGEO/PifDlg.h>
#include <zEngineF/PifInfoPopulator.h>
#include <zCapiO/QSFView.h>
#include <afxadv.h> // for mru stuff


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[]= __FILE__;
#endif

TCHAR DECIMAL_CHAR = '.';

/////////////////////////////////////////////////////////////////////////////
// CEntryrunApp

BEGIN_MESSAGE_MAP(CEntryrunApp, CWinApp)
        //{{AFX_MSG_MAP(CEntryrunApp)
        ON_COMMAND(ID_APP_ABOUT, OnAppAbout)
        ON_COMMAND(ID_FILE_OPEN, OnFileOpen)
        ON_COMMAND_EX_RANGE(ID_FILE_MRU_FILE1, ID_FILE_MRU_FILE16, OnOpenRecentFile)
        ON_UPDATE_COMMAND_UI(ID_FILE_MRU_FILE1, OnUpdateRecentFileMenu)
        ON_COMMAND(ID_OPENDAT_FILE, OnOpenDatFile)
        ON_UPDATE_COMMAND_UI(ID_OPENDAT_FILE, OnUpdateOpenDatFile)
        ON_COMMAND(ID_FILE_OPEN_DATORAPL, OnOpenDatFile)
        ON_UPDATE_COMMAND_UI(ID_FILE_OPEN_DATORAPL, OnUpdateOpenDatFile)
        //}}AFX_MSG_MAP
END_MESSAGE_MAP()

// Helper classes to be used with the window focus manager (see constructor below)
// Need one of these for each of the types of windows that you want to switch focus between.

// focus switcher for main form view
struct CRunViewFocusSwitcher : public CViewFocusSwitcher
{
    virtual bool MatchWindow(CWnd* pWnd) const
    {
        return pWnd->IsKindOf(RUNTIME_CLASS(CEntryrunView)) != FALSE;
    }
};

// focus switcher for main form view
struct CQTxtViewFocusSwitcher : public CViewFocusSwitcher
{
    virtual void FindWindows(CWindowFocusMgr* pMgr, CWnd* pAppMainWnd)
    {
        CMainFrame* pMainFrame = (CMainFrame*) pAppMainWnd;
        if (pMainFrame->m_wndCapiSplitter.m_bUseQuestionText) {
            CViewFocusSwitcher::FindWindows(pMgr, pAppMainWnd);
        }
    }

    virtual bool MatchWindow(CWnd* pWnd) const
    {
        return pWnd->IsKindOf(RUNTIME_CLASS(QSFView)) != FALSE;
    }
};

// focus switcher for tree controls in prop pages (case view and case trees)
struct CTreePropPageFocusSwitcher : public CViewFocusSwitcher
{
    virtual void FindWindows(CWindowFocusMgr* pMgr, CWnd* pAppMainWnd)
    {
        CMainFrame* pMainFrame = (CMainFrame*) pAppMainWnd;
        CRect rect;
        ::GetWindowRect(pMainFrame->GetLeftView()->GetSafeHwnd(),&rect);
        if (rect.Width() > 0) {
            if (pMainFrame->GetCaseView()) {
                pMgr->AddWindow(pMainFrame->GetCaseView()->GetParent(), this);
            }
            if (pMainFrame->GetCaseTree()) {
                pMgr->AddWindow(pMainFrame->GetCaseTree()->GetParent(), this);
            }
        }
    }

    virtual bool MatchWindow(CWnd* pWnd) const
    {
        return pWnd->IsKindOf(RUNTIME_CLASS(CPropertyPage)) != FALSE;
    }

    virtual void SetFocus(CWnd* pWnd)
    {
        ASSERT(pWnd->IsKindOf(RUNTIME_CLASS(CPropertyPage)));
        CPropertyPage* pPage = (CPropertyPage*) pWnd;
        CPropertySheet* pSheet = (CPropertySheet*) pPage->GetParent();
        ASSERT_VALID(pSheet);
        pSheet->SetActivePage(pPage);
        CMainFrame* pMainFrame = DYNAMIC_DOWNCAST(CMainFrame, AfxGetApp()->m_pMainWnd);
        if (pMainFrame->GetCaseTree() && pPage == pMainFrame->GetCaseTree()->GetParent()) {
            pMainFrame->GetCaseTree()->SetFocus();
            pMainFrame->SetActiveView(pMainFrame->GetLeftView(), FALSE);
            pMainFrame->m_bPage1StatusChangeStopFocusChangeHack = true;
        }
        else if (pMainFrame->GetCaseView() && pPage == pMainFrame->GetCaseView()->GetParent()) {
            pMainFrame->GetCaseView()->SetFocus();
            pMainFrame->SetActiveView(pMainFrame->GetLeftView(), FALSE);
            pMainFrame->m_bPage1StatusChangeStopFocusChangeHack = true;
        }
    }

    virtual bool MustBeVisible() const
    {
        return false;
    }

};

/////////////////////////////////////////////////////////////////////////////
// CEntryrunApp construction

CEntryrunApp::CEntryrunApp()
{
    InitializeCSProEnvironment();

    EnableHtmlHelp();

    // Place all significant initialization in InitInstance
    m_pRunAplEntry = NULL;
    m_pPifFile = NULL;
    m_bPffLaunchedFromCommandLine = false;

    // Setup the window focus manager - handles changing focus between
    // main app windows on a hotkey (for accessibility)
    // Called in PreTranslateMessage
    m_pWindowFocusMgr = new CWindowFocusMgr;
    m_pWindowFocusMgr->AddSwitcher(new CQTxtViewFocusSwitcher);
    m_pWindowFocusMgr->AddSwitcher(new CRunViewFocusSwitcher);
    m_pWindowFocusMgr->AddSwitcher(new CTreePropPageFocusSwitcher);
}


CEntryrunApp::~CEntryrunApp()
{
    ApplicationShutdown();
    delete m_pWindowFocusMgr;
}

int CEntryrunApp::ExitInstance()
{
    ApplicationShutdown(true);
    return CWinApp::ExitInstance();
}

void CEntryrunApp::ApplicationShutdown(bool bCSEntryClosing/* = false*/)
{
    if( m_pRunAplEntry != NULL )
    {
        if( m_pPifFile->GetApplication()->IsCompiled() )
        {
            m_pRunAplEntry->Stop();
            m_pRunAplEntry->End(FALSE);
        }

        SAFE_DELETE(m_pRunAplEntry);

        if( bCSEntryClosing && m_bPffLaunchedFromCommandLine )
            m_pPifFile->ExecuteOnExitPff();
    }

    SAFE_DELETE(m_pPifFile);
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CEntryrunApp object

CEntryrunApp theApp;

/////////////////////////////////////////////////////////////////////////////
//
//                        CEntryrunApp::InitInstance
//
/////////////////////////////////////////////////////////////////////////////
BOOL CEntryrunApp::InitInstance()
{
    AfxOleInit();
    AfxEnableControlContainer();

    // Standard initialization
    // If you are not using these features and wish to reduce the size
    //  of your final executable, you should remove from the following
    //  the specific initialization routines you do not need.

    // Change the registry key under which our settings are stored.
    // TODO: You should modify this string to be something appropriate
    // such as the name of your company or organization.
    SetRegistryKey(_T("U.S. Census Bureau"));

    LoadStdProfileSettings();  // Load standard INI file options (including MRU)

    CSingleDocTemplate* pDocTemplate;
    pDocTemplate = new CSingleDocTemplate(
        IDR_MAINFRAME,
        RUNTIME_CLASS(CEntryrunDoc),
        RUNTIME_CLASS(CMainFrame),       // main SDI frame window
        RUNTIME_CLASS(CEntryrunView));

    AddDocTemplate(pDocTemplate);

    ParseCommandLine(m_cmdInfo);
    m_cmdInfo.UpdateBinaryGen();


    CString csFileName = m_cmdInfo.m_strFileName;

    if( m_cmdInfo.m_nShellCommand == CCommandLineInfo::FileOpen && !csFileName.IsEmpty() )
    {
        m_cmdInfo.m_strFileName = _T("");
        m_cmdInfo.m_nShellCommand = CCommandLineInfo::FileNew;
    }

    // dispatch commands specified on the command line
    if( !ProcessShellCommand(m_cmdInfo) )
        return FALSE;


    // make sure that the filename, if specified, has the full path information
    if( !csFileName.IsEmpty() )
    {
        CString current_directory;
        GetCurrentDirectory(_MAX_PATH, current_directory.GetBuffer(_MAX_PATH));
        current_directory.ReleaseBuffer();

        csFileName = WS2CS(MakeFullPath(current_directory, CS2WS(csFileName)));
    }


    // create the .pen file and exit if generating a binary archive
    if( BinaryGen::isGeneratingBinary() )
    {
        CreatePenFile(csFileName);
        m_pMainWnd->DestroyWindow();
        delete m_pMainWnd;
        return FALSE;
    }


    // Add toolbar icons to menus
    CMainFrame* pMainframe = (CMainFrame*)m_pMainWnd;
    static UINT toolbars[] = { IDR_MAINFRAME };

    pMainframe->m_menu.LoadMenu(IDR_MAINFRAME);
    pMainframe->m_menu.LoadToolbars(toolbars,1);
    pMainframe->SetMenu(&pMainframe->m_menu);

    // to allow for custom menus in CSEntry, see if there is an override file in the application folder or the executables folder
    for( int i = 0; i < 2; ++i )
    {
        std::wstring directory = ( i == 0 ) ? PortableFunctions::PathGetDirectory(csFileName) :
                                              CSProExecutables::GetApplicationDirectory();

        std::wstring override_filename = PortableFunctions::PathAppendToPath(directory, CSEntryLanguageOverrideFile);

        if( PortableFunctions::FileIsRegular(override_filename) )
        {
            ActivateDynamicMenus(WS2CS(override_filename), pMainframe->m_menu);
            break;
        }
    }

    // The one and only window has been initialized, so show and update it.
    m_pMainWnd->ShowWindow(SW_MAXIMIZE);
    m_pMainWnd->SetWindowText(_T("CSEntry"));

    m_bPffLaunchedFromCommandLine = SO::EqualsNoCase(PortableFunctions::PathGetFileExtension(csFileName), FileExtensions::Pff);

    OpenApplicationHelper(csFileName);

    return TRUE;
}


// App command to run the dialog
void CEntryrunApp::OnAppAbout()
{
    CIMSAAboutDlg dlg;
    HICON m_hIcon = LoadIcon(IDR_MAINFRAME);
    dlg.m_hIcon = m_hIcon;
    dlg.m_csModuleName = _T("CSEntry");
    dlg.DoModal();
}

BOOL CEntryrunApp::PreTranslateMessage(MSG* pMsg)
{
    // test for switch window focus via keystroke
    if( m_pWindowFocusMgr->PreTranslateMessage(m_pMainWnd,pMsg) )
        return TRUE;

    return CWinApp::PreTranslateMessage(pMsg);
}


void CEntryrunApp::OnFileOpen()
{
    OpenApplicationHelper(_T(""));
}

void CEntryrunApp::OnOpenDatFile()
{
    OpenApplicationHelper(m_csCurrentDocumentName,true);
}

void CEntryrunApp::OnUpdateOpenDatFile(CCmdUI* pCmdUI)
{
    BOOL bEnable = FALSE;

    if( !m_csCurrentDocumentName.IsEmpty() )
        bEnable = !m_pPifFile->GetFileOpenFlag();

    pCmdUI->Enable(bEnable);
}

BOOL CEntryrunApp::OnOpenRecentFile(UINT nID)
{
    int nIndex = nID - ID_FILE_MRU_FILE1;

    CString csFilename = (*m_pRecentFileList)[nIndex];

    if( PortableFunctions::FileExists(csFilename) )
        OpenApplicationHelper(csFilename);

    else
    {
        CString csMessage;
        csMessage.Format(_T("The application %s no longer exists."),PortableFunctions::PathGetFilename(csFilename));
        AfxMessageBox(csMessage);

        m_pRecentFileList->Remove(nIndex);
    }

    return TRUE;
}

void CEntryrunApp::OnUpdateRecentFileMenu(CCmdUI* pCmdUI)
{
    ASSERT_VALID(this);

    if( m_pRecentFileList == NULL ) // no MRU files
        pCmdUI->Enable(FALSE);

    else
        m_pRecentFileList->UpdateMenu(pCmdUI);
}


bool CEntryrunApp::ShowPifDlg(bool bSavePif)
{
    PifInfoPopulator pif_info_populator(m_pRunAplEntry->GetEntryDriver()->GetEngineData(), *m_pPifFile);

    CPifDlg pif_dlg(pif_info_populator.GetPifInfo(), m_pPifFile->GetEvaluatedAppDescription());
    pif_dlg.m_pPifFile = m_pPifFile;

    if( pif_dlg.DoModal() != IDOK )
        return false;

    // potentially save the associations
    if( bSavePif )
        m_pPifFile->Save();

    return true;
}


bool CEntryrunApp::InitNCompileApp()
{
    CWaitCursor wait;

    if( !m_pPifFile->BuildAllObjects() )
        return false;

    Application* pApp = m_pPifFile->GetApplication();

    m_pRunAplEntry = new CRunAplEntry(m_pPifFile);

    pApp->SetCompiled(false);

    bool bCompileSuccess = m_pRunAplEntry->LoadCompile();

    if( pApp->GetAppLoader()->GetBinaryFileLoad() )
        APP_LOAD_TODO_GetArchive().CloseArchive();

    if( !bCompileSuccess )
    {
        SAFE_DELETE(m_pRunAplEntry);
        return false;
    }

    pApp->SetCompiled(true);

    return true;
}


void CEntryrunApp::CreatePenFile(const TCHAR* filename) // generate a .pen file
{
    // hide the CSEntry window
    m_pMainWnd->ShowWindow(SW_HIDE);

    // make sure that there is an .ent file specified
    std::wstring extension = PortableFunctions::PathGetFileExtension(filename);

    if( !SO::EqualsNoCase(extension, FileExtensions::EntryApplication) || !PortableFunctions::FileIsRegular(filename) )
    {
        AfxMessageBox(_T("You can only publish an entry application by specifying the application .ent file."));
        return;
    }

    // create a dummy pff object with the application name
    CNPifFile pff;
    pff.SetAppFName(filename);

    auto serializer = std::make_shared<Serializer>();
    APP_LOAD_TODO_SetArchive(serializer);
    bool success = false;

    try
    {
        serializer->CreateOutputArchive(BinaryGen::GetBinaryName());

        Application application;
        application.GetAppLoader()->SetBinaryFileLoad(false);

        application.Open(filename, true);
        *serializer & application;

        if( pff.BuildAllObjects() )
        {
            CRunAplEntry runAplEntry(&pff);

            if( !runAplEntry.LoadCompile() )
                throw CSProException("Compilation error");

            success = true;
        }

        serializer->CloseArchive();
    }

    catch( const CSProException& exception )
    {
        AfxMessageBox(FormatText(_T("There was an error creating the .pen file: %s"), exception.GetErrorMessage().c_str()));
    }

    APP_LOAD_TODO_SetArchive(nullptr);

    if( !success )
        PortableFunctions::FileDelete(BinaryGen::GetBinaryName());
}


void CEntryrunApp::OpenApplicationHelper(CString csFilename,bool bForceShowPifDlg/* = false*/)
{
    // if no filename is passed in, query the user for one
    if( csFilename.IsEmpty() )
    {
        CIMSAFileDialog fileDlg(TRUE,NULL,NULL,OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
            _T("Application Files (*.ent;*.pen)|*.ent;*.pen|PFF Files (*.pff)|*.pff||"));

        if( fileDlg.DoModal() != IDOK)
            return;

        csFilename = fileDlg.GetPathName();
    }


    // if entry is in process, make sure that the user wants to end it
    POSITION pos = this->GetFirstDocTemplatePosition();
    CDocTemplate* pTemplate = this->GetNextDocTemplate(pos);
    CEntryrunDoc* pRunDoc = NULL;

    if( pTemplate != NULL )
    {
        pos = pTemplate->GetFirstDocPosition();
        CDocument* pDoc = pTemplate->GetNextDoc(pos);

        if( pDoc != NULL && pDoc->IsKindOf(RUNTIME_CLASS(CEntryrunDoc)) )
        {
            pRunDoc = (CEntryrunDoc*)pDoc;

            bool bModified = pRunDoc->GetQModified() && !pRunDoc->GetRunApl()->IsNewCase();

            if( bModified )
            {
                if( AfxMessageBox(MGF::GetMessageText(MGF::DiscardQuestionnaire).c_str(), MB_YESNO) == IDNO )
                    return;
            }
        }
    }

    CMainFrame* pFrame = (CMainFrame*)AfxGetMainWnd();

    pFrame->OnStop();

    if( !pFrame->m_bOnStop )
        return;


    // terminate any objects that exist
    if( m_pRunAplEntry != NULL )
    {
        pRunDoc->DeleteContents();
        pRunDoc->OnNewDocument();
        ApplicationShutdown();
    }

    SAFE_DELETE(m_pPifFile);

    // load the application or close CSEntry upon failure
    if( !LoadApplication(csFilename,bForceShowPifDlg) )
    {
        pRunDoc->DeleteContents();
        ApplicationShutdown();
        AfxGetMainWnd()->SendMessage(WM_CLOSE);
        return;
    }

    PostLoadApplicationOperations();

    ProcessStartMode();
}


bool CEntryrunApp::LoadApplication(CString csFilename,bool bForceShowPifDlg/* = false*/)
{
    if( !PortableFunctions::FileExists(csFilename) )
        return false;

    CString csPffFilename;
    CString csExt = PathFindExtension(csFilename);

    bool bBinaryLoad = ( csExt.CompareNoCase(FileExtensions::WithDot::BinaryEntryPen) == 0 );
    bool bDisplayPifDialog = true;

    if( csExt.CompareNoCase(FileExtensions::WithDot::Pff) == 0 )
    {
        m_pPifFile = new CNPifFile(csFilename);
        m_pPifFile->SetAppType(ENTRY_TYPE);

        if( !m_pPifFile->LoadPifFile() )
            return false;

        else if( m_pPifFile->GetAppType() != ENTRY_TYPE )
        {
            AfxMessageBox(_T("You can only run data entry applications."));
            return false;
        }

        csPffFilename = csFilename;

        if( !bForceShowPifDlg )
            bDisplayPifDialog = false;
    }

    else if( csExt.CompareNoCase(FileExtensions::WithDot::EntryApplication) == 0 || bBinaryLoad )
    {
        csPffFilename = PortableFunctions::PathRemoveFileExtension<CString>(csFilename) + FileExtensions::WithDot::Pff;

        // if there is a PFF file and it points to this .ent file, load it
        if( PortableFunctions::FileExists(csPffFilename) )
        {
            m_pPifFile = new CNPifFile(csPffFilename);

            CString csEntFilename = csFilename;

            if( bBinaryLoad )
                csEntFilename = PortableFunctions::PathRemoveFileExtension<CString>(csFilename) + FileExtensions::WithDot::EntryApplication;

            if( !m_pPifFile->LoadPifFile() || ( m_pPifFile->GetAppType() != ENTRY_TYPE ) || ( m_pPifFile->GetAppFName().CompareNoCase(csEntFilename) != 0 ) )
            {
                SAFE_DELETE(m_pPifFile);
            }
        }

        if( m_pPifFile == NULL ) // create a PFF for this program
        {
            m_pPifFile = new CNPifFile(csPffFilename);
            m_pPifFile->SetAppType(ENTRY_TYPE);
        }

        m_pPifFile->SetAppFName(csFilename); // necessary in case the .pen file is being loaded
    }

    else if( csExt.CompareNoCase(FileExtensions::Old::WithDot::BinaryEntryPen) == 0 )
    {
        AfxMessageBox(_T("Starting with version 6.0, .enc files are no longer supported. Please regenerate your data entry application as a .pen file."));
        return false;
    }

    else
    {
        AfxMessageBox(IDS_INVLDFTYPE);
        return false;
    }


    // compile the application
    if( !InitNCompileApp() )
        return false;

    // if wildcards are present in the any of the dictionary data file names,
    // then the PFF must be shown (but potentially not saved)

    // show the PFF dialog if necessary
    if( ( bDisplayPifDialog || m_pPifFile->EntryConnectionStringsContainWildcards() ) && !ShowPifDlg(bDisplayPifDialog) )
        return false;

    // store the name of the current application
    m_csCurrentDocumentName = csFilename;

    return ( OpenDocumentFile(csFilename) != NULL );
}


void CEntryrunApp::PostLoadApplicationOperations()
{
    CMainFrame* pFrame = (CMainFrame*)AfxGetMainWnd();
    CEntryrunDoc* pDoc = (CEntryrunDoc*)pFrame->GetActiveDocument();
    Application* pApp = m_pPifFile->GetApplication();

    // set the decimal character
    DECIMAL_CHAR = pApp->GetDecimalMarkIsComma() ? ',' : '.';

    // set the window title and layout
    CString csTitle = pDoc->MakeTitle(true);
    pDoc->SetTitle(csTitle);
    pFrame->SetWindowText(csTitle);

    pFrame->DoInitialApplicationLayout(m_pPifFile);
}


void CEntryrunApp::ProcessStartMode()
{
    CMainFrame* pFrame = (CMainFrame*)AfxGetMainWnd();
    CCaseView* pCaseView = pFrame->GetCaseView();
    CTreeCtrl& caseTree = pCaseView->GetTreeCtrl();

    if( m_pPifFile->GetCaseListingLockFlag() ) // if the case listing is locked, action is only taken on one case
        m_pPifFile->SetAutoAddFlag(false);

    enum class KeyToOpenMode { None, Key, StartModeAdd, StartModeModify };
    KeyToOpenMode key_to_open_mode = KeyToOpenMode::None;
    CString key_to_open;

    // StartMode will take precedence over Key
    if( m_pPifFile->GetStartMode() != StartMode::None )
    {
        if( m_pPifFile->GetStartMode() == StartMode::Verify )
        {
            pFrame->OnVerifyCase();
            return;
        }

        if( m_pPifFile->GetStartMode() == StartMode::Add )
            key_to_open_mode = KeyToOpenMode::StartModeAdd;

        else if( m_pPifFile->GetStartMode() == StartMode::Modify )
            key_to_open_mode = KeyToOpenMode::StartModeModify;

        if( key_to_open_mode != KeyToOpenMode::None )
            key_to_open = m_pPifFile->GetStartKeyString();
    }

    else if( !m_pPifFile->GetKey().IsEmpty() )
    {
        key_to_open_mode = KeyToOpenMode::Key;
        key_to_open = m_pPifFile->GetKey();
    }

    // find the proper key to open
    if( key_to_open_mode != KeyToOpenMode::None )
    {
        if( !key_to_open.IsEmpty() )
        {
            HTREEITEM hItem = caseTree.GetRootItem();

            while( hItem != NULL )
            {
                NODEINFO* pNodeInfo = (NODEINFO*)caseTree.GetItemData(hItem);
                CString csThisKey = pNodeInfo->case_summary.GetKey();

                // the StartMode key in the PFF file gets trimmed so we should do the same before our comparison
                if( key_to_open_mode != KeyToOpenMode::Key )
                    csThisKey.TrimRight();

                if( csThisKey.Compare(key_to_open) == 0 )
                {
                    caseTree.Select(hItem,TVGN_CARET);
                    pFrame->OnModifyCase();
                    return;
                }

                hItem = caseTree.GetNextSiblingItem(hItem);
            }

            if( key_to_open_mode == KeyToOpenMode::StartModeModify )
            {
                CString csMsg;
                csMsg.Format(_T("Case specified in 'StartMode' parameter of .PFF file not found in data file\n\nKey is '%s'"), (LPCTSTR)key_to_open);
                AfxMessageBox(csMsg);
            }
        }

        if( ( key_to_open_mode == KeyToOpenMode::StartModeAdd ) ||
            ( key_to_open_mode == KeyToOpenMode::Key && key_to_open.GetLength() >= m_pRunAplEntry->GetInputDictionaryKeyLength() ) )
        {
            pFrame->OnAddCase();
            return;
        }
    }

    // if the case listing is locked, then there should have been a start mode so we are here in error
    if( m_pPifFile->GetCaseListingLockFlag() )
    {
        AfxMessageBox(_T("With the case listing locked, you must specify what case to work with in the PFF file's StartMode or Key attribute."));
        AfxGetMainWnd()->SendMessage(WM_CLOSE);
        return;
    }

    // if there are no casetainers, automatically go into add mode; otherwise select the first casetainer in the tree
    HTREEITEM hItem = caseTree.GetRootItem();

    if( hItem == NULL )
        pFrame->OnAddCase();

    else
    {
        caseTree.Select(hItem,TVGN_CARET);
        pCaseView->SetFocus();
    }
}
