#include "StdAfx.h"
#include "MainFrame.h"
#include "DocSetBaseFrame.h"


std::wstring CMainFrame::CreateHtmlPage(const CDocument& doc, const std::wstring& html)
{
    const std::wstring directory = doc.GetPathName().IsEmpty() ? CSProExecutables::GetModuleDirectory() :
                                                                 PortableFunctions::PathGetDirectory(doc.GetPathName());

    m_virtualFileMapping = std::make_unique<VirtualFileMapping>(m_fileServer.CreateVirtualHtmlFile(directory,
        [ html_utf8 = UTF8Convert::WideToUTF8(html) ]()
        {
            return html_utf8;
        }));

    return m_virtualFileMapping->GetUrl();
}


std::wstring CMainFrame::CreateHtmlCompilationErrorPage(const CDocument& doc)
{
    constexpr wstring_view DocTitle_sv = _T("~~DOCTITLE~~");
    constexpr wstring_view Errors_sv   = _T("~~ERRORS~~");

    if( m_compilationErrorPageHtml.empty() )
    {
        try
        {
            const std::wstring compilation_error_page_filename = PortableFunctions::PathAppendToPath(Html::GetDirectory(Html::Subdirectory::Document),
                                                                                                     _T("compilation-error.html"));
            m_compilationErrorPageHtml = FileIO::ReadText(compilation_error_page_filename);
        }

        catch(...)
        {
            // if the page cannot be found, create a simple one
            HtmlStringWriter html_writer;
            html_writer.WriteDefaultHeader(_T("Compilation Error"), Html::CSS::Common);
            html_writer << _T("<body><p>There were errors compiling ")
                        << std::wstring(DocTitle_sv).c_str()
                        << _T(".</p>")
                        << std::wstring(Errors_sv).c_str()
                        << _T("</body></html>\n");
            m_compilationErrorPageHtml = html_writer.str();
        }
    }

    std::wstring html = m_compilationErrorPageHtml;

    auto replace_section = [&](wstring_view section_marker_sv, const std::wstring& replacement_text)
    {
        size_t section_pos = html.find(section_marker_sv);

        if( section_pos != std::wstring::npos)
        {
            html.replace(section_pos, section_marker_sv.length(), replacement_text);
        }

        else
        {
            ASSERT(false);
        }
    };

    replace_section(DocTitle_sv, Encoders::ToHtml(SO::TrimLeft(doc.GetTitle(), '*')));

    std::wstring errors;

    if( m_buildWnd != nullptr )
    {
        for( const std::wstring& error : m_buildWnd->GetErrors() )
            SO::Append(errors, _T("<p>") + Encoders::ToHtml(error) + _T("</p>"));
    }

    replace_section(Errors_sv, errors);

    return CreateHtmlPage(doc, html);
}


void CMainFrame::OnWebMessageReceived(const std::wstring& message)
{
    try
    {
        const auto json_node = Json::Parse(message);
        const wstring_view action_sv = json_node.Get<wstring_view>(JK::action);

        // open message
        if( action_sv == _T("open") )
        {
            TextEditDoc* text_edit_doc = assert_nullable_cast<TextEditDoc*>(AfxGetApp()->OpenDocumentFile(json_node.Get<std::wstring>(JK::path).c_str()));

            // associate the CSPro Document with its Document Set
            if( text_edit_doc != nullptr && text_edit_doc->GetAssociatedDocSetSpec() == nullptr )
            {
                ASSERT(SO::EqualsNoCase(PortableFunctions::PathGetFileExtension(text_edit_doc->GetPathName()), FileExtensions::CSDocument));

                std::optional<int64_t> doc_set_spec_ptr = json_node.GetOptional<int64_t>(JK::docSet);

                if( doc_set_spec_ptr.has_value() )
                {
                    std::shared_ptr<DocSetSpec> doc_set_spec = FindSharedDocSetSpec(reinterpret_cast<DocSetSpec*>(*doc_set_spec_ptr), false);

                    if( doc_set_spec != nullptr )
                        text_edit_doc->SetAssociatedDocSetSpec(std::move(doc_set_spec));
                }
            }
        }


        // getDocSetJson message
        else if( action_sv == _T("getDocSetJson") )
        {
            DocSetBaseFrame* doc_set_base_frame = dynamic_cast<DocSetBaseFrame*>(GetActiveFrame());

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
