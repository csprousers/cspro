#include "StdAfx.h"
#include "DataManager.h"
#include "CaseFrame.h"
#include "DataSourceDocTemplate.h"
#include "DataSourceFrame.h"
#include "ProductionSyncer.h"
#include <zUtilO/CommandLineParsers.h>
#include <zUtilO/imsaDlg.H>
#include <zUtilF/CommonControls.h>


namespace
{
    // The one and only DataManagerApp object
    DataManagerApp theApp;
}


BEGIN_MESSAGE_MAP(DataManagerApp, CWinAppEx)
    ON_COMMAND(ID_APP_ABOUT, DataManagerApp::OnAppAbout)
    ON_COMMAND(ID_HELP_FINDER, CWinAppEx::OnHelpFinder)
    ON_COMMAND(ID_HELP, CWinAppEx::OnHelp)
END_MESSAGE_MAP()


DataManagerApp::DataManagerApp()
    :   m_caseDocTemplate(nullptr)
{
    InitializeCSProEnvironment();

    EnableHtmlHelp();
}


BOOL DataManagerApp::InitInstance()
{
    InitializeCommonControls();

    __super::InitInstance();

    // Initialize OLE libraries
    if( !AfxOleInit() )
        return FALSE;

    AfxEnableControlContainer();

    // Standard initialization
    // If you are not using these features and wish to reduce the size
    //  of your final executable, you should remove from the following
    //  the specific initialization routines you do not need.

    // Change the registry key under which our settings are stored.
    SetRegistryKey(L"U.S. Census Bureau");

    // process the command line
    auto connection_string_file_simulator = std::make_shared<ConnectionStringFileSimulator>();
    ConnectionStringCommandLineParser command_line_parser(connection_string_file_simulator);
    ParseCommandLine(command_line_parser);

    // potentially handle production syncs
    const std::variant<std::monostate, std::unique_ptr<ProductionSyncer>> production_syncer_result = ProcessProductionSyncs(command_line_parser.GetFilePaths());

    if( std::holds_alternative<std::unique_ptr<ProductionSyncer>>(production_syncer_result) &&
        std::get<std::unique_ptr<ProductionSyncer>>(production_syncer_result) == nullptr )
    {
        return FALSE;
    }

    // if here, the user wants to use Data Manager...

    LoadStdProfileSettings(_AFX_MRU_MAX_COUNT);  // Load standard INI file options (including MRU)

    InitContextMenuManager();

    // Register the application's document templates.  Document templates
    //  serve as the connection between documents, frame windows and views.
    auto add_doc_template = [&](CMultiDocTemplate* const pDocTemplate)
    {
        if( pDocTemplate == nullptr )
            return false;

        AddDocTemplate(pDocTemplate);

        return true;
    };

    if( !add_doc_template(new DataSourceDocTemplate(IDR_DATA_SOURCE_FRAME, RUNTIME_CLASS(DataSourceDoc), RUNTIME_CLASS(DataSourceFrame), RUNTIME_CLASS(HtmlView))) ||
        !add_doc_template(m_caseDocTemplate = new CMultiDocTemplate(IDR_CASE_FRAME, RUNTIME_CLASS(CaseDoc), RUNTIME_CLASS(CaseFrame), RUNTIME_CLASS(HtmlView))) )
    {
        return FALSE;
    }

    // create main MDI Frame window
    CMainFrame* const main_frame = new CMainFrame(connection_string_file_simulator);

    if( main_frame == nullptr || !main_frame->LoadFrame(IDR_MAINFRAME) )
        return FALSE;

    m_pMainWnd = main_frame;

    // allow drag-and-drop of files
    main_frame->DragAcceptFiles();

    // open data sources that were part of a non-silent PFF sync...
    if( std::holds_alternative<std::unique_ptr<ProductionSyncer>>(production_syncer_result) )
    {
        for( const ConnectionString& connection_string : std::get<std::unique_ptr<ProductionSyncer>>(production_syncer_result)->GetConnectionStringsForSyncedDataSources() )
            OpenDocumentFile(connection_string_file_simulator->GetFilePath(connection_string).c_str());
    }

    // ...or open any data sources specified on the command line
    else
    {
        for( const std::wstring& file_path : command_line_parser.GetFilePaths() )
            OpenDocumentFile(file_path.c_str());
    }

    // The main window has been initialized, so show and update it
    main_frame->ShowWindow(m_nCmdShow);
    main_frame->UpdateWindow();

    return TRUE;
}


int DataManagerApp::ExitInstance()
{
    if( m_pff != nullptr )
        m_pff->ExecuteOnExitPff();

    return __super::ExitInstance();
}


void DataManagerApp::OnAppAbout()
{
    CIMSAAboutDlg about_dlg(WindowsWS::LoadString(AFX_IDS_APP_TITLE), LoadIcon(IDR_MAINFRAME));
    about_dlg.DoModal();
}


void DataManagerApp::OpenCaseDoc(CaseDocData case_doc_data)
{
    ASSERT(m_caseDocData == nullptr);
    ASSERT(std::get<0>(case_doc_data) != nullptr &&
           std::get<1>(case_doc_data) != nullptr &&
           std::get<2>(case_doc_data).IsDefined() &&
           std::get<3>(case_doc_data) != nullptr &&
           std::get<4>(case_doc_data) >= ID_VIEW_CASE_HTML && std::get<4>(case_doc_data) <= ID_VIEW_CASE_QUESTIONNAIRE);

    m_caseDocData = std::make_unique<CaseDocData>(std::move(case_doc_data));

    m_caseDocTemplate->OpenDocumentFile(nullptr, FALSE, TRUE);
}


std::variant<std::monostate, std::unique_ptr<ProductionSyncer>> DataManagerApp::ProcessProductionSyncs(const std::vector<std::wstring>& file_paths)
{
    std::unique_ptr<ProductionSyncer> production_syncer;

    try
    {
        production_syncer = ProductionSyncer::Create(file_paths);

        if( production_syncer == nullptr )
            return std::monostate();

        m_pff = production_syncer->GetPff();

        production_syncer->RunSyncs();
    }

    catch( const UserCanceledException& )
    {
    }

    catch( const CSProException& exception )
    {
        ErrorMessage::Display(exception);

        // Data Manager will never be shown on error...
        production_syncer.reset();
    }

    // ...or when run silently
    if( m_pff != nullptr && m_pff->GetSilent() )
        production_syncer.reset();

    return production_syncer;
}
