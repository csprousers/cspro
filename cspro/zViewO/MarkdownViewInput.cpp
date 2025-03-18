#include "StdAfx.h"
#include "MarkdownViewInput.h"
#include <zMarkdown/Markdown.h>


void MarkdownViewInput::CreateUrl()
{
    std::string html = Markdown::ToHtmlDocument(Path::GetFilenameWithoutExtension(m_inputFilePath),
                                                FileIO::ReadText(m_inputFilePath),
                                                std::make_unique<CssProvider>(Html::CSS::Markdown, true).get());


    m_htmlVirtualFileMapping.emplace(PortableLocalhost::CreateVirtualHtmlFile(PortableFunctions::PathGetDirectory(m_inputFilePath),
        [ html_ = SharableString(std::move(html)) ]()
        {
            return html_;
        }));

    m_url = m_htmlVirtualFileMapping->GetUrl();
}
