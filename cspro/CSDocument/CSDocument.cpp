#include "StdAfx.h"
#include "CSDocument.h"
#include "CommandLineBuilder.h"
#include "CommandLineParser.h"
#include "CSDocFrame.h"
#include "CSProUsersBlogBuilder.h"
#include "DocSetComponentDocTemplate.h"
#include "DocSetComponentFrame.h"
#include "DocSetSpecFrame.h"
#include <zUtilO/ImsaDlg.h>
#include <zUtilF/CommonControls.h>
#include <zUtilF/MDIFrameWndHelpers.h>


namespace
{
    // The one and only CSDocumentApp object
    CSDocumentApp theApp;
}


BEGIN_MESSAGE_MAP(CSDocumentApp, CWinAppEx)
    ON_COMMAND(ID_FILE_NEW, CSDocumentApp::OnFileNew)
    ON_COMMAND(ID_FILE_OPEN, CSDocumentApp::OnFileOpen)
    ON_COMMAND(ID_APP_ABOUT, CSDocumentApp::OnAppAbout)
    ON_COMMAND(ID_HELP_FINDER, CWinAppEx::OnHelpFinder)
    ON_COMMAND(ID_HELP, CWinAppEx::OnHelp)
END_MESSAGE_MAP()


CSDocumentApp::CSDocumentApp()
    :   m_exitCode(0),
        m_csdocTemplate(nullptr)
{
    InitializeCSProEnvironment();

    EnableHtmlHelp();
}


BOOL CSDocumentApp::InitInstance()
{
    // ICC_LINK_CLASS is necessary to use the SysLink Controls
    InitializeCommonControls(ICC_LINK_CLASS);

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

    // load the global settings
    GlobalSettings global_settings;

    // parse the command line
    CommandLineParser command_line_parser;

    try
    {
        ParseCommandLine(command_line_parser);

        if( command_line_parser.DoCommandLineBuild() )
        {
            CommandLineBuilder builder(global_settings);
            builder.Build(command_line_parser);
            return FALSE;
        }

        else if( command_line_parser.CreateCSProUsersBlog() )
        {
            CSProUsersBlogBuilder builder(global_settings,
                                          command_line_parser.GetDocSetFilePath(),
                                          command_line_parser.GetOutputPath());
            builder.Build();
            return FALSE;
        }

        else if( command_line_parser.CreateNotepadPlusPlusColorizer() )
        {
            CommandLineBuilder builder(global_settings);
            builder.CreateNotepadPlusPlusColorizer();
            return FALSE;
        }
    }

    catch( const CSProException& exception )
    {
        m_exitCode = 1;
        ErrorMessage::Display(exception);
        return FALSE;
    }

    // if here, the user wants to use CSDocument...

    // if an instance of CSDocument is open and the user is opening files via the command line,
    // open them in the other instance (unless the shift key is depressed)
    if( GetKeyState(VK_SHIFT) >= 0 && !command_line_parser.GetFilePaths().empty() )
    {
        CWnd* const other_instance = CWnd::FindWindow(IMSA_WNDCLASS_CSDOCUMENT, nullptr);

        if( other_instance != nullptr )
        {
            // send the filenames using JSON
            const std::unique_ptr<JsonStringWriter> json_writer = Json::CreateStringWriter();
            json_writer->Write(command_line_parser.GetFilePaths());

            IMSASendMessage(IMSA_WNDCLASS_CSDOCUMENT, WM_IMSA_FILEOPEN, TC::ToWide(json_writer->GetString()));

            other_instance->SetForegroundWindow();

            return FALSE;
        }
    }

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

    if( !add_doc_template(m_csdocTemplate = new CMultiDocTemplate(IDR_CSDOC_FRAME, RUNTIME_CLASS(CSDocDoc), RUNTIME_CLASS(CSDocFrame), RUNTIME_CLASS(TextEditView))) ||
        !add_doc_template(new CMultiDocTemplate(IDR_DOCSET_SPEC_FRAME, RUNTIME_CLASS(DocSetSpecDoc), RUNTIME_CLASS(DocSetSpecFrame), RUNTIME_CLASS(TextEditView))) ||
        !add_doc_template(new DocSetComponentDocTemplate(IDR_DOCSET_COMPONENT_FRAME, RUNTIME_CLASS(DocSetComponentDoc), RUNTIME_CLASS(DocSetComponentFrame), RUNTIME_CLASS(TextEditView))) )
    {
        return FALSE;
    }

    // create main MDI Frame window
    CMainFrame* pMainFrame = new CMainFrame(std::move(global_settings));

    if( pMainFrame == nullptr || !pMainFrame->LoadFrame(IDR_MAINFRAME) )
        return FALSE;

    m_pMainWnd = pMainFrame;

    // allow drag-and-drop of files
    pMainFrame->DragAcceptFiles();

    // open any files specified on the command line
    bool file_opened = false;

    for( const std::string& file_path : command_line_parser.GetFilePaths() )
    {
        if( OpenDocumentFile(TC::ToWide(file_path).c_str()) )
            file_opened = true;
    }

    // if no file was opened, create a new CSPro Document
    if( !file_opened )
        OnFileNew();

    // The main window has been initialized, so show and update it
    pMainFrame->ShowWindow(m_nCmdShow);
    pMainFrame->UpdateWindow();

    return TRUE;
}


int CSDocumentApp::ExitInstance()
{
    __super::ExitInstance();
    return m_exitCode;
}


CDocument* CSDocumentApp::OpenDocumentFile(LPCTSTR lpszFileName)
{
    return MDIFrameWndHelpers::OpenDocumentFileAndCloseSingleUnmodifiedPathlessDocument<CWinAppEx>(*this, lpszFileName);
}


CDocument* CSDocumentApp::OpenDocumentFile(LPCTSTR lpszFileName, const BOOL bAddToMRU)
{
    return MDIFrameWndHelpers::OpenDocumentFileAndCloseSingleUnmodifiedPathlessDocument<CWinAppEx>(*this, lpszFileName, bAddToMRU);
}


void CSDocumentApp::OnFileNew()
{
    m_csdocTemplate->OpenDocumentFile(nullptr);
}


void CSDocumentApp::OnFileOpen()
{
    const std::string wildcard_text = FormatText("*.%s;*.%s", FileExtensions::CSDocument, FileExtensions::CSDocumentSet);
    const std::string filter = FormatText("CSPro Documents and Document Sets (%s)|%s|All Files (*.*)|*.*||", wildcard_text.c_str(), wildcard_text.c_str());

    OpenFileDlg open_file_dlg(0, nullptr, nullptr, filter, nullptr);
    open_file_dlg.UseInitialDirectoryOfActiveDocument(assert_cast<CMainFrame*>(AfxGetMainWnd()))
                 .SetMultiSelectBuffer();

    if( open_file_dlg.DoModal() != IDOK )
        return;

    for( const std::string& file_path : open_file_dlg.GetFilePaths() )
        OpenDocumentFile(TC::ToWide(file_path).c_str());
}


void CSDocumentApp::OnAppAbout()
{
    CIMSAAboutDlg about_dlg(WindowsWS::LoadString(AFX_IDS_APP_TITLE), LoadIcon(IDR_MAINFRAME));
    about_dlg.DoModal();
}


void CSDocumentApp::SetDocSetParametersForNextOpen(DocSetComponent doc_set_component, std::shared_ptr<DocSetSpec> doc_set_spec)
{
    // m_docSetParametersForNextOpen may not have been released when opening a Document that was already open
    ASSERT(!m_docSetParametersForNextOpen.has_value() || std::get<0>(*m_docSetParametersForNextOpen).type == DocSetComponent::Type::Document);
    ASSERT(doc_set_spec != nullptr);

    m_docSetParametersForNextOpen.emplace(std::move(doc_set_component), std::move(doc_set_spec));
}


std::tuple<std::shared_ptr<DocSetSpec>, DocSetComponent::Type> CSDocumentApp::ReleaseDocSetParametersForNextOpen()
{
    ASSERT(m_docSetParametersForNextOpen.has_value());
    std::tuple<std::shared_ptr<DocSetSpec>, DocSetComponent::Type> parameters(std::move(std::get<1>(*m_docSetParametersForNextOpen)),
                                                                              std::get<0>(*m_docSetParametersForNextOpen).type);

    m_docSetParametersForNextOpen.reset();

    return parameters;
}
