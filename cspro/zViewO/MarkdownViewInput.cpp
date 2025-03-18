#include "StdAfx.h"
#include "MarkdownViewInput.h"
#include <zMarkdown/Markdown.h>


std::string MarkdownViewInput::ToViewableHtml(const std::string& file_path, const std::string_view markdown_sv)
{
    return Markdown::ToHtmlDocument(Path::GetFilenameWithoutExtension(file_path),
                                    markdown_sv,
                                    std::make_unique<CssProvider>(Html::CSS::Markdown, true).get());
}


void MarkdownViewInput::CreateUrl()
{
    m_htmlVirtualFileMapping.emplace(PortableLocalhost::CreateVirtualHtmlFile(PortableFunctions::PathGetDirectory(m_inputFilePath),
        [ html_ = SharableString(ToViewableHtml(m_inputFilePath, FileIO::ReadText(m_inputFilePath))) ]()
        {
            return html_;
        }));

    m_url = m_htmlVirtualFileMapping->GetUrl();
}
