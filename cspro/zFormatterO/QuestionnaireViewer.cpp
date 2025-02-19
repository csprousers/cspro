#include "stdafx.h"
#include "QuestionnaireViewer.h"
#include <zToolsO/FileIO.h>
#include <zUtilO/MimeType.h>
#include <zUtilO/Viewers.h>
#include <zHtml/HtmlWriter.h>
#include <zHtml/PortableLocalhost.h>
#include <zAction/AccessToken.h>


namespace
{
    constexpr const char* QuestionnaireViewHtmlFilename = "index.html";
}


QuestionnaireViewer::QuestionnaireViewer()
    :   m_html(GetQuestionnaireViewHtml())
{
}


QuestionnaireViewer::~QuestionnaireViewer()
{
}


std::string QuestionnaireViewer::GetQuestionnaireViewHtml()
{
    const std::string questionnaire_view_file_path = Path::Combine(Html::GetDirectory(Html::Subdirectory::QuestionnaireView),
                                                                   QuestionnaireViewHtmlFilename);
    try
    {
        return FileIO::ReadText(questionnaire_view_file_path);
    }

    catch( const CSProException& exception )
    {
        // if there was a problem reading the file, create a page showing the exception
        HtmlStringWriter html_writer;

        html_writer.WriteDefaultHeader("Questionnaire View", Html::CSS::Common);

        html_writer << "<body><p>There was an error creating the questionnaire view: <b>";
        html_writer.WriteEncoded(exception.what());
        html_writer << "</b></p></body></html>";

        return html_writer.str();
    }
}


const std::string& QuestionnaireViewer::GetUrl()
{
    // set the directory for the URL as the application directory, which will allow relative
    // filenames such as images embedded in question text to display properly
    std::string directory_for_url = GetDirectoryForUrl();
    ASSERT(directory_for_url.empty() || PortableFunctions::PathGetDirectory(directory_for_url) == directory_for_url);

    HtmlContentServer* html_content_server;

    // if already created for this directory, use the existing virtual file
    const auto& html_content_server_lookup = m_htmlContentServers.find(directory_for_url);

    if( html_content_server_lookup != m_htmlContentServers.cend() )
    {
        html_content_server = &html_content_server_lookup->second;
    }

    // if a directory is specified, serve the content using a virtual HTML file located in that directory
    else if( !directory_for_url.empty() )
    {
        VirtualFileMapping virtual_file_mapping = PortableLocalhost::CreateVirtualHtmlFile(directory_for_url, [&]() { return m_html; });
        html_content_server = &m_htmlContentServers.try_emplace(std::move(directory_for_url), std::move(virtual_file_mapping)).first->second;
    }

    // otherwise serve the content as a generic virtual file
    else
    {
        auto virtual_file_mapping_handler = std::make_unique<DataVirtualFileMappingHandler<const std::string*>>(&m_html.GetString(), MimeType::Type::Html);
        PortableLocalhost::CreateVirtualFile(*virtual_file_mapping_handler, QuestionnaireViewHtmlFilename);
        html_content_server = &m_htmlContentServers.try_emplace(std::move(directory_for_url), std::move(virtual_file_mapping_handler)).first->second;
    }

    return std::holds_alternative<VirtualFileMapping>(*html_content_server) ?
        std::get<VirtualFileMapping>(*html_content_server).GetUrl() :
        std::get<std::unique_ptr<DataVirtualFileMappingHandler<const std::string*>>>(*html_content_server)->GetUrl();
}


std::string QuestionnaireViewer::GetInputData()
{
    const std::unique_ptr<JsonStringWriter> json_writer = Json::CreateStringWriter();

    json_writer->BeginObject()
                .Write(JK::name, GetDictionaryName())
                .WriteIfNotBlank(JK::uuid, GetCaseUuid())
                .WriteIfNotBlank(JK::key, GetCaseKey())
                .WriteIfNotBlank(JK::language, GetCurrentLanguageName())
                .Write(JK::showLanguageBar, ShowLanguageBar())
                .EndObject();

    return json_writer->ReleaseString();
}


void QuestionnaireViewer::View(const ViewerOptions* const base_viewer_options/* = nullptr*/)
{
    ViewerOptions viewer_options = ( base_viewer_options != nullptr ) ? *base_viewer_options :
                                                                        ViewerOptions();

    if( !viewer_options.title.IsSet() )
        viewer_options.title = "Questionnaire Viewer";

    viewer_options.action_invoker_ui_get_input_data = GetInputData();

    Viewer viewer;
    viewer.UseEmbeddedViewer()
          .SetOptions(std::move(viewer_options))
          .SetAccessInvokerAccessTokenOverride(std::string(ActionInvoker::AccessToken::QuestionnaireView_Index_sv))
          .ViewHtmlUrl(GetUrl());
}
