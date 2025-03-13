#include "StdAfx.h"
#include "MarkdownToHtml.h"
#include <zHtml/HtmlWriter.h>
#include <external/md4c/md4c-html.h>


struct MarkdownOutput
{
    static void AddToString(const MD_CHAR* const text, const MD_SIZE text_size, void* const userdata)
    {
        static_cast<std::string*>(userdata)->append(text, text_size);
    }
};


void Markdown::ToHtml(std::string& html, std::string_view markdown_sv) noexcept
{
    static constexpr unsigned ParserFlags = 0;   // MD_FLAG_... combinations
    static constexpr unsigned RendererFlags = 0; // MD_HTML_FLAG_... combinations

    md_html(markdown_sv.data(), markdown_sv.length(),
            MarkdownOutput::AddToString, &html,
            ParserFlags, RendererFlags);
}


std::string Markdown::ToHtml(const std::string_view markdown_sv) noexcept
{
    std::string html;
    ToHtml(html, markdown_sv);
    return html;
}


std::string Markdown::ToHtmlDocument(const std::string_view title_sv, const std::string_view markdown_sv) noexcept
{
    std::string html(HtmlStringWriter::DefaultHeader_sv);

    html.append("<title>")
        .append(Encoders::ToHtml(title_sv))
        .append("</title>\n"
                "</head>\n"
                "<body>\n");

    ToHtml(html, markdown_sv);

    html.append("</body>\n"
                "</html>\n");

    return html;
}
