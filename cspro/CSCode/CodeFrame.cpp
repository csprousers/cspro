#include "StdAfx.h"
#include "CodeFrame.h"
#include "HtmlDialogCodeView.h"
#include "LanguageSettingsPersister.h"
#include "ProcessorActionInvoker.h"
#include "ProcessorMarkdown.h"
#include <zUtilF/DynamicMenuBuilder.h>
#include <zDesignerF/TextTemplatePreviewer.h>


IMPLEMENT_DYNCREATE(CodeFrame, CMDIChildWndEx)

BEGIN_MESSAGE_MAP(CodeFrame, CMDIChildWndEx)

    ON_WM_MDIACTIVATE()
    ON_WM_SIZE()

    ON_MESSAGE(UWM::CSCode::CodeFrameActivate, OnCodeFrameActivate)
    ON_MESSAGE(UWM::CSCode::SyncCodeFrameSplitter, OnSyncCodeFrameSplitter)

    // Language menu
    ON_COMMAND_RANGE(ID_LANGUAGE_CSPRO_LOGIC, ID_LANGUAGE_NONE, OnLanguageType)
    ON_UPDATE_COMMAND_UI_RANGE(ID_LANGUAGE_CSPRO_LOGIC, ID_LANGUAGE_NONE, OnUpdateLanguageType)

    // Code menu
    ON_COMMAND(ID_CODE_STRING_ENCODER, OnStringEncoder)
    ON_COMMAND(ID_CODE_PATH_ADJUSTER, OnPathAdjuster)

    ON_COMMAND_RANGE(ID_CODE_DEPRECATION_WARNINGS_NONE, ID_CODE_DEPRECATION_WARNINGS_ALL, OnDeprecationWarnings)
    ON_UPDATE_COMMAND_UI_RANGE(ID_CODE_DEPRECATION_WARNINGS_NONE, ID_CODE_DEPRECATION_WARNINGS_ALL, OnUpdateDeprecationWarnings)

    // Run menu
    ON_COMMAND(ID_RUN_RUN, OnRunRun)
    ON_UPDATE_COMMAND_UI(ID_RUN_RUN, OnUpdateRunRun)

    ON_COMMAND(ID_RUN_PREVIEW_TEXT_TEMPLATE, OnRunPreviewTextTemplate)
    ON_UPDATE_COMMAND_UI(ID_RUN_PREVIEW_TEXT_TEMPLATE, OnUpdateRunPreviewTextTemplate)

    ON_COMMAND(ID_RUN_DISPLAY_RESULTS_AS_JSON, OnRunActionInvokerDisplayResultsAsJson)
    ON_UPDATE_COMMAND_UI(ID_RUN_DISPLAY_RESULTS_AS_JSON, OnUpdateRunActionInvokerDisplayResultsAsJson)
    ON_COMMAND(ID_RUN_ABORT_ON_FIRST_EXCEPTION, OnRunActionInvokerAbortOnFirstException)
    ON_UPDATE_COMMAND_UI(ID_RUN_ABORT_ON_FIRST_EXCEPTION, OnUpdateRunActionInvokerAbortOnFirstException)

    ON_COMMAND_RANGE(ID_RUN_CSPRO_LOGIC_V0, ID_RUN_CSPRO_LOGIC_V8, OnRunLogicVersion)
    ON_UPDATE_COMMAND_UI_RANGE(ID_RUN_CSPRO_LOGIC_V0, ID_RUN_CSPRO_LOGIC_V8, OnUpdateRunLogicVersion)

    ON_COMMAND_RANGE(ID_RUN_CSPRO_SPEC_FILE_APP, ID_RUN_CSPRO_SPEC_FILE_XL2CS, OnRunSpecFileType)
    ON_UPDATE_COMMAND_UI_RANGE(ID_RUN_CSPRO_SPEC_FILE_APP, ID_RUN_CSPRO_SPEC_FILE_XL2CS, OnUpdateRunSpecFileType)

    ON_COMMAND_RANGE(ID_RUN_JAVASCRIPT_MODULE_GLOBAL, ID_RUN_JAVASCRIPT_MODULE_MODULE, OnRunJavaScriptModuleType)
    ON_UPDATE_COMMAND_UI_RANGE(ID_RUN_JAVASCRIPT_MODULE_GLOBAL, ID_RUN_JAVASCRIPT_MODULE_MODULE, OnUpdateRunJavaScriptModuleType)

    ON_COMMAND(ID_RUN_SAVE_AS_HTML, OnRunSaveAsHtml)

    // Context menu
    ON_COMMAND(ID_COPY_FULL_PATH, OnCopyFullPath)
    ON_UPDATE_COMMAND_UI(ID_COPY_FULL_PATH, OnUpdateDocumentMustBeSavedToDisk)

    ON_COMMAND(ID_OPEN_CONTAINING_FOLDER, OnOpenContainingFolder)
    ON_UPDATE_COMMAND_UI(ID_OPEN_CONTAINING_FOLDER, OnUpdateDocumentMustBeSavedToDisk)

    ON_COMMAND(ID_OPEN_IN_ASSOCIATED_APPLICATION, OnOpenInAssociatedApplication)
    ON_UPDATE_COMMAND_UI(ID_OPEN_IN_ASSOCIATED_APPLICATION, OnUpdateDocumentMustBeSavedToDisk)

END_MESSAGE_MAP()


CodeFrame::CodeFrame()
    :   m_codeFrameActivatePostMessageCounter(0),
        m_lastCheckIfFileIsUpdatedTime(0)
{
}


CodeFrame::~CodeFrame()
{
}


BOOL CodeFrame::OnCreateClient(LPCREATESTRUCT /*lpcs*/, CCreateContext* const pContext)
{
    return m_splitterWnd.Create(this, 2, 1, CSize(1, 1), pContext, WS_CHILD | WS_VISIBLE | SPLS_DYNAMIC_SPLIT);
}


void CodeFrame::OnMDIActivate(const BOOL bActivate, CWnd* const pActivateWnd, CWnd* const pDeactivateWnd)
{
    __super::OnMDIActivate(bActivate, pActivateWnd, pDeactivateWnd);

    if( bActivate )
    {
        PostMessage(UWM::CSCode::CodeFrameActivate, ++m_codeFrameActivatePostMessageCounter);
    }

    else
    {
        // if this is the last code tab being deactivated, we need to update the file type in the
        // status bar to clear the file type
        AfxGetMainWnd()->PostMessage(UWM::CSCode::SetStatusBarFileType);
    }
}


void CodeFrame::OnSize(const UINT nType, const int cx, const int cy)
{
    // when showing two code views, resize them proportionally
    if( m_splitterWnd.GetRowCount() == 2 && nType != SIZE_MINIMIZED )
    {
        CRect splitter_rect;
        m_splitterWnd.GetClientRect(splitter_rect);

        int view1_height_current;
        int view1_height_min;
        m_splitterWnd.GetRowInfo(0, view1_height_current, view1_height_min);

        int non_view1_height_current = splitter_rect.Height() - m_splitterWnd.GetSplitterGap();

        if( non_view1_height_current > 0 )
        {
            const double view1_proportion_current = static_cast<double>(view1_height_current) / non_view1_height_current;

            const int view1_height_new = static_cast<int>(( cy - m_splitterWnd.GetSplitterGap() ) * view1_proportion_current);

            if( view1_height_new > view1_height_min )
                m_splitterWnd.SetRowInfo(0, view1_height_new, view1_height_min);
        }
    }

    __super::OnSize(nType, cx, cy);
}


LRESULT CodeFrame::OnCodeFrameActivate(const WPARAM wParam, LPARAM /*lParam*/)
{
    const WPARAM& post_message_counter = wParam;

    // OnMDIActivate gets called multiple times in some instances (e.g., when creating a new document),
    // so only process the message once by checking aginst the counter;
    // the message sent from CMainFrame::OnActivateApp will be processed because lParam will be -1
    if( post_message_counter == m_codeFrameActivatePostMessageCounter || post_message_counter == static_cast<WPARAM>(-1) )
    {
        m_codeFrameActivatePostMessageCounter = 0;

        // update the file type in the status bar
        AfxGetMainWnd()->PostMessage(UWM::CSCode::SetStatusBarFileType);

        // check if the file has been modified
        CheckIfFileIsUpdated();
    }

    return 1;
}


LRESULT CodeFrame::OnSyncCodeFrameSplitter(WPARAM /*wParam = 0*/, LPARAM /*lParam = 0*/)
{
    CodeDoc& code_doc = GetCodeDoc();

    const int code_views_to_use = code_doc.GetLanguageSettings().UsesTwoCodeViews() ? 2 : 1;

    if( code_views_to_use != m_splitterWnd.GetRowCount() )
    {
        if( code_views_to_use == 1 )
        {
            m_splitterWnd.DeleteRow(1);
        }

        else
        {
            // show the second code window in the bottom third of the frame
            CRect rect;
            GetClientRect(rect);
            const int height = rect.Height() * 2 / 3;

            ASSERT(code_doc.GetLanguageSettings().GetLanguageType() == LanguageType::CSProHtmlDialog);
            m_splitterWnd.SplitRow(height, RUNTIME_CLASS(HtmlDialogCodeView));
        }
    }

    return 1;
}


void CodeFrame::CheckIfFileIsUpdated()
{
    CodeDoc& code_doc = GetCodeDoc();

    bool file_on_disk_is_newer;
    int64_t file_on_disk_modified_time;
    std::tie(file_on_disk_is_newer, file_on_disk_modified_time) = code_doc.GetFileModificationTimeParameters();

    if( file_on_disk_is_newer && file_on_disk_modified_time > m_lastCheckIfFileIsUpdatedTime )
    {
        const int response = AfxMessageBox(FormatText("The file has been modified by another program.\nDo you want to reload '%s'?",
                                                      PortableFunctions::PathGetFilename(code_doc.GetFilePath()).c_str()), MB_YESNO);

        if( response == IDYES )
            code_doc.ReloadFromDisk();
    }

    m_lastCheckIfFileIsUpdatedTime = GetTimestamp();
}


void CodeFrame::OnLanguageType(const UINT nID)
{
    CodeDoc& code_doc = GetCodeDoc();
    LanguageSettings& doc_language_settings = GetCodeDoc().GetLanguageSettings();
    const LanguageType language_type = GetLanguageTypeFromId(nID);

    doc_language_settings.SetLanguageType(language_type, code_doc.GetFilePath());

    // refresh the logic control
    assert_cast<CodeView*>(m_splitterWnd.GetPane(0, 0))->RefreshLogicControlLexer();

    // make sure multiple code views are created (if appropriate)
    OnSyncCodeFrameSplitter();

    // update the file type in the status bar
    AfxGetMainWnd()->PostMessage(UWM::CSCode::SetStatusBarFileType);

    // if this is a new file, save this type to use for the next new file
    if( code_doc.GetPathName().IsEmpty() )
        assert_cast<CMainFrame*>(AfxGetMainWnd())->GetLanguageSettingsPersister().SetDefaultLanguageType(language_type);
}


void CodeFrame::OnUpdateLanguageType(CCmdUI* const pCmdUI)
{
    const LanguageSettings& doc_language_settings = GetCodeDoc().GetLanguageSettings();
    const LanguageType language_type = GetLanguageTypeFromId(pCmdUI->m_nID);

    pCmdUI->SetCheck(doc_language_settings.GetLanguageType() == language_type);
}


void CodeFrame::OnStringEncoder()
{
    LanguageSettings& doc_language_settings = GetCodeDoc().GetLanguageSettings();
    CLogicCtrl* const active_logic_ctrl = assert_cast<CodeView*>(GetActiveView())->GetLogicCtrl();

    CodeMenu::OnStringEncoder(doc_language_settings.GetOrCreateLogicSettings(), active_logic_ctrl->GetSelText());
}


void CodeFrame::OnPathAdjuster()
{
    const CodeDoc& code_doc = GetCodeDoc();
    const LanguageSettings& doc_language_settings = code_doc.GetLanguageSettings();

    CodeMenu::OnPathAdjuster(doc_language_settings.GetLexerLanguage(), code_doc.GetFilePath());
}


void CodeFrame::OnDeprecationWarnings(const UINT nID)
{
    CodeMenu::DeprecationWarnings::OnDeprecationWarnings(nID);
}


void CodeFrame::OnUpdateDeprecationWarnings(CCmdUI* const pCmdUI)
{
    CodeMenu::DeprecationWarnings::OnUpdateDeprecationWarnings(pCmdUI);
}


void CodeFrame::PopulateRunMenu(CMenu& popup_menu)
{
    // most options will display based on the document settings, but a few are based
    // on the view settings (e.g., for HTML dialogs, which have multiple code views)
    const LanguageSettings& doc_language_settings = GetCodeDoc().GetLanguageSettings();
    const LanguageSettings& view_language_settings = GetActiveCodeView().GetLanguageSettings();
    ASSERT(&doc_language_settings == &view_language_settings || doc_language_settings.GetLanguageType() == LanguageType::CSProHtmlDialog);

    DynamicMenuBuilder dynamic_menu_builder(popup_menu, ID_RUN_PLACEHOLDER);

    if( view_language_settings.CanCompileCode() )
    {
        dynamic_menu_builder.AddOption(ID_RUN_COMPILE_OR_VALIDATE, L"Co&mpile\tCtrl+K");
    }

    if( view_language_settings.CanValidateCode() )
    {
        const bool is_spec_file = ( view_language_settings.GetLanguageType() == LanguageType::CSProSpecFileJson );

        const std::wstring validate_text = FormatText(L"&Validate%s\tCtrl+K",
                                                      is_spec_file                                                                 ? L" Specification File" :
                                                      ( doc_language_settings.GetLanguageType() == LanguageType::CSProHtmlDialog ) ? L" JSON Input" :
                                                                                                                                     L"");
        dynamic_menu_builder.AddOption(ID_RUN_COMPILE_OR_VALIDATE, validate_text);

        if( is_spec_file )
            dynamic_menu_builder.AddOption(ID_RUN_VALIDATE_JSON_ONLY, L"Validate &JSON Only");
    }

    if( doc_language_settings.CanRunCode() )
    {
        dynamic_menu_builder.AddOption(ID_RUN_RUN, L"&Run\tCtrl+R");

        if( doc_language_settings.CanStopCode() )
            dynamic_menu_builder.AddOption(ID_RUN_STOP, L"Stop");
    }

    if( doc_language_settings.CanViewTextTemplatePreview() )
    {
        dynamic_menu_builder.AddSeparator();
        dynamic_menu_builder.AddOption(ID_RUN_PREVIEW_TEXT_TEMPLATE, L"Preview Text Template\tCtrl+F5");
    }

    if( view_language_settings.GetLexerLanguage() == SCLEX_JSON )
    {
        dynamic_menu_builder.AddSeparator();

        const std::wstring json_type = ( doc_language_settings.GetLanguageType() == LanguageType::CSProHtmlDialog ) ? L"JSON Input" :
                                                                                                                      L"JSON";

        dynamic_menu_builder.AddOption(ID_RUN_FORMAT_JSON, L"&Format " + json_type);
        dynamic_menu_builder.AddOption(ID_RUN_COMPRESS_JSON, L"&Compact " + json_type);
    }


    // add Action Invoker options
    if( view_language_settings.GetLanguageType() == LanguageType::CSProActionInvoker )
    {
        dynamic_menu_builder.AddSeparator();
        dynamic_menu_builder.AddOption(dynamic_menu_builder.GetIdAndMenuText(ID_RUN_DISPLAY_RESULTS_AS_JSON));
        dynamic_menu_builder.AddOption(dynamic_menu_builder.GetIdAndMenuText(ID_RUN_ABORT_ON_FIRST_EXCEPTION));
    }


    // add HTML dialog options
    /* TODO_RESTORE_FOR_CSPRO8X if( doc_language_settings.GetLanguageType() == LanguageType::CSProHtmlDialog )
    {
        dynamic_menu_builder.AddSeparator();
        dynamic_menu_builder.AddOption(dynamic_menu_builder.GetIdAndMenuText(ID_RUN_HTML_DIALOG_TEMPLATES));
    } */


    // add Markdown options
    if( view_language_settings.GetLanguageType() == LanguageType::CSProTextTemplateMarkdown ||
        view_language_settings.GetLanguageType() == LanguageType::Markdown )
    {
        dynamic_menu_builder.AddSeparator();
        dynamic_menu_builder.AddOption(dynamic_menu_builder.GetIdAndMenuText(ID_RUN_SAVE_AS_HTML));
    }


    // add the logic version submenu
    if( Lexers::UsesCSProLogic(doc_language_settings.GetLexerLanguage()) ||
        Lexers::IsCSProMessage(doc_language_settings.GetLexerLanguage()) )
    {
        static const std::vector<std::tuple<unsigned, std::wstring>> logic_versions =
        {
            dynamic_menu_builder.GetIdAndMenuText(ID_RUN_CSPRO_LOGIC_V0),
            dynamic_menu_builder.GetIdAndMenuText(ID_RUN_CSPRO_LOGIC_V8),
        };

        dynamic_menu_builder.AddSeparator();
        dynamic_menu_builder.AddSubmenu(L"CSPro Logic Version", logic_versions);
    }


    // add the spec file submenu
    if( doc_language_settings.GetLanguageType() == LanguageType::CSProSpecFileJson )
    {
        dynamic_menu_builder.AddSeparator();
        dynamic_menu_builder.AddSubmenu(L"CSPro Specification File", LanguageJsonSpecFile::GetSubmenuOptions());

        dynamic_menu_builder.AddOption(dynamic_menu_builder.GetIdAndMenuText(ID_RUN_CSPRO_DOWNGRADE_SPEC_FILE));
    }


    // add the JavaScript code type
    if( doc_language_settings.GetLanguageType() == LanguageType::JavaScript )
    {
        static const std::vector<std::tuple<unsigned, std::wstring>> module_texts =
        {
            dynamic_menu_builder.GetIdAndMenuText(ID_RUN_JAVASCRIPT_MODULE_GLOBAL),
            dynamic_menu_builder.GetIdAndMenuText(ID_RUN_JAVASCRIPT_MODULE_MODULE),
        };

        dynamic_menu_builder.AddSeparator();
        dynamic_menu_builder.AddSubmenu(L"JavaScript Module", module_texts);
    }


    dynamic_menu_builder.AddSeparator();
    dynamic_menu_builder.AddOption(dynamic_menu_builder.GetIdAndMenuText(ID_OPEN_IN_ASSOCIATED_APPLICATION));
}


void CodeFrame::OnRunRun()
{
    CodeDoc& code_doc = GetCodeDoc();
    const LanguageSettings& doc_language_settings = code_doc.GetLanguageSettings();
    const LanguageType language_type = doc_language_settings.GetLanguageType();

    if( language_type == LanguageType::CSProActionInvoker )
    {
        ProcessorActionInvoker::Run(code_doc);
    }

    else if( language_type == LanguageType::CSProHtmlDialog )
    {
        code_doc.GetHtmlProcessor().DisplayHtmlDialog(code_doc);
    }

    else if( language_type == LanguageType::Html )
    {
        code_doc.GetHtmlProcessor().DisplayHtml(code_doc);
    }

    else if( language_type == LanguageType::JavaScript )
    {
        code_doc.GetJavaScriptProcessor().Run();
    }

    else if( language_type == LanguageType::Markdown )
    {
        ProcessorMarkdown::Run(code_doc);
    }

    else
    {
        ASSERT(doc_language_settings.CanRunCode());
        // CODE_TODO run CSPro
    }
}


void CodeFrame::OnUpdateRunRun(CCmdUI* const pCmdUI)
{
    const LanguageSettings& doc_language_settings = GetCodeDoc().GetLanguageSettings();

    pCmdUI->Enable(!assert_cast<CMainFrame*>(AfxGetMainWnd())->IsRunOperationInProgress() &&
                   doc_language_settings.CanRunCode());
}


void CodeFrame::OnRunPreviewTextTemplate()
{
    HtmlViewerWnd* const html_viewer_wnd = assert_cast<CMainFrame*>(AfxGetMainWnd())->GetHtmlViewerWnd();

    if( html_viewer_wnd == nullptr )
        return;

    CodeDoc& code_doc = GetCodeDoc();
    LanguageSettings& doc_language_settings = code_doc.GetLanguageSettings();
    const LogicSettings& logic_settings = doc_language_settings.GetOrCreateLogicSettings();
    CLogicCtrl* const logic_ctrl = code_doc.GetPrimaryCodeView().GetLogicCtrl();

    try
    {
        const bool html_type = ( doc_language_settings.GetLanguageType() == LanguageType::CSProTextTemplateHtml );
        ASSERT(html_type || doc_language_settings.GetLanguageType() == LanguageType::CSProTextTemplateMarkdown);

        m_textTemplatePreviewer = std::make_unique<TextTemplatePreviewer>(code_doc.GetActualOrTempFilePath(html_type ? FileExtensions::HTML : FileExtensions::Markdown),
                                                                          logic_ctrl->GetText(),
                                                                          logic_settings);

        html_viewer_wnd->GetHtmlBrowser().NavigateTo(m_textTemplatePreviewer->GetUriResolver());
    }

    catch( const CSProException& exception )
    {
        ErrorMessage::Display(exception);
    }
}


void CodeFrame::OnUpdateRunPreviewTextTemplate(CCmdUI* const pCmdUI)
{
    const LanguageSettings& doc_language_settings = GetCodeDoc().GetLanguageSettings();

    pCmdUI->Enable(doc_language_settings.CanViewTextTemplatePreview());
}


void CodeFrame::OnRunActionInvokerDisplayResultsAsJson()
{
    CodeDoc& code_doc = GetCodeDoc();
    LanguageSettings& doc_language_settings = code_doc.GetLanguageSettings();

    doc_language_settings.SetActionInvokerDisplayResultsAsJson(!doc_language_settings.GetActionInvokerDisplayResultsAsJson(), code_doc.GetFilePath());
}


void CodeFrame::OnUpdateRunActionInvokerDisplayResultsAsJson(CCmdUI* const pCmdUI)
{
    const LanguageSettings& doc_language_settings = GetCodeDoc().GetLanguageSettings();

    pCmdUI->SetCheck(doc_language_settings.GetActionInvokerDisplayResultsAsJson());
}


void CodeFrame::OnRunActionInvokerAbortOnFirstException()
{
    CodeDoc& code_doc = GetCodeDoc();
    LanguageSettings& doc_language_settings = code_doc.GetLanguageSettings();

    doc_language_settings.SetActionInvokerAbortOnException(!doc_language_settings.GetActionInvokerAbortOnException(), code_doc.GetFilePath());
}


void CodeFrame::OnUpdateRunActionInvokerAbortOnFirstException(CCmdUI* const pCmdUI)
{
    const LanguageSettings& doc_language_settings = GetCodeDoc().GetLanguageSettings();

    pCmdUI->SetCheck(doc_language_settings.GetActionInvokerAbortOnException());
}


void CodeFrame::OnRunLogicVersion(const UINT nID)
{
    CodeDoc& code_doc = GetCodeDoc();
    LanguageSettings& doc_language_settings = code_doc.GetLanguageSettings();

    const LogicSettings::Version version = ( nID == ID_RUN_CSPRO_LOGIC_V8 ) ? LogicSettings::Version::V8_0 :
                                                                              LogicSettings::Version::V0;

    doc_language_settings.SetLogicVersion(version, code_doc.GetFilePath());

    // refresh the logic control
    code_doc.GetPrimaryCodeView().RefreshLogicControlLexer();
}


void CodeFrame::OnUpdateRunLogicVersion(CCmdUI* const pCmdUI)
{
    CodeDoc& code_doc = GetCodeDoc();
    LanguageSettings& doc_language_settings = code_doc.GetLanguageSettings();

    const bool using_logic_version_v8 = ( doc_language_settings.GetOrCreateLogicSettings().GetVersion() >= LogicSettings::Version::V8_0 );
    const bool id_is_v8 = ( pCmdUI->m_nID == ID_RUN_CSPRO_LOGIC_V8 );

    pCmdUI->SetCheck(using_logic_version_v8 == id_is_v8);
}


void CodeFrame::OnRunSpecFileType(const UINT nID)
{
    CodeDoc& code_doc = GetCodeDoc();
    LanguageSettings& doc_language_settings = code_doc.GetLanguageSettings();

    doc_language_settings.SetJsonSpecFileIndex(nID);

    // update the file type in the status bar
    AfxGetMainWnd()->PostMessage(UWM::CSCode::SetStatusBarFileType);
}


void CodeFrame::OnUpdateRunSpecFileType(CCmdUI* const pCmdUI)
{
    const CodeDoc& code_doc = GetCodeDoc();
    const LanguageSettings& doc_language_settings = code_doc.GetLanguageSettings();

    pCmdUI->SetCheck(doc_language_settings.GetJsonSpecFileIndex() == pCmdUI->m_nID);
}


void CodeFrame::OnRunJavaScriptModuleType(const UINT nID)
{
    CodeDoc& code_doc = GetCodeDoc();
    LanguageSettings& doc_language_settings = code_doc.GetLanguageSettings();

    doc_language_settings.SetJavaScriptModuleType(nID, code_doc.GetFilePath());

    // update the file type in the status bar
    AfxGetMainWnd()->PostMessage(UWM::CSCode::SetStatusBarFileType);
}


void CodeFrame::OnUpdateRunJavaScriptModuleType(CCmdUI* const pCmdUI)
{
    const CodeDoc& code_doc = GetCodeDoc();
    const LanguageSettings& doc_language_settings = code_doc.GetLanguageSettings();

    pCmdUI->SetCheck(doc_language_settings.GetJavaScriptModuleType() == pCmdUI->m_nID);
}


void CodeFrame::OnRunSaveAsHtml()
{
    CodeDoc& code_doc = GetCodeDoc();

    switch( code_doc.GetLanguageSettings().GetLanguageType() )
    {
        case LanguageType::CSProTextTemplateMarkdown:
            ProcessorMarkdown::SaveTextTemplateAsHtml(code_doc);
            break;

        case LanguageType::Markdown:
            ProcessorMarkdown::SaveAsHtml(code_doc);
            break;

        default:
            ASSERT(false);
            break;
    }
}


void CodeFrame::OnUpdateDocumentMustBeSavedToDisk(CCmdUI* const pCmdUI)
{
    const CodeDoc& code_doc = GetCodeDoc();

    pCmdUI->Enable(!code_doc.GetPathName().IsEmpty());
}


void CodeFrame::OnCopyFullPath()
{
    const CodeDoc& code_doc = GetCodeDoc();

    WinClipboard::PutText(this, code_doc.GetPathName());
}


void CodeFrame::OnOpenContainingFolder()
{
    const CodeDoc& code_doc = GetCodeDoc();

    OpenContainingFolder(code_doc.GetPathName());
}


void CodeFrame::OnOpenInAssociatedApplication()
{
    CodeDoc& code_doc = GetCodeDoc();

    if( code_doc.IsModified() )
    {
        const int result = AfxMessageBox(FormatText("Do you want to save '%s' before opening it?",
                                                    PortableFunctions::PathGetFilename(code_doc.GetFilePath()).c_str()), MB_YESNOCANCEL);

        if( ( result == IDCANCEL ) ||
            ( result == IDYES && !code_doc.DoFileSave() ) )
        {
            return;
        }
    }

    ShellExecute(nullptr, L"open", TC::ToWide(EscapeCommandLineArgument(code_doc.GetFilePath())).c_str(), nullptr, nullptr, SW_SHOW);
}
