#include "StdAfx.h"
#include "DocSetBaseFrame.h"
#include "CSDocCompilerSettings.h"
#include "TitleManager.h"
#include <zDesignerF/BuildWndJsonReaderInterface.h>


namespace
{
    constexpr const TCHAR* InvalidJsonMessage    = _T("The JSON is not valid and cannot be formatted. Fix any errors before formatting it.");
    constexpr const TCHAR* InvalidContentMessage = _T("The content is not valid and cannot be formatted. Fix any errors before formatting it.");

    constexpr bool DocSetComponentUseJson(DocSetComponent::Type doc_set_component_type)
    {
        return ( doc_set_component_type != DocSetComponent::Type::ContextIds );
    }

    constexpr bool DocSetComponentSupportsDetailedFormatting(DocSetComponent::Type doc_set_component_type)
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
    const TCHAR* doc_set_component_text;

    if( doc_set_component_type == DocSetComponent::Type::Spec )
    {
        doc_set_component_text = _T("Specification");
        dynamic_menu_builder.AddOption(ID_COMPILE_DOCSET_SPEC_ONLY, _T("Compile Specification Only"));
        dynamic_menu_builder.AddOption(ID_COMPILE, _T("Co&mpile Specification and Components\tCtrl+K"));
    }

    else
    {
        doc_set_component_text = ToString(doc_set_component_type);
        dynamic_menu_builder.AddOption(ID_COMPILE, FormatTextCS2WS(_T("Co&mpile %s\tCtrl+K"), doc_set_component_text));
    }

    ASSERT(DocSetComponentUseJson(doc_set_component_type) == ( GetTextEditDoc().GetLexerLanguage() == SCLEX_JSON ));

    if( DocSetComponentUseJson(doc_set_component_type) )
    {
        dynamic_menu_builder.AddSeparator();
        dynamic_menu_builder.AddOption(ID_FORMAT_JSON, _T("Format JSON"));
        dynamic_menu_builder.AddOption(ID_FORMAT_COMPONENT, FormatTextCS2WS(_T("Format %s\tCtrl+M"), doc_set_component_text));

        if( DocSetComponentSupportsDetailedFormatting(doc_set_component_type) )
            dynamic_menu_builder.AddOption(ID_FORMAT_COMPONENT_DETAILED, FormatTextCS2WS(_T("Format %s (Detailed)\tCtrl+Shift+M"), doc_set_component_text));
    }
}


void DocSetBaseFrame::CompileWrapper(std::wstring action, bool input_is_json, std::function<void(DocSetCompiler&, std::variant<JsonNode<wchar_t>, std::wstring>)> compilation_function)
{
    CMainFrame& main_frame = GetMainFrame();
    CSDocumentBuildWnd* build_wnd = main_frame.GetBuildWnd();
    HtmlOutputWnd* html_output_wnd = main_frame.GetHtmlOutputWnd();

    if( build_wnd == nullptr || html_output_wnd == nullptr )
        return;

    CDocument& doc = *assert_cast<CDocument*>(GetActiveDocument());
    CLogicCtrl* logic_ctrl = GetTextEditView().GetLogicCtrl();

    build_wnd->Initialize(logic_ctrl, &doc, std::move(action));

    try
    {
        CWaitCursor wait_cursor;

        DocSetCompiler doc_set_compiler(main_frame.GetGlobalSettings(), build_wnd);

        if( input_is_json )
        {
            BuildWndJsonReaderInterface json_reader_interface(doc, *build_wnd);
            const auto json_node = Json::Parse(logic_ctrl->GetText(), &json_reader_interface);

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

    std::wstring result_url;

    // show the result...
    if( build_wnd->GetErrors().empty() )
    {
        if( m_docSetPreviewUrl.empty() )
        {
            const std::wstring doc_set_preview_html_filename = PortableFunctions::PathAppendToPath(Html::GetDirectory(Html::Subdirectory::Document), _T("docset-preview.html"));
            m_docSetPreviewUrl = main_frame.GetSharedHtmlLocalFileServer().GetFilenameUrl(doc_set_preview_html_filename);
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

    HtmlOutputWnd* html_output_wnd = GetMainFrame().GetHtmlOutputWnd();
    DocSetSpec& doc_set_spec = GetDocSetSpec();
    const DocSetComponent::Type doc_set_component_type = GetDocSetComponentType();
    const bool is_spec_file = ( doc_set_component_type == DocSetComponent::Type::Spec );

    auto json_writer = Json::CreateStringWriter();

    json_writer->BeginObject();

    json_writer->Write(JK::docSet, reinterpret_cast<int64_t>(&doc_set_spec));

    // write a title describing what is being output
    std::wstring title = ToString(doc_set_component_type);
    const std::wstring subtitle = is_spec_file ? ValueOrDefault(doc_set_spec.GetTitle()) :
                                                 PortableFunctions::PathGetFilename(GetActiveDocument()->GetPathName());

    if( !subtitle.empty() )
        title.append(_T(" (") + subtitle + _T(")"));

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
                json_writer->Write(JK::path, doc_set_component.filename);

                try
                {
                    json_writer->Write(JK::title, title_manager.GetTitle(doc_set_component.filename));
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
        const std::vector<std::tuple<std::wstring, std::wstring>>& definitions = GetLastCompiledDefinitions();

        if( !definitions.empty() || !is_spec_file )
        {
            json_writer->Key(JK::definitions);
            DocSetSpec::WriteJsonDefinitions(*json_writer, definitions, false);
        }
    }

    // write context IDs when editing a spec or context ID file
    if( is_spec_file || doc_set_component_type == DocSetComponent::Type::ContextIds )
    {
        const std::map<std::wstring, unsigned>& context_ids = GetLastCompiledContextIds();

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
    CLogicCtrl* logic_ctrl = GetTextEditView().GetLogicCtrl();
    logic_ctrl->ClearErrorAndWarningMarkers();

    try
    {
        const auto json_node = Json::Parse(logic_ctrl->GetText());

        SetLogicCtrlTextWithFormattedText(*logic_ctrl, json_node.GetNodeAsString(DefaultJsonFileWriterFormattingOptions));
    }

    catch(...)
    {
        ErrorMessage::Display(InvalidJsonMessage);
    }
}


void DocSetBaseFrame::OnFormatComponent(UINT nID)
{
    CDocument& doc = *assert_cast<CDocument*>(GetActiveDocument());

    CLogicCtrl* logic_ctrl = GetTextEditView().GetLogicCtrl();
    logic_ctrl->ClearErrorAndWarningMarkers();

    const std::wstring doc_filename = CS2WS(doc.GetPathName());
    const bool detailed_format = ( nID == ID_FORMAT_COMPONENT_DETAILED );

    try
    {
        CWaitCursor wait_cursor;

        JsonReaderInterface json_reader_interface(PortableFunctions::PathGetDirectory(doc_filename));
        const auto json_node = Json::Parse(logic_ctrl->GetText(), &json_reader_interface);

        DocSetCompiler doc_set_compiler(GetMainFrame().GetGlobalSettings(), DocSetCompiler::ThrowErrors { });

        auto json_writer = Json::CreateStringWriterWithRelativePaths(doc_filename);

        WriteFormattedComponent(*json_writer, doc_set_compiler, json_node, detailed_format);

        SetLogicCtrlTextWithFormattedText(*logic_ctrl, json_writer->GetString());
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


void DocSetBaseFrame::OnUpdateFormatComponent(CCmdUI* pCmdUI)
{
    const DocSetComponent::Type doc_set_component_type = GetDocSetComponentType();
    const bool detailed_format = ( pCmdUI->m_nID == ID_FORMAT_COMPONENT_DETAILED );

    pCmdUI->Enable(detailed_format ? DocSetComponentSupportsDetailedFormatting(doc_set_component_type) :
                                     DocSetComponentUseJson(doc_set_component_type));                                     
}


void DocSetBaseFrame::SetLogicCtrlTextWithFormattedText(CLogicCtrl& logic_ctrl, std::wstring formatted_text)
{
    ASSERT(!formatted_text.empty());

    // make sure that the text ends in a newline
    if( formatted_text.back() != '\n' )
        formatted_text.push_back('\n');
        
    logic_ctrl.SetText(formatted_text);
}
