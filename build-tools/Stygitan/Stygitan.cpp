#include "StdAfx.h"
#include "Stygitan.h"
#include "CodePurifierView.h"
#include "EditorConfigApplierView.h"
#include "FilteredCommitsMessageCreatorView.h"
#include "MainFrame.h"
#include "ThreadRunnerFrame.h"
#include <zUtilO/ImsaDlg.h>
#include <zUtilF/CommonControls.h>


namespace
{
    // The one and only StygitanApp object
    StygitanApp theApp;
}


BEGIN_MESSAGE_MAP(StygitanApp, CWinApp)
    ON_COMMAND(ID_FILE_OPEN_CODE_PURIFIER, OnOpenCodePurifier)
    ON_COMMAND(ID_FILE_OPEN_EDITORCONFIG_APPLIER, OnOpenEditorConfigApplier)
    ON_COMMAND(ID_COMMITS_CREATE_MESSAGE_FROM_FILTERED_COMMITS, OnCreateMessageFromFilteredCommits)
    ON_COMMAND(ID_APP_ABOUT, OnAppAbout)
END_MESSAGE_MAP()


StygitanApp::StygitanApp()
    :   m_fileFreeDocManager(nullptr),
        m_codePurifierDocTemplate(nullptr)
{
}


BOOL StygitanApp::InitInstance()
{
    // ICC_LINK_CLASS is necessary to use the SysLink Controls
    InitializeCommonControls(ICC_LINK_CLASS);

    __super::InitInstance();

    AfxEnableControlContainer();

    // Standard initialization
    // If you are not using these features and wish to reduce the size
    //  of your final executable, you should remove from the following
    //  the specific initialization routines you do not need.

    // Change the registry key under which our settings are stored.
    SetRegistryKey(L"U.S. Census Bureau");

    LoadStdProfileSettings();  // Load standard INI file options (including MRU)

    // Register the application's document templates.  Document templates
    //  serve as the connection between documents, frame windows and views.
    m_fileFreeDocManager = new FileFreeDocManager();
    m_codePurifierDocTemplate = m_fileFreeDocManager->AddDocTemplate<IDR_CODE_PURIFIER, CodePurifierDoc, CMDIChildWnd, CodePurifierView>();
    m_fileFreeDocManager->AddDocTemplate<IDR_EDITORCONFIG_APPLIER, FileFreeDoc, ThreadRunnerFrame, EditorConfigApplierView>();
    m_fileFreeDocManager->AddDocTemplate<IDR_FILTERED_COMMITS_MESSAGE_CREATOR, FilteredCommitsMessageCreatorView>();
    m_pDocManager = m_fileFreeDocManager;

    // create main MDI Frame window
    MainFrame* const main_frame = new MainFrame();

    if( !main_frame->LoadFrame(IDR_MAINFRAME) )
        return FALSE;

    m_pMainWnd = main_frame;

    // allow drag-and-drop of files
    main_frame->DragAcceptFiles();

    // open any repositories specified on the command line
    for( const std::wstring& path : GetPathsFromCommandLine() )
        OnOpenCodePurifier(TC::ToUtf8(path));

    // The main window has been initialized, so show and update it
    main_frame->ShowWindow(m_nCmdShow);
    main_frame->UpdateWindow();

    return TRUE;
}


void StygitanApp::OnOpenCodePurifier()
{
    std::string directory = SelectFolderDialog(L"Select Git or Parent Directory");

    if( directory.empty() )
        return;

    ASSERT(!Path::IsSlashChar(directory.back()));

    if( Path::GetFilename(directory) != ".git" )
        Path::MakeCombine(directory, ".git");

    OnOpenCodePurifier(std::move(directory));
}


void StygitanApp::OnOpenCodePurifier(std::string directory)
{
    ASSERT(m_codePurifierDocTemplate != nullptr);

    Path::MakeToNativeSlash(directory);
    Path::MakeRemoveTrailingSlash(directory);

    const std::wstring wide_directory = TC::ToWide(directory);

    // only open the directory if it is not already open
    if( FileFreeDocManager::FindAndActivateOpenDocumentByPath(m_codePurifierDocTemplate, wide_directory.c_str()) == nullptr )
        m_codePurifierDocTemplate->OpenDocumentFile(wide_directory.c_str());
}


void StygitanApp::OnOpenEditorConfigApplier()
{
    m_fileFreeDocManager->Open(IDR_EDITORCONFIG_APPLIER, false);
}


void StygitanApp::OnCreateMessageFromFilteredCommits()
{
    m_fileFreeDocManager->Open(IDR_FILTERED_COMMITS_MESSAGE_CREATOR, false);
}


void StygitanApp::OnAppAbout()
{
    CIMSAAboutDlg about_dlg(WindowsWS::LoadString(AFX_IDS_APP_TITLE), LoadIcon(IDR_MAINFRAME));
    about_dlg.DoModal();
}
