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
#include <zUtilO/FileDlg.h>
#include <zUtilO/imsaDlg.H>
#include <zUtilO/WinFocSw.h>
#include <zUtilF/CommonControls.h>
#include <ZBRIDGEO/PifDlg.h>
#include <zEngineF/PifInfoPopulator.h>
#include <zCapiO/QSFView.h>
#include <afxadv.h> // for mru stuff


// The one and only CEntryrunApp object
CEntryrunApp theApp;


TCHAR DECIMAL_CHAR = '.';


BEGIN_MESSAGE_MAP(CEntryrunApp, CWinApp)
        ON_COMMAND(ID_APP_ABOUT, OnAppAbout)
        ON_COMMAND(ID_FILE_OPEN, OnFileOpen)
        ON_COMMAND_EX_RANGE(ID_FILE_MRU_FILE1, ID_FILE_MRU_FILE16, OnOpenRecentFile)
        ON_UPDATE_COMMAND_UI(ID_FILE_MRU_FILE1, OnUpdateRecentFileMenu)
        ON_COMMAND(ID_OPENDAT_FILE, OnOpenDatFile)
        ON_UPDATE_COMMAND_UI(ID_OPENDAT_FILE, OnUpdateOpenDatFile)
        ON_COMMAND(ID_FILE_OPEN_DATORAPL, OnOpenDatFile)
        ON_UPDATE_COMMAND_UI(ID_FILE_OPEN_DATORAPL, OnUpdateOpenDatFile)
END_MESSAGE_MAP()

// Helper classes to be used with the window focus manager (see constructor below)
// Need one of these for each of the types of windows that you want to switch focus between.

// focus switcher for main form view
struct CRunViewFocusSwitcher : public CViewFocusSwitcher
{
    bool MatchWindow(CWnd* pWnd) const override
    {
        return pWnd->IsKindOf(RUNTIME_CLASS(CEntryrunView)) != FALSE;
    }
};


// focus switcher for main form view
struct CQTxtViewFocusSwitcher : public CViewFocusSwitcher
{
    void FindWindows(CWindowFocusMgr* pMgr, CWnd* pAppMainWnd) override
    {
        CMainFrame* pMainFrame = (CMainFrame*) pAppMainWnd;
        if (pMainFrame->m_wndCapiSplitter.m_bUseQuestionText) {
            CViewFocusSwitcher::FindWindows(pMgr, pAppMainWnd);
        }
    }

    bool MatchWindow(CWnd* pWnd) const override
    {
        return pWnd->IsKindOf(RUNTIME_CLASS(QSFView)) != FALSE;
    }
};


// focus switcher for tree controls in prop pages (case view and case trees)
struct CTreePropPageFocusSwitcher : public CViewFocusSwitcher
{
    void FindWindows(CWindowFocusMgr* pMgr, CWnd* pAppMainWnd) override
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

    bool MatchWindow(CWnd* pWnd) const override
    {
        return pWnd->IsKindOf(RUNTIME_CLASS(CPropertyPage)) != FALSE;
    }

    void SetFocus(CWnd* pWnd) override
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

    bool MustBeVisible() const override
    {
        return false;
    }

};


CEntryrunApp::CEntryrunApp()
    :   m_pWindowFocusMgr(std::make_unique<CWindowFocusMgr>()),
        m_pffLaunchedFromCommandLine(false)
{
    InitializeCSProEnvironment();

    EnableHtmlHelp();

    // Place all significant initialization in InitInstance
    m_pRunAplEntry = NULL;
    m_pPifFile = NULL;

    // Setup the window focus manager - handles changing focus between
    // main app windows on a hotkey (for accessibility)
    // Called in PreTranslateMessage
    m_pWindowFocusMgr->AddSwitcher(new CQTxtViewFocusSwitcher);
    m_pWindowFocusMgr->AddSwitcher(new CRunViewFocusSwitcher);
    m_pWindowFocusMgr->AddSwitcher(new CTreePropPageFocusSwitcher);
}


CEntryrunApp::~CEntryrunApp()
{
    ApplicationShutdown();
}


int CEntryrunApp::ExitInstance()
{
    ApplicationShutdown(true);

    return __super::ExitInstance();
}


void CEntryrunApp::ApplicationShutdown(const bool csentry_closing/* = false*/)
{
    if( m_pRunAplEntry != NULL )
    {
        if( m_pPifFile->GetApplication()->IsCompiled() )
        {
            m_pRunAplEntry->Stop();
            m_pRunAplEntry->End(FALSE);
        }

        SAFE_DELETE(m_pRunAplEntry);

        if( csentry_closing && m_pffLaunchedFromCommandLine )
            m_pPifFile->ExecuteOnExitPff();
    }

    SAFE_DELETE(m_pPifFile);
}


BOOL CEntryrunApp::InitInstance()
{
    InitializeCommonControls();

    AfxOleInit();
    AfxEnableControlContainer();

    // Standard initialization
    // If you are not using these features and wish to reduce the size
    //  of your final executable, you should remove from the following
    //  the specific initialization routines you do not need.

    // Change the registry key under which our settings are stored.
    // TODO: You should modify this string to be something appropriate
    // such as the name of your company or organization.
    SetRegistryKey(L"U.S. Census Bureau");

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

    std::string file_path = TC::ToUtf8(m_cmdInfo.m_strFileName);

    // evaluate the full path
    if( !file_path.empty() )
        file_path = MakeFullPath(GetWorkingDirectory(), file_path);

    if( m_cmdInfo.m_nShellCommand == CCommandLineInfo::FileOpen && !m_cmdInfo.m_strFileName.IsEmpty() )
    {
        m_cmdInfo.m_strFileName.Empty();
        m_cmdInfo.m_nShellCommand = CCommandLineInfo::FileNew;
    }

    // dispatch commands specified on the command line
    if( !ProcessShellCommand(m_cmdInfo) )
        return FALSE;


    // create the .pen file and exit if generating a binary archive
    if( BinaryGen::isGeneratingBinary() )
    {
        CreatePenFile(file_path);
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
        std::string override_file_path = ( i == 0 ) ? PortableFunctions::PathGetDirectory(file_path) :
                                                      CSProExecutables::GetApplicationDirectory();
        Path::MakeCombine(override_file_path, CSEntryLanguageOverrideFilename);

        if( PortableFunctions::FileIsRegular(override_file_path) )
        {
            ActivateDynamicMenus(override_file_path, pMainframe->m_menu);
            break;
        }
    }

    // The one and only window has been initialized, so show and update it.
    m_pMainWnd->ShowWindow(SW_MAXIMIZE);
    m_pMainWnd->SetWindowText(L"CSEntry");

    m_pffLaunchedFromCommandLine = SO::EqualsNoCase(PortableFunctions::PathGetFileExtension(file_path), FileExtensions::Pff);

    OpenApplicationHelper(file_path);

    return TRUE;
}


void CEntryrunApp::OnAppAbout()
{
    CIMSAAboutDlg about_dlg(L"CSEntry", LoadIcon(IDR_MAINFRAME));
    about_dlg.DoModal();
}


BOOL CEntryrunApp::PreTranslateMessage(MSG* const pMsg)
{
    // test for switch window focus via keystroke
    if( m_pWindowFocusMgr->PreTranslateMessage(m_pMainWnd,pMsg) )
        return TRUE;

    return __super::PreTranslateMessage(pMsg);
}


void CEntryrunApp::OnFileOpen()
{
    OpenApplicationHelper(SO::Empty_string);
}


void CEntryrunApp::OnOpenDatFile()
{
    OpenApplicationHelper(m_currentApplicationFilePath, true);
}


void CEntryrunApp::OnUpdateOpenDatFile(CCmdUI* const pCmdUI)
{
    pCmdUI->Enable(( !m_currentApplicationFilePath.empty() &&
                     !m_pPifFile->GetFileOpenFlag() ));
}


BOOL CEntryrunApp::OnOpenRecentFile(const UINT nID)
{
    const int nIndex = nID - ID_FILE_MRU_FILE1;

    const std::string file_path = TC::ToUtf8((*m_pRecentFileList)[nIndex]);

    if( PortableFunctions::FileIsRegular(file_path) )
    {
        OpenApplicationHelper(file_path);
    }

    else
    {
        AfxMessageBox(FormatText("The application '%s' no longer exists.", PortableFunctions::PathGetFilename(file_path).c_str()));
        m_pRecentFileList->Remove(nIndex);
    }

    return TRUE;
}


void CEntryrunApp::OnUpdateRecentFileMenu(CCmdUI* const pCmdUI)
{
    ASSERT_VALID(this);

    if( m_pRecentFileList == nullptr ) // no MRU files
    {
        pCmdUI->Enable(FALSE);
    }

    else
    {
        m_pRecentFileList->UpdateMenu(pCmdUI);
    }
}


bool CEntryrunApp::ShowPifDlg(const bool save_pff)
{
    PifInfoPopulator pif_info_populator(m_pRunAplEntry->GetEntryDriver()->GetEngineData(), *m_pPifFile);

    CPifDlg pif_dlg(pif_info_populator.GetPifInfo(), m_pPifFile->GetEvaluatedAppDescription());
    pif_dlg.m_pPifFile = m_pPifFile;

    if( pif_dlg.DoModal() != IDOK )
        return false;

    // potentially save the associations
    if( save_pff )
        m_pPifFile->Save();

    return true;
}


bool CEntryrunApp::InitNCompileApp()
{
    CWaitCursor wait;

    if( !m_pPifFile->BuildAllObjects() )
        return false;

    Application* const pApp = m_pPifFile->GetApplication();

    m_pRunAplEntry = new CRunAplEntry(m_pPifFile);

    pApp->SetCompiled(false);

    const bool compile_success = m_pRunAplEntry->LoadCompile();

    if( pApp->GetAppLoader()->GetBinaryFileLoad() )
        APP_LOAD_TODO_GetArchive().CloseArchive();

    if( !compile_success )
    {
        SAFE_DELETE(m_pRunAplEntry);
        return false;
    }

    pApp->SetCompiled(true);

    return true;
}


void CEntryrunApp::CreatePenFile(const std::string& application_file_path)
{
    // hide the CSEntry window
    m_pMainWnd->ShowWindow(SW_HIDE);

    // make sure that there is an .ent file specified
    const std::string extension = PortableFunctions::PathGetFileExtension(application_file_path);

    if( !SO::EqualsNoCase(extension, FileExtensions::EntryApplication) ||
        !PortableFunctions::FileIsRegular(application_file_path) )
    {
        AfxMessageBox(L"You can only publish an entry application by specifying the application .ent file.");
        return;
    }

    // create a dummy pff object with the application name
    CNPifFile pff;
    pff.SetAppFName(UTF8_TODO::GetCString(application_file_path));

    auto serializer = std::make_shared<Serializer>();
    APP_LOAD_TODO_SetArchive(serializer);
    bool success = false;

    try
    {
        serializer->CreateOutputArchive(UTF8_TODO::GetUtf8(BinaryGen::GetBinaryName()));

        Application application;
        application.GetAppLoader()->SetBinaryFileLoad(false);

        application.Open(application_file_path, true);
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
        AfxMessageBox(FormatText("There was an error creating the .pen file: %s", exception.what()));
    }

    APP_LOAD_TODO_SetArchive(nullptr);

    if( !success )
        PortableFunctions::FileDelete(BinaryGen::GetBinaryName());
}


void CEntryrunApp::OpenApplicationHelper(std::string application_file_path, const bool force_show_file_associations/* = false*/)
{
    // if no file path is passed in, query the user for one
    if( application_file_path.empty() )
    {
        OpenFileDlg open_file_dlg(0, nullptr, nullptr, "Application Files (*.ent;*.pen)|*.ent;*.pen|PFF Files (*.pff)|*.pff||");

        if( open_file_dlg.DoModal() != IDOK)
            return;

        application_file_path = open_file_dlg.GetFilePath();
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
                if( AfxMessageBox(MGF::GetMessageText(MGF::DiscardQuestionnaire).GetString(), MB_YESNO) == IDNO )
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
    if( !LoadApplication(application_file_path, force_show_file_associations) )
    {
        pRunDoc->DeleteContents();
        ApplicationShutdown();
        AfxGetMainWnd()->SendMessage(WM_CLOSE);
        return;
    }

    PostLoadApplicationOperations();

    ProcessStartMode();
}


bool CEntryrunApp::LoadApplication(const std::string& file_path, const bool force_show_file_associations/* = false*/)
{
    if( !PortableFunctions::FileExists(file_path) )
        return false;

    std::string pff_file_path;
    const std::string extension = PortableFunctions::PathGetFileExtension(file_path);

    const bool binary_load = SO::EqualsNoCase(extension, FileExtensions::BinaryEntryPen);
    bool show_file_associations = true;

    if( !binary_load && SO::EqualsNoCase(extension, FileExtensions::Pff) )
    {
        m_pPifFile = new CNPifFile(UTF8_TODO::GetCString(file_path));
        m_pPifFile->SetAppType(APPTYPE::ENTRY_TYPE);

        if( !m_pPifFile->LoadPifFile() )
        {
            return false;
        }

        else if( m_pPifFile->GetAppType() != APPTYPE::ENTRY_TYPE )
        {
            AfxMessageBox(L"You can only run data entry applications.");
            return false;
        }

        pff_file_path = file_path;

        if( !force_show_file_associations )
            show_file_associations = false;
    }

    else if( binary_load || SO::EqualsNoCase(extension, FileExtensions::EntryApplication) )
    {
        pff_file_path = PortableFunctions::PathReplaceFileExtension(file_path, FileExtensions::Pff);

        // if there is a PFF file and it points to this .ent file, load it
        if( PortableFunctions::FileIsRegular(pff_file_path) )
        {
            m_pPifFile = new CNPifFile(UTF8_TODO::GetCString(pff_file_path));

            std::string ent_file_path = file_path;

            if( binary_load )
                ent_file_path = PortableFunctions::PathReplaceFileExtension(ent_file_path, FileExtensions::EntryApplication);

            if( !m_pPifFile->LoadPifFile() ||
                m_pPifFile->GetAppType() != APPTYPE::ENTRY_TYPE ||
                !SO::EqualsNoCase(ent_file_path, m_pPifFile->GetAppFName()) )
            {
                SAFE_DELETE(m_pPifFile);
            }
        }

        if( m_pPifFile == NULL ) // create a PFF for this program
        {
            m_pPifFile = new CNPifFile(UTF8_TODO::GetCString(pff_file_path));
            m_pPifFile->SetAppType(APPTYPE::ENTRY_TYPE);
        }

        m_pPifFile->SetAppFName(UTF8_TODO::GetCString(file_path)); // necessary in case the .pen file is being loaded
    }

    else if( SO::EqualsNoCase(extension, FileExtensions::Old::BinaryEntryPen) )
    {
        AfxMessageBox(L"Starting with version 6.0, .enc files are no longer supported. Please regenerate your data entry application as a .pen file.");
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
    if( ( show_file_associations || m_pPifFile->EntryConnectionStringsContainWildcards() ) && !ShowPifDlg(show_file_associations) )
        return false;

    // store the file path of the current application
    m_currentApplicationFilePath = std::move(file_path);

    return ( OpenDocumentFile(TC::ToWide(m_currentApplicationFilePath).c_str()) != nullptr );
}


void CEntryrunApp::PostLoadApplicationOperations()
{
    CMainFrame* pFrame = (CMainFrame*)AfxGetMainWnd();
    CEntryrunDoc* pDoc = (CEntryrunDoc*)pFrame->GetActiveDocument();
    Application* pApp = m_pPifFile->GetApplication();

    // set the decimal character
    DECIMAL_CHAR = pApp->GetDecimalMarkIsComma() ? ',' : '.';

    // set the window title and layout
    const std::wstring title = TC::ToWide(pDoc->MakeTitle(true));
    pDoc->SetTitle(title.c_str());
    pFrame->SetWindowText(title.c_str());

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
                CString csThisKey = UTF8_TODO::GetCString(pNodeInfo->case_summary.GetKey());

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
                csMsg.Format(L"Case specified in 'StartMode' parameter of .PFF file not found in data file\n\nKey is '%s'"), key_to_open.GetString();
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
        AfxMessageBox(L"With the case listing locked, you must specify what case to work with in the PFF file's StartMode or Key attribute.");
        AfxGetMainWnd()->SendMessage(WM_CLOSE);
        return;
    }

    // if there are no casetainers, automatically go into add mode; otherwise select the first casetainer in the tree
    HTREEITEM hItem = caseTree.GetRootItem();

    if( hItem == NULL )
    {
        pFrame->OnAddCase();
    }

    else
    {
        caseTree.Select(hItem,TVGN_CARET);
        pCaseView->SetFocus();
    }
}
