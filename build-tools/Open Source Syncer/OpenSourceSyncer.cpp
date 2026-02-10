#include "StdAfx.h"
#include "OpenSourceSyncer.h"
#include "MainFrame.h"
#include "OpenSourceSyncerDlg.h"
#include "SettingsDlg.h"
#include <zUtilO/ImsaDlg.h>
#include <zUtilF/CommonControls.h>


namespace
{
    // The one and only OpenSourceSyncerApp object
    OpenSourceSyncerApp theApp;
}


BEGIN_MESSAGE_MAP(OpenSourceSyncerApp, CWinApp)
    ON_COMMAND(ID_OPEN_SYNCER, OnOpenSyncer)
    ON_COMMAND(ID_SETTINGS, OnSettings)
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
    //m_fileFreeDocManager->AddDocTemplate<IDR_EDITORCONFIG_APPLIER, FileFreeDoc, ThreadRunnerFrame, EditorConfigApplierView>();
    //m_fileFreeDocManager->AddDocTemplate<IDR_FILTERED_COMMITS_MESSAGE_CREATOR, FilteredCommitsMessageCreatorView>();
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

    return TRUE;
}


void OpenSourceSyncerApp::OnOpenSyncer()
{
    OpenSourceSyncerDlg dlg;
    dlg.DoModal();
}


void OpenSourceSyncerApp::OnSettings()
{
    SettingsDlg dlg;
    dlg.DoModal();
}


void OpenSourceSyncerApp::OnAppAbout()
{
    CIMSAAboutDlg about_dlg(WindowsWS::LoadString(AFX_IDS_APP_TITLE), LoadIcon(IDR_MAINFRAME));
    about_dlg.DoModal();
}
