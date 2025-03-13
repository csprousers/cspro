#include "StdAfx.h"
#include "Markdown.h"
#include <zHtml/HtmlWriter.h>
#include <external/md4c/md4c-html.h>


namespace
{
    static constexpr unsigned ParserFlags   = 0; // MD_FLAG_... combinations
    static constexpr unsigned RendererFlags = 0; // MD_HTML_FLAG_... combinations
}


void Markdown::AddToStringCallback(const char* const text, const unsigned int text_size, void* const userdata)
{
    static_cast<std::string*>(userdata)->append(text, text_size);
}


void Markdown::ToHtml(std::string& html, const std::string_view markdown_sv)
{
    const int result = md_html(markdown_sv.data(), markdown_sv.length(),
                               AddToStringCallback, &html,
                               ParserFlags, RendererFlags);

    if( result != 0 )
        throw CSProException("There was an error creating HTML from the Markdown.");
}


std::string Markdown::ToHtml(const std::string_view markdown_sv)
{
    std::string html;
    ToHtml(html, markdown_sv);
    return html;
}


std::string Markdown::ToHtmlDocument(const std::string_view title_sv, const std::string_view markdown_sv)
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
