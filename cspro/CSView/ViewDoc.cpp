#include "StdAfx.h"
#include "ViewDoc.h"
#include <zHtml/HtmlWriter.h>
#include <zHtml/VirtualFileMapping.h>
#include <zToolsO/Utf8.h>
#include <zUtilO/MimeType.h>
#include <zUtilF/SystemIcon.h>
#include <zAppO/PFF.h>


IMPLEMENT_DYNCREATE(ViewDoc, CDocument)


ViewDoc::ViewDoc()
{
}


BOOL ViewDoc::OnNewDocument()
{
    ASSERT(m_inputProcessor == nullptr);

    return TRUE;
}


BOOL ViewDoc::OnOpenDocument(LPCTSTR lpszPathName)
{
    try
    {
        auto new_input_processor = std::make_unique<ViewInputProcessor>(TC::ToUtf8(lpszPathName));

        ProcessCloseDocument();

        m_inputProcessor = std::move(new_input_processor);

        SetPathName(TC::ToWide(m_inputProcessor->GetFilePath()).c_str());

        return TRUE;
    }

    catch( const CSProException& exception )
    {
        ErrorMessage::Display(exception);
        return FALSE;
    }
}


void ViewDoc::OnCloseDocument()
{
    ProcessCloseDocument();

    __super::OnCloseDocument();
}


const std::string* ViewDoc::GetDescription() const
{
    if( m_inputProcessor == nullptr )
        return nullptr;

    return &m_inputProcessor->GetDescription();
}


std::string ViewDoc::GetDocumentUrl(SharedHtmlLocalFileServer& file_server)
{
    if( m_inputProcessor == nullptr )
        return GetDocumentUrlForNoDocument(file_server);

    return file_server.CreateFileUrl(m_inputProcessor->GetFilePath());
}


std::string ViewDoc::GetDocumentUrlForNoDocument(SharedHtmlLocalFileServer& file_server)
{
    std::unique_ptr<VirtualFileMappingHandler>& doc_virtual_file_mapping_handler = m_noDocumentVirtualFileMappingHandlers[0];

    if( doc_virtual_file_mapping_handler == nullptr )
    {
        HtmlStringWriter html_writer;
        html_writer.WriteDefaultHeader("CSView", Html::CSS::Common);
        html_writer << "<body><center>";

        std::shared_ptr<const std::vector<std::byte>> csview_logo = SystemIcon::GetPngForExtension(FileExtensions::CSHTML);

        if( csview_logo == nullptr )
        {
            html_writer << "<h1>CSView</h1>";
        }

        else
        {
            std::unique_ptr<VirtualFileMappingHandler>& csview_logo_virtual_file_mapping_handler = m_noDocumentVirtualFileMappingHandlers[1];

            csview_logo_virtual_file_mapping_handler = std::make_unique<DataVirtualFileMappingHandler<std::shared_ptr<const std::vector<std::byte>>>>(std::move(csview_logo), MimeType::Type::ImagePng);
            file_server.CreateVirtualFile(*csview_logo_virtual_file_mapping_handler, "CSView.png");

            html_writer << "<p><img src=\"";
            html_writer.WriteTagValue(csview_logo_virtual_file_mapping_handler->GetUrl());
            html_writer << "\" alt=\"CSView Logo\" /></p>";
        }

        html_writer << "<p>Select <b>File</b> -> <b>Open</b> and choose a file to view.</p>"
                       "</center></body></html>";

        doc_virtual_file_mapping_handler = std::make_unique<TextVirtualFileMappingHandler>(html_writer.str(), MimeType::Type::Html);
        file_server.CreateVirtualFile(*doc_virtual_file_mapping_handler);
    }

    return doc_virtual_file_mapping_handler->GetUrl();
}


void ViewDoc::ProcessCloseDocument()
{
    if( m_inputProcessor != nullptr && m_inputProcessor->GetPff() != nullptr )
        m_inputProcessor->GetPff()->ExecuteOnExitPff();
}
