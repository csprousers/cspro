#include "StdAfx.h"
#include "NoDocumentViewInput.h"
#include <zHtml/HtmlWriter.h>
#include <zHtml/PortableLocalhost.h>
#include <zUtilO/MimeType.h>
#include <zUtilF/SystemIcon.h>


NoDocumentViewInput::NoDocumentViewInput()
    :   ViewInput(nullptr, std::string())
{
}


void NoDocumentViewInput::CreateUrl()
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
        m_logoVirtualFileMappingHandler = std::make_unique<DataVirtualFileMappingHandler<std::shared_ptr<const std::vector<std::byte>>>>(std::move(csview_logo), MimeType::Type::ImagePng);
        PortableLocalhost::CreateVirtualFile(*m_logoVirtualFileMappingHandler, "CSView.png");

        html_writer << "<p><img src=\"";
        html_writer.WriteTagValue(m_logoVirtualFileMappingHandler->GetUrl());
        html_writer << "\" alt=\"CSView Logo\" /></p>";
    }

    html_writer << "<p>Select <b>File</b> -> <b>Open</b> and choose a file to view.</p>"
                    "</center></body></html>";

    m_contentVirtualFileMappingHandler = std::make_unique<TextVirtualFileMappingHandler>(html_writer.str(), MimeType::Type::Html);
    PortableLocalhost::CreateVirtualFile(*m_contentVirtualFileMappingHandler);

    m_url = m_contentVirtualFileMappingHandler->GetUrl();
}
