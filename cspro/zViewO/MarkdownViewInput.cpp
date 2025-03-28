#include "StdAfx.h"
#include "MarkdownViewInput.h"
#include <zMarkdown/Markdown.h>


std::string MarkdownViewInput::ToHtml(const std::string& file_path, const std::string_view markdown_sv, const bool embed_css)
{
    CssProvider css_provider(Html::CSS::Markdown, embed_css);
    return Markdown::ToHtmlDocument(Path::GetFilenameWithoutExtension(file_path), markdown_sv, &css_provider);
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
