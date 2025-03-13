#include "StdAfx.h"
#include "ProcessorHtml.h"
#include <zUtilF/HtmlDialogFunctionRunner.h>


std::unique_ptr<VirtualFileMapping> ProcessorHtml::CreateHtmlVirtualFileMapping(SharableString html, const std::string& directory) const
{
    SharedHtmlLocalFileServer& file_server = assert_cast<CMainFrame*>(AfxGetMainWnd())->GetSharedHtmlLocalFileServer();

    return std::make_unique<VirtualFileMapping>(
        file_server.CreateVirtualHtmlFile(directory,
            [ html_ = std::move(html) ]()
            {
                return html_;
            }));
}


void ProcessorHtml::DisplayHtmlDialog(CodeDoc& code_doc)
{
    ASSERT(code_doc.GetLanguageSettings().GetLanguageType() == LanguageType::CSProHtmlDialog &&
           code_doc.GetSecondaryCodeView() != nullptr);

    const std::string single_input_text = code_doc.GetSecondaryCodeView()->GetLogicCtrl()->GetText();

    SharableString input_data;
    SharableString display_options_json;

    HtmlDialogFunctionRunner::ParseSingleInputText(single_input_text, input_data, display_options_json);
    ASSERT(input_data.IsSet());

    const std::unique_ptr<VirtualFileMapping> virtual_file_mapping = CreateHtmlVirtualFileMapping(code_doc.GetPrimaryCodeView().GetLogicCtrl()->GetText(),
                                                                                                  code_doc.GetActualOrTempDirectory());

    HtmlDialogFunctionRunner html_dialog_function_runner(NavigationAddress::CreateUriReference(virtual_file_mapping->GetUrl()),
                                                         std::move(input_data),
                                                         std::move(display_options_json));

    html_dialog_function_runner.DoModal();

    // show the results in the output window
    OutputWnd* const output_wnd = assert_cast<CMainFrame*>(AfxGetMainWnd())->GetOutputWnd();

    if( output_wnd == nullptr )
        return;

    output_wnd->Clear();

    // if there was an exception, display that
    try
    {
        html_dialog_function_runner.GetExceptionHolder().ThrowExceptions();
    }

    catch( const std::exception& exception )
    {
        output_wnd->AddText(FormatText("Action Invoker Exception: %s", exception.what()));
        return;
    }

    // otherwise, if the results are in JSON, format them nicely
    SharableString results = html_dialog_function_runner.GetResultsText();

    if( results.IsSet() )
    {
        try
        {
            results = Json::Parse(*results).GetNodeAsSharableString(JsonFormattingOptions::PrettySpacing);
        }
        catch(...) { }
    }

    if( results.IsSet() )
        output_wnd->AddText(std::move(results));
}


void ProcessorHtml::DisplayHtml(CodeDoc& code_doc)
{
    CLogicCtrl* const logic_ctrl = code_doc.GetPrimaryCodeView().GetLogicCtrl();
    ASSERT(logic_ctrl->GetLexer() == SCLEX_HTML);

    try
    {
        DisplayHtml(code_doc.GetPrimaryCodeView().GetLogicCtrl()->GetText(),
                    code_doc.GetActualOrTempFilePath(FileExtensions::HTML));
    }

    catch( const CSProException& exception )
    {
        ErrorMessage::Display(exception);
    }
}


void ProcessorHtml::DisplayHtml(SharableString html, std::string file_path)
{
    ASSERT(html.IsSet() && !file_path.empty());

    HtmlViewerWnd* const html_viewer_wnd = assert_cast<CMainFrame*>(AfxGetMainWnd())->GetHtmlViewerWnd();

    if( html_viewer_wnd == nullptr )
        return;

    m_htmlVirtualFileMapping = CreateHtmlVirtualFileMapping(std::move(html), PortableFunctions::PathGetDirectory(file_path));

    html_viewer_wnd->GetHtmlBrowser().NavigateTo(UriResolver::CreateUriDomain(m_htmlVirtualFileMapping->GetUrl(),
                                                                              m_htmlVirtualFileMapping->GetUrl(),
                                                                              std::move(file_path)));
}
