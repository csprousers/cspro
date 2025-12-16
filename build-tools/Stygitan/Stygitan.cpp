#include "StdAfx.h"
#include "Stygitan.h"
#include "MainFrame.h"
#include <zUtilO/ImsaDlg.h>
#include <zUtilF/CommonControls.h>


namespace
{
    // The one and only StygitanApp object
    StygitanApp theApp;
}


BEGIN_MESSAGE_MAP(StygitanApp, CWinApp)
    ON_COMMAND(ID_APP_ABOUT, OnAppAbout)
END_MESSAGE_MAP()


BOOL StygitanApp::InitInstance()
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

    // Register the application's document templates.  Document templates
    //  serve as the connection between documents, frame windows and views.
    m_fileFreeDocManager = new FileFreeDocManager();
    m_pDocManager = m_fileFreeDocManager;

    // To create the main window, this code creates a new frame window
    // object and then sets it as the application's main window object...
    MainFrame* const main_frame = new MainFrame();

    // ...create main MDI frame window
    if( !main_frame->LoadFrame(IDR_MAINFRAME) )
        return FALSE;

    // allow drag-and-drop of files
    main_frame->DragAcceptFiles();

    m_pMainWnd = main_frame;

    // The main window has been initialized, so show and update it
    main_frame->ShowWindow(m_nCmdShow);
    main_frame->UpdateWindow();

    return TRUE;
}


void StygitanApp::OnAppAbout()
{
    CIMSAAboutDlg about_dlg(WindowsWS::LoadString(AFX_IDS_APP_TITLE), LoadIcon(IDR_MAINFRAME));
    about_dlg.DoModal();
}
