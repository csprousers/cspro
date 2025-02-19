#include "StdAfx.h"
#include "CaseHoldingFrame.h"
#include "CaseHtmlContentCreator.h"
#include "CaseJsonContentCreator.h"
#include "CaseQuestionnaireContentCreator.h"
#include "CaseTextContentCreator.h"
#include "DataSourceFrame.h"
#include "DataSummaryContentCreator.h"
#include "ExtractNotesDlg.h"
#include "ExtractNotesTask.h"
#include "ExtractBinaryDataDlg.h"
#include "ExtractBinaryDataTask.h"
#include "LogicHelperContentCreator.h"
#include "TaskRunnerDlg.h"
#include <zUtilF/DynamicMenuBuilder.h>
#include <zAction/WebController.h>


BEGIN_MESSAGE_MAP(CaseHoldingFrame, CMDIChildWndEx)

    // File menu
    ON_COMMAND(ID_FILE_EXTRACT_DICTIONARY, OnFileExtractDictionary)

    ON_COMMAND(ID_FILE_EXTRACT_NOTES, OnFileExtractNotes)

    ON_COMMAND(ID_FILE_EXTRACT_BINARY_DATA, OnFileExtractBinaryData)
    ON_UPDATE_COMMAND_UI(ID_FILE_EXTRACT_BINARY_DATA, OnUpdateFileExtractBinaryData)

    ON_COMMAND(ID_FILE_SAVE_VIEW, OnFileSaveView)
    ON_UPDATE_COMMAND_UI(ID_FILE_SAVE_VIEW, OnUpdateFileSaveView)

    ON_COMMAND(ID_FILE_PRINT_VIEW, OnFilePrintView)
    ON_UPDATE_COMMAND_UI(ID_FILE_PRINT_VIEW, OnUpdateFilePrintView)

    // View menu
    ON_COMMAND(ID_VIEW_DATA_SUMMARY, OnViewDataSummaryPage)
    ON_COMMAND(ID_VIEW_LOGIC_HELPER, OnViewLogicHelperPage)
    ON_UPDATE_COMMAND_UI_RANGE(ID_VIEW_DATA_SUMMARY, ID_VIEW_LOGIC_HELPER, OnUpdateViewDataSourcePage)

    ON_COMMAND_RANGE(ID_VIEW_CASE_HTML, ID_VIEW_CASE_QUESTIONNAIRE, OnViewCasePage)
    ON_UPDATE_COMMAND_UI_RANGE(ID_VIEW_CASE_HTML, ID_VIEW_CASE_QUESTIONNAIRE, OnUpdateViewCasePage)

    // View Options menu
    ON_COMMAND_RANGE(ID_VIEW_OPTIONS_LANGUAGE0, ID_VIEW_OPTIONS_LANGUAGE9, OnViewOptionsLanguage)
    ON_UPDATE_COMMAND_UI_RANGE(ID_VIEW_OPTIONS_LANGUAGE0, ID_VIEW_OPTIONS_LANGUAGE9, OnUpdateViewOptionsLanguage)

    ON_COMMAND_RANGE(ID_VIEW_OPTIONS_HTML_DICTIONARY_LABELS, ID_VIEW_OPTIONS_TEXT_DETAILS_PANE, OnViewOptions)
    ON_UPDATE_COMMAND_UI_RANGE(ID_VIEW_OPTIONS_HTML_DICTIONARY_LABELS, ID_VIEW_OPTIONS_TEXT_DETAILS_PANE, OnUpdateViewOptions)

    ON_COMMAND_RANGE(ID_TOGGLE_DICTIONARY_DISPLAY, ID_TOGGLE_VALUE_DISPLAY, OnViewOptions)
    ON_UPDATE_COMMAND_UI_RANGE(ID_TOGGLE_DICTIONARY_DISPLAY, ID_TOGGLE_VALUE_DISPLAY, OnUpdateViewOptions)

    // message handlers
    ON_MESSAGE(UWM::DataManager::UpdateContentOnCaseListingSettingsChange, OnUpdateContentOnCaseListingSettingsChange)
    ON_MESSAGE(UWM::DataManager::UpdateContentOnCaseListingSelectionsChange, OnUpdateContentOnCaseListingSelectionsChange)
    ON_MESSAGE(UWM::DataManager::ShowDefaultPage, OnShowDefaultPage)
    ON_MESSAGE(UWM::DataManager::ProcessWebViewMessage, OnProcessWebViewMessage)

END_MESSAGE_MAP()


CaseHoldingFrame::CaseHoldingFrame()
{
}


CaseHoldingFrame::~CaseHoldingFrame()
{
}


void CaseHoldingFrame::OnFileExtractDictionary()
{
    CaseHoldingDoc& case_holding_doc = GetCaseHoldingDoc();

    if( !case_holding_doc.DictionaryAllowsExport("extracting the dictionary") )
        return;

    const CDataDict& dictionary = case_holding_doc.GetDictionary();
    std::string suggested_file_path = dictionary.GetFilePath();

    if( suggested_file_path.empty() )
    {
        suggested_file_path = PortableFunctions::CreateFilePath(case_holding_doc.GetDictionaryDirectory(),
                                                                dictionary.GetName(),
                                                                FileExtensions::Dictionary);
    }

    SaveFileDlg save_file_dlg(0, FileExtensions::Dictionary, suggested_file_path, FileFilters::Dictionary, this);
    save_file_dlg.SetTitle(FormatText("Save Dictionary '%s' As", dictionary.GetName().c_str()));

    if( save_file_dlg.DoModal() != IDOK )
        return;

    try
    {
        dictionary.Save(save_file_dlg.GetFilePath(), false);
    }

    catch( const CSProException& exception )
    {
        ErrorMessage::Display(exception);
    }
}


void CaseHoldingFrame::OnFileExtractNotes()
{
    OnFileExtractNotes(CreateCaseProvider());
}


void CaseHoldingFrame::OnFileExtractNotes(std::shared_ptr<CaseProvider> case_provider)
{
    ASSERT(case_provider != nullptr);

    CaseHoldingDoc& case_holding_doc = GetCaseHoldingDoc();

    if( !case_holding_doc.DictionaryAllowsExport("extracting notes") )
        return;

    const Case* const single_operation_case = case_provider->GetCaseIfSingleCaseOperation();

    if( single_operation_case != nullptr && single_operation_case->GetNotes().empty() )
    {
        ErrorMessage::Display(FormatText("There are no notes to export in case '%s'.",
                                         single_operation_case->GetSingleLineKey().c_str()));
        return;
    }

    const std::shared_ptr<ExtractNotesSettings> settings = case_holding_doc.GetSettings<ExtractNotesSettings>();

    ExtractNotesDlg dlg(case_holding_doc, *settings, case_provider);

    if( dlg.DoModal() != IDOK )
        return;

    *settings = dlg.GetSettings();

    RunCaseTask(std::move(case_provider),
                std::make_unique<ExtractNotesTask>(*settings));
}


void CaseHoldingFrame::OnFileExtractBinaryData()
{
    OnFileExtractBinaryData(CreateCaseProvider());
}


void CaseHoldingFrame::OnFileExtractBinaryData(std::shared_ptr<CaseProvider> case_provider)
{
    ASSERT(case_provider != nullptr);

    CaseHoldingDoc& case_holding_doc = GetCaseHoldingDoc();

    if( !case_holding_doc.DictionaryAllowsExport("extracting binary data") )
        return;

    const Case* const single_operation_case = case_provider->GetCaseIfSingleCaseOperation();

    if( single_operation_case != nullptr && !single_operation_case->HasDefinedBinaryData() )
    {
        ErrorMessage::Display(FormatText("There is no binary data in case '%s'.",
                                         single_operation_case->GetSingleLineKey().c_str()));
        return;
    }

    const std::shared_ptr<ExtractBinaryDataSettings> settings = case_holding_doc.GetSettings<ExtractBinaryDataSettings>();

    ExtractBinaryDataDlg dlg(case_holding_doc, *settings, case_provider);

    if( dlg.DoModal() != IDOK )
        return;

    *settings = dlg.GetSettings();

    RunCaseTask(std::move(case_provider),
                std::make_unique<ExtractBinaryDataTask>(*settings));
}


void CaseHoldingFrame::OnUpdateFileExtractBinaryData(CCmdUI* const pCmdUI)
{
    pCmdUI->Enable(GetCaseHoldingDoc().DictionaryUsesBinaryData());
}


void CaseHoldingFrame::OnFileSaveView()
{
    ASSERT(m_contentCreator != nullptr);

    CaseHoldingDoc& case_holding_doc = GetCaseHoldingDoc();

    if( !case_holding_doc.DictionaryAllowsExport("saving views of data") )
        return;

    const std::vector<const char*> extensions = m_contentCreator->GetSaveFormats();
    ASSERT(!extensions.empty() && extensions.back() == FileExtensions::HTML);

    std::string primary_filter_text;
    std::string primary_filter_extensions;

    for( const char* const extension : extensions )
    {
        ASSERT(extension == *std::find(extensions.cbegin(), extensions.cend(), extension));

        const char* const description = ( strcmp(extension, FileExtensions::HTML) == 0 )  ? "HTML" :
                                        ( strcmp(extension, FileExtensions::Json) == 0 )  ? "JSON" :
                                        ( strcmp(extension, FileExtensions::Logic) == 0 ) ? "Logic" :
                                        ( strcmp(extension, FileExtensions::Text) == 0  ) ? "Text" :
                                                                                            ReturnProgrammingError("");

        SO::AppendWithSeparator(primary_filter_text, description, ", ");
        SO::AppendWithSeparator(primary_filter_extensions, FileExtensions::CreateWildcard(extension), ";");
    }

    primary_filter_text.append(" Files (")
                       .append(primary_filter_extensions)
                       .append(")|")
                       .append(primary_filter_extensions)
                       .append("|All Files (*.*)|*.*||");

    // default to saving files in the data source or dictionary's directory when possible
    std::string suggested_file_path = Path::CreateValidFilename(m_contentCreator->GetSaveSuggestedFilename());
    const ConnectionString& connection_string = case_holding_doc.GetConnectionString();

    if( connection_string.HasFilePath() )
    {
        suggested_file_path = PortableFunctions::PathReplaceFilename(connection_string.GetFilePath(), suggested_file_path);
    }

    else if( PortableFunctions::FileIsRegular(case_holding_doc.GetDictionary().GetFilePath()) )
    {
        suggested_file_path = PortableFunctions::PathReplaceFilename(case_holding_doc.GetDictionary().GetFilePath(), suggested_file_path);
    }

    SaveFileDlg save_file_dlg(0, extensions.front(), suggested_file_path, primary_filter_text, this);
    save_file_dlg.SetTitle(m_contentCreator->GetSaveTitle())
                 .DisableExtensionCheck();

    if( save_file_dlg.DoModal() != IDOK )
        return;

    try
    {
        const bool save_as_html = ( extensions.size() == 1 ||
                                    FileExtensions::IsFileHtml(save_file_dlg.GetFilePath()) );

        const SharableString content = save_as_html ? m_contentCreator->GetHtmlContent(true) :
                                                      m_contentCreator->GetTextContent();

        FileIO::WriteText(save_file_dlg.GetFilePath(), *content, false);
    }

    catch( const CSProException& exception )
    {
        ErrorMessage::Display(exception);
    }
}


void CaseHoldingFrame::OnUpdateFileSaveView(CCmdUI* const pCmdUI)
{
    pCmdUI->Enable(( m_contentCreator != nullptr &&
                     m_contentCreator->GetSaveTitle() != nullptr ));
}


void CaseHoldingFrame::OnFilePrintView()
{
    if( !GetCaseHoldingDoc().DictionaryAllowsExport("printing views of data") )
        return;

    GetHtmlView().GetHtmlViewCtrl().ShowPrintUI();
}


void CaseHoldingFrame::OnUpdateFilePrintView(CCmdUI* const pCmdUI)
{
    pCmdUI->Enable(( m_contentCreator != nullptr ));
}


bool CaseHoldingFrame::IsShowingPage(const UINT command_id) const
{
    return ( m_contentCreator != nullptr &&
             m_contentCreator->GetCommandId() == command_id );
}


bool CaseHoldingFrame::IsShowingPageOrNothing(const UINT command_id) const
{
    return ( m_contentCreator == nullptr ||
             m_contentCreator->GetCommandId() == command_id );
}


void CaseHoldingFrame::ViewPage(std::unique_ptr<ContentCreator> content_creator)
{
    m_contentCreator = std::move(content_creator);
    ASSERT(m_contentCreator != nullptr);

    if( m_contentCreator->UsesActionInvoker() )
    {
        // make sure that the Action Invoker is set up for use to return input data
        HtmlViewCtrl& html_view_ctrl = GetHtmlView().GetHtmlViewCtrl();
        ActionInvoker::WebController* web_controller = html_view_ctrl.GetActionInvokerWebController();

        if( web_controller == nullptr )
            web_controller = &html_view_ctrl.RegisterCSProHostObject();

        web_controller->GetListener().SetOnGetInputDataCallback(
            [&]()
            {
                return ( m_contentCreator != nullptr ) ? m_contentCreator->GetActionInvokerInputData() :
                                                         ReturnProgrammingError(SharableString());
            });
    }

    UpdatePage();
}


void CaseHoldingFrame::UpdatePage()
{
    ASSERT(m_contentCreator != nullptr);

    try
    {
        GetHtmlView().GetHtmlViewCtrl().NavigateTo(m_contentCreator->GetUrl());
    }

    catch( const CSProException& exception )
    {
        m_contentCreator.reset();
        GetHtmlView().GetHtmlViewCtrl().SetHtml(CreateHtmlPageForException(exception));
        ErrorMessage::Display(exception);
    }
}


std::string CaseHoldingFrame::CreateHtmlPageForException(const CSProException& exception)
{
    return Encoders::ToPreformattedTextHtml("Data Manager Error", exception.what());
}


void CaseHoldingFrame::OnViewDataSummaryPage()
{
    DataSourceDoc& data_source_doc = assert_cast<DataSourceFrame*>(this)->GetDataSourceDoc();
    ViewPage(std::make_unique<DataSummaryContentCreator>(data_source_doc));
}


void CaseHoldingFrame::OnViewLogicHelperPage()
{
    DataSourceDoc& data_source_doc = assert_cast<DataSourceFrame*>(this)->GetDataSourceDoc();
    ViewPage(std::make_unique<LogicHelperContentCreator>(data_source_doc));
}


void CaseHoldingFrame::OnViewCasePage(const UINT nID)
{
    switch( nID )
    {
        case ID_VIEW_CASE_HTML:          return ViewPage(std::make_unique<CaseHtmlContentCreator>(GetCaseHoldingDoc()));
        case ID_VIEW_CASE_JSON:          return ViewPage(std::make_unique<CaseJsonContentCreator>(GetCaseHoldingDoc()));
        case ID_VIEW_CASE_TEXT:          return ViewPage(std::make_unique<CaseTextContentCreator>(GetCaseHoldingDoc()));
        case ID_VIEW_CASE_QUESTIONNAIRE: return ViewPage(std::make_unique<CaseQuestionnaireContentCreator>(GetCaseHoldingDoc()));
        default:                         ASSERT(false);
    }
}


void CaseHoldingFrame::OnUpdateViewDataSourcePage(CCmdUI* const pCmdUI)
{
    pCmdUI->SetCheck(IsShowingPage(pCmdUI->m_nID));
}


void CaseHoldingFrame::OnUpdateViewCasePage(CCmdUI* const pCmdUI)
{
    pCmdUI->SetCheck(IsShowingPage(pCmdUI->m_nID));
    pCmdUI->Enable(( GetCaseHoldingDoc().GetCurrentCase() != nullptr ));
}


LRESULT CaseHoldingFrame::OnUpdateContentOnCaseListingSettingsChange(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
    if( m_contentCreator != nullptr && m_contentCreator->ContentChangesOnCaseListingSettingsChange() )
        UpdatePage();

    return 1;
}


LRESULT CaseHoldingFrame::OnUpdateContentOnCaseListingSelectionsChange(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
    if( m_contentCreator != nullptr && m_contentCreator->ContentChangesOnCaseListingSelectionsChange() )
        UpdatePage();

    return 1;
}


LRESULT CaseHoldingFrame::OnShowDefaultPage(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
    PostMessage(WM_COMMAND, GetCaseHoldingDoc().GetDefaultPageCommandId());
    return 1;
}


LRESULT CaseHoldingFrame::OnProcessWebViewMessage(const WPARAM wParam, LPARAM /*lParam*/)
{
    const SharableString message = WindowsDesktopMessage::GetPostedObject<SharableString>(wParam);
    ASSERT(message.IsSet());

    if( m_contentCreator != nullptr )
    {
        try
        {
            const JsonNode json_node = Json::Parse(*message);
            m_contentCreator->ProcessWebViewMessage(json_node);
            return 1;
        }

        catch( const CSProException& exception )
        {
            ErrorMessage::Display(exception);
        }
    }

    return 0;
}


void CaseHoldingFrame::PopulateViewOptionsMenu(CMenu& popup_menu)
{
    DynamicMenuBuilder dynamic_menu_builder(popup_menu, ID_VIEW_OPTIONS_PLACEHOLDER);

    CaseHoldingDoc& case_holding_doc = GetCaseHoldingDoc();

    // add the dictionary languages
    std::vector<std::tuple<unsigned, std::string>> language_options;
    unsigned command_id = ID_VIEW_OPTIONS_LANGUAGE0;

    for( const Language& language : case_holding_doc.GetDictionary().GetLanguages() )
    {
        language_options.emplace_back(command_id, language.GetLabel());

        if( ++command_id > ID_VIEW_OPTIONS_LANGUAGE9 )
            break;
    }

    dynamic_menu_builder.AddSubmenu(L"Language", language_options);

    // add any page-specific options
    const UINT menu_resource_id = ( m_contentCreator != nullptr ) ? m_contentCreator->GetViewOptionsMenuResourceId() :
                                                                    0;

    if( menu_resource_id != 0 )
    {
        dynamic_menu_builder.AddSeparator();

        CMenu menu;
        menu.LoadMenu(menu_resource_id);
        ASSERT(menu.GetMenuItemCount() == 1);

        dynamic_menu_builder.AddOptions(menu.GetSubMenu(0));
    }
}


void CaseHoldingFrame::OnViewOptionsLanguage(const UINT nID)
{
    const CDataDict& dictionary = GetCaseHoldingDoc().GetDictionary();
    const size_t language_index = nID - ID_VIEW_OPTIONS_LANGUAGE0;
    ASSERT(language_index < dictionary.GetLanguages().size());

    if( language_index != dictionary.GetCurrentLanguageIndex() )
    {
        dictionary.SetCurrentLanguage(language_index);

        // update the page with the new language
        if( m_contentCreator != nullptr )
            UpdatePage();
    }
}


void CaseHoldingFrame::OnUpdateViewOptionsLanguage(CCmdUI* const pCmdUI)
{
    const CDataDict& dictionary = GetCaseHoldingDoc().GetDictionary();
    const size_t language_index = pCmdUI->m_nID - ID_VIEW_OPTIONS_LANGUAGE0;

    pCmdUI->SetCheck(( language_index == dictionary.GetCurrentLanguageIndex() ));
}


void CaseHoldingFrame::OnViewOptions(const UINT nID)
{
    ASSERT(m_contentCreator != nullptr);

    if( m_contentCreator != nullptr )
    {
        if( m_contentCreator->ProcessViewOptionsMenu(nID) )
            UpdatePage();
    }
}


void CaseHoldingFrame::OnUpdateViewOptions(CCmdUI* const pCmdUI)
{
    ASSERT(m_contentCreator != nullptr);

    if( m_contentCreator != nullptr )
        m_contentCreator->ProcessViewOptionsMenu(pCmdUI);
}


void CaseHoldingFrame::RunCaseTask(std::shared_ptr<CaseProvider> case_provider, std::unique_ptr<CaseTask> case_task)
{
    ASSERT(case_provider != nullptr && case_task != nullptr);

    case_task->SetCaseProvider(GetCaseHoldingDoc().GetSharedCaseAccess(), std::move(case_provider));

    TaskRunnerDlg task_runner_dlg(std::move(case_task), this);
    task_runner_dlg.DoModal();
}
