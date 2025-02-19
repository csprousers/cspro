#include "StdAfx.h"
#include "MainFrame.h"
#include "DocSetBaseFrame.h"


std::string CMainFrame::CreateHtmlPage(const CDocument& doc, SharableString html)
{
    const std::string directory = doc.GetPathName().IsEmpty() ? CSProExecutables::GetModuleDirectory() :
                                                                PortableFunctions::PathGetDirectory(TC::ToUtf8(doc.GetPathName()));

    m_virtualFileMapping = std::make_unique<VirtualFileMapping>(m_fileServer.CreateVirtualHtmlFile(directory,
        [html_ = std::move(html)]()
        {
            return html_;
        }));

    return m_virtualFileMapping->GetUrl();
}


std::string CMainFrame::CreateHtmlCompilationErrorPage(const CDocument& doc)
{
    constexpr std::string_view DocTitle_sv = "~~DOCTITLE~~";
    constexpr std::string_view Errors_sv   = "~~ERRORS~~";

    if( m_compilationErrorPageHtml.empty() )
    {
        try
        {
            const std::string compilation_error_page_file_path = Path::Combine(Html::GetDirectory(Html::Subdirectory::Document), "compilation-error.html");
            m_compilationErrorPageHtml = FileIO::ReadText(compilation_error_page_file_path);
        }

        catch(...)
        {
            // if the page cannot be found, create a simple one
            HtmlStringWriter html_writer;
            html_writer.WriteDefaultHeader("Compilation Error", Html::CSS::Common);
            html_writer << "<body><p>There were errors compiling ";
            html_writer.WriteRaw(DocTitle_sv);
            html_writer << ".</p>";
            html_writer.WriteRaw(Errors_sv);
            html_writer << "</body></html>\n";
            m_compilationErrorPageHtml = html_writer.str();
        }
    }

    std::string html = m_compilationErrorPageHtml;

    auto replace_section = [&](const std::string_view section_marker_sv, const std::string& replacement_text)
    {
        const size_t section_pos = html.find(section_marker_sv);

        if( section_pos != std::string::npos)
        {
            html.replace(section_pos, section_marker_sv.length(), replacement_text);
        }

        else
        {
            ASSERT(false);
        }
    };

    replace_section(DocTitle_sv, Encoders::ToHtml(SO::TrimLeft(TC::ToUtf8(doc.GetTitle()), '*')));

    std::string errors;

    if( m_buildWnd != nullptr )
    {
        for( const std::string& error : m_buildWnd->GetErrors() )
        {
            errors.append("<p>")
                  .append(Encoders::ToHtml(error))
                  .append("</p>");
        }
    }

    replace_section(Errors_sv, errors);

    return CreateHtmlPage(doc, std::move(html));
}


void CMainFrame::OnWebMessageReceived(const std::string_view message_sv)
{
    try
    {
        const JsonNode json_node = Json::Parse(message_sv);
        const std::string_view action_sv = json_node.Get<std::string_view>(JK::action);

        // open message
        if( action_sv == "open" )
        {
            TextEditDoc* const text_edit_doc = assert_nullable_cast<TextEditDoc*>(AfxGetApp()->OpenDocumentFile(TC::ToWide(json_node.Get<std::string_view>(JK::path)).c_str()));

            // associate the CSPro Document with its Document Set
            if( text_edit_doc != nullptr && text_edit_doc->GetAssociatedDocSetSpec() == nullptr )
            {
                ASSERT(SO::EqualsNoCase(PortableFunctions::PathGetFileExtension(text_edit_doc->GetPathName()), FileExtensions::CSDocument));

                const std::optional<int64_t> doc_set_spec_ptr = json_node.GetOptional<int64_t>(JK::docSet);

                if( doc_set_spec_ptr.has_value() )
                {
                    std::shared_ptr<DocSetSpec> doc_set_spec = FindSharedDocSetSpec(reinterpret_cast<DocSetSpec*>(*doc_set_spec_ptr), false);

                    if( doc_set_spec != nullptr )
                        text_edit_doc->SetAssociatedDocSetSpec(std::move(doc_set_spec));
                }
            }
        }


        // getDocSetJson message
        else if( action_sv == "getDocSetJson" )
        {
            DocSetBaseFrame* const doc_set_base_frame = dynamic_cast<DocSetBaseFrame*>(GetActiveFrame());

            if( doc_set_base_frame != nullptr )
                doc_set_base_frame->HandleWebMessage_getDocSetJson();
        }


        // unknown message
        else
        {
            throw CSProException("The web message could not be handled.");
        }
    }

    catch( const CSProException& exception )
    {
        ErrorMessage::Display(exception);
    }
}
