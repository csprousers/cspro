#include "StdAfx.h"
#include "ViewDoc.h"
#include <zHtml/HtmlWriter.h>
#include <zHtml/VirtualFileMapping.h>
#include <zToolsO/Utf8Convert.h>
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
        auto new_input_processor = std::make_unique<CSViewInputProcessor>(lpszPathName);

        ProcessCloseDocument();

        m_inputProcessor = std::move(new_input_processor);

        SetPathName(m_inputProcessor->GetFilename().c_str());

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


const std::wstring* ViewDoc::GetDescription() const
{
    if( m_inputProcessor == nullptr )
        return nullptr;

    return &m_inputProcessor->GetDescription();
}


std::wstring ViewDoc::GetDocumentUrl(SharedHtmlLocalFileServer& file_server)
{
    if( m_inputProcessor == nullptr )
        return GetDocumentUrlForNoDocument(file_server);

    return file_server.GetFilenameUrl(m_inputProcessor->GetFilename());
}


std::wstring ViewDoc::GetDocumentUrlForNoDocument(SharedHtmlLocalFileServer& file_server)
{
    std::unique_ptr<VirtualFileMappingHandler>& doc_virtual_file_mapping_handler = m_noDocumentVirtualFileMappingHandlers[0];

    if( doc_virtual_file_mapping_handler == nullptr )
    {
        HtmlStringWriter html_writer;
        html_writer.WriteDefaultHeader(_T("CSView"), Html::CSS::Common);
        html_writer << _T("<body><center>");

        std::shared_ptr<const std::vector<std::byte>> csview_logo = SystemIcon::GetPngForExtension(FileExtensions::CSHTML);

        if( csview_logo == nullptr )
        {
            html_writer << _T("<h1>CSView</h1>");
        }

        else
        {
            std::unique_ptr<VirtualFileMappingHandler>& csview_logo_virtual_file_mapping_handler = m_noDocumentVirtualFileMappingHandlers[1];

            csview_logo_virtual_file_mapping_handler = std::make_unique<DataVirtualFileMappingHandler<std::shared_ptr<const std::vector<std::byte>>>>(std::move(csview_logo), MimeType::Type::ImagePng);
            file_server.CreateVirtualFile(*csview_logo_virtual_file_mapping_handler, _T("CSView.png"));

            html_writer << _T("<p><img src=\"") << csview_logo_virtual_file_mapping_handler->GetUrl().c_str() << _T("\" alt=\"CSView Logo\" /></p>");
        }

        html_writer << _T("<p>Select <b>File</b> -> <b>Open</b> to choose a file to view.</p>")
                       _T("</center></body></html>");

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
