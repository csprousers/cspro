#include "StdAfx.h"
#include "ProcessorHtml.h"
#include <zUtilF/HtmlDialogFunctionRunner.h>


std::unique_ptr<VirtualFileMapping> ProcessorHtml::CreateHtmlVirtualFileMapping(CodeDoc& code_doc, const std::wstring& filename) const
{
    SharedHtmlLocalFileServer& file_server = assert_cast<CMainFrame*>(AfxGetMainWnd())->GetSharedHtmlLocalFileServer();

    return std::make_unique<VirtualFileMapping>(
        file_server.CreateVirtualHtmlFile(PortableFunctions::PathGetDirectory(filename),
            [ html = code_doc.GetPrimaryCodeView().GetLogicCtrl()->GetTextUtf8() ]()
            {
                return html;
            }));
}


void ProcessorHtml::DisplayHtmlDialog(CodeDoc& code_doc)
{
    ASSERT(code_doc.GetLanguageSettings().GetLanguageType() == LanguageType::CSProHtmlDialog &&
           code_doc.GetSecondaryCodeView() != nullptr);

    std::wstring single_input_text = code_doc.GetSecondaryCodeView()->GetLogicCtrl()->GetText();

    std::optional<std::wstring> input_data;
    std::optional<std::wstring> display_options_json;

    HtmlDialogFunctionRunner::ParseSingleInputText(single_input_text, input_data, display_options_json);
    ASSERT(input_data.has_value());

    std::unique_ptr<VirtualFileMapping> virtual_file_mapping = CreateHtmlVirtualFileMapping(code_doc, code_doc.GetPathNameOrFakeTempName(FileExtensions::HTML));

    HtmlDialogFunctionRunner html_dialog_function_runner(NavigationAddress::CreateUriReference(virtual_file_mapping->GetUrl()),
                                                         std::move(*input_data),
                                                         std::move(display_options_json));

    html_dialog_function_runner.DoModal();

    // show the results in the output window
    OutputWnd* output_wnd = assert_cast<CMainFrame*>(AfxGetMainWnd())->GetOutputWnd();

    if( output_wnd == nullptr )
        return;

    // if the results are in JSON, format them nicely
    std::optional<std::wstring> results = html_dialog_function_runner.GetResultsText();

    if( results.has_value() )
    {
        try
        {
            results = Json::Parse(*results).GetNodeAsString(JsonFormattingOptions::PrettySpacing);
        }
        catch(...) { }
    }

    output_wnd->Clear();

    if( results.has_value() )
        output_wnd->AddText(std::move(*results));
}


void ProcessorHtml::DisplayHtml(CodeDoc& code_doc)
{
    HtmlViewerWnd* html_viewer_wnd = assert_cast<CMainFrame*>(AfxGetMainWnd())->GetHtmlViewerWnd();

    if( html_viewer_wnd == nullptr )
        return;

    CLogicCtrl* logic_ctrl = code_doc.GetPrimaryCodeView().GetLogicCtrl();
    ASSERT(logic_ctrl->GetLexer() == SCLEX_HTML);

    try
    {
        std::wstring filename = code_doc.GetPathNameOrFakeTempName(FileExtensions::HTML);

        m_htmlVirtualFileMapping = CreateHtmlVirtualFileMapping(code_doc, filename);

        html_viewer_wnd->GetHtmlBrowser().NavigateTo(UriResolver::CreateUriDomain(m_htmlVirtualFileMapping->GetUrl(),
                                                                                  m_htmlVirtualFileMapping->GetUrl(),
                                                                                  std::move(filename)));
    }

    catch( const CSProException& exception )
    {
        ErrorMessage::Display(exception);
    }
}
