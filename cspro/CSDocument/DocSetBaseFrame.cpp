#include "StdAfx.h"
#include "DocSetBaseFrame.h"
#include "CSDocCompilerSettings.h"
#include "TitleManager.h"
#include <zDesignerF/BuildWndJsonReaderInterface.h>


namespace
{
    constexpr const char* InvalidJsonMessage    = "The JSON is not valid and cannot be formatted. Fix any errors before formatting it.";
    constexpr const char* InvalidContentMessage = "The content is not valid and cannot be formatted. Fix any errors before formatting it.";

    constexpr bool DocSetComponentUseJson(const DocSetComponent::Type doc_set_component_type)
    {
        return ( doc_set_component_type != DocSetComponent::Type::ContextIds );
    }

    constexpr bool DocSetComponentSupportsDetailedFormatting(const DocSetComponent::Type doc_set_component_type)
    {
        return ( doc_set_component_type == DocSetComponent::Type::Spec ||
                 doc_set_component_type == DocSetComponent::Type::TableOfContents ||
                 doc_set_component_type == DocSetComponent::Type::Index );
    }
}


BEGIN_MESSAGE_MAP(DocSetBaseFrame, DocSetBuildHandlerFrame)
    ON_COMMAND(ID_FORMAT_JSON, OnFormatJson)
    ON_COMMAND_RANGE(ID_FORMAT_COMPONENT, ID_FORMAT_COMPONENT_DETAILED, OnFormatComponent)
    ON_UPDATE_COMMAND_UI_RANGE(ID_FORMAT_COMPONENT, ID_FORMAT_COMPONENT_DETAILED, OnUpdateFormatComponent)
END_MESSAGE_MAP()


void DocSetBaseFrame::AddFrameSpecificItemsToBuildMenu(DynamicMenuBuilder& dynamic_menu_builder)
{
    const DocSetComponent::Type doc_set_component_type = GetDocSetComponentType();
    const char* doc_set_component_text;

    if( doc_set_component_type == DocSetComponent::Type::Spec )
    {
        doc_set_component_text = "Specification";
        dynamic_menu_builder.AddOption(ID_COMPILE_DOCSET_SPEC_ONLY, L"Compile Specification Only");
        dynamic_menu_builder.AddOption(ID_COMPILE, L"Co&mpile Specification and Components\tCtrl+K");
    }

    else
    {
        doc_set_component_text = ToString(doc_set_component_type);
        dynamic_menu_builder.AddOption(ID_COMPILE, FormatTextCS2WS(L"Co&mpile %s\tCtrl+K", TC::ToWide(doc_set_component_text).c_str()));
    }

    ASSERT(DocSetComponentUseJson(doc_set_component_type) == ( GetTextEditDoc().GetLexerLanguage() == SCLEX_JSON ));

    if( DocSetComponentUseJson(doc_set_component_type) )
    {
        dynamic_menu_builder.AddSeparator();
        dynamic_menu_builder.AddOption(ID_FORMAT_JSON, L"Format JSON");
        dynamic_menu_builder.AddOption(ID_FORMAT_COMPONENT, FormatTextCS2WS(L"Format %s\tCtrl+M", TC::ToWide(doc_set_component_text).c_str()));

        if( DocSetComponentSupportsDetailedFormatting(doc_set_component_type) )
            dynamic_menu_builder.AddOption(ID_FORMAT_COMPONENT_DETAILED, FormatTextCS2WS(L"Format %s (Detailed)\tCtrl+Shift+M", TC::ToWide(doc_set_component_text).c_str()));
    }
}


void DocSetBaseFrame::CompileWrapper(std::string action, const bool input_is_json, const std::function<void(DocSetCompiler&, std::variant<JsonNode, std::string>)> compilation_function)
{
    CMainFrame& main_frame = GetMainFrame();
    CSDocumentBuildWnd* const build_wnd = main_frame.GetBuildWnd();
    HtmlOutputWnd* const html_output_wnd = main_frame.GetHtmlOutputWnd();

    if( build_wnd == nullptr || html_output_wnd == nullptr )
        return;

    CDocument& doc = *assert_cast<CDocument*>(GetActiveDocument());
    CLogicCtrl* const logic_ctrl = GetTextEditView().GetLogicCtrl();

    build_wnd->Initialize(logic_ctrl, &doc, std::move(action));

    try
    {
        CWaitCursor wait_cursor;

        DocSetCompiler doc_set_compiler(main_frame.GetGlobalSettings(), build_wnd);

        if( input_is_json )
        {
            BuildWndJsonReaderInterface json_reader_interface(doc, *build_wnd);
            const JsonNode json_node = Json::Parse(logic_ctrl->GetText(), &json_reader_interface);

            compilation_function(doc_set_compiler, json_node);
        }

        else
        {
            compilation_function(doc_set_compiler, logic_ctrl->GetText());
        }
    }

    catch( const CSProException& exception )
    {
        build_wnd->AddError(exception);
    }

    build_wnd->Finalize();

    std::string result_url;

    // show the result...
    if( build_wnd->GetErrors().empty() )
    {
        if( m_docSetPreviewUrl.empty() )
        {
            const std::string doc_set_preview_html_file_path = Path::Combine(Html::GetDirectory(Html::Subdirectory::Document), "docset-preview.html");
            m_docSetPreviewUrl = main_frame.GetSharedHtmlLocalFileServer().CreateFileUrl(doc_set_preview_html_file_path);
        }

        result_url = m_docSetPreviewUrl;
    }

    // ...or the compilation errors
    else
    {
        result_url = main_frame.CreateHtmlCompilationErrorPage(doc);
    }

    ShowHtmlAndRestoreScrollbarState(html_output_wnd->GetHtmlViewCtrl(), std::move(result_url));
}


void DocSetBaseFrame::HandleWebMessage_getDocSetJson()
{
    CWaitCursor wait_cursor;

    HtmlOutputWnd* const html_output_wnd = GetMainFrame().GetHtmlOutputWnd();
    DocSetSpec& doc_set_spec = GetDocSetSpec();
    const DocSetComponent::Type doc_set_component_type = GetDocSetComponentType();
    const bool is_spec_file = ( doc_set_component_type == DocSetComponent::Type::Spec );

    const std::unique_ptr<JsonStringWriter> json_writer = Json::CreateStringWriter();

    json_writer->BeginObject();

    json_writer->Write(JK::docSet, reinterpret_cast<int64_t>(&doc_set_spec));

    // write a title describing what is being output
    std::string title = ToString(doc_set_component_type);
    const std::string subtitle = is_spec_file ? ValueOrDefault(doc_set_spec.GetTitle()) :
                                                PortableFunctions::PathGetFilename(TC::ToUtf8(GetActiveDocument()->GetPathName()));

    if( !subtitle.empty() )
    {
        title.append(" (")
             .append(subtitle)
             .append(")");
    }

    json_writer->Write(JK::title, title);

    // write the documents when editing a spec file
    if( is_spec_file  )
    {
        json_writer->BeginArray(JK::documents);

        TitleManager title_manager(&doc_set_spec);

        for( const DocSetComponent& doc_set_component : VI_V(doc_set_spec.GetComponents()) )
        {
            if( doc_set_component.type == DocSetComponent::Type::Document )
            {
                json_writer->BeginObject();
                json_writer->Write(JK::path, doc_set_component.file_path);

                try
                {
                    json_writer->Write(JK::title, title_manager.GetTitle(doc_set_component.file_path));
                }
                catch(...) { }

                json_writer->EndObject();
            }
        }

        json_writer->EndArray();
    }

    // write the table of contents when editing a spec file or table of contents file
    if( is_spec_file || doc_set_component_type == DocSetComponent::Type::TableOfContents )
    {
        const std::optional<DocSetTableOfContents>& table_of_contents = GetLastCompiledTableOfContents();

        if( table_of_contents.has_value() )
        {
            json_writer->Key(JK::tableOfContents);
            table_of_contents->WriteJson(*json_writer, &doc_set_spec, false, true);
        }
    }

    // write the index when editing a spec or index file
    if( is_spec_file || doc_set_component_type == DocSetComponent::Type::Index )
    {
        const std::optional<DocSetIndex>& index = GetLastCompiledIndex();

        if( index.has_value() )
        {
            DocSetIndex sorted_index(*index);
            sorted_index.SortByTitle(doc_set_spec);

            json_writer->Key(JK::index);
            sorted_index.WriteJson(*json_writer, &doc_set_spec, false, true);
        }
    }

    // write settings when editing a spec or settings file
    if( is_spec_file || doc_set_component_type == DocSetComponent::Type::Settings )
    {
        json_writer->Write(JK::settings, GetLastCompiledSettings());
    }

    // write definitions when editing a spec or definitions file
    if( is_spec_file || doc_set_component_type == DocSetComponent::Type::Definitions )
    {
        const std::vector<std::tuple<std::string, std::string>>& definitions = GetLastCompiledDefinitions();

        if( !definitions.empty() || !is_spec_file )
        {
            json_writer->Key(JK::definitions);
            DocSetSpec::WriteJsonDefinitions(*json_writer, definitions, false);
        }
    }

    // write context IDs when editing a spec or context ID file
    if( is_spec_file || doc_set_component_type == DocSetComponent::Type::ContextIds )
    {
        const std::map<std::string, unsigned>& context_ids = GetLastCompiledContextIds();

        if( !context_ids.empty() || !is_spec_file )
        {
            json_writer->Key(JK::contextIds);
            DocSetSpec::WriteJsonContextIds(*json_writer, context_ids);
        }
    }

    // write the scrollbar state to restore
    json_writer->Write(JK::scrollY, std::get<1>(m_currentUrlAndScrollbarStateToRestore));

    json_writer->EndObject();

    html_output_wnd->GetHtmlViewCtrl().PostWebMessageAsJson(json_writer->GetString());
}


void DocSetBaseFrame::OnFormatJson()
{
    CLogicCtrl* const logic_ctrl = GetTextEditView().GetLogicCtrl();
    logic_ctrl->ClearErrorAndWarningMarkers();

    try
    {
        const JsonNode json_node = Json::Parse(logic_ctrl->GetText());

        SetLogicCtrlTextWithFormattedText(*logic_ctrl, json_node.GetNodeAsString(DefaultJsonFileWriterFormattingOptions));
    }

    catch(...)
    {
        ErrorMessage::Display(InvalidJsonMessage);
    }
}


void DocSetBaseFrame::OnFormatComponent(const UINT nID)
{
    CDocument& doc = *assert_cast<CDocument*>(GetActiveDocument());

    CLogicCtrl* const logic_ctrl = GetTextEditView().GetLogicCtrl();
    logic_ctrl->ClearErrorAndWarningMarkers();

    std::string doc_file_path = TC::ToUtf8(doc.GetPathName());
    const bool detailed_format = ( nID == ID_FORMAT_COMPONENT_DETAILED );

    try
    {
        CWaitCursor wait_cursor;

        JsonReaderInterface json_reader_interface(PortableFunctions::PathGetDirectory(doc_file_path));
        const JsonNode json_node = Json::Parse(logic_ctrl->GetText(), &json_reader_interface);

        DocSetCompiler doc_set_compiler(GetMainFrame().GetGlobalSettings(), DocSetCompiler::ThrowErrors { });

        const std::unique_ptr<JsonStringWriter> json_writer = Json::CreateStringWriterWithRelativePaths(std::move(doc_file_path));

        WriteFormattedComponent(*json_writer, doc_set_compiler, json_node, detailed_format);

        SetLogicCtrlTextWithFormattedText(*logic_ctrl, json_writer->ReleaseString());
    }

    catch( const JsonParseException& )
    {
        ErrorMessage::Display(InvalidJsonMessage);
    }

    catch(...)
    {
        ErrorMessage::Display(InvalidContentMessage);
    }
}


void DocSetBaseFrame::OnUpdateFormatComponent(CCmdUI* const pCmdUI)
{
    const DocSetComponent::Type doc_set_component_type = GetDocSetComponentType();
    const bool detailed_format = ( pCmdUI->m_nID == ID_FORMAT_COMPONENT_DETAILED );

    pCmdUI->Enable(detailed_format ? DocSetComponentSupportsDetailedFormatting(doc_set_component_type) :
                                     DocSetComponentUseJson(doc_set_component_type));
}


void DocSetBaseFrame::SetLogicCtrlTextWithFormattedText(CLogicCtrl& logic_ctrl, std::string formatted_text)
{
    ASSERT(!formatted_text.empty());

    // make sure that the text ends in a newline
    if( formatted_text.back() != '\n' )
        formatted_text.push_back('\n');

    logic_ctrl.SetText(formatted_text);
}
