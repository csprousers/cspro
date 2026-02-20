#include "StdAfx.h"
#include "OpenSourceSyncer.h"
#include "ControllerThreadRunningFrame.h"
#include "FeatureBranchSyncerView.h"
#include "LogFrameAndView.h"
#include "MainFrame.h"
#include "ManageLibrariesView.h"
#include "ManageReleasesView.h"
#include "ManualMirrorerView.h"
#include "RepositoryComparerView.h"
#include "SettingsDlg.h"
#include "TagSyncerView.h"
#include <zUtilO/ImsaDlg.h>
#include <zUtilF/CommonControls.h>


namespace
{
    // The one and only OpenSourceSyncerApp object
    OpenSourceSyncerApp theApp;
}


BEGIN_MESSAGE_MAP(OpenSourceSyncerApp, CWinApp)
    ON_COMMAND(ID_SYNC_FEATURE_BRANCHES, OnSyncFeatureBranches)
    ON_COMMAND(ID_MANUALLY_MIRROR_COMMITS, OnManuallyMirrorCommits)
    ON_COMMAND(ID_MANAGE_RELEASES, OnManageReleases)
    ON_COMMAND(ID_SYNC_TAGS, OnSyncTags)
    ON_COMMAND(ID_MANAGE_LIBRARIES, OnManageLibraries)
    ON_COMMAND(ID_COMPARE_REPOSITORIES, OnCompareRepositories)
    ON_COMMAND(ID_SETTINGS, OnSettings)
    ON_UPDATE_COMMAND_UI(ID_SETTINGS, OnUpdateSettings)
    ON_COMMAND(ID_APP_ABOUT, OnAppAbout)
END_MESSAGE_MAP()


OpenSourceSyncerApp::OpenSourceSyncerApp()
    :   m_fileFreeDocManager(nullptr)
{
}


BOOL OpenSourceSyncerApp::InitInstance()
{
    InitializeCommonControls();

    __super::InitInstance();

    AfxEnableControlContainer();

    // Standard initialization
    // If you are not using these features and wish to reduce the size
    //  of your final executable, you should remove from the following
    //  the specific initialization routines you do not need.

    // Change the registry key under which our settings are stored.
    SetRegistryKey(L"U.S. Census Bureau");

    LoadStdProfileSettings();  // Load standard INI file options (including MRU)

    // initialize the controller
    try
    {
        m_controller.emplace();
    }

    catch( const CSProException& exception )
    {
        ErrorMessage::Display(exception);
        return FALSE;
    }

    // Register the application's document templates.  Document templates
    //  serve as the connection between documents, frame windows and views.
    m_fileFreeDocManager = new FileFreeDocManager();
    m_fileFreeDocManager->AddDocTemplate<IDR_LOG, FileFreeDoc, LogFrame, LogView>();
    m_fileFreeDocManager->AddDocTemplate<IDR_FEATURE_BRANCH_SYNCER, FileFreeDoc, ControllerThreadRunningFrame, FeatureBranchSyncerView>();
    m_fileFreeDocManager->AddDocTemplate<IDR_MANUAL_MIRRORER, FileFreeDoc, ControllerThreadRunningFrame, ManualMirrorerView>();
    m_fileFreeDocManager->AddDocTemplate<IDR_MANAGE_RELEASES, FileFreeDoc, ControllerThreadRunningFrame, ManageReleasesView>();
    m_fileFreeDocManager->AddDocTemplate<IDR_TAG_SYNCER, FileFreeDoc, ControllerThreadRunningFrame, TagSyncerView>();
    m_fileFreeDocManager->AddDocTemplate<IDR_MANAGE_LIBRARIES, FileFreeDoc, ControllerThreadRunningFrame, ManageLibrariesView>();
    m_fileFreeDocManager->AddDocTemplate<IDR_REPO_COMPARER, FileFreeDoc, ControllerThreadRunningFrame, RepositoryComparerView>();
    m_pDocManager = m_fileFreeDocManager;

    // create main MDI Frame window
    MainFrame* const main_frame = new MainFrame();

    if( !main_frame->LoadFrame(IDR_MAINFRAME) )
        return FALSE;

    m_pMainWnd = main_frame;

    // allow drag-and-drop of files
    main_frame->DragAcceptFiles();

    // The main window has been initialized, so show and update it
    main_frame->ShowWindow(m_nCmdShow);
    main_frame->UpdateWindow();

    // open the Feature Branch Syncer on startup
    OnSyncFeatureBranches();

    return TRUE;
}


void OpenSourceSyncerApp::OpenLog()
{
    m_fileFreeDocManager->Open(IDR_LOG, false);
}


void OpenSourceSyncerApp::OnSyncFeatureBranches()
{
    m_fileFreeDocManager->Open(IDR_FEATURE_BRANCH_SYNCER, false);
}


void OpenSourceSyncerApp::OnManuallyMirrorCommits()
{
    m_fileFreeDocManager->Open(IDR_MANUAL_MIRRORER, true);
}


void OpenSourceSyncerApp::OnManageReleases()
{
    m_fileFreeDocManager->Open(IDR_MANAGE_RELEASES, false);
}


void OpenSourceSyncerApp::OnSyncTags()
{
    m_fileFreeDocManager->Open(IDR_TAG_SYNCER, false);
}


void OpenSourceSyncerApp::OnManageLibraries()
{
    m_fileFreeDocManager->Open(IDR_MANAGE_LIBRARIES, false);
}


void OpenSourceSyncerApp::OnCompareRepositories()
{
    m_fileFreeDocManager->Open(IDR_REPO_COMPARER, true);
}


void OpenSourceSyncerApp::OnSettings()
{
    SettingsDlg dlg;
    dlg.DoModal();
}


void OpenSourceSyncerApp::OnUpdateSettings(CCmdUI* const pCmdUI)
{
    pCmdUI->Enable(!m_controller->IsOperationRunning());
}


void OpenSourceSyncerApp::OnAppAbout()
{
    CIMSAAboutDlg about_dlg(WindowsWS::LoadString(AFX_IDS_APP_TITLE), LoadIcon(IDR_MAINFRAME));
    about_dlg.DoModal();
}
